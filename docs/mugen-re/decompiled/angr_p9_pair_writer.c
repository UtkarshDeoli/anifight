// pass9 pair_writer entry=0x46eb80 (asked 0x46eb80)
typedef struct st_46eb80_0 {
    char padding_0[164];
    unsigned int field_a4;
} st_46eb80_0;

typedef struct st_46eb80_5 {
    char padding_0[4];
    unsigned int field_4;
    char padding_8[16];
    unsigned int field_18;
} st_46eb80_5;

typedef struct st_46eb80_4 {
    int field_0;
} st_46eb80_4;

typedef struct st_46eb80_3 {
    struct st_46eb80_0 *field_0;
    unsigned int field_4;
    unsigned int field_8;
    unsigned int field_c;
    unsigned int field_10;
    unsigned int field_14;
    char padding_18[76];
    unsigned int field_64;
    char padding_68[28];
    unsigned int field_84;
    unsigned int field_88;
    unsigned int field_8c;
    unsigned int field_90;
    char padding_94[308];
    struct st_46eb80_4 *field_1c8;
    unsigned int field_1cc;
    char padding_1d0[28];
    unsigned int field_1ec;
    char padding_1f0[4];
    unsigned int field_1f4;
    unsigned int field_1f8;
    char padding_1fc[28];
    unsigned int field_218;
    char padding_21c[80];
    struct st_46eb80_5 *field_26c;
    char padding_270[2504];
    unsigned int field_c38;
    unsigned int field_c3c;
    char padding_c40[536];
    unsigned int field_e58;
    char padding_e5c[24];
    unsigned int field_e74;
    char padding_e78[20];
    unsigned int field_e8c;
    unsigned int field_e90;
    char padding_e94[448];
    unsigned int field_1054;
    char padding_1058[348];
    unsigned int field_11b4;
    char padding_11b8[4];
    unsigned int field_11bc;
    unsigned int field_11c0;
    char padding_11c4[28];
    unsigned int field_11e0;
    unsigned int field_11e4;
    char padding_11e8[8];
    unsigned int field_11f0;
    char padding_11f4[12];
    unsigned int field_1200;
    unsigned int field_1204;
    unsigned int field_1208;
    char padding_120c[28];
    char field_1228;
    char padding_1229[11];
    unsigned int field_1234;
    unsigned int field_1238;
    unsigned int field_123c;
    unsigned int field_1240;
    unsigned int field_1244;
    char padding_1248[4];
    unsigned int field_124c;
    char padding_1250[4];
    unsigned int field_1254;
    unsigned int field_1258;
    unsigned int field_125c;
    char padding_1260[68];
    int field_12a4;
    char padding_12a8[8];
    int field_12b0;
    int field_12b4;
    unsigned int field_12b8;
    char padding_12bc[12];
    int field_12c8;
    int field_12cc;
    char padding_12d0[128];
    unsigned int field_1350;
    int field_1354;
    char padding_1358[156];
    unsigned int field_13f4;
    unsigned int field_13f8;
    unsigned int field_13fc;
} st_46eb80_3;

typedef struct st_46eb80_1 {
    struct st_46eb80_0 *field_0;
    int field_4;
    char padding_8[480];
    unsigned int field_1e8;
    char padding_1ec[44];
    unsigned int field_218;
    char padding_21c[1940];
    unsigned int field_9b0;
    char padding_9b4[644];
    unsigned int field_c38;
    unsigned int field_c3c;
    unsigned int field_c40;
    unsigned int field_c44;
    char padding_c48[532];
    unsigned int field_e5c;
    char padding_e60[4];
    unsigned int field_e64;
    char padding_e68[12];
    unsigned int field_e74;
    char padding_e78[20];
    unsigned int field_e8c;
    unsigned int field_e90;
    char padding_e94[448];
    unsigned int field_1054;
    unsigned int field_1058;
    unsigned int field_105c;
    unsigned int field_1060;
    unsigned int field_1064;
    char padding_1068[96];
    unsigned int field_10c8;
    char padding_10cc[52];
    unsigned int field_1100;
    unsigned int field_1104;
    char padding_1108[168];
    unsigned int field_11b0;
    char padding_11b4[252];
    unsigned int field_12b0;
    int field_12b4;
    char padding_12b8[384];
    unsigned int field_1438;
} st_46eb80_1;

