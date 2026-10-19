// pass9 round_pause entry=0x449bc0 (asked 0x449bc0)
typedef struct st_449bc0_7 {
    char padding_0[8];
    unsigned int field_8;
    char padding_c[12];
    unsigned int field_18;
} st_449bc0_7;

typedef struct st_449bc0_2 {
    char padding_0[28];
    unsigned int field_1c;
    char padding_20[400];
    unsigned int field_1b0;
    char padding_1b4[2712];
    unsigned int field_c4c;
    unsigned int field_c50;
    char padding_c54[512];
    unsigned int field_e54;
} st_449bc0_2;

typedef struct st_449bc0_14 {
    char padding_0[28];
    unsigned int field_1c;
    char padding_20[400];
    unsigned int field_1b0;
    char padding_1b4[2712];
    unsigned int field_c4c;
    char padding_c50[516];
    int field_e54;
    unsigned int field_e58;
    unsigned int field_e5c;
    char padding_e60[4];
    unsigned int field_e64;
} st_449bc0_14;

typedef struct st_449bc0_10 {
    unsigned int field_0;
    char padding_4[24];
    unsigned int field_1c;
    char padding_20[400];
    unsigned int field_1b0;
    char padding_1b4[2044];
    unsigned int field_9b0;
    char field_9b4;
    char padding_9b5[643];
    unsigned int field_c38;
    unsigned int field_c3c;
    unsigned int field_c40;
    unsigned int field_c44;
    char padding_c48[4];
    unsigned int field_c4c;
    unsigned int field_c50;
    char padding_c54[512];
    unsigned int field_e54;
    char padding_e58[12];
    unsigned int field_e64;
} st_449bc0_10;

typedef struct st_449bc0_19 {
    char padding_0[28];
    unsigned int field_1c;
    char padding_20[400];
    unsigned int field_1b0;
    char padding_1b4[2688];
    struct st_449bc0_5 *field_c34;
    char padding_c38[4];
    struct st_449bc0_5 *field_c3c;
    char padding_c40[12];
    unsigned int field_c4c;
    unsigned int field_c50;
    char padding_c54[556];
    unsigned int field_e80;
} st_449bc0_19;

typedef struct st_449bc0_3 {
    char padding_0[4];
    unsigned int field_4;
    unsigned int field_8;
    char padding_c[12];
    unsigned int field_18;
    unsigned int field_1c;
    unsigned int field_20;
    unsigned int field_24;
} st_449bc0_3;

typedef struct st_449bc0_5 {
    struct st_449bc0_3 *field_0;
} st_449bc0_5;

typedef struct st_449bc0_4 {
    char padding_0[28];
    unsigned int field_1c;
    char padding_20[400];
    unsigned int field_1b0;
    char padding_1b4[180];
    struct st_449bc0_5 *field_268;
} st_449bc0_4;

typedef struct st_449bc0_1 {
    char padding_0[70192];
    struct st_449bc0_2 *field_11230;
} st_449bc0_1;

typedef struct st_449bc0_0 {
    char padding_0[7888];
    unsigned int field_1ed0;
    char padding_1ed4[11072];
    unsigned int field_4a14;
    char padding_4a18[6884];
    int field_64fc;
    char padding_6500[6880];
    unsigned int field_7fe0;
    char padding_7fe4[4];
    int field_7fe8;
    unsigned int field_7fec;
    char padding_7ff0[12];
    unsigned int field_7ffc;
    char field_8000;
    char padding_8001[15];
    char field_8010;
    char padding_8011[55];
    char field_8048;
    char padding_8049[15];
    char field_8058;
    char padding_8059[199];
    unsigned int field_8120;
    char padding_8124[24920];
    int field_e27c;
    int field_e280;
    char padding_e284[1772];
    unsigned int field_e970;
    char padding_e974[248];
    unsigned int field_ea6c;
    unsigned int field_ea70;
    char padding_ea74[488];
    unsigned int field_ec5c;
    unsigned int field_ec60;
    unsigned int field_ec64;
    unsigned int field_ec68;
    unsigned int field_ec6c;
    unsigned int field_ec70;
    char padding_ec74[512];
    unsigned int field_ee74;
    char padding_ee78[8212];
    unsigned int field_10e8c;
    double field_10e90;
    double field_10e98;
    char padding_10ea0[20];
    unsigned int field_10eb4;
    unsigned int field_10eb8;
    char padding_10ebc[44];
    unsigned int field_10ee8;
    unsigned int field_10eec;
    unsigned int field_10ef0;
    unsigned int field_10ef4;
    char padding_10ef8[1336];
    unsigned int field_11430;
    char padding_11434[536];
    unsigned int field_1164c;
    char padding_11650[8];
    char field_11658;
    char field_11659;
    char field_1165a;
    char padding_1165b[1];
    char field_1165c;
    char padding_1165d[2];
    char field_1165f;
    char padding_11660[8];
    unsigned int field_11668;
    char padding_1166c[44];
    unsigned int field_11698;
    char padding_1169c[72];
    int field_116e4;
    int field_116e8;
    int field_116ec;
    unsigned int field_116f0;
    unsigned int field_116f4;
    unsigned int field_116f8;
    unsigned int field_116fc;
    unsigned int field_11700;
    unsigned int field_11704;
    int field_11708;
    unsigned int field_1170c;
    unsigned int field_11710;
    unsigned int field_11714;
    unsigned int field_11718;
    unsigned int field_1171c;
    int field_11720;
    unsigned int field_11724;
    unsigned int field_11728;
    unsigned int field_1172c;
    char padding_11730[176];
    unsigned int field_117e0;
    unsigned int field_117e4;
    unsigned int field_117e8;
    char padding_117ec[1008];
    unsigned int field_11bdc;
} st_449bc0_0;

