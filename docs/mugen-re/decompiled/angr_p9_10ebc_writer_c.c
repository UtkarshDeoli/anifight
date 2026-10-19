// pass9 10ebc_writer_c entry=0x459a10 (asked 0x459a10)
typedef struct st_459a10_0 {
    char padding_0[8];
    unsigned int field_8;
    unsigned int field_c;
    char padding_10[12];
    int field_1c;
    char field_20;
} st_459a10_0;

typedef struct st_459a10_1 {
    unsigned int field_0;
    int field_4;
} st_459a10_1;

typedef struct st_459a10_2 {
    unsigned int field_0;
    unsigned int field_4;
    unsigned int field_8;
    unsigned int field_c;
    unsigned int field_10;
    unsigned int field_14;
    unsigned int field_18;
    unsigned int field_1c;
    unsigned int field_20;
    unsigned int field_24;
    char field_28;
    char padding_29[31];
    unsigned int field_48;
    unsigned int field_4c;
    unsigned int field_50;
} st_459a10_2;

extern unsigned int g_52a9e8;
extern unsigned int g_52a9ec;
extern unsigned int g_52adf0;
extern unsigned int g_52adf4;
extern char g_52aed8;
extern unsigned int g_52b0e4;
extern unsigned int g_52b338;
extern unsigned int g_52b33c;
extern st_459a10_2 *g_52b34c;
extern void* g_52b40c;
extern char g_52b950;

