#ifndef __COGEN_INSTR_H__
#define __COGEN_INSTR_H__


void PrintDefines(StrBuff *b, InstrumentParse *instr) {
    for (s32 i = 0; i < instr->params.len; ++i) {
        Parameter p = instr->params.arr[i];
        StrBuffPrint1K(b, "    #define %.*s spec->%.*s\n", 4, p.name.len, p.name.str, p.name.len, p.name.str);
    }
    StrBuffPrint1K(b, "\n", 0);
    for (s32 i = 0; i < instr->declare_members.len; ++i) {
        StructMember m = instr->declare_members.arr[i];
        StrBuffPrint1K(b, "    #define %.*s spec->%.*s\n", 4, m.name.len, m.name.str, m.name.len, m.name.str);
    }
}

void PrintUndefs(StrBuff *b, InstrumentParse *instr) {
    for (s32 i = 0; i < instr->params.len; ++i) {
        Parameter p = instr->params.arr[i];
        StrBuffPrint1K(b, "    #undef %.*s\n", 2, p.name.len, p.name.str);
    }
    StrBuffPrint1K(b, "\n", 0);
    for (s32 i = 0; i < instr->declare_members.len; ++i) {
        StructMember m = instr->declare_members.arr[i];
        StrBuffPrint1K(b, "    #undef %.*s\n", 2, m.name.len, m.name.str);
    }
}

void AmendIdentifiesInRValue(Str *value) {
    Tokenizer t = {};
    t.Init(value->str);
    char *t_at0 = t.at;

    while (true) {
        Token tok = GetToken(&t);
        if (tok.type == TOK_IDENTIFIER) {
            Str identifier = tok.GetValue();
            Str amend = StrCat(StrL("spec->"), identifier);
            *value = StrInsertReplace(*value, amend, identifier);

            s32 t_advance = t.at - t_at0;
            t.at = value->str;
            t_at0 = t.at;
            t.at += t_advance + 6;
        }

        if (t.at >= (value->str + value->len)) {
            break;
        }
    }
}