typedef struct st_449bc0_6 {
    char padding_0[732];
    struct st_449bc0_7 *field_2dc;
} st_449bc0_6;

extern unsigned int g_52b34c;
extern unsigned int g_52b370;
extern unsigned int g_52b388;
extern st_449bc0_0 *g_52b40c;
extern int g_52be24;

unsigned int sub_449bc0(void)
{
    st_449bc0_0 *v15;  // ebp
    void* v16;  // ecx
    unsigned int iter;  // edi
    unsigned int *v115;  // edi
    unsigned int v116;  // eax
    unsigned int v117;  // eax
    unsigned int v118;  // eax
    unsigned int v119;  // ecx
    unsigned int v120;  // eax
    unsigned int v121;  // eax
    unsigned int v122;  // eax
    unsigned int v123;  // eax
    unsigned int v124;  // eax
    int v26;  // ebx
    unsigned int v125;  // edx
    unsigned int v126;  // eax
    unsigned int v127;  // ebx
    int v128;  // esi
    int v129;  // ecx
    unsigned int v130;  // ecx
    unsigned int v131;  // eax
    unsigned int v132;  // edx
    st_449bc0_1 *iter1;  // eax
    unsigned int *v134;  // ecx
    int v27;  // esi
    unsigned int iter2;  // ecx
    unsigned int *v29;  // eax
    unsigned int *v30;  // edx
    unsigned int v31;  // eax
    unsigned int v32;  // ecx
    int v33;  // esi
    unsigned int v34;  // eax
    unsigned int v17;  // esi
    int v35;  // esi
    unsigned int v36;  // eax
    int v37;  // esi
    unsigned int v38;  // eax
    int v39;  // esi
    unsigned int v40;  // eax
    unsigned int v41;  // ecx
    unsigned int v42;  // eax
    st_449bc0_1 *v43;  // edx
    unsigned int *v44;  // eax
    int v18;  // ebx
    st_449bc0_4 **v45;  // ebx
    st_449bc0_4 *v46;  // eax
    unsigned int *v47;  // esi
    unsigned int v48;  // eax
    unsigned int v49;  // edx
    unsigned int v50;  // 4102
    unsigned int idx;  // ecx
    unsigned int v52;  // eax
    st_449bc0_6 *v53;  // eax
    unsigned int v54;  // ecx
    unsigned int v19;  // esi
    unsigned int *v55;  // eax
    unsigned int v56;  // edx
    unsigned int v57;  // 4101
    unsigned int v58;  // eax
    unsigned int v59;  // eax
    unsigned int v60;  // eax
    unsigned int v61;  // eax
    unsigned int v62;  // ecx
    st_449bc0_1 *v63;  // edx
    unsigned int *v64;  // eax
    void* v20;  // ecx
    int v65;  // eax
    int idx1;  // esi
    int v67;  // ecx
    unsigned int v68;  // edx
    int v69;  // eax
    unsigned int v70;  // eax
    int idx2;  // esi
    unsigned int v72;  // ecx
    st_449bc0_1 *v73;  // ebx
    st_449bc0_10 *v74;  // eax
    unsigned int v21;  // edi
    st_449bc0_10 *v75;  // esi
    unsigned int v76;  // eax
    unsigned int v77;  // eax
    unsigned int v78;  // ecx
    unsigned int v79;  // eax
    unsigned int v80;  // edx
    unsigned int v81;  // edi
    unsigned int v82;  // eax
    int v83;  // esi
    unsigned int v84;  // ecx
    int j;  // ebx
    unsigned int v85;  // esi
    unsigned int v86;  // edx
    int v87;  // eax
    unsigned int v90;  // eax
    st_449bc0_1 *v91;  // edx
    unsigned int *v92;  // ecx
    unsigned int v93;  // esi
    unsigned int v94;  // edi
    unsigned int *v23;  // ecx
    unsigned int v95;  // eax
    unsigned int v96;  // ecx
    unsigned int v97;  // ebx
    unsigned int v98;  // edi
    st_449bc0_1 *v99;  // esi
    unsigned int v100;  // edx
    unsigned int v101;  // edx
    st_449bc0_14 *v102;  // eax
    unsigned int v103;  // eax
    st_449bc0_1 *v104;  // edx
    unsigned int v24;  // ebx
    unsigned int *v105;  // eax
    unsigned int index;  // ecx
    unsigned int v107;  // eax
    unsigned int v108;  // eax
    unsigned int v109;  // edi
    unsigned int v110;  // ebx
    double v111;  // mm5
    unsigned int v112;  // eax
    int v113;  // esi
    unsigned int *v114;  // edi
    unsigned int v0;  // [bp-0x4c]
    unsigned int v1;  // [bp-0x48]
    st_449bc0_19 **v2;  // [bp-0x34], Other Possible Types: unsigned int
    void* node;  // [bp-0x30], Other Possible Types: unsigned int
    double v4;  // [bp-0x2c]
    int v5;  // [bp-0x2c], Other Possible Types: unsigned int
    unsigned int i;  // [bp-0x24]
    unsigned int v7;  // [bp-0x24]
    unsigned int v8;  // [bp-0x20]
    unsigned int v9;  // [bp-0x1c]
    unsigned int v10;  // [bp-0x18]
    unsigned int v11;  // [bp-0x14]
    unsigned int v12;  // [bp-0x10]
    int v13;  // [bp-0xc]

    v15 = g_52b40c;
    v2 = &g_52b40c->padding_8124[24912];
    v16 = &v13;
    v17 = &g_52b40c->field_7fec;
    v9 = 0xffffffff;
    v10 = 0xffffffff;
    v11 = 0xffffffff;
    v12 = 0;
    v13 = 0;
    v8 = 0;
    node = &v13;
    *(unsigned int *)&v4 = v17;
    i = 2;
    do
    {
        v7 = i;
        v18 = *(int *)(v17 - 4);
        v19 = v17;
        v20 = v16;
        if (*(int *)(v19 - 4) > 0)
        {
            v21 = *(int *)v16;
            do
            {
                j = v18;
                v23 = *(int *)&g_52b40c->padding_10ef8[824 + 4 * *(int *)v17];
                v17 += 20;
                v24 = j - 1;
                v21 = (v23[928] != 0) + v21 + v23[110] * 1000 / v23[111];
                v18 = v24;
            } while (j != 1);
            v20 = node;
            v19 = *(unsigned int *)&v4;
            *(unsigned int *)v20 = v21;
        }
        v17 = v19 + 92;
        v16 = v20 + 4;
        i = v7 - 1;
        *(unsigned int *)&v4 = v17;
        node = v16;
        *(int *)((char *)v16 - 4) = *(int *)v20 / *(int *)(v19 - 4);
    } while (i);
    iter = &g_52b40c->field_7ffc;
    i = 2;
    do
    {
        v7 = i;
        v26 = *(int *)(iter - 20);
        v27 = 0;
        if (v26 > 0)
        {
            iter2 = iter;
            do
            {
                v29 = *(int *)&g_52b40c->padding_10ef8[824 + 4 * *(int *)(iter2 - 16)];
                if (v29 && v29[108] && !v29[7])
                {
                    v30 = &(&v9)[v29[3]];
                    if ((&v9)[v29[3]] == 0xffffffff)
                        *v30 = 1;
                    if ((*(int *)(iter - 28) != 1 || v27 < 1 || g_52b40c->field_11bdc || !*(int *)iter2 || *(int *)(iter2 - 20)) && v29[928])
                        *v30 = 0;
                }
            } while ((v27 += 1, iter2 += 20, v27 < v26));
        }
    } while ((iter += 92, i = v7 - 1, v7 != 1));
    if (v10 != 0xffffffff && v11 != 0xffffffff)
    {
        if (v10)
        {
            if (v11)
            {
                g_52b40c->field_11718 = 2;
                goto LABEL_449ed7;
            }
            else
            {
                g_52b40c->field_11718 = 1;
                g_52b40c->field_11714 = 2;
            }
        }
        else
        {
            if (v11)
            {
                g_52b40c->field_11718 = 1;
                g_52b40c->field_11714 = 1;
            }
            else
            {
                if (g_52b40c->field_11720)
                    goto LABEL_449ee1;
                v31 = v13;
                v32 = *(unsigned int *)((void*)&v13 + 4);
                g_52b40c->field_11718 = 3;
                if ((int)v31 > (int)v32)
                {
                    g_52b40c->field_11714 = 1;
                    if (g_52b40c->field_116e8 < 10 && g_52b40c->field_11710 != 3)
                    {
                        v33 = g_52b40c->field_116e8;
                        *(unsigned int *)&g_52b40c->padding_11730[4 * v33] = 4;
                        v34 = sub_44ee90();
                        *(unsigned int *)&g_52b40c->padding_11730[80 + 4 * v33] = (0 < v34 ? 8 : 0xffffffff);
                    }
                }
                else if ((int)v31 < (int)v32)
                {
                    g_52b40c->field_11714 = 2;
                    if (g_52b40c->field_11710 != 3 && g_52b40c->field_116ec < 10)
                        *(unsigned int *)&g_52b40c->padding_11730[40 + 4 * g_52b40c->field_116ec] = 4;
                    if (g_52b40c->field_116ec < 10 && g_52b40c->field_11710 != 3)
                    {
                        v35 = g_52b40c->field_116ec;
                        *(unsigned int *)&g_52b40c->padding_11730[40 + 4 * v35] = 4;
                        v36 = sub_44ee90();
                        *(unsigned int *)&g_52b40c->padding_11730[120 + 4 * v35] = (0 < v36 ? 8 : 0xffffffff);
                    }
                }
                else
                {
                    if (g_52b40c->field_116e8 < 10 && g_52b40c->field_11710 != 3)
                    {
                        v37 = g_52b40c->field_116e8;
                        *(unsigned int *)&g_52b40c->padding_11730[4 * v37] = 4;
                        v38 = sub_44ee90();
                        *(unsigned int *)&g_52b40c->padding_11730[80 + 4 * v37] = (0 < v38 ? 8 : 0xffffffff);
                    }
                    if (g_52b40c->field_116ec < 10 && g_52b40c->field_11710 != 3)
                    {
                        v39 = g_52b40c->field_116ec;
                        *(unsigned int *)&g_52b40c->padding_11730[40 + 4 * v39] = 4;
                        v40 = sub_44ee90();
                        *(unsigned int *)&g_52b40c->padding_11730[120 + 4 * v39] = (0 < v40 ? 8 : 0xffffffff);
                    }
LABEL_449ed7:
                    g_52b40c->field_11714 = 0;
                }
            }
        }
    }
LABEL_449ee1:
    if (g_52b40c->field_10ee8)
        g_52b40c->field_11718 = 0;
    if (g_52b40c->field_11710 != 1 && g_52b40c->field_11710)
        g_52b40c->field_11728 = 0;
    if (g_52b40c->field_11728 && !g_52b40c->field_10eec)
    {
        g_52b40c->field_11728 = 0;
        g_52b40c->field_11710 = 2;
        sub_459460();
        if (*(int *)(g_52b34c + 16) != 0x100 || *(int *)(g_52b34c + 20) != 0x100 || *(int *)(g_52b34c + 24) != 0x100)
        {
            *(unsigned int *)(g_52b34c + 16) = 0x100;
            *(unsigned int *)(g_52b34c + 20) = 0x100;
            *(unsigned int *)(g_52b34c + 24) = 0x100;
            *(unsigned int *)(g_52b34c + 72) = 1;
            *(unsigned int *)(g_52b34c + 80) = 0;
        }
        v41 = g_52b40c->field_11430;
        g_52b40c->field_10ef0 = &g_52be24;
        v42 = g_52b40c->field_10ef4;
        g_52b40c->field_1171c = 0xffffffff;
        g_52b40c->field_10eec = 1;
        g_52b40c->field_10ef4 = -v42;
        if ((int)v41 <= *(int *)&g_52b40c->padding_11434[0])
        {
            v43 = &g_52b40c->padding_0[4 * v41 + 70192];
            do
            {
                v44 = *(int *)&v43->padding_0[0];
                if (v44 && v44[108] && !v44[7])
                {
                    v44[788] = v44[787];
                    v44[787] = 0;
                    v44[917] = 0;
                }
            } while ((v41 += 1, v43 = &v43->padding_0[4], (int)v41 <= *(int *)&g_52b40c->padding_11434[0]));
        }
        node = g_52b40c->field_11430;
        if ((int)node <= *(int *)&g_52b40c->padding_11434[0])
        {
            v45 = node * 4 + 70192;
            while (1)
            {
                if (*(int *)(v45 + v15))
                    *(unsigned int *)(*(int *)(v45 + v15) + 436) = 0;
                v46 = *(int *)(v45 + v15);
                if (v46)
                {
                    if (v46->field_1b0 && !v46->field_1c)
                    {
                        v47 = *(int *)v46->field_268;
                        if (v47[2])
                        {
                            v48 = v47[9];
                            v49 = v47[1];
                            v50 = v47[6];
                            v47[8] = v47[9];
                            if (v48 * v49 + v50)
                            {
                                do
                                {
                                    idx = *(int *)*(int *)(*(int *)(v45 + v15) + 616);
                                    v52 = v47[8] * *(int *)(idx + 4);
                                    v53 = v52 + *(int *)(idx + 24);
                                    if (v52 + *(int *)(idx + 24))
                                    {
                                        if (v53->field_2dc)
                                        {
                                            v53->field_2dc->field_18 = 0;
                                            v53->field_2dc->field_8 = 0;
                                            v53->field_2dc = NULL;
                                        }
                                        sub_403170();
                                    }
                                    v54 = v47[8];
                                    if ((int)v54 >= (int)v47[10])
                                        break;
                                    v55 = v54 * 16 + v47[7];
                                    do
                                    {
                                        v55 += 4;
                                        v54 += 1;
                                    } while (!*v55);
                                    v56 = v47[1];
                                    v57 = v47[6];
                                    v47[8] = v54;
                                } while (v54 * v56 + v57);
                            }
                        }
                    }
                    else if (v46 && v46->field_1b0 && v46->field_1c == 1)
                    {
                        sub_4508f0();
                        v15 = g_52b40c;
                    }
                }
                v45 += 1;
                node += 1;
                if ((int)node > *(int *)&v15->padding_11434[0])
                    break;
            }
        }
        sub_445540();
        sub_445540();
        v15 = g_52b40c;
    }
    v58 = v15->field_11710;
    v15->field_1164c = 1;
    while (1)
    {
        if (v58 <= 4)
        {
            switch (v58)
            {
            case 0:
                v59 = v15->field_1171c;
                v60 = v59 - 2;
                v15->field_1164c = 0;
                if ((int)(v59 - 2) < 0)
                {
                    v60 = 0;
                }
                else if ((int)v60 >= 16)
                {
                    if (*(int *)(g_52b34c + 16) != 0x100 || *(int *)(g_52b34c + 20) != 0x100 || *(int *)(g_52b34c + 24) != 0x100)
                    {
                        *(unsigned int *)(g_52b34c + 16) = 0x100;
                        *(unsigned int *)(g_52b34c + 20) = 0x100;
                        *(unsigned int *)(g_52b34c + 24) = 0x100;
                        *(unsigned int *)(g_52b34c + 72) = 1;
                        *(unsigned int *)(g_52b34c + 80) = 0;
                    }
LABEL_44a1bd:
                    if ((int)v15->field_1171c >= *(int *)(v2 + 8))
                    {
                        if (*(int *)(g_52b34c + 16) != 0x100 || *(int *)(g_52b34c + 20) != 0x100 || *(int *)(g_52b34c + 24) != 0x100)
                        {
                            *(unsigned int *)(g_52b34c + 16) = 0x100;
                            *(unsigned int *)(g_52b34c + 20) = 0x100;
                            *(unsigned int *)(g_52b34c + 24) = 0x100;
                            *(unsigned int *)(g_52b34c + 72) = 1;
                            *(unsigned int *)(g_52b34c + 80) = 0;
                        }
                        v15->field_11710 = 1;
                        v15->field_1171c = 0;
                        goto LABEL_44a3ab;
                    }
                    goto LABEL_44a3c4;
                }
                v61 = v60 * 16;
                if (*(int *)(g_52b34c + 16) != v61 || *(int *)(g_52b34c + 20) != v61 || *(int *)(g_52b34c + 24) != v61)
                {
                    *(unsigned int *)(g_52b34c + 16) = v61;
                    *(unsigned int *)(g_52b34c + 20) = v61;
                    *(unsigned int *)(g_52b34c + 24) = v61;
                    *(unsigned int *)(g_52b34c + 72) = 1;
                    *(unsigned int *)(g_52b34c + 80) = 0;
                    goto LABEL_44a1bd;
                }
                continue;
            case 1:
                v62 = v15->field_11430;
                if ((int)v62 <= *(int *)&v15->padding_11434[0])
                {
                    v63 = &v15->padding_0[4 * v62 + 70192];
                    do
                    {
                        v64 = *(int *)&v63->padding_0[0];
                        if (v64 && v64[108] && !v64[7] && v15->field_116e4 == 1)
                        {
                            if ((int)v15->field_1171c >= 0)
                                v64[109] = 0;
                            else
                                v15->field_11658 = 1;
                        }
                    } while ((v62 += 1, v63 = &v63->padding_0[4], (int)v62 <= *(int *)&v15->padding_11434[0]));
                }
                if (v15->field_11658)
                    v15->field_11704 = 0;
                if (v15->field_11704 == *(int *)(v2 + 1788) + 1 && !(v65 = v15->field_116e4, v65 >= 10 || v15->field_11668))
                {
                    i = *(int *)(v2 + v65 * 4 + 1832);
                    v4 = v15->field_10e90 / 100.0 * v15->field_10e98 / 100.0 * 100.0;
                    if (v4 < 0.0)
                    {
                        v4 = 0.0;
                    }
                    else if (v4 > 100.0)
                    {
                        v4 = 100.0;
                    }
                    if (v15->field_10e8c && v15->field_8120 && !v15->field_10eb8 && *(int *)(v2 + v65 * 4 + 0x700) != 0xffffffff && g_52b370 == 1)
                    {
                        idx1 = sub_415520();
                        if (idx1 != 0xffffffff && sub_415590(idx1, *(unsigned int *)&v4) != 0xffffffff)
                            *(unsigned int *)(g_52b388 + idx1 * 40) = 0xffffffff;
                        v15 = g_52b40c;
                    }
                }
                if ((int)v15->field_11704 >= *(int *)(v2 + 12))
                {
                    v15->field_11710 = 2;
                    v15->field_1171c = 0;
                    sub_459460();
                    goto LABEL_44a3ab;
                }
                else
                {
                    v15->field_11704 = v15->field_11704 + 1;
                    goto LABEL_44a3c8;
                }
            case 2:
                if (v15->field_11698 == 0xffffffff && !v15->field_11700 && (v67 = *(int *)&v15->padding_ec74[508], v68 = v15->field_1171c, (int)v68 >= v67 && (v69 = v15->field_11720, v69 > 0 && !v15->field_1165f && (v15->field_11720 = v69 - 1, v69 == 1 && (int)v68 > v67))))
                {
                    v15->field_11710 = 3;
                    v15->field_1171c = 0;
                    goto LABEL_44a3ab;
                }
                v70 = v2;
                if (v15->field_1171c == *(int *)(v70 + 2036) && !v15->field_11668)
                {
                    i = *(int *)(v70 + 0x7fc);
                    v4 = v15->field_10e90 / 100.0 * v15->field_10e98 / 100.0 * 100.0;
                    if (v4 < 0.0)
                    {
                        v4 = 0.0;
                    }
                    else if (v4 > 100.0)
                    {
                        v4 = 100.0;
                    }
                    if (v15->field_10e8c && v15->field_8120 && !v15->field_10eb8 && *(int *)(v70 + 0x7f8) != 0xffffffff && g_52b370 == 1)
                    {
                        idx2 = sub_415520();
                        if (idx2 != 0xffffffff && sub_415590(idx2, *(unsigned int *)&v4) != 0xffffffff)
                            *(unsigned int *)(g_52b388 + idx2 * 40) = 0xffffffff;
                        v15 = g_52b40c;
                    }
                }
                if (v15->field_1171c == *(int *)(v2 + 0xbfc))
                {
                    v72 = v15->field_11430;
                    node = v72;
                    if ((int)v72 <= *(int *)&v15->padding_11434[0])
                    {
                        v73 = &v15->padding_0[4 * v72 + 70192];
                        do
                        {
                            v74 = *(int *)&v73->padding_0[0];
                            if (v74 && v74->field_1b0 && !v74->field_1c)
                            {
                                v74->field_e64 = 1;
                                if (*(int *)(*(int *)&v73->padding_0[0] + 3148))
                                {
                                    v75 = *(int *)&v73->padding_0[0];
                                    if (v75->field_9b0 == v75->field_c40)
                                    {
                                        memset(&v75->field_9b4, 0, 128);
                                        v75->field_9b0 = v75->field_0 + 1056;
                                    }
                                    v75->field_c50 = v75->field_c4c;
                                    v72 = node;
                                    v75->field_c3c = 0;
                                    v75->field_c38 = 0xffffffff;
                                    v75->field_c44 = 0;
                                    v75->field_c40 = 0;
                                    v75->field_c4c = 0;
                                    v75->field_e54 = 0;
                                }
                            }
                        } while ((v72 += 1, v73 = &v73->padding_0[4], node = v72, (int)v72 <= *(int *)&v15->padding_11434[0]));
                    }
                }
                v76 = v15->field_11718;
                if (v76 && !v15->field_1165a && (int)v15->field_1171c > *(int *)&v15->padding_ec74[508])
                {
                    v15->field_11710 = 3;
                    v15->field_1171c = 0;
                    if (v76 != 3 && !v15->field_1165c)
                    {
                        v15->field_116f8 = *(int *)(v2 + 0xc00);
                        v15->field_116fc = 500;
                        v15->field_11700 = 500;
                        goto LABEL_44a3ab;
                    }
                }
                else
                {
                    v15->field_11724 = v15->field_11724 + 1;
                    goto LABEL_44a3c4;
                }
                goto LABEL_44a3c4;
            case 3:
                v77 = v15->field_1171c;
                v78 = v2;
                v15->field_11708 = v77;
                if (v77 == *(int *)(v78 + 2532))
                {
                    v79 = v15->field_11718;
                    if (v79 == 3)
                    {
                        v80 = *(int *)(v78 + 2556);
                        v81 = *(int *)(v78 + 2552);
                    }
                    else if (v79 == 2)
                    {
                        v80 = *(int *)(v78 + 2548);
                        v81 = *(int *)(v78 + 2544);
                    }
                    else
                    {
                        if (v79 != 1)
                            goto LABEL_44a7d6;
                        v82 = v78;
                        v80 = *(int *)(v82 + 2540);
                        v81 = *(int *)(v82 + 2536);
                    }
                    i = v80;
                    v4 = v15->field_10e90 / 100.0 * v15->field_10e98 / 100.0 * 100.0;
                    if (v4 < 0.0)
                    {
                        v4 = 0.0;
                    }
                    else if (v4 > 100.0)
                    {
                        v4 = 100.0;
                    }
                    if (v15->field_10e8c && v15->field_8120 && !v15->field_10eb8 && v81 != 0xffffffff && g_52b370 == 1)
                    {
                        v83 = sub_415520();
                        if (v83 != 0xffffffff && sub_415590(v83, *(unsigned int *)&v4) != 0xffffffff)
                            *(unsigned int *)(g_52b388 + v83 * 40) = 0xffffffff;
                        v15 = g_52b40c;
                    }
                }
LABEL_44a7d6:
                if (v15->field_11718 != 3)
                {
                    v84 = v15->field_ee74;
                    v85 = v15->field_1171c;
                    v86 = 1717986919 * (v84 * 5) >> 0x22;
                    v87 = (v86 >> 31) + v86;
                    if ((int)v85 < v87)
                    {
                        if (v85 == 5)
                        {
                            v15->field_116fc = 200;
                            v15->field_11700 = 200;
                        }
                        if (v85 != v87)
                            goto LABEL_44a832;
                    }
                    else if (!(v85 == v87))
                    {
                        goto LABEL_44a832;
                    }
                    v15->field_116fc = 100;
                    v15->field_11700 = 100;
LABEL_44a832:
                    if (v85 == ((unsigned int)((int)(1717986919 * (v84 * 7) >> 0x22)) >> 31) + (int)(1717986919 * (v84 * 7) >> 0x22))
                    {
                        v15->field_116fc = 50;
                        v15->field_11700 = 50;
                    }
                    if (v85 == ((unsigned int)((int)(1717986919 * (v84 * 9) >> 0x22)) >> 31) + (int)(1717986919 * (v84 * 9) >> 0x22))
                    {
                        v15->field_116fc = 25;
                        v15->field_11700 = 25;
                        goto LABEL_44a88a;
                    }
                }
                else
                {
LABEL_44a88a:
                    if (v15->field_1171c == *(int *)(v2 + 3076) || !v15->field_1171c && !v15->field_11720)
                    {
                        v90 = v15->field_11430;
                        if ((int)v90 <= *(int *)&v15->padding_11434[0])
                        {
                            v91 = &v15->padding_0[4 * v90 + 70192];
                            do
                            {
                                v92 = *(int *)&v91->padding_0[0];
                                if (v92 && v92[108])
                                    v92[164] = 1;
                            } while ((v90 += 1, v91 = &v91->padding_0[4], (int)v90 <= *(int *)&v15->padding_11434[0]));
                        }
                    }
                    v93 = 0;
                    if (v15->field_1ed0 && v15->field_1171c == *(int *)&v15->padding_ee78[4])
                    {
                        node = 0;
                        v5 = 2;
                        do
                        {
                            v94 = v93 + 1;
                            if (v15->field_11714 == v94)
                                continue;
                            v95 = *(int *)&v15->padding_4a18[16 + node] + 1;
                            if ((int)v95 < *(int *)&v15->padding_4a18[20 + node])
                            {
                                sub_40acd0("start pre-caching team %i next %i\n", v94, v95);
                            }
                            else if (2 - v15->field_4a14 == v93 && sub_459580() == v5)
                            {
                                sub_4727c0();
                                continue;
                            }
                            else if (!(v15->field_4a14 == v93 && sub_459580() == v94))
                            {
                                continue;
                            }
                            sub_472750();
                            v15 = g_52b40c;
                        } while ((node += 6876, v93 = v94, v5 -= 1, v5 > 0));
                        sub_4728b0();
                        v15 = g_52b40c;
                    }
                    if ((int)v15->field_1171c >= *(int *)(v2 + 3076) + 180)
                        v15->field_1164c = 0;
                    v96 = v15->field_11430;
                    v97 = *(int *)&v15->padding_11434[0];
                    v98 = 1;
                    if ((int)v96 <= (int)v97)
                    {
                        v99 = &v15->padding_0[4 * v96 + 70192];
                        v100 = v97 - v96 + 1;
                        do
                        {
                            v101 = v100;
                            v102 = *(int *)&v99->padding_0[0];
                            if (v102 && v102->field_1b0 && !v102->field_1c)
                            {
                                if ((!v102->field_e64 || v102->field_e58 != 1 || v102->field_e5c) && (v102->field_c4c != 5150 || v102->field_e54 <= 0))
                                    v103 = 0;
                                else
                                    v103 = 1;
                                v98 &= v103;
                            }
                        } while ((v99 = &v99->padding_0[4], v100 = v101 - 1, v101 != 1));
                    }
                    if ((int)v15->field_1171c < 900 && !v98 || (int)v15->field_1171c < *(int *)(v2 + 3076))
                    {
LABEL_44a3c4:
                        goto LABEL_44a3c8;
                    }
                    else
                    {
                        if ((int)v96 <= (int)v97)
                        {
                            v104 = &v15->padding_0[4 * v96 + 70192];
                            do
                            {
                                v105 = *(int *)&v104->padding_0[0];
                                if (v105 && v105[108] && !v105[7])
                                    v105[921] = 0;
                            } while ((v96 += 1, v104 = &v104->padding_0[4], (int)v96 <= *(int *)&v15->padding_11434[0]));
                        }
                        v15->field_11710 = 4;
                        v15->field_1171c = 0;
                        v15->field_116f8 = 0;
LABEL_44a3ab:
                        v15->field_1164c = 1;
                        v58 = v15->field_11710;
                        continue;
                    }
                }
            case 4:
                index = v2;
                v107 = v15->field_1171c;
                v15->field_1164c = 0;
                if (v107 + *(int *)(index + 3076) != *(int *)(index + 2532))
                {
LABEL_44ac85:
LABEL_44ac89:
                    if (v15->field_1171c == *(int *)(v2 + 0xc0c))
                    {
                        node = v15->field_11430;
                        if ((int)node <= *(int *)&v15->padding_11434[0])
                        {
                            v2 = node * 4 + 70192;
                            do
                            {
                                v114 = *(int *)(v2 + v15);
                                if (!v114 || !v114[108] || v114[7])
                                    continue;
                                if (v15->field_11714 == v114[3])
                                {
                                    *(unsigned int *)(*(int *)(v2 + v15) + 656) = 0;
                                    if (!*(int *)(*(int *)(v2 + v15) + 3712))
                                        continue;
                                    v115 = *(int *)(v2 + v15);
                                    v116 = v115[783];
                                    *(unsigned int *)&v4 = 1;
                                    if (!v115[783])
                                        v116 = v115[781];
                                    i = *(int *)v116;
                                    v117 = sub_4032f0();
                                    if (v117 == 0xffffffff || !(v117 * *(int *)(i + 4) + *(int *)(i + 24)))
                                        v118 = 0;
                                    else
                                        v118 = *(unsigned int *)&v4;
                                    v119 = v115[787];
                                    v115[788] = v119;
                                    v115[787] = 180;
                                    v115[917] = 0;
                                    if (v118)
                                        continue;
                                    v1 = v119;
                                    v0 = 180;
                                }
                                else if (v15->field_11714)
                                {
                                    if (!v114[928])
                                        continue;
                                    v120 = v114[783];
                                    i = 1;
                                    if (!v114[783])
                                        v120 = v114[781];
                                    *(int *)&v4 = *(int *)v120;
                                    v121 = sub_4032f0();
                                    if (v121 == 0xffffffff || !(v121 * *(int *)(*(unsigned int *)&v4 + 4) + *(int *)(*(unsigned int *)&v4 + 24)))
                                        i = 0;
                                    v122 = v114[787];
                                    v114[788] = v122;
                                    v114[787] = 0xaa;
                                    v114[917] = 0;
                                    if (i)
                                        continue;
                                    v1 = v122;
                                    v0 = 0xaa;
                                }
                                else
                                {
                                    if (!v114[928])
                                        continue;
                                    v123 = v114[783];
                                    i = 1;
                                    if (!v114[783])
                                        v123 = v114[781];
                                    *(int *)&v4 = *(int *)v123;
                                    v124 = sub_4032f0();
                                    if (v124 == 0xffffffff || !(v124 * *(int *)(*(unsigned int *)&v4 + 4) + *(int *)(*(unsigned int *)&v4 + 24)))
                                        i = 0;
                                    v125 = v114[787];
                                    v114[788] = v125;
                                    v114[787] = 175;
                                    v114[917] = 0;
                                    if (i)
                                        continue;
                                    v1 = v125;
                                    v0 = 175;
                                }
                                sub_443d10("changed to invalid state %i (from state %i)\n");
                                v15 = g_52b40c;
                            } while ((v2 += 1, node += 1, (int)node <= *(int *)&v15->padding_11434[0]));
                        }
                        v126 = v15->field_11714;
                        if (v126)
                        {
                            v15->field_116f0 = 1;
                            (&v15->field_116e4)[v126] = (&v15->field_116e4)[v126] + 1;
                            if (!sub_459580())
                            {
                                v127 = v15->field_11714 - 1;
                                v128 = 0;
                                i = &v15->padding_0[6876 * v127 + 92 * *(int *)&v15->padding_4a18[16 + 6876 * v127] + 18996];
                                if (*(int *)(i + 8) > 0)
                                {
                                    v5 = i + 12;
                                    do
                                    {
                                        sub_44ecb0();
                                        sub_46af90();
                                        v5 += 20;
                                        v128 += 1;
                                    } while (v128 < *(int *)(i + 8));
                                }
                            }
                        }
                        else
                        {
                            v15->field_116f4 = v15->field_116f4 + 1;
                            if ((int)v15->field_116f4 > v15->field_64fc)
                                v15->field_116e8 = v15->field_116e8 + 1;
                            if ((int)v15->field_116f4 > *(int *)&v15->padding_6500[6872])
                                v15->field_116ec = v15->field_116ec + 1;
                            v15->field_116f0 = 1;
                        }
                    }
                    if (v15->field_11659 && !v15->field_1172c)
                        v15->field_1170c = v15->field_1171c + 19;
                    v129 = v15->field_1170c - v15->field_1171c;
                    if (v129 < 16 && !(v130 = v129 * 16, *(int *)(g_52b34c + 16) == v130 && *(int *)(g_52b34c + 20) == v130 && *(int *)(g_52b34c + 24) == v130))
                    {
                        *(unsigned int *)(g_52b34c + 16) = v130;
                        *(unsigned int *)(g_52b34c + 20) = v130;
                        *(unsigned int *)(g_52b34c + 24) = v130;
                        *(unsigned int *)(g_52b34c + 72) = 1;
                        *(unsigned int *)(g_52b34c + 80) = 0;
                    }
                    if ((int)v15->field_1171c >= (int)v15->field_1170c)
                    {
                        v131 = sub_459580();
                        if (v131)
                        {
                            v15->field_117e0 = v131;
                            v15->field_117e8 = 1;
                            v15->field_117e4 = *(int *)&v15->padding_1ed4[4216 + 6876 * v131];
                        }
                        if (!v15->field_117e8)
                        {
                            sub_4597c0();
                            v132 = g_52b40c->field_11430;
                            if ((int)v132 <= *(int *)&g_52b40c->padding_11434[0])
                            {
                                iter1 = &g_52b40c->padding_0[4 * v132 + 70192];
                                do
                                {
                                    v134 = *(int *)&iter1->padding_0[0];
                                    if (v134 && v134[108] && !v134[7])
                                        v134[2210] = v134[2210] + 1;
                                } while ((v132 += 1, iter1 = &iter1->padding_0[4], (int)v132 <= *(int *)&g_52b40c->padding_11434[0]));
                            }
                            sub_4729a0();
                            sub_4354b0();
                            g_52b40c->field_10eb4 = 0;
                            sub_4351b0();
                            if (!g_52b40c->field_10eb4)
                                sub_4351b0();
                            sub_458470();
                            v15 = g_52b40c;
                        }
                        v8 = 1;
                        goto LABEL_44a3c8;
                    }
                }
                v108 = v15->field_11718;
                if (v108 == 3)
                {
                    v109 = *(int *)(index + 2552);
                    v110 = v15->field_8120;
                    i = *(int *)(index + 2556);
                    v111 = v15->field_10e90 / 100.0 * v15->field_10e98 / 100.0 * 100.0;
                    v4 = v111;
                    if (v111 < 0.0)
                    {
                        v4 = 0.0;
                        goto LABEL_44abbc;
                    }
                }
                else if (v108 == 2)
                {
                    v109 = *(int *)(index + 2544);
                    v110 = v15->field_8120;
                    i = *(int *)(index + 2548);
                    v111 = v15->field_10e90 / 100.0 * v15->field_10e98 / 100.0 * 100.0;
                    v4 = v111;
                    if (v111 < 0.0)
                    {
                        v4 = 0.0;
                        goto LABEL_44abbc;
                    }
                }
                else
                {
                    if (v108 != 1)
                        goto LABEL_44ac85;
                    v110 = v15->field_8120;
                    v112 = index;
                    v109 = *(int *)(v112 + 2536);
                    i = *(int *)(v112 + 2540);
                    v111 = v15->field_10e90 / 100.0 * v15->field_10e98 / 100.0 * 100.0;
                    v4 = v111;
                    if (v111 < 0.0)
                    {
                        v4 = 0.0;
LABEL_44abbc:
                        if (v15->field_10e8c && v110 && !v15->field_10eb8 && v109 != 0xffffffff && g_52b370 == 1)
                        {
                            v113 = sub_415520();
                            if (v113 != 0xffffffff && sub_415590(v113, *(unsigned int *)&v4) != 0xffffffff)
                                *(unsigned int *)(g_52b388 + v113 * 40) = 0xffffffff;
                            v15 = g_52b40c;
                            goto LABEL_44ac89;
                        }
                    }
                }
                if (v111 > 100.0)
                {
                    v4 = 100.0;
                    goto LABEL_44abbc;
                }
                else
                {
                    goto LABEL_44abbc;
                }
            }
        }
        else
        {
LABEL_44a3c8:
            if ((int)v15->field_116f8 > 0)
            {
                v15->field_116f8 = v15->field_116f8 - 1;
                break;
            }
        }
    }
    if (!v15->field_116f8 && v15->field_116fc)
    {
        v15->field_116fc = 0;
        v15->field_11700 = 0;
    }
    v15->field_1171c = v15->field_1171c + 1;
    return v8;
}
