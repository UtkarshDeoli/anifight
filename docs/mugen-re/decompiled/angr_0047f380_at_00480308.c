// contains 0x480308; entry=0x47f380
typedef struct st_47f380_2 {
    char padding_0[184];
    unsigned int field_b8;
} st_47f380_2;

typedef struct st_47f380_3 {
    struct st_47f380_2 *field_0;
    int field_4;
    char padding_8[3140];
    int field_c4c;
} st_47f380_3;

typedef struct st_47f380_0 {
    char padding_0[8];
    unsigned int field_8;
    unsigned int field_c;
    char padding_10[4];
    int field_14;
    int field_18;
    int field_1c;
    int field_20;
    unsigned int field_24;
    unsigned int field_28;
    unsigned int field_2c;
    unsigned int field_30;
    int field_34;
    unsigned int field_38;
    unsigned int field_3c;
    unsigned int field_40;
    unsigned int field_44;
    unsigned int field_48;
    unsigned int field_4c;
    int field_50;
    int field_54;
    unsigned int field_58;
    int field_5c;
    int field_60;
    unsigned int field_64;
    int field_68;
    int field_6c;
    unsigned int field_70;
    unsigned int field_74;
    unsigned int field_78;
    int field_7c;
    unsigned int field_80;
    int field_84;
    int field_88;
    unsigned int field_8c;
    char padding_90[20];
    int field_a4;
    unsigned int field_a8;
    unsigned int field_ac;
    unsigned int field_b0;
    unsigned int field_b4;
    unsigned int field_b8;
    unsigned int field_bc;
    unsigned int field_c0;
    unsigned int field_c4;
    float field_c8;
    float field_cc;
    unsigned int field_d0;
    int field_d4;
    char padding_d8[24];
    int field_f0;
    unsigned int field_f4;
    int field_f8;
    unsigned int field_fc;
    unsigned int field_100;
    unsigned int field_104;
    unsigned int field_108;
    int field_10c;
    unsigned int field_110;
    unsigned int field_114;
    unsigned int field_118;
    unsigned int field_11c;
    int field_120;
    unsigned int field_124;
    int field_128;
    int field_12c;
    float field_130;
    unsigned int field_134;
    unsigned int field_138;
    unsigned int field_13c;
    unsigned int field_140;
    int field_144;
    unsigned int field_148;
    float field_14c;
    float field_150;
    float field_154;
    unsigned int field_158;
    int field_15c;
    float field_160;
    float field_164;
    float field_168;
    float field_16c;
    int field_170;
    int field_174;
    int field_178;
    unsigned int field_17c;
    int field_180;
    int field_184;
    float field_188;
    float field_18c;
    float field_190;
    int field_194;
    float field_198;
    float field_19c;
} st_47f380_0;

typedef struct st_47f380_1 {
    char padding_0[69288];
    int field_10ea8;
} st_47f380_1;

extern st_47f380_1 *g_52b40c;

