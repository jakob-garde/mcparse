#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstddef>

#include "lib/jg_baselayer.h"


// TODO: export to the baselayer 
// TODO: have a StrIntern type-thing which is better && works with HasMap (the Str struct itself is pushed)
Str FindDirCategory(Str path) {
    Str category_name = {};
    Str dirpath = StrDirPath(path);

    if (dirpath.len) {
        StrLst *dirname = StrSplit(dirpath, '/');

        while (dirname->next) {
            dirname = dirname->next;
        }
        category_name = dirname->GetStr();
    }
    return category_name;
}


#include "src/parsecore.h"
#include "src/parsehelpers.h"
#include "src/parse_comp.h"
#include "src/parse_instr.h"
#include "src/cogen_comp.h"
#include "src/cogen_instr.h"


struct ParseStats {
    s32 total_cnt = 0;
    s32 parsed_cnt = 0;
    s32 registered_cnt = 0;
    s32 duplicate_cnt = 0;
    s32 parse_error_cnt = 0;
    s32 type_error_cnt = 0;
};

bool RegisterComponentType(ComponentParse *comp, HashMap *map) {
    u64 val = MapGet(map, comp->type);
    bool type_was_unique = (val == 0);
    if (type_was_unique) {
        MapPut(map, comp->type, comp);
    }

    return type_was_unique;
}

bool RegisterInstrument(InstrumentParse *instr, HashMap *map) {
    u64 val = MapGet(map, instr->name);
    bool name_is_unique = (val == 0);
    if (name_is_unique) {
        MapPut(map, instr->name, instr);
    }

    return name_is_unique;
}

ParseStats ParseInstruments(MArena *a_dest, HashMap *map_instrs, StrLst *fpaths) {
    ParseStats ps = {};

    while (fpaths) {
        Str filename = StrLstNext(&fpaths);
        Str text = LoadTextFileFSeek(a_dest, filename);
        if (text.len == 0) {
            continue;
        }

        printf("parsing  #%.3d: %.*s", ps.total_cnt, filename.len, filename.str);

        InstrumentParse *instr = ParseInstrument(a_dest, text);
        instr->path = filename;
        instr->check_idx = ps.total_cnt;

        if (instr->parse_error) {
            ps.parse_error_cnt++;
        }
        else {
            ps.parsed_cnt++;

            if (RegisterInstrument(instr, map_instrs)) {
                ps.registered_cnt++;
            }
            else {
                ps.parsed_cnt++;
                ps.duplicate_cnt++;
            }
        }
        printf("\n");

        ps.total_cnt++;
    }

    return ps;
}


ParseStats ParseComponents(MArena *a_dest, HashMap *map_comps, StrLst *fpaths) {
    ParseStats ps = {};

    while (fpaths) {
        Str filename = StrLstNext(&fpaths);
        Str text = LoadTextFileFSeek(a_dest, filename);
        if (text.len == 0) {
            continue;
        }

        printf("parsing  #%.3d: %.*s", ps.total_cnt, filename.len, filename.str);
        ComponentParse *comp = ParseComponent(a_dest, text);
        comp->file_path = filename;
        comp->category = FindDirCategory(filename);

        if (comp->parse_error == true) {
            ps.parse_error_cnt++;
        }
        else {
            ps.parsed_cnt++;

            if (RegisterComponentType(comp, map_comps)) {
                ps.registered_cnt++;
            }
            else {
                ps.duplicate_cnt++;
            }
        }
        printf("\n");

        ps.total_cnt++;
    }

    return ps;
}


ComponentCall *_FindByName(Array<ComponentCall> comps, Str name) {
    for (s32 i = 0; i < comps.len; ++i) {
        if (StrEqual(name, comps.arr[i].name)) {
            return comps.arr + i;
        }
    }
    return NULL;
}


