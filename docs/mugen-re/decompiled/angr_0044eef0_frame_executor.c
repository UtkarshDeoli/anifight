// frame_executor entry=0x44eef0
typedef struct st_44eef0_4 {
    char padding_0[8];
    unsigned int field_8;
    unsigned int field_c;
} st_44eef0_4;

typedef struct st_44eef0_1 {
    char padding_0[4];
    unsigned int field_4;
    char padding_8[16];
    unsigned int field_18;
    unsigned int field_1c;
} st_44eef0_1;

typedef struct st_44eef0_2 {
    struct st_44eef0_3 *field_0;
    char padding_4[8];
    unsigned int field_c;
    struct st_44eef0_4 *field_10;
    unsigned int field_14;
    char padding_18[4];
    unsigned int field_1c;
} st_44eef0_2;

typedef struct st_44eef0_3 {
    struct st_44eef0_1 *field_0;
} st_44eef0_3;

typedef struct st_44eef0_5 {
    char padding_0[432];
    unsigned int field_1b0;
    char padding_1b4[2692];
    unsigned int field_c38;
    unsigned int field_c3c;
    char padding_c40[8];
    unsigned int field_c48;
    unsigned int field_c4c;
    char padding_c50[516];
    unsigned int field_e54;
    unsigned int field_e58;
    unsigned int field_e5c;
    char padding_e60[4];
    unsigned int field_e64;
    char padding_e68[48];
    unsigned int field_e98;
    char padding_e9c[1404];
    struct st_44eef0_2 *field_1418;
    char padding_141c[3728];
    unsigned int field_22ac;
} st_44eef0_5;

typedef struct st_44eef0_0 {
    char padding_0[32980];
    unsigned int field_80d4;
    char padding_80d8[36304];
    unsigned int field_10ea8;
    unsigned int field_10eac;
    char padding_10eb0[5548];
    unsigned int field_1245c;
} st_44eef0_0;

extern st_44eef0_0 *g_52b40c;
extern unsigned int g_52be24;

