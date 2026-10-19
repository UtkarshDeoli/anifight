// pass9 hitresolve entry=0x46d2d0 (asked 0x46d2d0)
typedef struct st_46d2d0_1 {
    char padding_0[164];
    unsigned int field_a4;
} st_46d2d0_1;

typedef struct st_46d2d0_4 {
    char padding_0[4];
    unsigned int field_4;
    char padding_8[16];
    unsigned int field_18;
} st_46d2d0_4;

typedef struct st_46d2d0_3 {
    int field_0;
} st_46d2d0_3;

typedef struct st_46d2d0_7 {
    char padding_0[4516];
    float field_11a4;
} st_46d2d0_7;

typedef struct st_46d2d0_2 {
    struct st_46d2d0_1 *field_0;
    char padding_4[8];
    unsigned int field_c;
    char padding_10[84];
    unsigned int field_64;
    char padding_68[352];
    struct st_46d2d0_3 *field_1c8;
    unsigned int field_1cc;
    char padding_1d0[28];
    unsigned int field_1ec;
    char padding_1f0[4];
    unsigned int field_1f4;
    unsigned int field_1f8;
    char padding_1fc[28];
    unsigned int field_218;
    char padding_21c[59];
    char field_257;
    char padding_258[20];
    struct st_46d2d0_4 *field_26c;
    unsigned int field_270;
    float field_274;
    char padding_278[12];
    unsigned int field_284;
    char padding_288[2480];
    unsigned int field_c38;
    unsigned int field_c3c;
    char padding_c40[564];
    unsigned int field_e74;
    char padding_e78[20];
    unsigned int field_e8c;
    unsigned int field_e90;
    int field_e94;
    char padding_e98[444];
    unsigned int field_1054;
    char padding_1058[344];
    unsigned int field_11b0;
    char padding_11b4[8];
    unsigned int field_11bc;
    unsigned int field_11c0;
    char padding_11c4[16];
    int field_11d4;
    int field_11d8;
    char padding_11dc[4];
    unsigned int field_11e0;
    unsigned int field_11e4;
    int field_11e8;
    int field_11ec;
    unsigned int field_11f0;
    unsigned int field_11f4;
    char padding_11f8[12];
    unsigned int field_1204;
    int field_1208;
    char padding_120c[28];
    char field_1228;
    char padding_1229[11];
    unsigned int field_1234;
    unsigned int field_1238;
    unsigned int field_123c;
    char padding_1240[20];
    unsigned int field_1254;
    unsigned int field_1258;
    unsigned int field_125c;
    char padding_1260[68];
    int field_12a4;
    char padding_12a8[4];
    int field_12ac;
    unsigned int field_12b0;
    int field_12b4;
    unsigned int field_12b8;
    unsigned int field_12bc;
    char padding_12c0[8];
    int field_12c8;
    int field_12cc;
    char padding_12d0[40];
    unsigned int field_12f8;
    char padding_12fc[56];
    unsigned int field_1334;
    char padding_1338[24];
    unsigned int field_1350;
    int field_1354;
    char padding_1358[76];
    unsigned int field_13a4;
    char padding_13a8[76];
    unsigned int field_13f4;
    unsigned int field_13f8;
    unsigned int field_13fc;
} st_46d2d0_2;

typedef struct st_46d2d0_6 {
    struct st_46d2d0_1 *field_0;
    int field_4;
    char padding_8[20];
    unsigned int field_1c;
    char padding_20[100];
    unsigned int field_84;
    unsigned int field_88;
    unsigned int field_8c;
    unsigned int field_90;
    char padding_94[292];
    unsigned int field_1b8;
    char padding_1bc[44];
    unsigned int field_1e8;
    char padding_1ec[44];
    unsigned int field_218;
    char padding_21c[3132];
    unsigned int field_e58;
    unsigned int field_e5c;
    char padding_e60[4];
    unsigned int field_e64;
    unsigned int field_e68;
    char padding_e6c[488];
    int field_1054;
    char padding_1058[44];
    unsigned int field_1084;
    char padding_1088[64];
    unsigned int field_10c8;
    char padding_10cc[52];
    unsigned int field_1100;
    unsigned int field_1104;
    unsigned int field_1108;
    char padding_110c[812];
    unsigned int field_1438;
} st_46d2d0_6;