bool CheckInstrument(MArena *a_tmp, InstrumentParse *instr, HashMap *comps, ParseStats *stats, bool dbg_print_missing_types = false) {
    TimeFunction;

    s32 max_copy_comps = 1000;
    HashMap map_cpys = InitMap(a_tmp, max_copy_comps);

    bool type_error = false;
    bool nameref_error = false;

    printf("checking  #%d: ", instr->check_idx);
    StrPrint(instr->name);

    for (s32 i = 0; i < instr->comps.len; ++i) {
        ComponentCall *c = instr->comps.arr + i;

        // Handle "... = COPY (copy_type)":
        //      Meaning, we eliminate any COPY notes and reference the .type and .args.
        if (c->copy_type.len) {
            if (StrEqual(c->copy_type, "PREVIOUS")) {

                // TODO: on error, set error flag and skip
                if (i == 0) {
                    nameref_error = true;
                    printf("\nERROR: COPY(PREVIOUS) can not be the first component call\n\n");
                }
                else {
                    Str prev_type = instr->comps.arr[i-1].type;
                    c->type = prev_type;
                }
            }
            else {
                ComponentCall *org_comp = _FindByName(instr->comps, c->copy_type);
                if (org_comp == NULL) {
                    StrPrint("\nERROR: Referenced component name not found: ", c->copy_type, "\n\n");

                    nameref_error = true;
                }

                // reference the type and args of the copied component
                c->type = org_comp->type;
                c->args = org_comp->args;
            }
        }

        // Handle "COPY (copy_name) = ...":
        //      Meaning, we eliminate any COPY notes and coyp the .name.
        if (c->copy_name.len) {
            // We don't need to check that a component by name exists unless we are dealing with PREVIOUS.
            // We just need the index to put in the component name.
            u64 copy_name_index = MapGet(&map_cpys, c->copy_name);
            copy_name_index++;
            MapPut(&map_cpys, c->copy_name, (void*) copy_name_index);

            c->name = StrAlloc(c->copy_name.len + 4);
            StrCopy(c->copy_name, c->name);

            char subscript[4];
            sprintf(subscript, "_%lu", copy_name_index);
            strcat(c->name.str, subscript);
        }

        u64 comp_exists = MapGet(comps, c->type);
        if (comp_exists == 0) {
            type_error = true;
            stats->type_error_cnt++;

            if (dbg_print_missing_types) {
                printf("\n");
                printf("    Missing component type (idx %d): ", i);
                StrPrint(c->type);
            }
        }
    }

    instr->type_checked = ! type_error;
    instr->namerefs_checked = ! nameref_error;

    if ((instr->type_checked == true) && (instr->namerefs_checked == true)) { printf(" - OK"); }
    if ((instr->type_checked == false && dbg_print_missing_types == false)) { printf("\n    ERROR: Missing component types"); }
    if ((instr->namerefs_checked == false)) { printf("\n    ERROR: Component instance name reference"); }
    if ((type_error || nameref_error) && dbg_print_missing_types) { printf("\n"); } 
    printf("\n");

    return (! type_error) && (! nameref_error);
}