void CogenInstrumentConfig(StrBuff *b, InstrumentParse *instr) {
    // header guard
    StrBuffPrint1K(b, "#ifndef __%.*s__\n", 2, instr->name.len, instr->name.str);
    StrBuffPrint1K(b, "#define __%.*s__\n", 2, instr->name.len, instr->name.str);
    StrBuffPrint1K(b, "\n\n", 0);

    // struct
    StrBuffPrint1K(b, "struct %.*s {\n", 2, instr->name.len, instr->name.str);
    StrBuffPrint1K(b, "\n    // parameters\n", 0);

    // parameters
    Str string_s { (char*) "string", 6 };
    Str vector_s { (char*) "vector", 6 };
    for (s32 i = 0; i < instr->params.len; ++i) {
        Parameter p = instr->params.arr[i];

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
    for (s32 i = 0; i < instr->declare_members.len; ++i) {
        StructMember m = instr->declare_members.arr[i];

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
    StrBuffPrint1K(b, "};\n\n\n", 0);


    // parameter access
    StrBuffPrint1K(b, "int GetParameterCount_%.*s() {\n", 2, instr->name.len, instr->name.str);
    StrBuffPrint1K(b, "    return %u;\n", 2, instr->params.len);
    StrBuffPrint1K(b, "}\n", 0);
    StrBuffPrint1K(b, "void GetParameters_%.*s(Array<Param> *pars, %.*s *instr) {\n", 4, instr->name.len, instr->name.str, instr->name.len, instr->name.str);
    for (s32 j = 0; j < instr->params.len; ++j) {
        Parameter p = instr->params.arr[j];

        if (StrEqual(p.type, "string")) {
            StrBuffPrint1K(b, "    pars->Add( Param { CPT_STRING, \"%.*s\", instr->%.*s } );\n", 4, p.name.len, p.name.str, p.name.len, p.name.str);
        }
        else {
            StrBuffPrint1K(b, "    pars->Add( Param { CPT_FLOAT, \"%.*s\", &instr->%.*s } );\n", 4, p.name.len, p.name.str, p.name.len, p.name.str);
        }
        // TODO: vectors
    }
    StrBuffPrint1K(b, "}\n\n", 0);


    // signature
    StrBuffPrint1K(b, "InstrumentConfig InitAndConfig_%.*s(MArena *a_dest, u32 ncount) {\n", 2, instr->name.len, instr->name.str);
    StrBuffPrint1K(b, "    %.*s _spec = {};\n", 2, instr->name.len, instr->name.str);
    StrBuffPrint1K(b, "    %.*s *spec = (%.*s*) ArenaPush(a_dest, &_spec, sizeof(%.*s));\n", 6, instr->name.len, instr->name.str, instr->name.len, instr->name.str, instr->name.len, instr->name.str);
    StrBuffPrint1K(b, "\n\n    // initialize\n\n\n", 0);


    // init instrument
    if (instr->initalize_block.len) {
        PrintDefines(b, instr);
        StrBuffPrint1K(b, "    ////////////////////////////////////////////////////////////////\n\n", 0);

        StrBuffAppend(b, instr->initalize_block);

        StrBuffPrint1K(b, "\n\n    ////////////////////////////////////////////////////////////////\n", 0);
        PrintUndefs(b, instr);
    }
    StrBuffPrint1K(b, "\n\n", 0);


    // "trace" e.g. configure & initialize components
    StrBuffPrint1K(b, "    // configuration pre-amble\n\n\n", 0);
    StrBuffPrint1K(b, "    InstrumentConfig config = {};\n", 0);
    StrBuffPrint1K(b, "    config.scenegraph = SceneGraphInit(cbui.ctx->a_pers);\n", 0);
    StrBuffPrint1K(b, "    Instrument *instr = &config.instr;\n", 0);
    StrBuffPrint1K(b, "    instr->ncount_target = ncount;\n", 0);
    StrBuffPrint1K(b, "    SceneGraphHandle *sg = &config.scenegraph;\n", 0);
    StrBuffPrint1K(b, "\n", 0);
    StrBuffPrint1K(b, "    instr->name = (char*) \"%.*s\";\n", 2, instr->name.len, instr->name.str);
    StrBuffPrint1K(b, "    config.comps = InitArray<Component*>(a_dest, 32);\n", 0);
    StrBuffPrint1K(b, "    f32 at_x, at_y, at_z;\n", 0);
    StrBuffPrint1K(b, "    f32 phi_x, phi_y, phi_z;\n", 0);
    StrBuffPrint1K(b, "    s32 index = 0;\n", 0);
    StrBuffPrint1K(b, "\n\n", 0);
    StrBuffPrint1K(b, "    // configure components\n\n\n", 0);

    for (s32 i = 0; i < instr->comps.len; ++i) {
        ComponentCall c = instr->comps.arr[i];

        StrBuffPrint1K(b, "    Component *%.*s = CreateComponent(a_dest, CT_%.*s, index++, \"%.*s\");\n", 6, c.name.len, c.name.str, c.type.len, c.type.str, c.name.len, c.name.str);
        StrBuffPrint1K(b, "    config.comps.Add(%.*s);\n", 2, c.name.len, c.name.str);
        StrBuffPrint1K(b, "    %.*s *%.*s_comp = (%.*s*) %.*s->comp;\n", 8, c.type.len, c.type.str, c.name.len, c.name.str, c.type.len, c.type.str, c.name.len, c.name.str);


        // TODO: move to dedicated code section
        // amend parameter asignments
        for (s32 j = 0; j < c.args.len; ++j) {
            Parameter *p = c.args.arr + j;

            // put "spec->" in front of identifiers in these rvalues
            AmendIdentifiesInRValue(&p->default_val);
        }

        for (s32 j = 0; j < c.args.len; ++j) {
            Parameter p = c.args.arr[j];
            if (p.default_val.len && p.default_val.str[0] == '"') {
                StrBuffPrint1K(b, "    %.*s_comp->%.*s = (char*) %.*s;\n", 6, c.name.len, c.name.str, p.name.len, p.name.str, p.default_val.len, p.default_val.str);
            }
            else {
                StrBuffPrint1K(b, "    %.*s_comp->%.*s = %.*s;\n", 6, c.name.len, c.name.str, p.name.len, p.name.str, p.default_val.len, p.default_val.str);
            }
        }
        StrBuffPrint1K(b, "    Init_%.*s(%.*s_comp, instr);\n", 4, c.type.len, c.type.str, c.name.len, c.name.str);


        // pre-fix instrument variable use in AT/ROT with "spec->" which makes those available in generated code
        AmendIdentifiesInRValue(&c.at_x);
        AmendIdentifiesInRValue(&c.at_y);
        AmendIdentifiesInRValue(&c.at_z);
        if (c.rot_defined) {

            // TODO: move to dedicated code section
            AmendIdentifiesInRValue(&c.rot_x);
            AmendIdentifiesInRValue(&c.rot_y);
            AmendIdentifiesInRValue(&c.rot_z);
        }

        // eliminate any use of the PREVIOUS keyword
        if ( StrEqual(c.at_relative_to, StrL("PREVIOUS")) ) {
            assert(i > 0);
            c.at_relative_to = instr->comps.arr[i-1].name;
        }
        if ( StrEqual(c.rot_relative_to, StrL("PREVIOUS")) ) {
            assert(i > 0);
            c.rot_relative_to = instr->comps.arr[i-1].name;
        }

        // NOTE: The ABSOLUTE is handled inline using the flags at_absolute and rot_absolute
        if (c.rot_defined == false) {

            // only AT is defined
            StrBuffPrint1K(b, "    // case #1:      Only AT is defined\n", 0);
            StrBuffPrint1K(b, "    // AT:  (%.*s, %.*s, %.*s) RELATIVE %.*s\n", 8, c.at_x.len, c.at_x.str, c.at_y.len, c.at_y.str, c.at_z.len, c.at_z.str, c.at_relative_to.len, c.at_relative_to.str);

            StrBuffPrint1K(b, "    at_x = %.*s;\n", 2, c.at_x.len, c.at_x.str);
            StrBuffPrint1K(b, "    at_y = %.*s;\n", 2, c.at_y.len, c.at_y.str);
            StrBuffPrint1K(b, "    at_z = %.*s;\n", 2, c.at_z.len, c.at_z.str);

            if (c.at_absolute) {
                StrBuffPrint1K(b, "    %.*s->transform = SceneGraphAlloc(sg);\n", 2, c.name.len, c.name.str);
            }
            else {
                StrBuffPrint1K(b, "    %.*s->transform = SceneGraphAlloc(sg, %.*s->transform);\n", 4, c.name.len, c.name.str, c.at_relative_to.len, c.at_relative_to.str);
            }
            StrBuffPrint1K(b, "    %.*s->transform->t_loc = TransformBuildTranslation( { at_x, at_y, at_z } );\n", 2, c.name.len, c.name.str);
        }

        else if (c.rot_defined) {

            bool same_at_rot_relative = StrEqual(c.at_relative_to, c.rot_relative_to);
            if (same_at_rot_relative) {
                StrBuffPrint1K(b, "    // case #2:      AT and ROT are defined RELATIVE to the same parent (defined through SceneGraphAlloc)\n", 0);
            }
            else {
                StrBuffPrint1K(b, "    // case #3:      AT and ROT are defined RELATIVE to different parents (SceneGraphAlloc defines the AT-parent, SceneGraphSetRotParent defines the ROT-parent)\n", 0);
            }

            StrBuffPrint1K(b, "    // AT:  (%.*s, %.*s, %.*s) RELATIVE %.*s\n", 8, c.at_x.len, c.at_x.str, c.at_y.len, c.at_y.str, c.at_z.len, c.at_z.str, c.at_relative_to.len, c.at_relative_to.str);
            StrBuffPrint1K(b, "    // ROT: (%.*s, %.*s, %.*s) RELATIVE %.*s\n", 8, c.rot_x.len, c.rot_x.str, c.rot_y.len, c.rot_y.str, c.rot_z.len, c.rot_z.str, c.rot_relative_to.len, c.rot_relative_to.str);

            StrBuffPrint1K(b, "    at_x = %.*s;\n", 2, c.at_x.len, c.at_x.str);
            StrBuffPrint1K(b, "    at_y = %.*s;\n", 2, c.at_y.len, c.at_y.str);
            StrBuffPrint1K(b, "    at_z = %.*s;\n", 2, c.at_z.len, c.at_z.str);
            StrBuffPrint1K(b, "    phi_x = %.*s;\n", 2, c.rot_x.len, c.rot_x.str);
            StrBuffPrint1K(b, "    phi_y = %.*s;\n", 2, c.rot_y.len, c.rot_y.str);
            StrBuffPrint1K(b, "    phi_z = %.*s;\n", 2, c.rot_z.len, c.rot_z.str);

            if (c.at_absolute) {
                StrBuffPrint1K(b, "    %.*s->transform = SceneGraphAlloc(sg);\n", 2, c.name.len, c.name.str);
            }
            else {
                StrBuffPrint1K(b, "    %.*s->transform = SceneGraphAlloc(sg, %.*s->transform);\n", 4, c.name.len, c.name.str, c.at_relative_to.len, c.at_relative_to.str);
            }
            StrBuffPrint1K(b, "    %.*s->transform->t_loc = TransformBuildTranslation( { at_x, at_y, at_z } ) * TransformBuildRotateZ( phi_z * deg2rad ) * TransformBuildRotateY( phi_y * deg2rad ) * TransformBuildRotateX( phi_x * deg2rad );\n", 2, c.name.len, c.name.str);

            // amend for case #3
            if (same_at_rot_relative == false) {
                StrBuffPrint1K(b, "    SceneGraphSetRotParent(sg, %.*s->transform, %.*s->transform);\n", 4, c.name.len, c.name.str, c.rot_relative_to.len, c.rot_relative_to.str);
            }
        }
        StrBuffPrint1K(b, "\n", 0);
    }
    StrBuffPrint1K(b, "    SceneGraphUpdate(sg);\n", 0);
    StrBuffPrint1K(b, "    UpdateLegacyTransforms(config.comps);\n", 0);
    StrBuffPrint1K(b, "\n", 0);
    StrBuffPrint1K(b, "    instr->parameters = InitArray<Param>(a_dest, GetParameterCount_%.*s());\n", 2, instr->name.len, instr->name.str);
    StrBuffPrint1K(b, "    GetParameters_%.*s(&instr->parameters, (%.*s*) instr);\n", 4, instr->name.len, instr->name.str, instr->name.len, instr->name.str);
    StrBuffPrint1K(b, "\n", 0);
    StrBuffPrint1K(b, "    return config;\n", 0);
    StrBuffPrint1K(b, "}\n\n\n", 0);


    // TODO: cogen FINALLY section


    // close header guard
    StrBuffPrint1K(b, "#endif // %.*s\n", 2, instr->name.len, instr->name.str);
}


#endif
