#ifndef __COGENCOMP_H__
#define __COGENCOMP_H__


void PrintDefines(StrBuff *b, ComponentParse *comp) {
    for (s32 i = 0; i < comp->setting_params.len; ++i) {
        Parameter p = comp->setting_params.arr[i];
        StrBuffPrint1K(b, "    #define %.*s comp->%.*s\n", 4, p.name.len, p.name.str, p.name.len, p.name.str);
    }
    StrBuffPrint1K(b, "\n", 0);
    for (s32 i = 0; i < comp->declare_members.len; ++i) {
        StructMember m = comp->declare_members.arr[i];
        StrBuffPrint1K(b, "    #define %.*s comp->%.*s\n", 4, m.name.len, m.name.str, m.name.len, m.name.str);
    }

    // TODO: output, state, ..., params
}
void PrintUndefs(StrBuff *b, ComponentParse *comp) {
    for (s32 i = 0; i < comp->setting_params.len; ++i) {
        Parameter p = comp->setting_params.arr[i];
        StrBuffPrint1K(b, "    #undef %.*s\n", 2, p.name.len, p.name.str);
    }
    StrBuffPrint1K(b, "\n", 0);
    for (s32 i = 0; i < comp->declare_members.len; ++i) {
        StructMember m = comp->declare_members.arr[i];
        StrBuffPrint1K(b, "    #undef %.*s\n", 2, m.name.len, m.name.str);
    }

    // TODO: output, state, ..., params
}