unsigned int sub_47f380(unsigned int *a0, st_47f380_0 *a1, st_47f380_3 *a2)
{
    int v4;  // eax
    int v5;  // eax
    int v14;  // eax
    int v15;  // eax
    int v16;  // eax
    int v17;  // eax
    int v18;  // eax
    unsigned int v19;  // eax
    int v20;  // eax
    double v21;  // mm7
    int v22;  // eax
    int v23;  // eax
    int v6;  // eax
    int v24;  // eax
    int v25;  // eax
    int v26;  // eax
    int v27;  // eax
    int v28;  // eax
    float v29;  // 128
    int v30;  // eax
    double v31;  // mm0
    int v32;  // eax
    int v33;  // eax
    int v7;  // eax
    int v34;  // eax
    int v35;  // eax
    int v36;  // eax
    int v37;  // eax
    int v38;  // eax
    int v8;  // eax
    int v9;  // eax
    int v10;  // eax
    int v11;  // eax
    int v12;  // eax
    int v13;  // eax
    int v0;  // [bp-0x10]
    int v1;  // [bp-0xc]
    int v2;  // [bp-0x8]
    int v3;  // [bp-0x4]

    a1->field_8 = *a0;
    a1->field_c = a0[1];
    a1->field_bc = a0[0x66];
    v4 = sub_439260(1, v0, v1, v2, v3);
    a1->field_14 = v4;
    if (v4 < 0)
    {
        a1->field_14 = 0;
        if (a2)
            sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "assigned negative hitID");
    }
    v5 = sub_439260(1);
    a1->field_18 = v5;
    if (v5 < -0x1)
    {
        a1->field_18 = -0x1;
        if (a2)
            sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "specified negative chainID");
    }
    v6 = sub_439260(1);
    a1->field_1c = v6;
    if (v6 < -0x1)
    {
        a1->field_1c = -0x1;
        if (a2)
            sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "specified negative nochainID");
    }
    v7 = sub_439260(1);
    a1->field_20 = v7;
    if (v7 < -0x1)
    {
        a1->field_20 = -0x1;
        if (a2)
            sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "specified negative nochainID2");
    }
    v8 = a1->field_18;
    if ((v8 == a1->field_1c || v8 == a1->field_20) && v8 != -0x1)
        memset(&a1->field_18, -0x1, 12);
    a1->field_24 = -(-(0 < sub_439260(1)));
    a1->field_28 = -(-(0 < sub_439260(1)));
    a1->field_2c = -(-(0 < sub_439260(1)));
    a1->field_30 = -(-(0 < sub_439260(1)));
    v9 = sub_439260(1);
    a1->field_34 = v9;
    if (v9 < -0x1)
    {
        a1->field_34 = -0x1;
        if (a2)
            sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "illegal juggle points");
    }
    a1->field_38 = sub_439260(1);
    a1->field_3c = sub_439260(1);
    a1->field_40 = sub_439260(0);
    a1->field_44 = sub_439260(0);
    a1->field_48 = sub_439260(0);
    a1->field_4c = sub_439260(0);
    v10 = sub_439260(1);
    a1->field_50 = v10;
    if (v10 < 0)
    {
        a1->field_50 = 0;
        if (a2)
            sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "specified negative p2shaketime");
    }
    v11 = sub_439260(1);
    a1->field_54 = v11;
    if (v11 < 0)
    {
        a1->field_54 = 0;
        if (a2)
            sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "specified negative pausetime");
    }
    if (a2->field_0->field_b8 < (unsigned int)*(int *)&g_52b40c->padding_0[1032] && a1->field_54 > 0 && !a1->field_bc)
        a1->field_54 = a1->field_54 - 1;
    a1->field_58 = sub_439260(1);
    v12 = sub_439260(1);
    a1->field_5c = v12;
    if (v12 < -0x1)
    {
        a1->field_5c = -0x1;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "specified illegal sound group");
    }
    v13 = sub_439260(1);
    a1->field_60 = v13;
    if (v13 < -0x1)
    {
        a1->field_60 = -0x1;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "specified illegal sound no");
    }
    a1->field_64 = a0[62];
    v14 = sub_439260(1);
    a1->field_68 = v14;
    if (v14 < -0x1)
    {
        a1->field_68 = -0x1;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "specified illegal gsound group");
    }
    v15 = sub_439260(1);
    a1->field_6c = v15;
    if (v15 < -0x1)
    {
        a1->field_6c = -0x1;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "specified illegal gsound no");
    }
    a1->field_70 = a0[69];
    a1->field_74 = a0[70];
    a1->field_78 = a0[71];
    v16 = sub_439260(1);
    a1->field_7c = v16;
    if (v16 < 0)
    {
        a1->field_7c = 0;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "specified negative hit priority");
    }
    a1->field_80 = a0[75];
    v17 = sub_439260(1);
    a1->field_84 = v17;
    if (v17 < -0x1)
    {
        a1->field_84 = -0x1;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "specified negative p1stateno");
    }
    v18 = sub_439260(1);
    a1->field_88 = v18;
    if (v18 < -0x1)
    {
        a1->field_88 = -0x1;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "specified negative p2stateno");
    }
    v19 = -(0 < sub_439260(1));
    a1->field_8c = -v19;
    if (v19)
        sub_48bc00();
    v20 = sub_439260(1);
    a1->field_a4 = v20;
    if (v20 < -0x1)
    {
        a1->field_a4 = 1;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "specified illegal p2bindtime");
    }
    a1->field_a8 = sub_439260(1);
    a1->field_ac = sub_439260(1);
    a1->field_b0 = a0[94];
    a1->field_b4 = -(-(0 < sub_439260(1)));
    a1->field_b8 = -(-(0 < sub_439260(1)));
    if (sub_439260(1))
        a1->field_78 = a1->field_78 | 1;
    a1->field_c0 = sub_439260(1);
    a1->field_c4 = a0[106];
    sub_439330();
    a1->field_c8 = (float)v21;
    sub_439330();
    a1->field_cc = (float)v21;
    a1->field_d0 = -(-(0 < sub_439260(1)));
    v22 = sub_439260(1);
    a1->field_d4 = v22;
    if (v22 < 0)
    {
        a1->field_d4 = 4;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "specified negative recovertime");
    }
    a1->field_f4 = a0[141];
    v23 = sub_439260(1);
    a1->field_f0 = v23;
    if (v23 < -0x2)
    {
        a1->field_f0 = -0x2;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "illegal spark no");
    }
    a1->field_fc = a0[145];
    v24 = sub_439260(1);
    a1->field_f8 = v24;
    if (v24 < -0x2)
    {
        a1->field_f8 = -0x2;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "illegal gspark no");
    }
    a1->field_100 = sub_439260(1);
    a1->field_104 = sub_439260(1);
    a1->field_108 = (int)sub_439260(1) < 0;
    v25 = sub_439260(1);
    if (v25 < 0)
    {
        v25 = -0x1;
    }
    else if (v25 > 0)
    {
        v25 = 1;
    }
    else if (!v25)
    {
        goto LABEL_47fce1;
    }
    a1->field_108 = 0;
