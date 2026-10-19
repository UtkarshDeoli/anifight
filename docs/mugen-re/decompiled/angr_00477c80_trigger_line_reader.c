// trigger_line_reader entry=0x477c80
typedef struct st_473ed0_3 {
    char padding_0[184];
    int field_b8;
} st_473ed0_3;

typedef struct st_477c80_1 {
    unsigned int field_0;
} st_477c80_1;

typedef struct st_477c80_3 {
    char padding_0[4];
    int field_4;
    unsigned int field_8;
    int field_c;
    char padding_10[4];
    unsigned int field_14;
    unsigned int field_18;
    unsigned int field_1c;
    char padding_20[4];
    unsigned int field_24;
    unsigned int field_28;
} st_477c80_3;

typedef struct st_477c80_0 {
    char padding_0[4];
    unsigned int field_4;
    char padding_8[4];
    unsigned int field_c;
    char padding_10[4];
    int field_14;
    unsigned int field_18;
    char padding_1c[16];
    struct st_477c80_1 *field_2c;
} st_477c80_0;

typedef struct st_477c80_2 {
    char field_0;
} st_477c80_2;

extern void* g_52b40c;

int sub_477c80(void)
{
    unsigned int v24;  // esi
    int v25;  // ebp
    st_477c80_2 **v34;  // eax
    int v35;  // ecx
    int v36;  // 4117
    int v37;  // eax
    st_473ed0_3 *v38;  // esi
    st_477c80_3 *v39;  // esi
    unsigned int v40;  // eax
    unsigned int v41;  // ebp
    unsigned int v42;  // edx
    unsigned int v43;  // ecx
    st_477c80_0 *v26;  // eax
    unsigned int *v44;  // ecx
    unsigned int *v45;  // eax
    unsigned int v46;  // edx
    unsigned int v47;  // eax
    unsigned int v48;  // edx
    unsigned int v49;  // eax
    unsigned int v50;  // eax
    unsigned int idx;  // eax
    char *v52;  // eax
    void* iter;  // esi
    unsigned int v27;  // eax
    char *v54;  // eax
    char *v55;  // eax
    unsigned int v56;  // eax
    unsigned int v57;  // edi
    unsigned int v58;  // edi
    unsigned int iter1;  // edi
    unsigned int v60;  // ecx
    unsigned int v61;  // ecx
    char *v62;  // eax
    void* iter2;  // esi
    unsigned int index;  // ecx
    char *v64;  // eax
    char *v65;  // eax
    unsigned int v66;  // eax
    unsigned int v67;  // edx
    unsigned int v68;  // edx
    unsigned int node;  // edx
    unsigned int v70;  // ecx
    unsigned int v71;  // ecx
    int v72;  // ebp
    int j;  // eax
    st_477c80_2 **v29;  // eax
    st_473ed0_3 *v74;  // ebx
    unsigned int v75;  // edi
    unsigned int v76;  // edi
    unsigned int v77;  // edi
    unsigned int v78;  // ecx
    void* k;  // esi
    char *v80;  // eax
    char *v81;  // eax
    int v30;  // ecx
    int v31;  // 4114
    unsigned int v32;  // eax
    unsigned int idx1;  // ecx
    unsigned int v0;  // [bp-0xd4]
    int v1;  // [bp-0xd0]
    int v2;  // [bp-0xcc]
    int v3;  // [bp-0xc8]
    int v4;  // [bp-0xc4]
    int v5;  // [bp-0xc0]
    unsigned int v6;  // [bp-0xbc]
    unsigned int v7;  // [bp-0xb8]
    int i;  // [bp-0xb4]
    unsigned int v9;  // [bp-0xb0]
    char v10;  // [bp-0xac], Other Possible Types: unsigned int
    unsigned int v11;  // [bp-0xa8]
    int v12;  // [bp-0xa4]
    char v13[8];  // [bp-0xa0]
    char v14;  // [bp-0x98]
    st_477c80_3 *v15;  // [bp-0x88], Other Possible Types: char
    unsigned int v16;  // [bp-0x84]
    unsigned int num;  // [bp-0x80]
    unsigned int num1;  // [bp-0x7c]
    int v19;  // [bp-0x78]
    char v20[32];  // [bp-0x20]
    int v22;  // [bp+0x8]
    st_473ed0_3 *v23;  // [bp+0xc]

    v24 = 0;
    v25 = 0;
    v5 = 0;
    memset(&v15, 0, 100, v1, v2, v3, v4, v5);
    v27 = *(int *)&v26->field_2c[v26->field_18] + 1;
    v26->field_c = v27;
    index = v26->field_c;
    v26->field_14 = v27;
    v29 = *(int *)&v26->padding_1c[8];
    v30 = v26->field_14;
    v31 = *(int *)&v26->padding_1c[12];
    *(st_477c80_2 **)v26->padding_10 = &(v29[index])[-1 * v26->field_4];
    *(unsigned int *)&v26->padding_1c[0] = 0;
    if (v30 >= v31)
    {
        *(unsigned int *)&v26->padding_1c[0] = 1;
    }
    else if (*(char *)v29[v30] == 91)
    {
        *(unsigned int *)&v26->padding_1c[0] = 1;
    }
    if (sub_433410(v26, "triggerall"))
    {
        v5 = 1;
        if (sub_4335d0("triggerall"))
        {
            do
            {
                v5 += 1;
            } while (sub_4335d0("triggerall"));
        }
    }
    strncpy(v13, "trig", 4);
    strncpy(&v13[4], "ger1", 4);
    v14 = 0;
    v32 = *(int *)&v26->field_2c[v26->field_18] + 1;
    v26->field_c = v32;
    idx1 = v26->field_c;
    v26->field_14 = v32;
    v34 = *(int *)&v26->padding_1c[8];
    v35 = v26->field_14;
    v36 = *(int *)&v26->padding_1c[12];
    *(st_477c80_2 **)v26->padding_10 = &(v34[idx1])[-1 * v26->field_4];
    *(unsigned int *)&v26->padding_1c[0] = 0;
    if (v35 >= v36 || *(char *)v34[v35] == 91)
        *(unsigned int *)&v26->padding_1c[0] = 1;
    if (sub_433410(v26, v13))
    {
        do
        {
            do
            {
                v25 += 1;
            } while (sub_4335d0(v13));
        } while ((v24 += 1, v0 = v24 + 1, sprintf(v13, "trigger%i"), sub_433410(v26, v13)));
        v12 = v25;
        if (v25)
        {
            v15 = sub_402f10(12);
            if (!v15)
                return;
            v16 = v24;
            v7 = 0;
            sub_4331e0();
            v9 = 0;
            v10 = 0;
            v11 = 0;
            v11 = 0xffffffff;
            v9 = 0;
            v10 = 0;
            i = 0;
            if (v25 > 0)
            {
                do
                {
                    v0 = v7 + 1;
                    sprintf(v13, "trigger%i");
                    v37 = sub_433410(v26, v13);
                    v6 = 0;
                    do
                    {
                        v38 = v23;
                        if (!sub_438b20(v37, &v9, v38, 0, 0))
                        {
                            v0 = v6 + 1;
                            sprintf(v20, ", %i");
                            v48 = g_52b40c + 73820;
                            v49 = v48 - 1;
                            do
                            {
                                v50 = v49;
                                idx = v50 + 1;
                                v49 = idx;
                            } while (*(char *)(v50 + 1));
                            *(unsigned int *)idx = 1920091402;
                            *(unsigned int *)(idx + 4) = 1881174639;
                            *(unsigned int *)(idx + 8) = 1769173601;
                            *(unsigned int *)(idx + 12) = 2123630;
                            v52 = &v13[0];
                            iter = v13;
                            do
                            {
                                v54 = v52;
                                v55 = v54 + 1;
                                v52 = v55;
                            } while (*v54);
                            v56 = v55 - v13;
                            v57 = v48 - 1;
                            do
                            {
                                v58 = v57;
                                iter1 = v58 + 1;
                                v57 = iter1;
                            } while (*(char *)(v58 + 1));
                            for (v60 = v56 >> 2; v60; iter += 4)
                            {
                                v60 -= 1;
                                *(int *)iter1 = *(int *)iter;
                                iter1 += 4;
                            }
                            v61 = v56 & 3;
                            for (v62 = &v20[0]; v61; iter += 1)
                            {
                                v61 -= 1;
                                *(char *)iter1 = *(char *)iter;
                                iter1 += 1;
                            }
                            iter2 = v20;
                            do
                            {
                                v64 = v62;
                                v65 = v64 + 1;
                                v62 = v65;
                            } while (*v64);
                            v66 = v65 - v20;
                            v67 = v48 - 1;
                            do
                            {
                                v68 = v67;
                                node = v68 + 1;
                                v67 = node;
                            } while (*(char *)(v68 + 1));
                            for (v70 = v66 >> 2; v70; iter2 += 4)
                            {
                                v70 -= 1;
                                *(int *)node = *(int *)iter2;
                                node += 4;
                            }
                            for (v71 = v66 & 3; v71; iter2 += 1)
                            {
                                v71 -= 1;
                                *(char *)node = *(char *)iter2;
                                node += 1;
                            }
                            return;
                        }
                        *(unsigned int *)&v38[12].padding_0[24] = *(int *)&v38[12].padding_0[24] + 1;
                        v39 = v15;
                        v40 = v39->field_8;
                        if ((int)v40 >= v39->field_c)
                            continue;
                        if (!v40)
                        {
                            v41 = 0;
                        }
                        else
                        {
                            v42 = v39->field_28;
                            v43 = v39->field_24;
                            if ((int)(v42 - v43 + 1) > (int)v40)
                            {
                                v41 = v43 + 1;
                                v44 = v41 * 16;
                                if (*(int *)((char *)v44 + v39->field_1c))
                                {
                                    v45 = v39->field_1c + (char *)v44;
                                    do
                                    {
                                        v45 += 4;
                                        v41 += 1;
                                    } while (*v45);
                                }
                            }
                            else if ((int)v43 > 0)
                            {
                                v41 = v43 - 1;
                            }
                            else
                            {
                                if ((int)v42 >= v39->field_c - 1)
                                    goto LABEL_477f80;
                                v41 = v42 + 1;
                            }
                            if (v41 == 0xffffffff)
                                goto LABEL_477f80;
                        }
                        memcpy(v41 * v39->field_4 + v39->field_18, &v9, v39->field_4);
                        v46 = v39->field_1c;
                        v39->field_8 = v39->field_8 + 1;
                        v47 = v41 * 16;
                        *(unsigned int *)(v47 + v46) = 1;
                        *(unsigned int *)(v39->field_1c + v47 + 4) = v39->field_14;
                        *(unsigned int *)(v39->field_1c + v47 + 8) = v7;
                        *(unsigned int *)(v39->field_1c + v47 + 12) = v6;
                        v39->field_14 = v39->field_14 + 1;
                        if ((int)v41 < (int)v39->field_24)
                            v39->field_24 = v41;
                        if ((int)v41 > (int)v39->field_28)
                            v39->field_28 = v41;
LABEL_477f80:
                        v25 = v12;
                    } while ((v6 += 1, i += 1, v37 = sub_4335d0(v13), v37));
                    v7 += 1;
                } while (i < v25);
            }
            v72 = 0;
            if (v5 > 0)
            {
                sub_4331e0();
                j = sub_433410(v26, "triggerall");
                if (!j)
                    goto LABEL_478125;
                do
                {
                    v74 = v23;
                    if (!sub_438b20(j, &v9, v74, 0, 0))
                    {
                        v75 = g_52b40c + 73819;
                        do
                        {
                            v76 = v75;
                            v77 = v76 + 1;
                            v75 = v77;
                        } while (*(char *)(v76 + 1));
                        v78 = 6;
                        for (k = "\nError parsing triggerall"; v78; k += 4)
                        {
                            v78 -= 1;
                            *(int *)v77 = *(int *)k;
                            v77 += 4;
                        }
                        *(short *)v77 = *(short *)k;
                        return;
                    }
                    *(unsigned int *)&v74[12].padding_0[24] = *(int *)&v74[12].padding_0[24] + 1;
                    sub_4030f0(&v9, -0x1, v72);
                    v72 += 1;
                    j = sub_4335d0("triggerall");
                } while (j);
            }
            else
            {
LABEL_478125:
                v74 = v23;
            }
            num = 1;
            v80 = sub_433410(v26, "persistent");
            if (v80)
                num = atoi(v80);
            num1 = 0;
            v81 = sub_433410(v26, "ignorehitpause");
            if (v81)
                num1 = atoi(v81);
            if (!sub_473ed0(v26, &v15, v74))
                return;
            sub_4030f0(&v15, v19, v22);
            return;
        }
    }
    sprintf(g_52b40c + 73820, "Need at least 1 trigger");
    return;
}