void CogenComponent(StrBuff *b, ComponentParse *comp) {
    // header guard
    StrBuffPrint1K(b, "#ifndef __%.*s__\n", 2, comp->type.len, comp->type.str);
    StrBuffPrint1K(b, "#define __%.*s__\n", 2, comp->type.len, comp->type.str);
    StrBuffPrint1K(b, "\n\n", 0);

    //
    // share block

    StrBuffPrint1K(b, "// share block\n", 0);
    StrBuffPrint1K(b, "\n\n", 0);
    if (comp->share_block.len) {
        StrBuffAppend(b, comp->share_block);
        StrBuffPrint1K(b, "\n\n", 0);
    }

    //
    // component struct

    StrBuffPrint1K(b, "struct %.*s {\n", 2, comp->type.len, comp->type.str);
    StrBuffPrint1K(b, "    int index;\n", 0);
    StrBuffPrint1K(b, "    char *name;\n", 0);
    StrBuffPrint1K(b, "    char *type;\n", 0);
    StrBuffPrint1K(b, "    Coords position_absolute;\n", 0);
    StrBuffPrint1K(b, "    Coords position_relative;\n", 0);
    StrBuffPrint1K(b, "    Rotation rotation_absolute;\n", 0);
    StrBuffPrint1K(b, "    Rotation rotation_relative;\n", 0);
    StrBuffPrint1K(b, "\n    // parameters\n", 0);

    // struct parameters
    Str string_s { (char*) "string", 6 };
    Str vector_s { (char*) "vector", 6 };
    for (s32 i = 0; i < comp->setting_params.len; ++i) {
        Parameter p = comp->setting_params.arr[i];

        if (StrEqual(p.type, string_s)) {
            StrBuffPrint1K(b, "    char *", 0);
        }
        else if (StrEqual(p.type, vector_s)) {
            StrBuffPrint1K(b, "    double ", 0);
        }
        else if (p.type.len) {
            StrBuffPrint1K(b, "    %.*s ", 2, p.type.len, p.type.str);
        }
        else {
            StrBuffPrint1K(b, "    double ", 0);
        }

        if (StrEqual(p.type, vector_s)) {
            // just scan how many commas it has
            assert(p.default_val.len > 0);
            s32 cnt = 1;
            for (s32 k = 0; k < p.default_val.len; ++k) {
                if (p.default_val.str[k] == ',') {
                    ++cnt;
                }
            }
            StrBuffPrint1K(b, "%.*s[%d]", 3, p.name.len, p.name.str, cnt);
        }
        else {
            StrBuffPrint1K(b, "%.*s", 2, p.name.len, p.name.str);
        }

        if (p.default_val.len) {
            if (StrEqual(p.type, string_s)) {
                StrBuffPrint1K(b, " = (char*) %.*s", 2, p.default_val.len, p.default_val.str);
            }
            else {
                StrBuffPrint1K(b, " = %.*s", 2, p.default_val.len, p.default_val.str);
            }
        }
        StrBuffPrint1K(b, ";\n", 0);
    }

    // declare members
    StrBuffPrint1K(b, "\n    // declares\n", 0);
    for (s32 i = 0; i < comp->declare_members.len; ++i) {
        StructMember m = comp->declare_members.arr[i];

        StrBuffPrint1K(b, "    %.*s ", 2, m.type.len, m.type.str);
        if (m.is_pointer_type) {
            StrBuffPrint1K(b, "*", 0);
        }
        StrBuffPrint1K(b, "%.*s", 2, m.name.len, m.name.str);

        if (m.is_array_type) {
            StrBuffPrint1K(b, "[%d]", 1, m.array_type_sz);
        }

        if (m.defval.len) {
            StrBuffPrint1K(b, " = %.*s", 2, m.defval.len, m.defval.str);
        }
        StrBuffPrint1K(b, ";\n", 0);
    }
    StrBuffPrint1K(b, "};\n\n", 0);

    //
    //  Constructor

    StrBuffPrint1K(b, "%.*s Create_%.*s(s32 index, char *name) {\n", 4, comp->type.len, comp->type.str, comp->type.len, comp->type.str);
    StrBuffPrint1K(b, "    %.*s _comp = {};\n", 2, comp->type.len, comp->type.str);
    StrBuffPrint1K(b, "    %.*s *comp = &_comp;\n", 2, comp->type.len, comp->type.str);
    StrBuffPrint1K(b, "    comp->type = (char*) \"%.*s\";\n", 2, comp->type.len, comp->type.str);
    StrBuffPrint1K(b, "    comp->name = name;\n", 0);
    StrBuffPrint1K(b, "    comp->index = index;\n", 0);
    StrBuffPrint1K(b, "\n", 0);
    StrBuffPrint1K(b, "    return _comp;\n", 0);
    StrBuffPrint1K(b, "}\n\n", 0);

    StrBuffPrint1K(b, "int GetParameterCount_%.*s() {\n", 2, comp->type.len, comp->type.str);
    StrBuffPrint1K(b, "    return %u;\n", 1, comp->setting_params.len);
    StrBuffPrint1K(b, "}\n\n", 0);

    StrBuffPrint1K(b, "void GetParameters_%.*s(Array<Param> *pars, %.*s *comp) {\n", 4, comp->type.len, comp->type.str, comp->type.len, comp->type.str);
    for (s32 j = 0; j < comp->setting_params.len; ++j) {
        Parameter p = comp->setting_params.arr[j];

        if (StrEqual(p.type, "string")) {
            StrBuffPrint1K(b, "    pars->Add( Param { CPT_STRING, \"%.*s\", comp->%.*s } );\n", 4, p.name.len, p.name.str, p.name.len, p.name.str);
        }
        else if (StrEqual(p.type, "int")) {
            StrBuffPrint1K(b, "    pars->Add( Param { CPT_INT, \"%.*s\", &comp->%.*s } );\n", 4, p.name.len, p.name.str, p.name.len, p.name.str);
        }
        else {
            StrBuffPrint1K(b, "    pars->Add( Param { CPT_FLOAT, \"%.*s\", &comp->%.*s } );\n", 4, p.name.len, p.name.str, p.name.len, p.name.str);
        }
        // TODO: vectors
    }
    StrBuffPrint1K(b, "}\n\n", 0);

    //
    //  Init

    StrBuffPrint1K(b, "void Init_%.*s(%.*s *comp, Instrument *instrument) {\n", 4, comp->type.len, comp->type.str, comp->type.len, comp->type.str);
    StrBuffPrint1K(b, "\n", 0);
    if (comp->initalize_block.len) {
        PrintDefines(b, comp);
        StrBuffPrint1K(b, "    ////////////////////////////////////////////////////////////////\n\n", 0);

        StrBuffAppend(b, comp->initalize_block);

        StrBuffPrint1K(b, "\n\n    ////////////////////////////////////////////////////////////////\n", 0);
        PrintUndefs(b, comp);
        StrBuffPrint1K(b, "\n", 0);
    }
    StrBuffPrint1K(b, "}\n\n", 0);

    //
    //  Trace

    StrBuffPrint1K(b, "void Trace_%.*s(%.*s *comp, Neutron *particle, Instrument *instrument) {\n", 4, comp->type.len, comp->type.str, comp->type.len, comp->type.str);
    if (comp->trace_block.len) {
        StrBuffPrint1K(b, "    #define x particle->x\n", 0);
        StrBuffPrint1K(b, "    #define y particle->y\n", 0);
        StrBuffPrint1K(b, "    #define z particle->z\n", 0);
        StrBuffPrint1K(b, "    #define vx particle->vx\n", 0);
        StrBuffPrint1K(b, "    #define vy particle->vy\n", 0);
        StrBuffPrint1K(b, "    #define vz particle->vz\n", 0);
        StrBuffPrint1K(b, "    #define sx particle->sx\n", 0);
        StrBuffPrint1K(b, "    #define sy particle->sy\n", 0);
        StrBuffPrint1K(b, "    #define sz particle->sz\n", 0);
        StrBuffPrint1K(b, "    #define t particle->t\n", 0);
        StrBuffPrint1K(b, "    #define p particle->p\n", 0);

        StrBuffPrint1K(b, "\n", 0);
        PrintDefines(b, comp);
        StrBuffPrint1K(b, "    ////////////////////////////////////////////////////////////////\n\n", 0);

        StrBuffAppend(b, comp->trace_block);

        StrBuffPrint1K(b, "\n\n    ////////////////////////////////////////////////////////////////\n", 0);
        PrintUndefs(b, comp);
        StrBuffPrint1K(b, "\n", 0);

        StrBuffPrint1K(b, "    #undef x\n", 0);
        StrBuffPrint1K(b, "    #undef y\n", 0);
        StrBuffPrint1K(b, "    #undef z\n", 0);
        StrBuffPrint1K(b, "    #undef vx\n", 0);
        StrBuffPrint1K(b, "    #undef vy\n", 0);
        StrBuffPrint1K(b, "    #undef vz\n", 0);
        StrBuffPrint1K(b, "    #undef sx\n", 0);
        StrBuffPrint1K(b, "    #undef sy\n", 0);
        StrBuffPrint1K(b, "    #undef sz\n", 0);
        StrBuffPrint1K(b, "    #undef t\n", 0);
        StrBuffPrint1K(b, "    #undef p\n", 0);
    }
    StrBuffPrint1K(b, "}\n\n", 0);

    //
    //  Save

    StrBuffPrint1K(b, "void Save_%.*s(%.*s *comp) {\n", 4, comp->type.len, comp->type.str, comp->type.len, comp->type.str);
    StrBuffPrint1K(b, "\n", 0);
    if (comp->save_block.len) {
        PrintDefines(b, comp);
        StrBuffPrint1K(b, "    ////////////////////////////////////////////////////////////////\n\n", 0);

        StrBuffAppend(b, comp->save_block);

        StrBuffPrint1K(b, "\n\n    ////////////////////////////////////////////////////////////////\n", 0);
        PrintUndefs(b, comp);
    }
    StrBuffPrint1K(b, "}\n\n", 0);

    //
    //  Finally

    StrBuffPrint1K(b, "void Finally_%.*s(%.*s *comp) {\n", 4, comp->type.len, comp->type.str, comp->type.len, comp->type.str);
    StrBuffPrint1K(b, "\n", 0);
    if (comp->finally_block.len) {
        PrintDefines(b, comp);
        StrBuffPrint1K(b, "    ////////////////////////////////////////////////////////////////\n\n", 0);

        StrBuffAppend(b, comp->finally_block);

        StrBuffPrint1K(b, "\n\n    ////////////////////////////////////////////////////////////////\n", 0);
        PrintUndefs(b, comp);
    }
    StrBuffPrint1K(b, "}\n\n", 0);

    //
    //  Display

    StrBuffPrint1K(b, "void Display_%.*s(%.*s *comp) {\n", 4, comp->type.len, comp->type.str, comp->type.len, comp->type.str);
    if (comp->display_block.len) {
        StrBuffPrint1K(b, "    #define magnify mcdis_magnify\n", 0);
        StrBuffPrint1K(b, "    #define line mcdis_line\n", 0);
        StrBuffPrint1K(b, "    #define dashed_line mcdis_dashed_line\n", 0);
        StrBuffPrint1K(b, "    #define multiline mcdis_multiline\n", 0);
        StrBuffPrint1K(b, "    #define rectangle mcdis_rectangle\n", 0);
        StrBuffPrint1K(b, "    #define box mcdis_box\n", 0);
        StrBuffPrint1K(b, "    #define circle mcdis_circle\n", 0);
        StrBuffPrint1K(b, "    #define Circle mcdis_Circle\n", 0);
        StrBuffPrint1K(b, "    #define cylinder mcdis_cylinder\n", 0);
        StrBuffPrint1K(b, "    #define cone mcdis_cone\n", 0);
        StrBuffPrint1K(b, "    #define sphere mcdis_sphere\n", 0);

        StrBuffPrint1K(b, "\n", 0);
        PrintDefines(b, comp);
        StrBuffPrint1K(b, "    ////////////////////////////////////////////////////////////////\n\n", 0);

        StrBuffAppend(b, comp->display_block);

        StrBuffPrint1K(b, "\n\n    ////////////////////////////////////////////////////////////////\n", 0);
        PrintUndefs(b, comp);
        StrBuffPrint1K(b, "\n", 0);

        StrBuffPrint1K(b, "    #undef magnify\n", 0);
        StrBuffPrint1K(b, "    #undef line\n", 0);
        StrBuffPrint1K(b, "    #undef dashed_line\n", 0);
        StrBuffPrint1K(b, "    #undef multiline\n", 0);
        StrBuffPrint1K(b, "    #undef rectangle\n", 0);
        StrBuffPrint1K(b, "    #undef box\n", 0);
        StrBuffPrint1K(b, "    #undef circle\n", 0);
        StrBuffPrint1K(b, "    #undef Circle\n", 0);
        StrBuffPrint1K(b, "    #undef cylinder\n", 0);
        StrBuffPrint1K(b, "    #undef cone\n", 0);
        StrBuffPrint1K(b, "    #undef sphere\n", 0);
    }
    StrBuffPrint1K(b, "}\n\n\n", 0);


    // close header guard
    StrBuffPrint1K(b, "#endif\n", 0);
}