typedef struct st_46d2d0_0 {
    char padding_0[7028];
    float field_1b74;
    char padding_1b78[62256];
    int field_10ea8;
    char padding_10eac[60];
    unsigned int field_10ee8;
    char padding_10eec[12];
    float field_10ef8;
    float field_10efc;
    char padding_10f00[16];
    float field_10f10;
    float field_10f14;
    char padding_10f18[2040];
    unsigned int field_11710;
    char padding_11714[1176];
    unsigned int field_11bac;
} st_46d2d0_0;

extern st_46d2d0_0 *g_52b40c;

void sub_46d2d0(st_46d2d0_2 *a0, st_46d2d0_6 *a1)
{
    st_46d2d0_0 *v23;  // ebx
    st_46d2d0_6 *v24;  // esi
    unsigned int v33;  // ecx
    double v34;  // mm7
    unsigned int v35;  // eax
    unsigned int v36;  // edx
    double v37;  // mm7
    double v38;  // mm0
    unsigned int v39;  // eax
    unsigned int v40;  // ebx
    int *v41;  // edx
    int v42;  // edx
    st_46d2d0_2 *v25;  // edi
    st_46d2d0_2 *i;  // esi
    st_46d2d0_6 *iter;  // edi
    unsigned int v45;  // ecx
    st_46d2d0_7 *v46;  // ecx
    st_46d2d0_7 *v47;  // ecx
    st_46d2d0_6 *v48;  // esi
    double v51;  // mm0
    unsigned int v26;  // eax
    unsigned int v27;  // eax
    int v28;  // edx
    st_46d2d0_6 *node;  // ecx
    unsigned int v30;  // eax
    unsigned int *v31;  // eax
    unsigned int v32;  // eax
    st_46d2d0_1 *v0;  // [bp-0x6c]
    st_46d2d0_7 *v1;  // [bp-0x68], Other Possible Types: unsigned int
    unsigned int v2;  // [bp-0x50]
    int v3;  // [bp-0x4c]
    unsigned int v4;  // [bp-0x48]
    unsigned int *v5;  // [bp-0x44]
    int v6;  // [bp-0x40], Other Possible Types: double, unsigned int
    unsigned int v7;  // [bp-0x34]
    st_46d2d0_6 *v8;  // [bp-0x30]
    int v9;  // [bp-0x2c], Other Possible Types: unsigned int
    float v10;  // [bp-0x2c]
    int v11;  // [bp-0x28]
    unsigned int v12;  // [bp-0x24], Other Possible Types: float
    float v13;  // [bp-0x20]
    unsigned int v14;  // [bp-0x1c]
    st_46d2d0_6 *v15;  // [bp-0x1c]
    unsigned int v16;  // [bp-0x18]
    unsigned int v17;  // [bp-0x14]
    unsigned int v18;  // [bp-0x10]
    unsigned int v19;  // [bp-0xc]
    unsigned int v20;  // [bp-0x8]
    unsigned int v21;  // [bp-0x4]
    st_46d2d0_2 *v22;  // [bp+0x4]

    v23 = g_52b40c;
    v24 = a1;
    v2 = *(int *)&a0->padding_1d0[24];
    v25 = &a0->field_11b0;
    v22 = v25;
    v5 = NULL;
    v11 = *(int *)&a0->padding_1d0[24] != 1;
    v9 = *(int *)&g_52b40c->padding_10f18[1996 + 4 * a0->field_c];
    if (v24->field_1054 > 0)
        return;
    if (*(int *)&v25->padding_10[32])
    {
        if (v25->field_0)
        {
            sub_470140(a0, v24);
            v23 = g_52b40c;
        }
        else
        {
            return;
        }
    }
    v3 = 0;
    v7 = 0;
    if (sub_470030() && v24->field_e68 && !a0->field_257)
    {
        v26 = sub_46e760(a0->field_11ec, a0->field_11d8);
        if ((int)(v24->field_1b8 - v24->field_1084 - v26) > 0)
        {
            v7 = 1;
        }
        else if (v23->field_11bac)
        {
            v3 = 1;
            if (v23->field_11710 != 3 && (int)v9 < 10)
                sub_44edf0();
        }
    }
    else
    {
        v27 = sub_46e760(a0->field_11e8, a0->field_11d4);
        if ((int)(v24->field_1b8 - v24->field_1084 - v27) <= 0 && v24->field_1c != 1)
        {
            v3 = 1;
            sub_44ed40();
        }
    }
    v28 = 0;
    if (v23->field_10ee8)
        v3 = 0;
    v4 = 0;
    node = &v24->field_1100;
    do
    {
        v8 = node;
        if (node->field_0 && (v30 = node->field_4 & v25->field_c, (char)v30 & 7 && v30 & 0xfffffff8))
        {
            v4 = 1;
            v7 = 0;
            v3 = 0;
            if (*(int *)&v25->padding_68[36])
                return;
            if (*(int *)&v25->padding_68[28] != 0xffffffff)
                return;
            goto LABEL_46d4f1;
        }
    } while ((v28 += 1, node = &node->padding_8[12], v28 < 8));
    v8 = NULL;
    v31 = sub_4650e0();
    v5 = v31;
    if (!v31)
    {
        v14 = 0;
        v16 = 0;
        v17 = 0;
        v18 = 0;
        v19 = 0;
        v20 = 0;
        v21 = 0;
        v15 = v24;
        v16 = a0->field_64;
        v32 = sub_4030f0(&v15, 0, 0);
        if (v32 == 0xffffffff)
            return;
        v31 = v32 * a0->field_26c->field_4 + a0->field_26c->field_18;
        v5 = v31;
    }
    v24 = a1;
    memset(v31 + 3, 0, 12);
    v23 = g_52b40c;
    v31[2] = *(int *)&v25->padding_10[0];
LABEL_46d4f1:
    a0->field_218 = *(int *)&v25->padding_68[64];
    v24->field_218 = *(int *)&v25->padding_68[0x44];
    v33 = v24->field_1e8;
    v12 = v33;
    if (v33 != v2)
    {
        v6 = (v24->field_e58 == 3 ? v24->field_90 : v24->field_88);
        v34 = (double)*(int *)&v25->padding_68[152];
        v1 = v33;
        sub_465250((float)v34);
        v0 = v24->field_0;
        v9 = *(unsigned int *)&*(float *)&v24->field_0->field_a4 * v6;
        v6 = v34;
        floor(v9 + 0.5f);
        v9 = sub_499690();
    }
    else
    {
        v35 = -v33;
        if (v24->field_e58 == 3)
        {
            if (v33 == v35)
            {
                v36 = v24->field_90;
                goto LABEL_46d5f7;
            }
            else
            {
                v6 = v24->field_8c;
            }
        }
        else
        {
            if (v33 == v35)
            {
                v33 = v24->field_88;
                v6 = v33;
            }
            else
            {
                v36 = v24->field_84;
LABEL_46d5f7:
                v6 = v36;
            }
        }
        v9 = -*(int *)&v25->padding_68[152];
        v37 = (double)(int)v9;
        v1 = v33;
        sub_465250((float)v37);
        v0 = v24->field_0;
        v9 = *(unsigned int *)&*(float *)&v24->field_0->field_a4 * v6;
        v6 = v37;
        floor(v9 + 0.5f);
        v9 = sub_499690();
    }
    v6 = sub_499690();
    *(unsigned int *)&v12 = *(unsigned int *)&v23->field_10f10 * *(unsigned int *)&v23->field_1b74;
    v12 += v23->field_10ef8;
    v6 = sub_499690();
    v10 = (float)*(int *)&v25->padding_68[156];
    if (a0->field_c3c && a0->field_c38 != 0xffffffff && *(int *)&v23->padding_10f18[792 + 4 * a0->field_c38])
        v38 = *(float *)(*(int *)*(int *)&v23->padding_10f18[792 + 4 * a0->field_c38] + 164);
    else
        v38 = *(float *)&a0->field_0->field_a4;
    v10 = (float)(v38 * v10);
    *(unsigned int *)&v12 = *(unsigned int *)&v23->field_10f14 * *(unsigned int *)&v23->field_1b74;
    v12 += v23->field_10efc;
    v9 = sub_499690();
    if (!v7)
    {
        if (!v4)
        {
            if (v24->field_10c8 || v25->padding_68[16] & 1 || *(int *)&v25->padding_68[224] && v24->field_e58 == 3)
            {
                v5[1] = v5[1] - a0->field_13a4;
                *(unsigned int *)&v25->padding_10[36] = 0;
            }
            if (*(int *)&v25->padding_68[160])
            {
                v39 = -*(int *)&a0->padding_1d0[24];
                a0->field_1ec = v39;
                v2 = v39;
            }
            else
            {
                v39 = v2;
            }
            if (*(int *)&v25->padding_68[164])
            {
                if (*(int *)&v25->padding_68[164] == 1)
                {
                    if (v39 != v24->field_1e8)
                        goto LABEL_46d7b1;
                }
                else
                {
                    if (v39 == v24->field_1e8)
                    {
LABEL_46d7b1:
                        v2 = -*(int *)&a0->padding_1d0[24];
                        a0->field_1ec = v2;
                    }
                }
            }
        }
        v40 = sub_46db20(v25, *(float *)&a0->field_1f4, *(float *)&a0->field_1f8, v2, v3);
        if (v40 || v4 != v40)
        {
            v41 = a0->field_1c8;
            a0->field_e74 = *(int *)&v25->padding_10[0x44];
            sub_46b070(*v41 + *(int *)&v25->padding_10[48]);
            v42 = a0->field_e94;
            a0->field_13f4 = a0->field_13f4 + *(int *)&v25->padding_10[72];
            if (v42 < g_52b40c->field_10ea8)
                a0->field_13f8 = a0->field_13f8 + *(int *)&v25->padding_10[72];
            a0->field_13fc = a0->field_13fc + *(int *)&v25->padding_10[72];
            a0->field_e8c = 2;
            a0->field_e90 = 0;
            sub_44c970(*(int *)&v25->padding_10[72]);
            if (!v4)
            {
                if (*(int *)&v25->padding_68[316] > 0 || *(int *)&v25->padding_68[316] == 0xffffffff)
                {
                    i = &v25->padding_68[312];
                    iter = &a1->field_1438;
                    for (v45 = 15; v45; i = i->padding_4)
                    {
                        v45 -= 1;
                        iter->field_0 = i->field_0;
                        iter = &iter->field_4;
                    }
                    v25 = v22;
                }
                if (v25->field_c & 448)
                {
                    a1->field_1054 = *(int *)&v25->padding_10[0x44] + 1;
                    a0->field_1054 = *(int *)&v25->padding_10[0x44] + 1;
                }
            }
        }
        sub_470260(a0, v6, v9, *(int *)&v25->padding_68[140], v11, 0);
        if (!v4)
        {
            if (*(int *)&v25->padding_68[28] != 0xffffffff)
            {
                sub_46af10();
                v25 = v22;
            }
            if (*(int *)&v25->padding_68[36])
            {
                sub_48bb50();
                v25 = v22;
                a1->field_e5c = 2;
            }
            if (*(int *)&v25->padding_68[32] != 0xffffffff)
            {
                v48 = a1;
                sub_470140(v48, 0);
                sub_446320(v48->field_4);
                sub_46af10();
                v25 = v22;
                v48->field_e64 = 0;
            }
            v5[5] = *(int *)&v25->padding_68[60];
            v1 = v47;
            sub_465250((float)*(int *)&v25->padding_68[176]);
            v5[3] = sub_499690();
            v1 = v46;
            sub_465250((float)*(int *)&v25->padding_68[180]);
            v5[4] = sub_499690();
        }
        else if (*(int *)&v8->padding_8[0] != 0xffffffff)
        {
            sub_46af10();
            v25 = v22;
        }
    }
    else
    {
        a0->field_e74 = *(int *)&v25->padding_68[284];
        a0->field_e8c = 1;
        a0->field_e90 = 0;
        v40 = sub_46e7c0(a0, v2);
        if (g_52b40c->field_11710 != 3 && g_52b40c->field_11710 != 4)
        {
            *(int *)a0->field_1c8 = *(int *)a0->field_1c8 + *(int *)&v25->padding_10[52];
            if (*(int *)a0->field_1c8 < 0)
                *(int *)a0->field_1c8 = 0;
            if (*(int *)a0->field_1c8 > *(int *)a0->field_1cc)
                *(int *)a0->field_1c8 = *(int *)a0->field_1cc;
        }
        sub_470260(a0, v6, v9, *(int *)&v25->padding_68[148], v11, 1);
    }
    if (v40 && sub_464c60())
    {
        v51 = v46->field_11a4;
        a0->field_284 = 0xffffffff;
        v12 = (float)(v51 * (double)*(int *)&a0->padding_1d0[24]);
        *(float *)&a0->field_270 = v12;
        v13 = 0.0f;
        *(float *)&a0->padding_278[0] = 0.699999988079071f;
        a0->field_274 = v13;
        *(float *)&a0->padding_278[8] = 1.0f;
    }
    a0->field_e94 = g_52b40c->field_10ea8;
    if (!*(int *)&v25->padding_10[32])
        return;
    v25->field_0 = 0;
    return;
}
