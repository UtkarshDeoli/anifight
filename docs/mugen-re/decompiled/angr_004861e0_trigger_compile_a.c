// trigger_compile_a entry=0x4861e0
typedef struct st_4861e0_0 {
    char padding_0[184];
    unsigned int field_b8;
} st_4861e0_0;

typedef struct st_4861e0_6 {
    char padding_0[504];
    unsigned int field_1f8;
} st_4861e0_6;

typedef struct st_4861e0_1 {
    char padding_0[504];
    float field_1f8;
} st_4861e0_1;

typedef struct st_4861e0_5 {
    char padding_0[12];
    unsigned int field_c;
    unsigned int field_10;
    unsigned int field_14;
    unsigned int field_18;
    unsigned int field_1c;
    char padding_20[8];
    unsigned int field_28;
    char padding_2c[5200];
    struct st_4861e0_4 *field_147c;
    unsigned int field_1480;
} st_4861e0_5;

typedef struct st_4861e0_4 {
    struct st_4861e0_0 *field_0;
    unsigned int field_4;
    unsigned int field_8;
    unsigned int field_c;
    char padding_10[12];
    unsigned int field_1c;
    char padding_20[512];
    unsigned int field_220;
    char padding_224[60];
    unsigned int field_260;
    char padding_264[3060];
    unsigned int field_e58;
    unsigned int field_e5c;
    char padding_e60[44];
    unsigned int field_e8c;
    char padding_e90[512];
    unsigned int field_1090;
    unsigned int field_1094;
    char padding_1098[44];
    unsigned int field_10c4;
    char padding_10c8[16];
    unsigned int field_10d8;
    unsigned int field_10dc;
    char padding_10e0[928];
    unsigned int field_1480;
    char padding_1484[4];
    unsigned int field_1488;
    char padding_148c[1696];
    unsigned int field_1b2c;
} st_4861e0_4;

extern unsigned int g_49d27c[4];
extern unsigned int g_49d280[4];
extern int g_4a905c;
extern void* g_52b40c;
extern unsigned int g_52b440;
extern unsigned int g_52b640;
extern void g_52b848;
extern char g_52be60;