int main (int argc, char **argv) {
    TimeProgram;

    BaselayerAssertVersion(0, 2, 4);

    if (CLAContainsArg("--help", argc, argv) || CLAContainsArg("-h", argc, argv) || argc == 1) {
        printf("Usage:\n");
        printf("    mcparse [<lib-path> | --comps <comp-lib-path> --instrs <inst-lib-path>] [--cogen]\n");
        printf("\n");
        printf("Examples:\n");
        printf("    mcparse mcstas-comps\n");
        printf("    mcparse --comps mcstas-comps\n");
        printf("    mcparse --comps mcstas-comps --cogen\n");
        printf("\n");
        printf("Parameters:\n");
        printf("--help                  display help (this text)\n");
        printf("--comps                 component file or library path\n");
        printf("--instrs                instrument file or library path\n");
        printf("--cogen                 generate code\n");
        printf("--version               display mcparse version\n");
        printf("--test                  run enabled tests\n");
        printf("\n");
        exit(0);
    }

    else if (CLAContainsArg("--version", argc, argv)) {
        // any test code here
        printf("0.1.0\n");
    }

    else if (CLAContainsArg("--test", argc, argv)) {
        // any test code here
        printf("Running enabled tests ...\n");
    }

    else {
        bool do_cogen = false;
        if (CLAContainsArg("--cogen", argc, argv)) { do_cogen = true; }


        // get input
        char *comp_lib_path = NULL;
        char *instr_lib_path = NULL;
        if (CLAContainsArg("--comps", argc, argv) || CLAContainsArg("--comp", argc, argv)) {
            comp_lib_path = CLAGetArgValue("--comps", argc, argv);
        }
        if (CLAContainsArg("--instrs", argc, argv) || CLAContainsArg("--instr", argc, argv)) {
            instr_lib_path = CLAGetArgValue("--instrs", argc, argv);
        }
        if (CLAContainsArg("--instr", argc, argv)) {
            instr_lib_path = CLAGetArgValue("--instr", argc, argv);
        }
        if (argc > 1 && comp_lib_path == NULL && instr_lib_path == NULL) {
            comp_lib_path = argv[1];
            instr_lib_path = argv[1];
        }
        if (CLAContainsArg("--dbg", argc, argv)) {
            g_parse_error_causes_exit = true;
        }


        // init
        MContext *ctx = InitBaselayer();
        StrBuff buff = StrBuffInit();
        MapIter iter = {};


        // components
        HashMap comp_map = {};
        ParseStats comp_stats = {};
        if (comp_lib_path) {
            StrLst *comp_paths = GetFiles(comp_lib_path, "comp", true);
            comp_map = InitMap(ctx->a_life, StrListLen(comp_paths) * 3);
            comp_stats = ParseComponents(ctx->a_life, &comp_map, comp_paths);

            iter = {};
            while (ComponentParse *comp = (ComponentParse*) MapNextVal(&comp_map, &iter)) {

                // cogen components
                if (do_cogen) {
                    // print component names
                    StrPrint("Cogen: ", comp->type, " -> ");
                    StrBuffClear(&buff);
                    CogenComponent(&buff, comp);

                    Str f_safe = StrPathBuild(StrDirPath(comp->file_path), StrBasename(comp->file_path), StrL("h"));
                    StrPrint(f_safe);
                    printf("\n");

                    SaveFile(StrZ(f_safe), buff.str, buff.len);
                }
            }
            if (do_cogen) {
                printf("\n");
                StrBuffClear(&buff);
                CogenComponentMeta(&buff, &comp_map);

                // save component aggregate file
                Str dirpath = StrDirPath( StrL(comp_lib_path) );
                Str savefile = StrPathBuild(dirpath, StrL("comps_meta"), StrL("h"));
                StrPrint("Saving component meta file to: ", savefile, "\n\n");

                SaveFile(StrZ(savefile), buff.str, buff.len);
            }
        }


        // instruments
        HashMap instr_map = {};
        ParseStats instr_stats = {};
        if (instr_lib_path) {
            StrLst *instr_paths = GetFiles(instr_lib_path, "instr", true);
            instr_map = InitMap(ctx->a_life, StrListLen(instr_paths) * 3);
            instr_stats = ParseInstruments(ctx->a_life, &instr_map, instr_paths);
            printf("\n");

            // print instruments
            MArena a_tmp = ArenaCreate();
            iter = {};
            while (InstrumentParse *instr = (InstrumentParse*) MapNextVal(&instr_map, &iter)) {
                if (instr->parse_error == true) { continue; }

                CheckInstrument(&a_tmp, instr, &comp_map, &instr_stats, (comp_lib_path != NULL));

                if (CLAContainsArg("--print_instr", argc, argv)) {
                    InstrumentPrint(instr, true, true, true);
                }

                if (do_cogen) {
                    StrBuffClear(&buff);
                    CogenInstrumentConfig(&buff, instr);

                    // save instrument config file
                    Str dirpath = StrDirPath( StrL(instr_lib_path) );
                    Str basename = StrCat(instr->name, "_config");
                    Str savefile = StrPathBuild(dirpath, basename, StrL("h"));
                    StrPrint("Saving instument config file to: ", savefile, "\n");

                    SaveFile(StrZ(savefile), buff.str, buff.len);
                }
            }
        }
        printf("\n");

        // final reporting
        if (comp_lib_path) {
            printf("Component parse: %d total, %d parsed, %d errors [dupes: %d])\n",
                comp_stats.total_cnt, comp_stats.registered_cnt, comp_stats.parse_error_cnt, comp_stats.duplicate_cnt);
        }

        if (instr_lib_path) {
            printf("Instrument parse: %d total, %d parsed, %d errors, type-errs: %d [dupes: %d]\n",
                instr_stats.total_cnt, instr_stats.registered_cnt, instr_stats.parse_error_cnt, instr_stats.type_error_cnt, instr_stats.duplicate_cnt);
        }
        printf("\n");
    }
}