void sub_459a10(float a0)
{
    void* v17;  // ebp
    unsigned int v18;  // ebx
    void* v27;  // eax
    void* v28;  // eax
    unsigned int v29;  // eax
    unsigned int v30;  // edi
    unsigned int v31;  // edi
    unsigned int iter;  // edi
    unsigned int v33;  // ecx
    unsigned int v34;  // ecx
    unsigned int v36;  // ebp
    unsigned int v19;  // edi
    unsigned int v37;  // edi
    unsigned int v38;  // edi
    unsigned int iter1;  // edi
    void* v40;  // eax
    unsigned int v41;  // ecx
    unsigned int *k;  // esi
    void* iter2;  // ecx
    void* v44;  // eax
    void* v45;  // eax
    unsigned int v46;  // eax
    unsigned int v20;  // ebx
    unsigned int v47;  // edi
    unsigned int v48;  // edi
    unsigned int node;  // edi
    unsigned int v50;  // ecx
    unsigned int v51;  // ecx
    float *v52;  // ecx
    int v53;  // edi
    int v54;  // eax
    int v55;  // edx
    void* v56;  // edi
    unsigned int v21;  // edi
    st_459a10_0 *v57;  // esi
    unsigned int v58;  // edx
    unsigned int v59;  // eax
    st_459a10_2 *m;  // ebx
    unsigned int v61;  // edi
    unsigned int v62;  // ecx
    unsigned int v22;  // edi
    unsigned int v23;  // ecx
    unsigned int *i;  // esi
    void* v25;  // eax
    void* j;  // ecx
    unsigned int v0;  // [bp-0x448]
    unsigned int v1;  // [bp-0x444]
    int v2;  // [bp-0x430]
    int v3;  // [bp-0x42c]
    int v4;  // [bp-0x428], Other Possible Types: unsigned int
    int v5;  // [bp-0x424]
    st_459a10_1 *v6;  // [bp-0x420], Other Possible Types: unsigned int
    unsigned int v7;  // [bp-0x41c], Other Possible Types: int
    unsigned int v8;  // [bp-0x418]
    float v9;  // [bp-0x414]
    unsigned int v10;  // [bp-0x410]
    unsigned int v11;  // [bp-0x40c]
    unsigned int v12;  // [bp-0x408]
    unsigned int v13;  // [bp-0x404]
    char v14;  // [bp-0x400]
    char v15;  // [bp-0x200]

    v17 = (void*)a0;
    v10 = 0;
    v11 = 0;
    v8 = 0;
    v5 = 320;
    v3 = 240;
    if (!v17)
    {
        return;
    }
    else if (*(char *)v17)
    {
        sub_444320("Playing storyboard %s: %s\n", v17, "scene");
        if (!sub_444150(v17, &v14, &v15, 0))
        {
            sub_444320("Storyboard file not found: %s", v17);
            return;
        }
        v6 = sub_432a30();
        if (!v6)
        {
            v18 = g_52b40c + 73820;
            v19 = v18 - 1;
            v20 = v18;
            do
            {
                v21 = v19;
                v22 = v21 + 1;
                v19 = v22;
            } while (*(char *)(v21 + 1));
            v23 = 7;
            i = "\nError loading storyboard: ";
            for (v25 = v17; v23; i += 1)
            {
                v23 -= 1;
                *(unsigned int *)v22 = *i;
                v22 += 4;
            }
            j = v25;
            do
            {
                v27 = v25;
                v28 = v27 + 1;
                v25 = v28;
            } while (*(char *)v27);
            v29 = v28 - j;
            v30 = v20 - 1;
            do
            {
                v31 = v30;
                iter = v31 + 1;
                v30 = iter;
            } while (*(char *)(v31 + 1));
            for (v33 = v29 >> 2; v33; j += 4)
            {
                v33 -= 1;
                *(int *)iter = *(int *)j;
                iter += 4;
            }
            v34 = v29 & 3;
            v1 = v20;
            for (v0 = "%s\n"; v34; j += 1)
            {
                v34 -= 1;
                *(char *)iter = *(char *)j;
                iter += 1;
            }
            sub_444320();
            return;
        }
        else
        {
            if (sub_433050() != 0xffffffff)
            {
                sub_433790("%i,%i", &v5, &v3);
                if (v5 < 1)
                    v5 = 1;
                if (v3 < 1)
                    v3 = 1;
            }
            v4 = sub_499690();
            v7 = sub_499690();
            v9 = (float)(v5 * v7 <= v3 * v4 ? (double)v7 / (double)v3 : (double)v4 / (double)v5);
            v7 = sub_499690();
            v7 = -(sub_499690() / 2);
            v4 = sub_499690();
            v2 = -(sub_499690() / 2);
            sub_432980();
            strncpy(&g_52aed8, "font/", 0x1ff);
            v6 = sub_40bb30();
            if (!v6)
            {
                v36 = g_52b40c + 73820;
                v37 = v36 - 1;
                do
                {
                    v38 = v37;
                    iter1 = v38 + 1;
                    v37 = iter1;
                } while (*(char *)(v38 + 1));
                v40 = (void*)a0;
                v41 = 7;
                for (k = "\nError loading storyboard: "; v41; k += 1)
                {
                    v41 -= 1;
                    *(unsigned int *)iter1 = *k;
                    iter1 += 4;
                }
                iter2 = v40;
                do
                {
                    v44 = v40;
                    v45 = v44 + 1;
                    v40 = v45;
                } while (*(char *)v44);
                v46 = v45 - iter2;
                v47 = v36 - 1;
                do
                {
                    v48 = v47;
                    node = v48 + 1;
                    v47 = node;
                } while (*(char *)(v48 + 1));
                for (v50 = v46 >> 2; v50; iter2 += 4)
                {
                    v50 -= 1;
                    *(int *)node = *(int *)iter2;
                    node += 4;
                }
                v51 = v46 & 3;
                v1 = v36;
                for (v0 = "%s\n"; v51; iter2 += 1)
                {
                    v51 -= 1;
                    *(char *)node = *(char *)iter2;
                    node += 1;
                }
                sub_444320();
                return;
            }
            else
            {
                v4 = v6->field_0;
                if (!v6->field_0)
                {
                    sub_40cc60(v9, v9);
                    v52[565] = (float)v7;
                    v52[566] = (float)v2;
                }
                v13 = g_52b40c + 69612;
                v12 = g_52b40c + 69560;
                sub_434ee0();
                sub_4354b0();
                sub_437650();
                sub_40f180();
                g_52a9ec = 0;
                g_52a9e8 = 0;
                g_52adf4 = 0;
                g_52adf0 = 0;
                sub_411cf0();
                g_52b33c = 0;
                g_52b338 = 0;
                sub_445540();
                sub_499690();
                sub_499690();
                sub_4352b0(v7);
                sub_4447a0();
                while (1)
                {
                    if (!(int)g_52b40c[69308])
                    {
LABEL_459dbb:
                        if (v8)
                        {
                            v8 = 0;
                            sub_4089b0();
                        }
                    }
                    else if (!v8)
                    {
                        v8 = 1;
                        sub_408950();
                    }
                    else if (!(int)g_52b40c[69308])
                    {
                        goto LABEL_459dbb;
                    }
                    v2 = 0;
                    v53 = 0;
                    sub_40f180();
                    if (g_52a9e8 != g_52a9ec)
                        v2 = sub_414af0();
                    if (sub_415050())
                        v53 = sub_415000();
                    sub_443c30();
                    if (sub_411ec0())
                        sub_411e70();
                    v54 = sub_411970();
                    if (v54 & 0xff0)
                    {
                        sub_444320("Storyboard playback canceled %X:%i k%X p%X\n", v54, v55, v2, v53);
                        v10 = 1;
                    }
                    if (v2 == 27 || g_52b0e4)
                    {
                        sub_444320("Storyboard playback canceled by Esc\n");
                        v10 = 1;
                    }
                    v56 = g_52b40c;
                    v57 = *(int *)(int)g_52b40c[32956];
                    memset(v57->field_1c, 0, v57->field_c * 16);
                    v57->field_8 = 0;
                    memset(&v57->field_20, 0, 12);
                    if (!(int)g_52b40c[69308])
                    {
                        if (v4 || sub_40cd70(v6->field_4))
                        {
                            sub_444320("Storyboard playback finished OK\n");
                            v56 = g_52b40c;
                            v10 = 1;
                            goto LABEL_459edf;
                        }
                        else
                        {
                            v56 = g_52b40c;
                        }
                    }
LABEL_459edf:
                    v58 = g_52b950;
                    v59 = v11;
                    *(unsigned int *)&v56[69308] = v58;
                    if (v59)
                    {
                        v59 = 0;
                        v11 = 0;
                        g_52b950 = 1;
                    }
                    if (v58 && v2 && !v59 && !(v2 & 0x200))
                    {
                        if (v2 == 293)
                            goto LABEL_459f3a;
                        if (v2 == 302)
                        {
                            g_52b950 = 0;
                            v11 = 1;
                            goto LABEL_459f45;
                        }
                    }
                    if (v2 == 293)
                    {
LABEL_459f3a:
                        sub_444870();
                        v56 = g_52b40c;
                    }
LABEL_459f45:
                    if (!(int)v56[69300])
                    {
                        g_52b34c->field_4 = 0;
                        g_52b34c->field_8 = 0;
                        g_52b34c->field_c = 0;
                        g_52b34c->field_1c = 0;
                        g_52b34c->field_20 = 0;
                        g_52b34c->field_18 = 0x100;
                        g_52b34c->field_14 = 0x100;
                        g_52b34c->field_10 = 0x100;
                        g_52b34c->field_0 = 0x100;
                        g_52b34c->field_48 = 1;
                        g_52b34c->field_50 = 1;
                        if (!v4)
                            sub_40d390(v6->field_4);
                        if (g_52b34c->field_48)
                        {
                            if (sub_413be0())
                                g_52b34c->field_48 = 0;
                            else
                                g_52b34c->field_4c = g_52b34c->field_4c + 1;
                        }
                        if (!v4)
                        {
                            sub_40d4e0(v6->field_4);
                            v56 = g_52b40c;
                        }
                    }
                    if (!(int)v56[74844])
                    {
                        sub_437730();
                        v56 = g_52b40c;
                    }
                    if (!(int)v56[69300])
                    {
                        sub_4351b0();
                        m = g_52b34c;
                        if (!(int)g_52b40c[69300] && g_52b34c->field_48)
                        {
                            v61 = &g_52b34c->field_24;
                            for (v62 = 9; v62; m = &m->field_4)
                            {
                                v62 -= 1;
                                *(unsigned int *)v61 = m->field_0;
                                v61 += 4;
                            }
                            g_52b34c->field_48 = 0;
                        }
                    }
                    sub_4447c0();
                    if (v10)
                        break;
                }
                sub_4447a0();
                sub_413d10();
                *(unsigned int *)&g_52b40c[69308] = 0;
                g_52b950 = 0;
                sub_40d940();
                sub_444320("Finished playing storyboard %s: %s\n", a0, "scene");
                return;
            }
        }
    }
    else
    {
        return;
    }
}