typedef struct st_46eb80_6 {
    char padding_0[7028];
    float field_1b74;
    char padding_1b78[62336];
    float field_10ef8;
    float field_10efc;
    char padding_10f00[16];
    float field_10f10;
    float field_10f14;
    char padding_10f18[2040];
    unsigned int field_11710;
} st_46eb80_6;

typedef struct st_46eb80_7 {
    char padding_0[71672];
    unsigned int field_117f8;
    char padding_117fc[8];
    int field_11804;
    unsigned int field_11808;
} st_46eb80_7;

extern st_46eb80_6 *g_52b40c;

void sub_46eb80(st_46eb80_3 *a0, st_46eb80_1 *a1)
{
    st_46eb80_1 *v22;  // edi
    unsigned int v23;  // ebx
    double v32;  // mm7
    double v33;  // mm7
    double v34;  // mm7
    double v35;  // mm7
    unsigned int v36;  // eax
    int v37;  // eax
    double v38;  // mm0
    double v39;  // mm0
    unsigned int v40;  // eax
    unsigned int *v41;  // edx
    int v24;  // edx
    unsigned int v42;  // eax
    unsigned int v43;  // ecx
    st_46eb80_7 *v44;  // eax
    st_46eb80_1 *iter;  // edi
    st_46eb80_3 *i;  // esi
    unsigned int v47;  // ecx
    st_46eb80_3 *v48;  // esi
    unsigned int v49;  // edx
    st_46eb80_1 *v50;  // esi
    double v51;  // mm0
    st_46eb80_1 *node;  // ecx
    double v52;  // mm0
    unsigned int v26;  // eax
    st_46eb80_3 *v27;  // esi
    unsigned int v28;  // eax
    st_46eb80_3 *v29;  // ecx
    st_46eb80_6 *v30;  // ebx
    unsigned int v31;  // eax
    unsigned int v0;  // [bp-0x40]
    float v1;  // [bp-0x3c]
    unsigned int v2;  // [bp-0x3c], Other Possible Types: int
    float v3;  // [bp-0x38]
    unsigned int v4;  // [bp-0x30], Other Possible Types: float
    int v5;  // [bp-0x30]
    unsigned int v6;  // [bp-0x2c]
    float v7;  // [bp-0x2c]
    int v8;  // [bp-0x28], Other Possible Types: unsigned int
    float v9;  // [bp-0x28]
    unsigned int v10;  // [bp-0x24]
    int v11;  // [bp-0x20]
    st_46eb80_1 *v12;  // [bp-0x1c]
    unsigned int v13;  // [bp-0x1c]
    unsigned int v14;  // [bp-0x18]
    unsigned int v15;  // [bp-0x14]
    unsigned int v16;  // [bp-0x10]
    unsigned int v17;  // [bp-0xc]
    unsigned int v18;  // [bp-0x8]
    unsigned int v19;  // [bp-0x4]
    st_46eb80_3 *idx;  // [bp+0x4]
    float v21;  // [bp+0x8]

    v22 = a1;
    v10 = *(int *)&a0->padding_1d0[24];
    v11 = v22->field_1e8 != 1;
    if (a0->field_11e0)
    {
        if (!a0->field_11b4)
        {
            sub_470140(a0, v22);
            a0->field_11b4 = 1;
        }
        else
        {
            return;
        }
    }
    v23 = 0;
    v0 = 0;
    v24 = 0;
    node = &v22->field_1100;
    do
    {
        if (node->field_0 && (v26 = node->field_4 & a0->field_11bc, (char)v26 & 7 && v26 & 0xfffffff8))
        {
            v23 = 1;
            v0 = 1;
            break;
        }
    } while ((v24 += 1, node = &node->padding_8[12], v24 < 8));
    v27 = &a0->padding_1058[344];
    if (a0->field_1238 == 0xffffffff)
    {
        v0 = 0;
    }
    else
    {
        idx = a0;
        if (v23)
            goto LABEL_46ecb6;
    }
    idx = sub_4650e0();
    if (!idx)
    {
        v13 = 0;
        v14 = 0;
        v15 = 0;
        v16 = 0;
        v17 = 0;
        v18 = 0;
        v19 = 0;
        v12 = v22;
        v14 = a0->field_64;
        v28 = sub_4030f0(&v12, 0, 0);
        if (v28 == 0xffffffff)
            return;
        v29 = v28 * a0->field_26c->field_4 + a0->field_26c->field_18;
        idx = v29;
    }
    else
    {
        v29 = idx;
    }
    v27 = &a0->padding_1058[344];
    memset(&v29->field_c, 0, 12);
    v29->field_8 = v27->field_10;
LABEL_46ecb6:
    v30 = g_52b40c;
    a0->field_218 = *(int *)&v27->padding_94[20];
    v31 = v22->field_c3c;
    v22->field_218 = *(int *)&v27->padding_94[24];
    if (v22->field_1e8 != v10)
    {
        v1 = (float)(int)v22->field_12b0;
        if (v31 && v22->field_c38 != 0xffffffff && *(int *)&g_52b40c->padding_10f18[792 + 4 * v22->field_c38])
            v32 = *(float *)(*(int *)*(int *)&g_52b40c->padding_10f18[792 + 4 * v22->field_c38] + 164);
        else
            v32 = *(float *)&v22->field_0->field_a4;
        v4 = (float)(v32 * v1);
        v1 = (float)*(int *)&v27->padding_94[108];
        if (a0->field_c3c && a0->field_c38 != 0xffffffff && *(int *)&g_52b40c->padding_10f18[792 + 4 * a0->field_c38])
            v33 = *(float *)(*(int *)*(int *)&g_52b40c->padding_10f18[792 + 4 * a0->field_c38] + 164);
        else
            v33 = *(float *)&a0->field_0->field_a4;
        v6 = *(int *)&a0->padding_1d0[24];
        v3 = (float)(v33 * v1);
        v2 = (a0->field_e58 == 3 ? a0->field_90 : a0->field_88);
        v8 = &a0->field_0->field_a4;
        v2 *= *(unsigned int *)&*(float *)v8;
        floor(v2 + 0.5f);
        v2 = sub_499690();
    }
    else
    {
        v8 = -v22->field_12b0;
        v4 = (float)(int)v8;
        if (v31 && v22->field_c38 != 0xffffffff && *(int *)&g_52b40c->padding_10f18[792 + 4 * v22->field_c38])
            v34 = *(float *)(*(int *)*(int *)&g_52b40c->padding_10f18[792 + 4 * v22->field_c38] + 164);
        else
            v34 = *(float *)&v22->field_0->field_a4;
        v3 = (float)(v34 * v4);
        v4 = (float)*(int *)&v27->padding_94[108];
        if (a0->field_c3c && a0->field_c38 != 0xffffffff && *(int *)&g_52b40c->padding_10f18[792 + 4 * a0->field_c38])
            v35 = *(float *)(*(int *)*(int *)&g_52b40c->padding_10f18[792 + 4 * a0->field_c38] + 164);
        else
            v35 = *(float *)&a0->field_0->field_a4;
        v36 = -*(int *)&a0->padding_1d0[24];
        v4 = (float)(v35 * v4);
        v6 = *(int *)&a0->padding_1d0[24];
        if (a0->field_e58 == 3)
        {
            if (v6 == v36)
            {
                v37 = a0->field_90;
                goto LABEL_46eeb0;
            }
            else
            {
                v2 = a0->field_8c;
            }
        }
        else
        {
            if (v6 == v36)
            {
                v2 = a0->field_88;
            }
            else
            {
                v37 = a0->field_84;
LABEL_46eeb0:
                v2 = v37;
            }
        }
        v8 = &a0->field_0->field_a4;
        v5 = v2 * *(unsigned int *)&*(float *)v8;
        v3 -= v4;
        floor(v5 + 0.5f);
        v4 = sub_499690();
    }
    v2 = sub_499690();
    *(unsigned int *)&v7 = *(unsigned int *)&g_52b40c->field_10f10 * *(unsigned int *)&g_52b40c->field_1b74;
    v7 += g_52b40c->field_10ef8;
    v2 = sub_499690();
    v7 = (float)v22->field_12b4;
    if (v22->field_c3c && v22->field_c38 != 0xffffffff && *(int *)&g_52b40c->padding_10f18[792 + 4 * v22->field_c38])
        v38 = *(float *)(*(int *)*(int *)&g_52b40c->padding_10f18[792 + 4 * v22->field_c38] + 164);
    else
        v38 = *(float *)&v22->field_0->field_a4;
    v4 = (float)(v38 * v7);
    v7 = (float)*(int *)&v27->padding_94[112];
    if (a0->field_c3c && a0->field_c38 != 0xffffffff && *(int *)&g_52b40c->padding_10f18[792 + 4 * a0->field_c38])
        v39 = *(float *)(*(int *)*(int *)&g_52b40c->padding_10f18[792 + 4 * a0->field_c38] + 164) * v7;
    else
        v39 = v7 * *(float *)v8;
    v9 = (float)v39;
    *(unsigned int *)&v9 = *(unsigned int *)&g_52b40c->field_10f14 * *(unsigned int *)&g_52b40c->field_1b74;
    v9 += g_52b40c->field_10efc;
    v8 = sub_499690();
    if (!v0)
    {
        sub_46db20(v27, *(float *)&a0->field_1f4, *(float *)&a0->field_1f8, v10, 0);
        if (v22->field_10c8 || v27->padding_68[16] & 1)
        {
            idx->field_4 = idx->field_4 - *(int *)&v27->padding_18[28];
            *(unsigned int *)&v27->padding_18[28] = 0;
        }
        v30 = g_52b40c;
        if (*(int *)&v27->padding_94[116])
            a0->field_1ec = -*(int *)&a0->padding_1d0[24];
    }
    a0->field_e74 = *(int *)&v27->padding_18[60];
    v22->field_e74 = *(int *)&v27->padding_18[56];
    if (v30->field_11710 != 3 && v30->field_11710 != 4)
    {
        *(int *)a0->field_1c8 = *(int *)a0->field_1c8 + *(int *)&v27->padding_18[40];
        if (*(int *)a0->field_1c8 < 0)
            *(unsigned int *)a0->field_1c8 = 0;
        if (*(int *)a0->field_1c8 > *(int *)a0->field_1cc)
            *(int *)a0->field_1c8 = *(int *)a0->field_1cc;
    }
    v22->field_105c = 0xfff;
    v22->field_1058 = 0xfff;
    v22->field_e8c = 4;
    v22->field_e90 = 0;
    v22->field_1060 = 0;
    v22->field_1064 = 0;
    v22->field_1054 = *(int *)&v27->padding_18[60] + 1;
    if (v27->field_c & 448)
        a0->field_1054 = *(int *)&v27->padding_18[60] + 1;
    a0->field_13f4 = a0->field_13f4 + *(int *)&v27->padding_18[64];
    a0->field_13f8 = a0->field_13f8 + *(int *)&v27->padding_18[64];
    a0->field_13fc = a0->field_13fc + *(int *)&v27->padding_18[64];
    v40 = a0->field_c;
    a0->field_e90 = 0;
    a0->field_e8c = 2;
    v41 = &v30->padding_0[468 * v40 + 56568];
    if (v40 == 1 || v40 == 2)
    {
        v42 = v40 * 3;
        *(int *)&v30[1].padding_0[232 + 8 * v42] = *(int *)&v30[1].padding_0[232 + 8 * v42] + *(int *)&v27->padding_18[64];
        v43 = *(int *)&v30[1].padding_0[232 + 8 * v42];
        v44 = &v30->padding_0[8 * v42 + 0x117f8];
        if (!*(int *)&v30[1].padding_0[228 + 8 * v42])
        {
            if ((int)v43 < 2)
                goto LABEL_46f1b6;
            *(unsigned int *)&v44->padding_0[16] = v41[2];
        }
        if ((int)v43 >= 2)
        {
            *(unsigned int *)&v44->padding_0[0] = v41[3];
            *(unsigned int *)&v44->padding_0[12] = v43;
            if ((int)v43 > 999)
                *(int *)&v44->padding_0[12] = 999;
        }
    }
LABEL_46f1b6:
    if (!v0 && (*(int *)&v27->padding_94[272] > 0 || *(int *)&v27->padding_94[272] == 0xffffffff))
    {
        iter = &v22->field_1438;
        i = &a0->field_1350;
        for (v47 = 15; v47; i = &i->field_4)
        {
            v47 -= 1;
            iter->field_0 = i->field_0;
            iter = &iter->field_4;
        }
        v22 = a1;
    }
    v48 = &a0->padding_1058[344];
    sub_470260(a0, v2, v8, a0->field_12a4, v11, 0);
    if (!v0)
    {
        if (v48->field_84 != 0xffffffff)
        {
            sub_46af10();
            v22 = a1;
        }
        if (a0->field_123c)
        {
            v22->field_c3c = a0->field_1244;
            v22->field_c38 = a0->field_1240;
            v49 = v22->field_9b0;
            v22->field_c44 = a0->field_124c;
            *(char [4])&v22->field_c40 = a0->padding_1248;
            if (v49 != a0->padding_1248)
            {
                memset(v22->padding_9b4, 0, 128);
                *(char [4])&v22->field_9b0 = a0->padding_1248;
            }
            v22->field_e5c = 2;
        }
        if (a0->field_1238 != 0xffffffff)
        {
            v50 = a1;
            sub_470140(v50, 0);
            sub_446320(v50->field_4);
            v22 = v50;
            sub_46af10();
            v50->field_e64 = 0;
        }
        v48 = &a0->padding_1058[344];
        idx->field_14 = a0->field_1254;
        v21 = (float)*(int *)&v48->padding_94[132];
        if (a0->field_c3c && a0->field_c38 != 0xffffffff && *(int *)&g_52b40c->padding_10f18[792 + 4 * a0->field_c38])
            v51 = *(float *)(*(int *)*(int *)&g_52b40c->padding_10f18[792 + 4 * a0->field_c38] + 164);
        else
            v51 = *(float *)&a0->field_0->field_a4;
        v21 = (float)(v51 * v21);
        idx->field_c = sub_499690();
        v21 = (float)*(int *)&v48->padding_94[0x88];
        if (a0->field_c3c && a0->field_c38 != 0xffffffff && *(int *)&g_52b40c->padding_10f18[792 + 4 * a0->field_c38])
            v52 = *(float *)(*(int *)*(int *)&g_52b40c->padding_10f18[792 + 4 * a0->field_c38] + 164);
        else
            v52 = *(float *)&a0->field_0->field_a4;
        v21 = (float)(v52 * v21);
        idx->field_10 = sub_499690();
    }
    v22->field_11b0 = 0;
    if (*(int *)&v48->padding_18[24])
        v48->field_4 = 1;
    return;
}