void CogenComponentMeta(StrBuff *b, HashMap *components) {
    StrBuffPrint1K(b, "#ifndef __COMPS_META___\n", 0);
    StrBuffPrint1K(b, "#define __COMPS_META___\n\n\n", 0);

    // include component sources
    MArena *a_tmp = GetContext()->a_tmp;
    u32 component_cnt = 0;
    MapIter iter = {};
    while (ComponentParse *comp = (ComponentParse*) MapNextVal(components, &iter)) {
        StrBuffPrint1K(b, "#include \"comps/%.*s.h\"\n", 2, comp->type.len, comp->type.str);

        component_cnt++;
    }
    StrBuffPrint1K(b, "\n\n", 0);

    // build a category map
    HashMap comp_categories_tmp = InitMap(a_tmp, component_cnt);

    // type enum
    StrBuffPrint1K(b, "enum CompType {\n", 0);
    StrBuffPrint1K(b, "    CT_UNDEF,\n\n", 0);
    iter = {};
    while (ComponentParse *comp = (ComponentParse*) MapNextVal(components, &iter)) {
        StrBuffPrint1K(b, "    CT_%.*s,\n", 2, comp->type.len, comp->type.str);

        Str cat = FindDirCategory(comp->file_path);
        MapPut(&comp_categories_tmp, cat, ArenaPush(a_tmp, &cat, sizeof(Str)));
    }
    StrBuffPrint1K(b, "\n    CT_CNT\n", 0);
    StrBuffPrint1K(b, "};\n\n\n", 0);

    // categories
    StrBuffPrint1K(b, "enum CompCategory {\n", 0);
    StrBuffPrint1K(b, "    CCAT_UNDEF,\n\n", 0);
    iter = {};
    while (Str *cat = (Str*) MapNextVal(&comp_categories_tmp, &iter)) {
        StrBuffPrint1K(b, "    CCAT_%.*s,\n", 2, cat->len, cat->str);
    }
    StrBuffPrint1K(b, "    \n", 0);
    StrBuffPrint1K(b, "    CCAT_CNT\n", 0);
    StrBuffPrint1K(b, "};\n\n\n", 0);

    StrBuffPrint1K(b, "Str StrLS(char *str) {\n", 0);
    StrBuffPrint1K(b, "    return Str { str, (u32) strlen(str) };\n", 0);
    StrBuffPrint1K(b, "}\n\n\n", 0);

    // create
    StrBuffPrint1K(b, "Component *CreateComponent(MArena *a_dest, CompType type, s32 index, const char *name) {\n", 0);
    StrBuffPrint1K(b, "    Component *comp = (Component*) ArenaAlloc(a_dest, sizeof(Component));\n", 0);
    StrBuffPrint1K(b, "    comp->type = type;\n", 0);
    StrBuffPrint1K(b, "\n", 0);
    StrBuffPrint1K(b, "    switch (type) {\n", 0);
    iter = {};
    while (ComponentParse *comp = (ComponentParse*) MapNextVal(components, &iter)) {
        StrBuffPrint1K(b, "        case CT_%.*s: {\n", 2, comp->type.len, comp->type.str);
        StrBuffPrint1K(b, "            %.*s comp_spec = Create_%.*s(index, (char*) name);\n", 6, comp->type.len, comp->type.str, comp->type.len, comp->type.str);
        StrBuffPrint1K(b, "            comp->comp = ArenaPush(a_dest, &comp_spec, sizeof(%.*s));\n", 2, comp->type.len, comp->type.str);
        StrBuffPrint1K(b, "            comp->type_name = StrLS(comp_spec.type);\n", 0);
        StrBuffPrint1K(b, "            comp->name = StrLS(comp_spec.name);\n", 0);
        StrBuffPrint1K(b, "            comp->cat = CCAT_%.*s;\n", 2, comp->category.len, comp->category.str);
        StrBuffPrint1K(b, "            comp->interactable = true;\n", 0);
        StrBuffPrint1K(b, "\n", 0);
        StrBuffPrint1K(b, "            comp->parameters = InitArray<Param>(a_dest, GetParameterCount_%.*s());\n", 2, comp->type.len, comp->type.str);
        StrBuffPrint1K(b, "            GetParameters_%.*s(&comp->parameters, (%.*s*) comp->comp);\n", 4, comp->type.len, comp->type.str, comp->type.len, comp->type.str);
        StrBuffPrint1K(b, "        } break;\n", 0);
        StrBuffPrint1K(b, "\n", 0);
    }
    StrBuffPrint1K(b, "        default: { } break;\n    }\n\n", 0);
    StrBuffPrint1K(b, "    return comp;\n}\n\n\n", 0);

    // init
    StrBuffPrint1K(b, "void InitComponent(Component *comp, Instrument *instr = NULL) {\n", 0);
    StrBuffPrint1K(b, "    switch (comp->type) {\n", 0);
    iter = {};
    while (ComponentParse *comp = (ComponentParse*) MapNextVal(components, &iter)) {
        StrBuffPrint1K(b, "        case CT_%.*s: { Init_%.*s((%.*s*) comp->comp, instr); } break;\n", 6, comp->type.len, comp->type.str, comp->type.len, comp->type.str, comp->type.len, comp->type.str);
    }
    StrBuffPrint1K(b, "\n", 0);
    StrBuffPrint1K(b, "        default: { } break;\n    }\n}\n\n\n", 0);

    // display
    StrBuffPrint1K(b, "void DisplayComponent(Component *comp) {\n", 0);
    StrBuffPrint1K(b, "    switch (comp->type) {\n", 0);
    iter = {};
    while (ComponentParse *comp = (ComponentParse*) MapNextVal(components, &iter)) {
        StrBuffPrint1K(b, "        case CT_%.*s: { Display_%.*s((%.*s*) comp->comp); } break;\n", 6, comp->type.len, comp->type.str, comp->type.len, comp->type.str, comp->type.len, comp->type.str);
    }
    StrBuffPrint1K(b, "\n", 0);
    StrBuffPrint1K(b, "        default: { } break;\n    }\n}\n\n\n", 0);

    // trace
    StrBuffPrint1K(b, "void TraceComponent(Component *comp, Neutron *particle, Instrument *instr = NULL) {\n", 0);
    StrBuffPrint1K(b, "    switch (comp->type) {\n", 0);
    iter = {};
    while (ComponentParse *comp = (ComponentParse*) MapNextVal(components, &iter)) {
        StrBuffPrint1K(b, "        case CT_%.*s: { Trace_%.*s((%.*s*) comp->comp, particle, instr); } break;\n", 6, comp->type.len, comp->type.str, comp->type.len, comp->type.str, comp->type.len, comp->type.str);
    }
    StrBuffPrint1K(b, "\n", 0);
    StrBuffPrint1K(b, "        default: { } break;\n    }\n}\n\n\n", 0);

    // save
    StrBuffPrint1K(b, "void SaveComponent(Component *comp) {\n", 0);
    StrBuffPrint1K(b, "    switch (comp->type) {\n", 0);
    iter = {};
    while (ComponentParse *comp = (ComponentParse*) MapNextVal(components, &iter)) {
        StrBuffPrint1K(b, "        case CT_%.*s: { Save_%.*s((%.*s*) comp->comp); } break;\n", 6, comp->type.len, comp->type.str, comp->type.len, comp->type.str, comp->type.len, comp->type.str);
    }
    StrBuffPrint1K(b, "\n", 0);
    StrBuffPrint1K(b, "        default: { } break;\n    }\n}\n\n\n", 0);

    // finally
    StrBuffPrint1K(b, "void FinallyComponent(Component *comp) {\n", 0);
    StrBuffPrint1K(b, "    switch (comp->type) {\n", 0);
    iter = {};
    while (ComponentParse *comp = (ComponentParse*) MapNextVal(components, &iter)) {
        StrBuffPrint1K(b, "        case CT_%.*s: { Finally_%.*s((%.*s*) comp->comp); } break;\n", 6, comp->type.len, comp->type.str, comp->type.len, comp->type.str, comp->type.len, comp->type.str);
    }
    StrBuffPrint1K(b, "\n", 0);
    StrBuffPrint1K(b, "        default: { } break;\n    }\n}\n\n\n", 0);

    // close header guard
    StrBuffPrint1K(b, "#endif // __META_COMPS__\n", 0);
}



#endif