double sub_4861e0(double a0)
{
    double v18;  // mm7
    st_4861e0_5 *v19;  // ebx
    unsigned int v28;  // eax
    int v29;  // ebx
    void* *v30;  // eax
    unsigned int v31;  // eax
    int v32;  // ebp
    unsigned int v33;  // eax
    int v34;  // ebp
    unsigned int v35;  // eax
    int v36;  // ebx
    unsigned int v37;  // eax
    unsigned int node;  // esi
    int v38;  // ebx
    unsigned int v39;  // eax
    unsigned int v40;  // edx
    void* v41;  // eax
    void* v42;  // ecx
    void* v43;  // edx
    unsigned int v44;  // esi
    void* v45;  // ecx
    st_4861e0_6 *v46;  // ecx
    unsigned int v47;  // eax
    unsigned int v21;  // eax
    unsigned int v48;  // eax
    int iter;  // edi
    unsigned int v50;  // ecx
    unsigned int v52;  // esi
    unsigned int idx;  // ebp
    unsigned int v55;  // eax
    unsigned int v56;  // ebx
    unsigned int v57;  // ebp
    st_4861e0_5 *v22;  // edi
    int v58;  // eax
    unsigned int v59;  // eax
    unsigned int v60;  // eax
    st_4861e0_0 *v61;  // edx, Other Possible Types: st_4861e0_1 *, st_4861e0_5 *, unsigned int
    unsigned int v62;  // ecx
    unsigned int v63;  // eax
    unsigned int v64;  // ecx
    unsigned int v65;  // ebx
    unsigned int v66;  // ecx
    st_4861e0_5 *v23;  // ebp
    unsigned int v68;  // ecx
    unsigned int v71;  // eax
    st_4861e0_1 *v74;  // edx
    st_4861e0_5 *v75;  // ecx
    st_4861e0_5 *v76;  // 120
    unsigned int v24;  // ebx
    st_4861e0_1 *v77;  // 120
    double v78;  // mm6
    st_4861e0_1 *v79;  // 120
    st_4861e0_1 *v80;  // 120
    unsigned int v81;  // eax
    unsigned int v82;  // eax
    int idx1;  // eax
    st_4861e0_5 *v83;  // 128
    unsigned int v84;  // eax
    st_4861e0_5 *v85;  // ecx
    unsigned int v86;  // eax
    unsigned int v87;  // eax
    unsigned int v88;  // ebx
    unsigned int v89;  // eax
    unsigned int v90;  // eax
    unsigned int v91;  // ebx
    unsigned int v26;  // eax
    unsigned int v92;  // eax
    unsigned int v93;  // eax
    int v27;  // ebp
    double v0;  // [bp-0x5c]
    st_4861e0_5 *v1;  // [bp-0x58], Other Possible Types: st_4861e0_1 *, st_4861e0_6 *, unsigned int
    unsigned int v2;  // [bp-0x58]
    st_4861e0_1 *v3;  // [bp-0x44]
    void* v4;  // [bp-0x3c]
    unsigned int v5;  // [bp-0x38]
    void* v6;  // [bp-0x34]
    void* v7;  // [bp-0x30]
    double v8;  // [bp-0x2c]
    unsigned int idx2;  // [bp-0x20]
    void* v10;  // [bp-0x1c], Other Possible Types: unsigned int
    st_4861e0_5 *index;  // [bp-0x18], Other Possible Types: unsigned int
    st_4861e0_1 *v12;  // [bp-0x14], Other Possible Types: st_4861e0_5 *, int, double, float, unsigned int
    st_4861e0_1 *v13;  // [bp-0xc]
    unsigned int v14;  // [bp-0x8]
    st_4861e0_5 *v15;  // [bp+0x4], Other Possible Types: st_4861e0_1 *, int, float, unsigned int
    st_4861e0_5 *v16;  // [bp+0x4], Other Possible Types: st_4861e0_6 *, int, unsigned int
    st_4861e0_5 *v17;  // [bp+0x8], Other Possible Types: unsigned int

    v18 = 0.0;
    v19 = *(unsigned int *)((void*)&a0 + 4);
    v3 = (st_4861e0_1 *)0.0f;
    node = 0;
    v14 = v19->field_14;
    v21 = v19->field_10;
    v22 = *(unsigned int *)&a0;
    v4 = NULL;
    v6 = NULL;
    *(unsigned int *)&v8 = 0;
    index = v22;
    v2 = 0;
    v13 = NULL;
    if (v21 == 1)
    {
        v23 = v22->field_147c;
        if (!v22->field_147c)
        {
            sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", (int)g_52b40c[69288], v22->padding_20, *(int *)&v22->padding_0[4], *(int *)&v22->padding_2c[3104], "has no parent");
            v24 = v19->field_c;
            g_4a905c = g_4a905c - (v24 - 320 <= 31 ? 1 : v24 == 310 ? 2 : 0 < v24 - 305 ? 0 : 3);
            if (g_4a905c < 127)
            {
                idx1 = g_4a905c + 1;
                g_4a905c = idx1;
                (&g_52b640)[idx1] = 0;
                (&g_52b440)[idx1] = 4;
                return 0.0;
            }
            sprintf(&g_52be60, "Expression stack overflow.");
            goto LABEL_4862c1;
        }
    }
    else if (v21 == 2)
    {
        v23 = v22->field_1480;
        if (!v22->field_1480)
        {
            sub_443cd0();
            v26 = sub_486000();
            g_4a905c = g_4a905c - v26;
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return 0.0;
        }
    }
    else
    {
        if (v21 == 3)
        {
            v27 = sub_439260(1);
            if (v27 < 0)
            {
                v27 = 0;
                sub_443cd0();
            }
            v23 = sub_450b00(v22, v27);
            if (!v23)
            {
                sub_443d10("has no helper %i", v27);
                v28 = sub_486000();
                g_4a905c = g_4a905c - v28;
                v17 = 4;
                v15 = 0;
                sub_482a90();
                return 0.0;
            }
            goto LABEL_4865c1;
        }
        if (v21 == 4)
        {
            v29 = sub_439260(1);
            if (v29 < -0x1)
            {
                v29 = -0x1;
                sub_443cd0();
            }
            v30 = sub_465130();
            if (!v30)
            {
                sub_443d10("has no target with hit ID %i", v29);
                v31 = sub_486000();
                g_4a905c = g_4a905c - v31;
                v17 = 4;
                v15 = 0;
                sub_482a90();
                return 0.0;
            }
            v23 = *v30;
            v19 = *(unsigned int *)((void*)&a0 + 4);
        }
        else
        {
            if (v21 == 5)
            {
                v32 = sub_439260(1);
                if (v32 < 0)
                {
                    v32 = 0;
                    sub_443cd0();
                }
                v23 = sub_464990(v32);
                if (v23)
                    goto LABEL_4865bd;
                sub_443d10("has no %i-th partner", v32);
                v33 = sub_486000();
                g_4a905c = g_4a905c - v33;
                goto LABEL_4865a5;
            }
            if (v21 == 6)
            {
                v34 = sub_439260(1);
                if (v34 < 0)
                {
                    v34 = 0;
                    sub_443cd0();
                }
                v23 = sub_464a50(v22, v34);
                if (!v23)
                {
                    sub_443d10("has no %i-th enemy", v34);
                    v35 = sub_486000();
                    g_4a905c = g_4a905c - v35;
                    v17 = 4;
                    v15 = 0;
                    sub_482a90();
                    return 0.0;
                }
                goto LABEL_4865c1;
            }
            if (v21 == 7)
            {
                v36 = sub_439260(1);
                if (v36 < 0)
                {
                    v36 = 0;
                    sub_443cd0();
                }
                v23 = sub_464b20();
                if (v23)
                    goto LABEL_4865bd;
                sub_443d10("has no %i-th nearest enemy", v36);
                v37 = sub_486000();
                g_4a905c = g_4a905c - v37;
                goto LABEL_4865a5;
            }
            else
            {
                v23 = v22;
                if (v21 == 8)
                {
                    v38 = sub_439260(1);
                    if (v38 < 0)
                    {
                        v38 = 0;
                        sub_443cd0();
                    }
                    v23 = sub_464c10();
                    if (!v23)
                    {
                        sub_443d10("tried to access non-existent player with ID %i", v38);
                        v39 = sub_486000();
                        g_4a905c = g_4a905c - v39;
LABEL_4865a5:
                        v17 = 4;
                        v15 = 0;
                        sub_482a90();
                        return 0.0;
                    }
LABEL_4865bd:
                    v19 = *(unsigned int *)((void*)&a0 + 4);
LABEL_4865c1:
                }
            }
        }
    }
    v40 = v23->field_1c;
    if (v23->field_1c != 1)
        v12 = NULL;
    else
        v12 = ~-(0 < v23->field_147c->field_4 - *(int *)&v23->padding_2c[5196]) & v23->field_147c;
    idx2 = v23->field_1480;
    v41 = g_52b40c;
    v5 = v23 != v22;
    if (!(int)g_52b40c[69440])
    {
        v42 = NULL;
        v10 = NULL;
        v7 = NULL;
        if (v23->field_c == 1)
        {
            v42 = (int)g_52b40c[70200];
            v43 = (int)g_52b40c[70208];
        }
        else
        {
            if (v23->field_c != 2)
                goto LABEL_48664b;
            v42 = (int)g_52b40c[70196];
            v43 = (int)g_52b40c[70204];
        }
        v7 = v43;
        v10 = v42;
LABEL_48664b:
        v4 = *(int *)&v23[1].padding_2c[3536];
        if (v42 && v7)
        {
            if (v4 == v42)
                goto LABEL_4866d8;
            if (v4 == v7)
            {
                *(void* *)&v8 = v10;
            }
            else
            {
                sub_464cd0();
                v8 = fabs(0.0);
                sub_464cd0();
                v18 = fabs(v8);
                if (v18 > v8)
                {
                    *(void* *)&v8 = v7;
                    v41 = g_52b40c;
                }
                else
                {
                    v41 = g_52b40c;
                    goto LABEL_4866dc;
                }
            }
        }
        else if (v4)
        {
            *(void* *)&v8 = v42;
            if ((int)v4[28] == 1)
            {
                if ((int)v4[5248] == v42)
                {
                    *(void* *)&v8 = v7;
                    goto LABEL_4866e0;
                }
            }
            else
            {
                if (v4 == v42)
                {
LABEL_4866d8:
                    v10 = v7;
LABEL_4866dc:
                    *(void* *)&v8 = v10;
                    goto LABEL_4866e0;
                }
            }
        }
LABEL_4866e0:
        v40 = v23->field_1c;
        if (v23->field_1c != 1)
        {
            v44 = *(int *)&v23->padding_0[8];
            goto LABEL_486731;
        }
        if (idx2)
        {
            v44 = *(int *)(idx2 + 8);
LABEL_486731:
            node = v44 + 2;
            v2 = node;
            if ((int)node > 4)
            {
                node -= 4;
                v2 = node;
            }
            v45 = *(int *)((char *)v41 + 4 * node + 70192);
        }
        else if (v23->field_c == 1)
        {
            v6 = (int)v41[70196];
            if ((int)v41[70196])
                goto LABEL_48674f;
            v45 = (int)v41[70204];
        }
        else
        {
            if (v23->field_c != 2 || !(v6 = (int)v41[70200], !(int)v41[70200]))
                goto LABEL_48674f;
            v45 = (int)v41[70208];
        }
        v6 = v45;
LABEL_48674f:
        if (v23->field_c == 1)
        {
            index = (int)v41[70196];
            v40 = v23->field_1c;
        }
        if (v23->field_c == 2)
            index = (int)v41[70200];
    }
    v10 = v19->field_c;
    switch (v10)
    {
    case 0:
        node = *(int *)&v23->padding_2c[3624];
        break;
    case 1:
        idx = *(int *)&v23->padding_2c[5100];
        node = *(int *)(idx + 28) - *(int *)(*(int *)(idx + 12) * *(int *)(*(int *)*(int *)idx + 4) + *(int *)(*(int *)*(int *)idx + 24) + 8);
        break;
    case 2:
        v55 = sub_402460();
        if (v55 == 0xffffffff)
        {
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
        node = *(int *)(*(int *)&v23->padding_2c[5100] + 28) - v55;
        break;
    case 3:
        node = *(int *)(*(int *)(*(int *)&v23->padding_2c[5100] + 12) * 16 + *(int *)(*(int *)*(int *)*(int *)&v23->padding_2c[5100] + 28) + 12);
        break;
    case 4:
        if (v19->field_1c)
        {
            v56 = v19->field_1c - 1;
            if (v19->field_1c != 1)
                goto LABEL_486bb7;
            v18 = *(float *)&v23->padding_2c[460];
            goto LABEL_4867ed;
        }
        else
        {
            v15 = *(float *)&v23->padding_2c[456] - (float)g_52b40c[69368];
            v18 = v15;
            goto LABEL_4867ed;
        }
    case 5:
        if (v19->field_1c)
        {
            v56 = v19->field_1c - 1;
            if (v19->field_1c != 1)
            {
LABEL_486bb7:
                if (v56 == 1 && (unsigned int)*(int *)(*(int *)&v23->padding_0[0] + 184) < (unsigned int)(int)v41[1032])
                {
                    v57 = *(int *)&v23->padding_0[8] - 1;
                    if (idx2)
                        v57 = *(int *)(idx2 + 8) - 1;
                    if ((int)v57 > 4)
                    {
                        v18 = (double)(int)v41[7156];
                        v3 = (st_4861e0_1 *)(float)v18;
                        break;
                    }
                    else
                    {
                        if ((int)v57 < 0)
                            v57 = 0;
                        v18 = (double)*(int *)((char *)v41 + 4 * v57 + 7140);
                        v3 = (st_4861e0_1 *)(float)v18;
                        break;
                    }
                }
            }
            else
            {
                v12 = *(float *)&v23->padding_2c[460];
                sub_456ea0();
                v15 = (float)(v12 - *(float *)&v23->padding_2c[460] + (double)(int)g_52b40c[32968]);
                v18 = v15;
                goto LABEL_4867ed;
            }
        }
        else
        {
            v12 = *(float *)&v23->padding_2c[456];
            sub_456e70();
            v16 = &v58;
            v15 = (float)(v12 - *(float *)&v23->padding_2c[456] + (double)v16);
            v18 = v15;
            goto LABEL_4867ed;
        }
    case 6:
        if (!v19->field_1c)
        {
            v15 = *(int *)&v23->padding_2c[444] * *(unsigned int *)&*(float *)&v23->padding_2c[472];
            v18 = *(float *)&v15;
            goto LABEL_4867ed;
        }
        else if (v19->field_1c == 1)
        {
            v18 = *(float *)&v23->padding_2c[476];
            goto LABEL_4867ed;
        }
    case 7:
        node = *(int *)&v23->padding_2c[3624] % *(int *)&v19->padding_20[4];
        break;
    case 8:
        if (!v4)
        {
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
        if (!v19->field_1c)
            goto LABEL_487d66;
        if (v19->field_1c == 1)
        {
            v15 = (float)v4[504] - *(float *)&v23->padding_2c[460];
            v18 = v15;
            goto LABEL_4867ed;
        }
        break;
    case 9:
        if (!v4)
        {
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
        if (!v19->field_1c)
        {
            sub_464d20();
            goto LABEL_4867ed;
        }
        else if (v19->field_1c == 1)
        {
            v15 = (float)v4[504] - *(float *)&v23->padding_2c[460];
            v18 = v15;
            goto LABEL_4867ed;
        }
    case 10:
        if (v4)
        {
            node = (int)v4[440];
            break;
        }
        else
        {
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
    case 11: case 12:
        sub_46c5b0();
        v48 = *(int *)(*(unsigned int *)((void*)&a0 + 4) + 36);
        iter = 0;
        if (v48)
        {
            v50 = *(int *)&v23->padding_2c[3628];
            if (v50 == 3)
                iter = 1;
            if (v50 == 4)
                iter += 2;
        }
        if (*(int *)(*(unsigned int *)((void*)&a0 + 4) + 12) == 11)
        {
            if (v48)
            {
                v12 = *(int *)&v23->padding_2c[532];
                v12 *= *(unsigned int *)&*(float *)(*(int *)&v23->padding_0[0] + 164);
                floor(v12 + 0.5f);
                iter += sub_499690();
            }
            v52 = *(int *)&v23->padding_2c[444];
            if (v52 == 1)
            {
                v50 = *(int *)&v23->padding_2c[504];
                v10 = *(int *)&v23->padding_2c[504];
            }
            else
            {
                v10 = *(int *)&v23->padding_2c[500];
            }
            v12 = iter * v52;
            v1 = v50;
            v12 = (float)((double)v12 + *(float *)&v23->padding_2c[456] - (double)(int)v10);
            sub_4652a0(v12);
            v15 = -v52;
            v18 = v12 * (double)(int)v15;
            node = sub_499690();
            break;
        }
        else
        {
            if (v48)
            {
                v12 = (*(int *)&v23->padding_2c[444] == -*(int *)&v23->padding_2c[444] ? *(int *)&v23->padding_2c[532] : *(int *)&v23->padding_2c[536]);
                v12 *= *(unsigned int *)&*(float *)(*(int *)&v23->padding_0[0] + 164);
                floor(v12 + 0.5f);
                iter += sub_499690();
            }
            index = *(int *)&v23->padding_2c[444];
            if (index == 1)
            {
                v50 = *(int *)&v23->padding_2c[500];
                v10 = *(int *)&v23->padding_2c[500];
            }
            else
            {
                v10 = *(int *)&v23->padding_2c[504];
            }
            v12 = iter * index;
            v1 = v50;
            v12 = (float)(*(float *)&v23->padding_2c[456] - (double)v12 - (double)(int)v10);
            sub_4652a0(v12);
            v18 = v12 * (double)(int)index;
            node = sub_499690();
            break;
        }
    case 15:
        node = *(int *)&v23->padding_2c[396];
        break;
    case 16:
        node = *(int *)&v23->padding_2c[400];
        break;
    case 17:
        node = *(int *)*(int *)&index->padding_2c[412];
        break;
    case 18:
        node = *(int *)*(int *)&index->padding_2c[416];
        break;
    case 19:
        node = *(int *)&v23->padding_2c[3668];
        break;
    case 20:
        v59 = v19->field_1c;
        node = v59 - 1;
        if (v23->padding_2c[2440 + v59])
        {
            node = v59;
            if (!v14)
            {
                sub_460200(node);
                break;
            }
        }
        break;
    case 21:
        if (!v19->field_1c)
        {
            v46 = (int)g_52b40c[69392] - (int)g_52b40c[69400];
            v16 = v46;
            v16 *= __float_as_int((float)g_52b40c[7028]);
            v15 = *(float *)&v16 + (float)g_52b40c[69368];
            v18 = v15;
            goto LABEL_4867ed;
        }
        else if (v19->field_1c == 1)
        {
            v16 = (int)g_52b40c[69396] - (int)g_52b40c[69404];
            v16 *= __float_as_int((float)g_52b40c[7028]);
            v15 = *(float *)&v16 + (float)g_52b40c[69372];
            v18 = v15;
            goto LABEL_4867ed;
        }
    case 25:
        if (!*(int *)&v23[1].padding_0[4] || (int)v41[0x4a00] == 3 && !*(int *)&v23[1].padding_2c[1660])
            goto LABEL_486e28;
        node = *(int *)&v23[1].padding_2c[3492];
        break;
    case 30:
        node = *(int *)&v23->padding_2c[5068];
        break;
    case 31:
        node = *(int *)&v23->padding_2c[5072];
        break;
    case 32:
        if (*(int *)&v23->padding_2c[3680] == 2)
            goto LABEL_486df7;
        goto LABEL_486e28;
    case 33:
        if (*(int *)&v23->padding_2c[3680] != 1)
            goto LABEL_486e28;
        node = *(int *)&v23->padding_2c[3684] + 1;
        break;
    case 34:
        if (*(int *)&v23->padding_2c[3680] != 2 && *(int *)&v23->padding_2c[3680] != 1)
            goto LABEL_486e28;
LABEL_486df7:
        node = *(int *)&v23->padding_2c[3684] + 1;
        break;
    case 35:
        if (*(int *)&v23->padding_2c[3680] != 4)
            goto LABEL_486e28;
        node = *(int *)&v23->padding_2c[3684] + 1;
        break;
    case 36:
        node = sub_464ea0(v23);
        break;
    case 40:
        node = *(int *)&v23->padding_2c[3104];
        break;
    case 41:
        node = *(int *)&v23->padding_2c[3108];
        break;
    case 42:
        if (v4)
        {
            node = (int)v4[3148];
            break;
        }
        else
        {
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
    case 43:
        node = *(int *)&v23->padding_2c[3628];
        break;
    case 44:
        if (v4)
        {
            node = (int)v4[3672];
            break;
        }
        else
        {
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
    case 45:
        node = *(int *)&v23->padding_2c[3632];
        break;
    case 46:
        if (v4)
        {
            node = (int)v4[3676];
            break;
        }
        else
        {
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
    case 47:
        node = *(int *)&v23->padding_2c[3640];
        break;
    case 48:
        node = 0;
        if (*(int *)&v23->padding_2c[3632] == 1)
        {
            v60 = *(int *)&v23->padding_2c[4496] & v19->field_1c;
            if ((char)v60 & 7 && v60 & 0xfffffff8)
                node = 1;
        }
        if (v19->field_14 == 1)
        {
            node = 1 - node;
            break;
        }
        break;
    case 51:
        node = sub_4599c0() % 1000;
        break;
    case 60:
        node = sub_4733e0();
        break;
    case 62:
        if (*(int *)&v23->padding_2c[564] != 2 || *(int *)&v23->padding_2c[560] != *(int *)&v19->padding_20[4] && *(int *)&v19->padding_20[4])
        {
            v17 = 0;
            v15 = !v19->field_28;
            sub_482a90();
            return v18;
        }
        goto LABEL_4871f4;
    case 63:
        if (*(int *)&v23->padding_2c[564] == 1 && (*(int *)&v23->padding_2c[560] == *(int *)&v19->padding_20[4] || !*(int *)&v19->padding_20[4]))
        {
            node = *(int *)&v23->padding_2c[568];
            if (!v19->field_28)
            {
                v13 = 0x1;
                break;
            }
        }
        else
        {
            v17 = 0;
            v15 = !v19->field_28;
            sub_482a90();
            return v18;
        }
    case 64:
        if (!*(int *)&v23->padding_2c[564] || *(int *)&v23->padding_2c[564] == 3 || *(int *)&v23->padding_2c[560] != *(int *)&v19->padding_20[4] && *(int *)&v19->padding_20[4])
        {
            v17 = 0;
            v15 = !v19->field_28;
            sub_482a90();
            return v18;
        }
LABEL_4871f4:
        node = *(int *)&v23->padding_2c[568];
        if (!v19->field_28)
        {
            v13 = 0x1;
            break;
        }
        break;
    case 90:
        v61 = *(int *)&v23->padding_0[0];
        goto LABEL_4874e1;
    case 91:
        if (v4)
        {
            v61 = *(int *)v4;
            v1 = *(int *)&v19->padding_2c[8];
            goto LABEL_4874e5;
        }
        goto LABEL_486e28;
    case 92:
        if (v6)
        {
            v61 = *(int *)v6;
            v1 = *(int *)&v19->padding_2c[8];
            goto LABEL_4874e5;
        }
        goto LABEL_486e28;
    case 93:
        if (*(unsigned int *)&v8)
        {
            v61 = *(int *)*(unsigned int *)&v8;
            v1 = *(int *)&v19->padding_2c[8];
            goto LABEL_4874e5;
        }
        goto LABEL_486e28;
    case 94:
        v1 = *(int *)&v19->padding_2c[8];
        v61 = *(int *)&v23->padding_0[0] + 96;
        goto LABEL_4874e6;
    case 95:
        node = sub_4649f0();
        break;
    case 96:
        node = sub_464ac0();
        break;
    case 97:
        node = *(int *)&v23->padding_0[4];
        break;
    case 99:
        node = *(int *)&v23->padding_2c[444];
        break;
    case 100:
LABEL_487587:
        node = *(int *)&v23->padding_2c[4252];
        break;
    case 101:
        node = *(int *)&v23->padding_2c[4196] <= 0;
        break;
    case 102:
        node = *(int *)&v23->padding_2c[4200] < 0;
        break;
    case 103:
        if (!v19->field_1c)
        {
            v18 = (double)*(int *)&v23->padding_2c[444] * *(float *)&v23->padding_2c[4228];
            v3 = (st_4861e0_1 *)(float)v18;
            break;
        }
        else if (v19->field_1c == 1)
        {
            v18 = *(float *)&v23->padding_2c[4232];
            v3 = (st_4861e0_1 *)(float)v18;
            break;
        }
    case 104:
        node = *(int *)&v23->padding_2c[3652];
        break;
    case 105:
        if (!*(int *)&v23->padding_2c[4268] || *(int *)&v23->padding_2c[4272] < *(int *)&v23->padding_2c[4276])
            goto LABEL_486e28;
        node = 1;
        break;
    case 107:
        node = *(int *)&v23->padding_2c[5108] + 1;
        break;
    case 108:
        if (!(int)v41[71448] || (int)v41[71444] != v23->field_c || !sub_44ee90())
            goto LABEL_486e28;
        node = 1;
        break;
    case 109:
        if ((int)v41[71448] != *(int *)&v19->padding_20[4] && *(int *)&v19->padding_20[4] != 0xffffffff || (int)v41[71444] != v23->field_c)
            goto LABEL_486e28;
        node = 1;
        break;
    case 110:
        if ((int)v41[71448] != *(int *)&v19->padding_20[4] && *(int *)&v19->padding_20[4] != 0xffffffff || (int)v41[71444] == v23->field_c || !(int)v41[71444])
            goto LABEL_486e28;
        node = 1;
        break;
    case 111:
        if ((int)v41[71444] || !(int)v41[71448])
            goto LABEL_486e28;
        node = 1;
        break;
    case 112:
        v47 = sub_459580();
        goto LABEL_4885ff;
    case 113:
        node = (int)v41[69288];
        break;
    case 114:
        node = (int)v41[71396];
        break;
    case 115:
        node = (int)v41[71440];
        break;
    case 116:
        node = (int)v41[71640];
        break;
    case 117:
        node = *(int *)&v23[1].padding_2c[3544];
        break;
    case 120:
        node = v23->field_c;
        break;
    case 121:
        node = *(int *)(92 * v23->field_10 + (char *)v41 + 0x7fe0);
        if (*(int *)((char *)v41 + 6876 * v23->field_10 + 18988) >= 2)
        {
            node = 3;
            break;
        }
        break;
    case 122:
        node = (!(int)v41[18964] ? v23->field_c == 1 : v23->field_c == 3 - (int)v41[18964]);
        break;
    case 150:
        v62 = *(int *)((int)v41[64688] + 4);
        node = 0;
        if (*(int *)((int)v41[64688] + 4) > 0)
        {
            v63 = *(int *)(int)v41[64688];
            do
            {
                v64 = v62;
                if (*(int *)v63 && *(int *)(v63 + 12) == *(int *)&v23->padding_0[4])
                    node += 1;
            } while ((v63 += 264, v62 = v64 - 1, v64 != 1));
        }
        break;
    case 160:
        v2 = 0xffffffff;
        goto LABEL_487b42;
    case 180:
        switch (v19->field_1c)
        {
        case 0:
            v18 = (double)*(int *)&v23->padding_2c[4156];
            goto LABEL_4877ee;
        case 1:
            v18 = (double)*(int *)&v23->padding_2c[4160];
            goto LABEL_4877ee;
        case 2:
            node = *(int *)&v23->padding_2c[4168];
            break;
        case 3:
            node = *(int *)&v23->padding_2c[4172];
            break;
        case 4:
            node = *(int *)&v23->padding_2c[4176];
            break;
        case 5:
            node = *(int *)&v23->padding_2c[4180];
            break;
        case 6:
            node = *(int *)&v23->padding_2c[4184];
            break;
        case 7:
            node = *(int *)&v23->padding_2c[4188];
            break;
        case 8:
            node = *(int *)&v23->padding_2c[4192];
            break;
        case 9:
            node = *(int *)&v23->padding_2c[4196];
            break;
        case 10:
            node = *(int *)&v23->padding_2c[4200];
            break;
        case 11:
            node = *(int *)&v23->padding_2c[4204];
            break;
        case 12:
            node = *(int *)&v23->padding_2c[4208];
            break;
        case 13:
            node = *(int *)&v23->padding_2c[4212];
            break;
        case 14:
            v18 = *(float *)&v23->padding_2c[4220];
            goto LABEL_4867ed;
        case 15:
            v18 = *(float *)&v23->padding_2c[4224];
            goto LABEL_4867ed;
        case 16:
            v18 = 0.0;
            v3 = (st_4861e0_1 *)0.0f;
            break;
        case 17:
            v18 = *(float *)&v23->padding_2c[4228];
            goto LABEL_4867ed;
        case 18:
            v18 = *(float *)&v23->padding_2c[4232];
            goto LABEL_4867ed;
        case 19:
            v18 = *(float *)&v23->padding_2c[4236];
            goto LABEL_4867ed;
        case 20:
            node = *(int *)&v23->padding_2c[4240];
            break;
        case 21:
            node = *(int *)&v23->padding_2c[4244];
            break;
        case 22:
            node = *(int *)&v23->padding_2c[4248] != 0;
            break;
        case 24:
            node = *(int *)&v23->padding_2c[4256];
            break;
        case 25:
            v18 = *(float *)&v23->padding_2c[4260];
            goto LABEL_4867ed;
        case 26:
            v18 = *(float *)&v23->padding_2c[4264];
            goto LABEL_4867ed;
        case 27:
            node = *(int *)&v23->padding_2c[4268];
            break;
        case 28:
            node = *(int *)&v23->padding_2c[4272];
            break;
        case 29:
            node = *(int *)&v23->padding_2c[4276];
            break;
        case 30:
            node = *(int *)&v23->padding_2c[4280];
            break;
        case 31:
            node = *(int *)&v23->padding_2c[4288];
            break;
        case 32:
            node = *(int *)&v23->padding_2c[4292];
            break;
        case 33:
            v18 = *(float *)&v23->padding_2c[4300];
LABEL_4877ee:
            v1 = v46;
            sub_4652a0((float)v18);
            goto LABEL_4877fc;
        case 34:
            v18 = *(float *)&v23->padding_2c[4296];
            v3 = (st_4861e0_1 *)(float)v18;
            break;
        case 35:
            v18 = *(float *)&v23->padding_2c[4304];
            v3 = (st_4861e0_1 *)(float)v18;
            break;
        case 23:
            goto LABEL_487587;
        default:
            if (v5)
            {
                sub_443d80();
                break;
            }
            else
            {
                sub_443cd0();
                break;
            }
        }
    case 181:
        v65 = v19->field_1c;
        if (!g_49d27c[3 * v19->field_1c])
        {
            node = *(int *)&v23->padding_2c[36 + 4 * v65];
            v66 = v65 * 3;
            v2 = *(int *)&v23->padding_2c[36 + 4 * v65];
            if (g_49d280[v66])
            {
                v1 = v66;
                v15 = *(unsigned int *)&*(float *)(*(int *)&v23->padding_0[0] + 164) * v2;
                v18 = *(float *)&v15;
                sub_4652a0((float)v18);
                node = sub_40f160((float)v18);
                break;
            }
        }
        else
        {
            v18 = *(float *)&v23->padding_2c[36 + 4 * v65];
            v3 = (st_4861e0_1 *)(float)v18;
            if (g_49d280[3 * v65])
            {
                v46 = *(int *)&v23->padding_0[0];
                v15 = (float)(*(float *)(*(int *)&v23->padding_0[0] + 164) * v3);
                v18 = v15;
                goto LABEL_4867ed;
            }
        }
    case 182:
        if (*(int *)&v19->padding_20[4])
        {
            if (*(int *)&v19->padding_20[4] == 1)
            {
                v1 = *(int *)&v19->padding_2c[8];
                v61 = g_52b40c + 0x1b00;
                goto LABEL_4874e6;
            }
            if (*(int *)&v19->padding_20[4] != 4)
            {
                sub_443cd0();
                node = 0;
                break;
            }
            else
            {
                v61 = g_52b40c + 6960;
LABEL_4874e1:
                v1 = *(int *)&v19->padding_2c[8];
LABEL_4874e5:
                goto LABEL_4874e6;
            }
        }
        else
        {
            v1 = *(int *)&v19->padding_2c[8];
            v61 = g_52b40c + 6864;
LABEL_4874e6:
            node = -(0 < (unsigned int)_stricmp(v61, v1)) + 1;
            break;
        }
    case 190:
        node = *(int *)&g_52b848;
        break;
    case 201:
        node = v40 == 1;
        break;
    case 202:
        node = sub_450a90(0);
        break;
    case 203:
        if (!v12)
        {
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
        if (!v19->field_1c)
            goto LABEL_487d66;
        if (v19->field_1c == 1)
        {
            v15 = v12->field_1f8 - *(float *)&v23->padding_2c[460];
            v18 = v15;
            goto LABEL_4867ed;
        }
        break;
    case 204:
        if (!idx2)
        {
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
        if (!v19->field_1c)
        {
LABEL_487d66:
            sub_464cd0();
            goto LABEL_4867ed;
        }
        else if (v19->field_1c == 1)
        {
            v46 = idx2;
            v15 = *(float *)&v46->field_1f8 - *(float *)&v23->padding_2c[460];
            v18 = v15;
            goto LABEL_4867ed;
        }
    case 205:
        node = *(int *)&v23[1].padding_2c[3588];
        break;
    case 210:
        v15 = sub_499690();
        v18 = (double)v15;
        goto LABEL_4867ed;
    case 211:
        v15 = sub_499690();
        v18 = (double)v15;
LABEL_4867ed:
        v1 = v46;
        goto LABEL_4867ee;
    case 300:
        v18 = 3.1415927410125732;
        v3 = (st_4861e0_1 *)3.1415927410125732f;
        break;
    case 301:
        v18 = 2.7182817459106445;
        v3 = (st_4861e0_1 *)2.7182817459106445f;
        break;
    case 305:
        if (!sub_437db0(&index))
        {
            sub_488f20(); /* do not return */
        }
        else if (!sub_437db0(&v12))
        {
            sub_488f20(); /* do not return */
        }
        else if (!sub_437db0(&v14))
        {
            sub_488f20(); /* do not return */
        }
        else if (v14 == 3)
        {
            v17 = 4;
            v15 = *(unsigned int *)&a0;
            sub_482a90();
            return v18;
        }
        else
        {
            v74 = v12;
            if (v74 == 3)
            {
                v1 = 4;
                v61 = *(unsigned int *)((void*)&a0 + 4);
                sub_482a90();
                return v18;
            }
            v75 = index;
            if (v75 == 3)
            {
                v17 = 4;
                v15 = v13;
                sub_482a90();
                return v18;
            }
            else if (v14 == 2)
            {
                if (*(unsigned int *)&a0 != 0.0f)
                {
                    v1 = v74;
                    v61 = *(unsigned int *)((void*)&a0 + 4);
                    sub_482a90();
                }
                else
                {
                    v17 = v75;
                    v15 = v13;
                    sub_482a90();
                }
                return 0.0;
            }
            else
            {
                if (*(unsigned int *)&a0)
                {
                    v1 = v74;
                    v61 = *(unsigned int *)((void*)&a0 + 4);
                    sub_482a90();
                }
                else
                {
                    v1 = v75;
                    v61 = v13;
                    sub_482a90();
                }
                return v18;
            }
        }
    case 310:
        node = sub_488ec0();
        if (sub_488ec0() == 3)
        {
            v17 = 4;
            v16 = sub_499690();
            sub_482a90();
            return (double)*(unsigned int *)&a0;
        }
        if (node == 3)
        {
            v17 = 4;
            v15 = sub_499690();
            sub_482a90();
            return (double)v12;
        }
        v18 = 0.0;
        v76 = *(unsigned int *)&a0;
        if (v76 > 0.0f)
        {
            if (v76 == 1.0f || !(v77 = v12, v18 = (double)v77, v77 > 0.0f))
                goto LABEL_487fc2;
            _CIlog();
            v12 = (double)v77;
            _CIlog();
            v3 = (st_4861e0_1 *)(float)(v12 / 1.0);
            break;
        }
        else
        {
            v18 = (double)v76;
LABEL_487fc2:
            if (v5)
                sub_443d80();
            else
                sub_443cd0();
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
    case 320:
        if (sub_488ec0() == 3)
        {
            v17 = 4;
            v15 = sub_499690();
            sub_482a90();
            return (double)v3;
        }
        v18 = (double)v3;
        _CIsin();
        v3 = (st_4861e0_1 *)(float)v78;
        break;
    case 321:
        if (sub_488ec0() == 3)
        {
            v17 = 4;
            v15 = sub_499690();
            sub_482a90();
            return (double)v3;
        }
        v18 = (double)v3;
        _CIcos();
        v3 = (st_4861e0_1 *)(float)v78;
        break;
    case 322:
        if (sub_488ec0() == 3)
        {
            v17 = 4;
            v15 = sub_499690();
            sub_482a90();
            return (double)v3;
        }
        v18 = (double)v3;
        _CItan();
        v3 = (st_4861e0_1 *)(float)v78;
        break;
    case 323:
        if (sub_488ec0() == 3)
        {
            v17 = 4;
            v15 = sub_499690();
            sub_482a90();
            return (double)v3;
        }
        v79 = v3;
        v18 = (double)v79;
        if (v79 >= -1.0f && 1.0 >= v18)
        {
            _CIasin();
            v3 = (st_4861e0_1 *)1.0f;
            break;
        }
        else
        {
            if (v5)
                sub_443d80();
            else
                sub_443cd0();
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
    case 324:
        if (sub_488ec0() == 3)
        {
            v17 = 4;
            v15 = sub_499690();
            sub_482a90();
            return (double)v3;
        }
        v80 = v3;
        v18 = (double)v80;
        if (v80 >= -1.0f && 1.0 >= v18)
        {
            _CIacos();
            v3 = (st_4861e0_1 *)1.0f;
            break;
        }
        else
        {
            if (v5)
                sub_443d80();
            else
                sub_443cd0();
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
    case 325:
        if (sub_488ec0() == 3)
        {
            v17 = 4;
            v15 = sub_499690();
            sub_482a90();
            return (double)v3;
        }
        v18 = (double)v3;
        _CIatan();
        v3 = (st_4861e0_1 *)(float)v78;
        break;
    case 326:
        if (sub_488ec0() == 3)
        {
            v17 = 4;
            v15 = sub_499690();
            sub_482a90();
            return (double)v3;
        }
        v18 = (double)v3;
        _CIexp();
        v3 = (st_4861e0_1 *)(float)v78;
        break;
    case 327:
        if (sub_488ec0() == 3)
        {
            v17 = 4;
            v15 = sub_499690();
            sub_482a90();
            return (double)v3;
        }
        v18 = (double)v3;
        if (v18 <= 0.0)
        {
            if (v5)
                sub_443d80();
            else
                sub_443cd0();
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
        _CIlog();
        v3 = (st_4861e0_1 *)(float)v18;
        break;
    case 328:
        if (!sub_437db0(&v2))
            sub_488f20(); /* do not return */
        v81 = v2;
        if (v81 == 4)
        {
            v17 = 4;
            v15 = *(unsigned int *)&a0;
            sub_482a90();
            return v18;
        }
        else if (!v81)
        {
            v17 = 0;
            v15 = (*(unsigned int *)&a0 ^ (int)(*(unsigned int *)&a0) >> 31) - ((int)(*(unsigned int *)&a0) >> 31);
            sub_482a90();
            return v18;
        }
        else if (v81 == 1)
        {
            v16 = fabs(*(unsigned int *)&a0);
            v17 = 1;
            sub_482a90();
            return fabs((double)*(unsigned int *)&a0);
        }
        goto LABEL_488396;
    case 329: case 330:
        if (!sub_437db0(&v2))
            sub_488f20(); /* do not return */
        v82 = v2;
        if (v82 == 4)
        {
            v17 = 4;
            v15 = *(unsigned int *)&a0;
            sub_482a90();
            return v18;
        }
        if (!v82)
        {
            node = *(unsigned int *)&a0;
            break;
        }
        else
        {
            if (v82 != 1)
            {
LABEL_488396:
                sprintf(&g_52be60, "Panic: aborting");
                sub_443e80(); /* do not return */
            }
            v83 = *(unsigned int *)&a0;
            v0 = (double)v83;
            if (v10 == 329)
            {
                floor(*(unsigned int *)&v0);
                v12 = v83;
                v18 = (double)v12;
                if (v12 < -2147483648.0f)
                {
                    node = 0x80000000;
                    sub_443cd0();
                    break;
                }
                else
                {
                    if (2147483648.0 >= v18)
                        goto LABEL_4877fc;
                    node = 0x7fffffff;
                    sub_443cd0();
                    break;
                }
            }
            else
            {
                v12 = (float)ceil(v0);
                v18 = v12;
                if (v12 < -2147483648.0f)
                {
                    node = 0x80000000;
                    sub_443cd0();
                    break;
                }
                else if (2147483648.0 >= v18)
                {
LABEL_4877fc:
                    node = sub_499690();
                    break;
                }
                else
                {
                    node = 0x7fffffff;
                    sub_443cd0();
                    break;
                }
            }
        }
    case 331:
        if (sub_488e20() == 3)
        {
            v17 = 4;
            v15 = v2;
            sub_482a90();
            return v18;
        }
        v84 = sub_402460();
        if (v84 == 0xffffffff)
        {
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
        node = *(int *)(*(int *)&v23->padding_2c[5100] + 28) - v84;
        break;
    case 332:
        if (sub_488e20() == 3)
        {
            v17 = 4;
            v15 = v2;
            sub_482a90();
            return v18;
        }
        node = sub_4032f0() != 0xffffffff;
        break;
    case 333:
        if (sub_488e20() == 3)
        {
            v17 = 4;
            v15 = v2;
            sub_482a90();
            return v18;
        }
        node = sub_4650a0();
        break;
    case 334:
        if (sub_488e20() == 3)
        {
            v17 = 4;
            v15 = v2;
            sub_482a90();
            return v18;
        }
        v47 = sub_464c10();
LABEL_4885ff:
        node = -(-(0 < v47));
        break;
    case 335:
        v71 = sub_488e20();
        if (v23->field_1c != 1)
        {
LABEL_486e28:
            node = 0;
            break;
        }
        else
        {
            if (v71 == 3)
            {
                v17 = 4;
                v15 = v2;
                sub_482a90();
                return v18;
            }
            node = v2 == *(int *)&v23->padding_2c[5192];
            break;
        }
    case 336:
        if (sub_488e20() == 3)
        {
            v17 = 4;
            v15 = v2;
            sub_482a90();
            return v18;
        }
        node = sub_450a90(v2);
        break;
    case 337:
        if (sub_488e20() == 3)
        {
            v17 = 4;
            v15 = v2;
            sub_482a90();
            return v18;
        }
        if ((int)v2 < 0)
        {
            v2 = 0;
            if (v5)
                sub_443d80();
            else
                sub_443cd0();
        }
        if (*(int *)&v23->padding_2c[564] == 2 && (*(int *)&v23->padding_2c[560] == v2 || !v2))
            goto LABEL_4872a0;
        goto LABEL_4872ab;
    case 338:
        if (sub_488e20() == 3)
        {
            v17 = 4;
            v15 = v2;
            sub_482a90();
            return v18;
        }
        if ((int)v2 < 0)
        {
            v2 = 0;
            if (v5)
                sub_443d80();
            else
                sub_443cd0();
        }
        if (*(int *)&v23->padding_2c[564] != 1)
            goto LABEL_4872ab;
        if (*(int *)&v23->padding_2c[560] == v2)
            goto LABEL_4872a0;
        if (v2)
            goto LABEL_4872ab;
        node = *(int *)&v23->padding_2c[568];
        break;
    case 339:
        if (sub_488e20() == 3)
        {
            v17 = 4;
            v15 = v2;
            sub_482a90();
            return v18;
        }
        if ((int)v2 < 0)
        {
            v2 = 0;
            if (v5)
                sub_443d80();
            else
                sub_443cd0();
        }
        if (!*(int *)&v23->padding_2c[564] || *(int *)&v23->padding_2c[564] == 3)
        {
LABEL_4872ab:
            node = 0xffffffff;
            break;
        }
        else if (*(int *)&v23->padding_2c[560] == v2)
        {
LABEL_4872a0:
            node = *(int *)&v23->padding_2c[568];
            break;
        }
        else
        {
            if (v2)
                goto LABEL_4872ab;
            node = *(int *)&v23->padding_2c[568];
            break;
        }
    case 340:
        if (sub_488e20() == 3)
        {
            v17 = 4;
            v15 = v2;
            sub_482a90();
            return v18;
        }
        if ((int)v2 < 0)
        {
            v2 = 0;
            if (v5)
                sub_443d80();
            else
                sub_443cd0();
        }
        node = sub_473440(v2);
        break;
    case 341:
        if (sub_488e20() == 3)
        {
            v17 = 4;
            v15 = v2;
            sub_482a90();
            return v18;
        }
        if ((int)v2 < -0x1)
        {
            if (v5)
                sub_443d80();
            else
                sub_443cd0();
        }
        node = sub_446400();
        break;
    case 342:
        if (sub_488e20() == 3)
        {
            v17 = 4;
            v15 = v2;
            sub_482a90();
            return v18;
        }
        if ((int)v2 < -0x1)
        {
            v2 = 0xffffffff;
            if (v5)
            {
                sub_443d80();
                v2 = 0xffffffff;
                goto LABEL_487b42;
            }
            else
            {
                sub_443cd0();
            }
        }
LABEL_487b42:
        node = sub_465190(v2);
        break;
    case 343:
        if (sub_488e20() == 3)
        {
            v17 = 4;
            v15 = v2;
            sub_482a90();
            return v18;
        }
        if (v2 <= 59)
        {
            node = *(int *)&v23->padding_2c[3696 + 4 * v2];
            break;
        }
        else
        {
            if (v5)
                sub_443d80();
            else
                sub_443cd0();
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
    case 344:
        if (sub_488e20() == 3)
        {
            v17 = 4;
            v15 = v2;
            sub_482a90();
            return v18;
        }
        node = v2;
        if (node <= 39)
        {
            v18 = *(float *)&v23->padding_2c[3936 + 4 * node];
            v3 = (st_4861e0_1 *)(float)v18;
            break;
        }
        else
        {
            if (v5)
                sub_443d80();
            else
                sub_443cd0();
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
    case 345:
        if (sub_488e20() == 3)
        {
            v17 = 4;
            v15 = v2;
            sub_482a90();
            return v18;
        }
        if (v2 <= 4)
        {
            node = *(int *)&v23->padding_2c[0x1000 + 4 * v2];
            break;
        }
        else
        {
            if (v5)
                sub_443d80();
            else
                sub_443cd0();
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
    case 346:
        if (sub_488e20() == 3)
        {
            v17 = 4;
            v15 = v2;
            sub_482a90();
            return v18;
        }
        node = v2;
        if (node <= 4)
        {
            v18 = *(float *)&v23->padding_2c[4116 + 4 * node];
            v3 = (st_4861e0_1 *)(float)v18;
            break;
        }
        else
        {
            if (v5)
                sub_443d80();
            else
                sub_443cd0();
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
    case 347:
        if (sub_488e20() == 3)
        {
            v17 = 4;
            v15 = v2;
            sub_482a90();
            return v18;
        }
        if ((int)v2 < 0)
        {
            v2 = 0;
            if (v5)
                sub_443d80();
            else
                sub_443cd0();
        }
        if (*(int *)&v23->padding_2c[564] != 3)
            goto LABEL_4872ab;
        if (*(int *)&v23->padding_2c[560] == v2)
            goto LABEL_4872a0;
        if (v2)
            goto LABEL_4872ab;
        node = *(int *)&v23->padding_2c[568];
        break;
    case 348:
        if (sub_488e20() == 3)
        {
            v17 = 4;
            v15 = v2;
            sub_482a90();
            return v18;
        }
        node = sub_402490();
        if (node == 0xffffffff)
        {
            v17 = 4;
            v15 = 0;
            sub_482a90();
            return v18;
        }
    case 349:
        v18 = (double)v3;
        if (sub_488ec0() == 3)
        {
            v17 = 4;
            v15 = sub_499690();
            sub_482a90();
            return v18;
        }
        v1 = v68;
        sub_456e40((float)v18);
LABEL_4867ee:
        sub_4652a0((float)v18);
        v3 = (st_4861e0_1 *)(float)v18;
        break;
    case 350:
        if (sub_488ec0() == 3)
        {
            v17 = 4;
            v15 = sub_499690();
            sub_482a90();
            return (double)v3;
        }
        v16 = sub_499690();
        v15 = (float)((double)v16 * (double)v3 / 640.0);
        v18 = v15;
        goto LABEL_4867ed;
        goto LABEL_4867ed;
    case 351:
        if (sub_488ec0() == 3)
        {
            v17 = 4;
            v15 = sub_499690();
            sub_482a90();
            return (double)v3;
        }
        v16 = sub_499690();
        v15 = (float)((double)v16 * (double)v3 / 1280.0);
        v18 = v15;
        goto LABEL_4867ed;
        goto LABEL_4867ed;
    case 400:
        v88 = *(int *)&v19->padding_2c[8];
        v89 = sub_4393b0(v23, v88, &v15 + 1, &v13);
        v90 = v89 - 1;
        if (v89 != 1)
        {
            if (v90 != 1)
            {
                if (v90 == 2)
                    goto LABEL_4887bf;
                sub_443cd0();
            }
            else
            {
                v18 = 0.0;
                if (0.0 != (double)v13)
                    goto LABEL_4887f2;
            }
LABEL_488774:
            v91 = v88 + 24;
        }
        else
        {
            if (!*(unsigned int *)((void*)&a0 + 4))
                goto LABEL_488774;
LABEL_4887f2:
            v91 = v88 + 12;
        }
        v92 = sub_4393b0(v23, v91, &v15 + 1, &v13);
        v93 = v92 - 1;
        if (v92 == 1)
        {
            v1 = 0;
            v61 = *(unsigned int *)((void*)&a0 + 4);
            sub_482a90();
            return v18;
        }
        else if (v93 == 1)
        {
            v1 = 1;
            v15 = v13;
            v61 = v15;
            sub_482a90();
            return (double)v13;
        }
        else if (v93 != 2)
        {
            sub_443cd0();
            v1 = 4;
            v61 = 0;
            sub_482a90();
            return v18;
        }
LABEL_4887bf:
        v1 = 4;
        v61 = *(unsigned int *)((void*)&a0 + 4);
        sub_482a90();
        return v18;
    default:
        v1 = v10;
        sprintf(g_52b40c + 73820, "Missing trigger code %i");
        sub_443e80(); /* do not return */
    }
    v85 = *(unsigned int *)((void*)&a0 + 4);
    v86 = v85->field_c;
    if (v86 == 20 || v86 == 2 || (int)v86 >= 90 && (int)v86 <= 93 || v86 == 43 || v86 == 44 || v86 == 45 || v86 == 46 || v86 == 7 || v86 == 62 || v86 == 64 || v86 == 63 || v86 == 94 || v86 == 121 || (v87 = sub_482ad0(), sub_482ad0()))
    {
        switch (v14)
        {
        case 0:
            node = node == v85->field_1c;
            break;
        case 1:
            node = node != v85->field_1c;
            break;
        case 2:
            node = (int)node > (int)v85->field_1c;
            break;
        case 3:
            node = (int)node >= (int)v85->field_1c;
            break;
        case 4:
            node = (int)node < (int)v85->field_1c;
            break;
        case 5:
            node = (int)node <= (int)v85->field_1c;
            break;
        case 10:
            if ((int)node >= (int)v85->field_1c && (int)node <= *(int *)&v85->padding_20[0])
                goto LABEL_48870e;
            goto LABEL_4886c1;
        case 11:
            goto LABEL_4886c1;
        case 12:
            if ((int)node <= (int)v85->field_1c || (int)node > *(int *)&v85->padding_20[0])
                goto LABEL_4886c1;
            node = 1;
            break;
        case 13:
            if ((int)node <= (int)v85->field_1c || (int)node >= *(int *)&v85->padding_20[0])
            {
LABEL_4886c1:
                node = 0;
                break;
            }
            else
            {
LABEL_48870e:
                node = 1;
                break;
            }
        }
        if (v13)
            node = 1 - node;
    }
    if (!v85->field_18)
    {
        v1 = 0;
        v61 = node;
        sub_482a90();
        return v18;
    }
    else if (v85->field_18 == 1)
    {
        v15 = v3;
        v17 = 1;
        sub_482a90();
        return (double)v3;
    }
LABEL_4862c1:
    sub_443e80(); /* do not return */
}