void sub_44eef0(unsigned int a0)
{
    st_44eef0_5 *v17;  // esi
    double v18;  // mm7
    char *v27;  // eax
    char *v28;  // eax
    char *v29;  // eax
    unsigned int v30;  // ebp
    int v31;  // 128
    char *v32;  // eax
    char *v33;  // eax
    char *v34;  // eax
    unsigned int v35;  // ebp
    int v36;  // 128
    double v19;  // mm7
    unsigned int v20;  // eax
    unsigned int v21;  // ecx
    char *v22;  // eax
    char *v23;  // eax
    char *v24;  // eax
    unsigned int *v25;  // eax
    st_44eef0_2 *v26;  // eax
    unsigned int v0;  // [bp-0x174], Other Possible Types: float
    unsigned int v1;  // [bp-0x170], Other Possible Types: float
    unsigned int v2;  // [bp-0x16c]
    unsigned int v3;  // [bp-0x168]
    unsigned int v4;  // [bp-0x164]
    unsigned int v5;  // [bp-0x160]
    float v6;  // [bp-0x160]
    unsigned int v7;  // [bp-0x15c]
    int v8;  // [bp-0x158]
    int v9;  // [bp-0x154]
    int v10;  // [bp-0x150]
    int v11;  // [bp-0x14c]
    float v12;  // [bp-0x148]
    unsigned int v13;  // [bp-0x148]
    unsigned int v14;  // [bp-0x144]
    char v15[256];  // [bp-0x140]
    char v16[64];  // [bp-0x40]

    v17 = *(int *)&g_52b40c->padding_10eb0[896 + 4 * a0];
    if (!a0)
        return;
    v14 = g_52be24 - 39;
    sub_437470(v8, v9, v10, v11);
    v12 = (float)v18;
    v19 = 0.0;
    if (v12 > 0.0f)
        v19 = v12;
    v12 = (float)v19;
    v6 = v12;
    v4 = g_52b40c->field_10ea8;
    _snprintf(v15, 0x100, "Frames: %i (%.1f FPS)");
    v12 = (float)(int)v14;
    sub_40f830(10.0f, v12, 1.0f, 1.0f, 1, 0, 0x100, 0x100, 0x100);
    v7 = (g_52b40c->field_10eac == 0xffffffff ? 97 : g_52b40c->field_10eac + 48);
    v5 = g_52b40c->field_1245c;
    v4 = g_52b40c->field_80d4;
    _snprintf(v15, 0x100, "VRet: %i, Speed: %i, Skip: %c");
    sub_40f830(110.0f, v12, 1.0f, 1.0f, 1, 0, 0x100, 0x100, 0x100);
    if (v17 && v17->field_1b0)
    {
        v20 = *(int *)&v17->padding_0[4];
        v21 = a0 - 1;
        if (v20 == v21)
        {
            v7 = v20;
            v5 = &v17->padding_0[32];
            _snprintf(v15, 0x100, "%s %i");
        }
        else
        {
            v7 = v21;
            v5 = v20;
            v4 = &v17->padding_0[32];
            _snprintf(v15, 0x100, "%s %i (%i)");
        }
        if (v17->field_c3c)
        {
            v7 = *(int *)(*(int *)&g_52b40c->padding_10eb0[896 + 4 * v17->field_c38] + 4);
            v5 = *(int *)&g_52b40c->padding_10eb0[896 + 4 * v17->field_c38] + 32;
            _snprintf(v16, 64, " (in %s %i's state rc)");
            v22 = &v15[0];
            do
            {
                v23 = v22;
                v24 = v23 + 1;
                v22 = v24;
            } while (*v23);
            strncat(v15, v16, 0xff - (v24 - &v15[1]));
        }
        v13 = v14 + 8;
        sub_40f830(10.0f, (float)(int)v13, 1.0f, 1.0f, 1, 0, 200, 200, 220);
        v25 = v17->field_1418->field_10;
        v13 = (!v25 ? 0xffffffff : v25[3]);
        v26 = v17->field_1418;
        v7 = *(int *)(*(int *)(*(int *)v26->field_0 + 24) + v26->field_c * *(int *)(*(int *)v26->field_0 + 4) + 8);
        v5 = v26->field_1c;
        v4 = *(int *)(v26->field_c * *(int *)(*(int *)v26->field_0 + 4) + *(int *)(*(int *)v26->field_0 + 24) + 4);
        v3 = v26->field_14 + 1;
        v2 = v13;
        v1 = (!v25 ? 0xffffffff : v25[2]);
        v0 = *(int *)(*(int *)(*(int *)v26->field_0 + 28) + v26->field_c * 16 + 12);
        _snprintf(v15, 0x100, "ActionID: %i; Spr: %i,%i; Elemno: %i/%i; Time: %i/%i");
        if (v17->field_22ac)
        {
            v27 = &v15[0];
            do
            {
                v28 = v27;
                v29 = v28 + 1;
                v27 = v29;
            } while (*v28);
            strncat(v15, " (invalid)", 0xff - (v29 - &v15[1]));
        }
        v30 = v14;
        if (!v17->field_c48)
        {
            v7 = 0x100;
            v5 = 0x100;
            v4 = 0x100;
            v3 = 0;
            v2 = 1;
            v1 = 1.0f;
            v13 = v30 + 15;
            v0 = 1.0f;
            v31 = __double_as_longlong((double)(int)v13);
        }
        else
        {
            v7 = 128;
            v5 = 0x100;
            v4 = 0x100;
            v3 = 0;
            v2 = 1;
            v1 = 1.0f;
            v13 = v30 + 15;
            v0 = 1.0f;
            v31 = __double_as_longlong((double)(int)v13);
        }
        sub_40f830(10.0f, v31);
        v7 = v17->field_e54 - 1;
        v5 = v17->field_e5c;
        v4 = v17->field_e58;
        v3 = v17->field_e64;
        v2 = v17->field_c4c;
        _snprintf(v15, 0x100, "State no: %i; Ctrl: %i; Type: %i; Movetype %i; Time: %i");
        if (!v17->field_e98 && (int)v17->field_c4c >= 200)
        {
            v32 = &v15[0];
            do
            {
                v33 = v32;
                v34 = v33 + 1;
                v32 = v34;
            } while (*v33);
            strncat(v15, " (invalid)", 0xff - (v34 - &v15[1]));
        }
        v35 = v30 + 22;
        if (!v17->field_c3c)
        {
            v7 = 0x100;
            v5 = 0x100;
            v4 = 0x100;
            v3 = 0;
            v2 = 1;
            v1 = 1.0f;
            v13 = v35;
            v0 = 1.0f;
            v36 = __double_as_longlong((double)(int)v13);
        }
        else
        {
            v7 = 128;
            v5 = 0x100;
            v4 = 0x100;
            v3 = 0;
            v2 = 1;
            v1 = 1.0f;
            v13 = v35;
            v0 = 1.0f;
            v36 = __double_as_longlong((double)(int)v13);
        }
        sub_40f830(10.0f, v36);
        sub_443dd0(50);
        sub_443dd0(g_52be24 - 10);
        return;
    }
    v7 = a0 - 1;
    _snprintf(v15, 0x100, "Player %i Disabled");
    v13 = v14 + 8;
    sub_40f830(10.0f, (float)(int)v13, 1.0f, 1.0f, 1, 0, 200, 200, 220);
    return;
}