LABEL_47fce1:
    a1->field_10c = v25;
    a1->field_110 = sub_439260(1);
    a1->field_114 = sub_439260(1);
    a1->field_118 = sub_439260(1);
    a1->field_11c = sub_439260(1);
    v26 = sub_439260(1);
    a1->field_120 = v26;
    if (v26 < -0x1 || v26 > 1)
    {
        a1->field_120 = 0;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "Illegal p2facing parameter");
    }
    a1->field_124 = a0[173];
    v27 = sub_439260(1);
    a1->field_128 = v27;
    if (v27 < 0)
    {
        a1->field_128 = 0;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "negative ground.p2hittime");
    }
    v28 = sub_439260(1);
    a1->field_12c = v28;
    if (v28 < 0)
    {
        a1->field_12c = 0;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "negative ground.p2slidetime");
    }
    sub_439330();
    a1->field_130 = (float)v21;
    sub_439330();
    v29 = (float)v21;
    *(float *)&a1->field_134 = v29;
    if (v29 != 0.0f && !a0[101])
        a1->field_b4 = 1;
    sub_439330();
    *(float *)&a1->field_138 = v29;
    a1->field_13c = a0[189];
    a1->field_140 = a0[190];
    v30 = sub_439260(1);
    a1->field_144 = v30;
    if (v30 < 0)
    {
        a1->field_144 = 0;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "negative air.p2hittime");
    }
    sub_439330();
    a1->field_148 = sub_499690();
    sub_439330();
    a1->field_14c = (float)v31;
    sub_439330();
    a1->field_150 = (float)v31;
    sub_439330();
    a1->field_154 = (float)v31;
    a1->field_158 = -(-(0 < sub_439260(1)));
    v32 = sub_439260(1);
    a1->field_15c = v32;
    if (v32 < 0)
    {
        a1->field_15c = 0;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "negative down.p2hittime");
    }
    sub_439330();
    a1->field_160 = (float)v31;
    sub_439330();
    a1->field_164 = (float)v31;
    sub_439330();
    a1->field_168 = (float)v31;
    sub_439330();
    a1->field_16c = (float)v31;
    v33 = sub_439260(1);
    a1->field_170 = v33;
    if (v33 < 0)
    {
        a1->field_170 = 0;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "negative guard.p2hittime");
    }
    v34 = sub_439260(1);
    a1->field_174 = v34;
    if (v34 < 0)
    {
        a1->field_174 = 0;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "negative guard.p2slidetime");
    }
    v35 = sub_439260(1);
    a1->field_178 = v35;
    if (v35 < 0)
    {
        a1->field_178 = 0;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "negative guard.p2ctrltime");
    }
    a1->field_17c = sub_439260(1);
    v36 = sub_439260(1);
    a1->field_180 = v36;
    if (v36 < 0)
    {
        a1->field_180 = 0;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "negative guard.p2shaketime");
    }
    v37 = sub_439260(1);
    a1->field_184 = v37;
    if (v37 < 0)
    {
        a1->field_184 = 0;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "negative guard.p1pausetime");
    }
    if (a2->field_0->field_b8 < (unsigned int)*(int *)&g_52b40c->padding_0[1032] && a1->field_184 > 0)
        a1->field_184 = a1->field_184 - 1;
    sub_439330();
    a1->field_188 = (float)v31;
    sub_439330();
    a1->field_18c = (float)v31;
    sub_439330();
    a1->field_190 = (float)v31;
    v38 = sub_439260(1);
    a1->field_194 = v38;
    if (v38 < 0)
    {
        a1->field_194 = 0;
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, &a2->padding_8[24], a2->field_4, a2->field_c4c, "negative airguard.p2ctrltime");
    }
    sub_439330();
    a1->field_198 = (float)v31;
    sub_439330();
    a1->field_19c = (float)v31;
    sub_47d050(a0 + 260);
    sub_47c440();
    sub_47c440();
    return 1;
}
