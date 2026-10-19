// param_reader entry=0x451c20
extern char g_4f2e4b;
extern unsigned int g_52b370;
extern void* g_52b40c;
extern unsigned int g_52be10;

int sub_451c20(void)
{
    char *v29;  // eax
    int v30;  // esi
    unsigned int v39;  // eax
    char *v129;  // eax
    unsigned int v130;  // eax
    unsigned int v131;  // esi
    char *v132;  // eax
    char *v133;  // eax
    int v134;  // eax
    int v135;  // eax
    int v136;  // eax
    int v137;  // eax
    int v138;  // eax
    int v40;  // esi
    unsigned int v139;  // eax
    int v140;  // eax
    unsigned int v141;  // eax
    unsigned int v142;  // esi
    int v143;  // eax
    int v144;  // eax
    int v145;  // eax
    int v146;  // eax
    int v147;  // eax
    unsigned int v148;  // eax
    unsigned int v41;  // eax
    unsigned int v149;  // eax
    int v150;  // edi
    char *v151;  // eax
    unsigned int idx;  // ecx
    unsigned int v153;  // eax
    unsigned int v154;  // eax
    unsigned int idx1;  // edi
    int v43;  // esi
    unsigned int v44;  // eax
    int v45;  // esi
    unsigned int v46;  // eax
    void* v47;  // ecx
    unsigned int v48;  // eax
    unsigned int v31;  // ebx
    unsigned int v49;  // eax
    unsigned int v50;  // eax
    unsigned int v51;  // eax
    unsigned int v52;  // edi
    unsigned int v53;  // edi
    unsigned int iter;  // edi
    unsigned int v55;  // ecx
    void* i;  // esi
    unsigned int iter1;  // edi
    int v58;  // esi
    char *v32;  // eax
    unsigned int iter2;  // eax
    unsigned int l;  // ecx
    unsigned int v61;  // ecx
    int v63;  // esi
    unsigned int v64;  // edi
    unsigned int v65;  // eax
    unsigned int m;  // ecx
    unsigned int v67;  // ecx
    char *v33;  // eax
    int v69;  // esi
    unsigned int v70;  // edi
    unsigned int v71;  // eax
    unsigned int n;  // ecx
    unsigned int v73;  // ecx
    unsigned int v75;  // edx
    unsigned int v76;  // ecx
    int v77;  // edi
    unsigned int v78;  // esi
    char *v34;  // eax
    unsigned int v79;  // eax
    unsigned int i0;  // ecx
    unsigned int v81;  // ecx
    int v83;  // edi
    int v84;  // esi
    unsigned int idx2;  // eax
    unsigned int v86;  // esi
    int v87;  // edi
    char *v88;  // eax
    char *v35;  // eax
    unsigned int v89;  // eax
    unsigned int v90;  // esi
    int v91;  // edi
    char *v92;  // eax
    unsigned int v93;  // eax
    unsigned int v94;  // ecx
    int v95;  // edi
    char *v96;  // eax
    char *v97;  // eax
    unsigned int v98;  // eax
    unsigned int v36;  // esi
    unsigned int v99;  // edi
    unsigned int v100;  // eax
    unsigned int v101;  // eax
    unsigned int v102;  // eax
    unsigned int v103;  // esi
    char *v104;  // eax
    unsigned int v105;  // eax
    unsigned int v106;  // eax
    unsigned int v107;  // esi
    unsigned int v108;  // ecx
    unsigned int v37;  // esi
    unsigned int v109;  // eax
    char *v110;  // eax
    char *v111;  // eax
    char *v112;  // eax
    unsigned int index;  // esi
    unsigned int v114;  // 4116
    unsigned int v115;  // ecx
    unsigned int v116;  // edx
    unsigned int v117;  // esi
    char *v118;  // eax
    unsigned int n;  // esi
    char *v119;  // eax
    char *v120;  // eax
    unsigned int v121;  // esi
    unsigned int v122;  // 4116
    unsigned int v123;  // eax
    unsigned int v124;  // eax
    unsigned int v125;  // eax
    char *v126;  // eax
    char *v127;  // eax
    char *v128;  // eax
    unsigned int v0;  // [bp-0x68c]
    char *v1;  // [bp-0x688], Other Possible Types: int, unsigned int
    int v2;  // [bp-0x684]
    int v3;  // [bp-0x680]
    int v4;  // [bp-0x67c]
    int v5;  // [bp-0x678]
    int v6;  // [bp-0x674], Other Possible Types: unsigned int
    int k;  // [bp-0x670], Other Possible Types: unsigned int
    unsigned int v8;  // [bp-0x670]
    unsigned int v9;  // [bp-0x66c], Other Possible Types: int
    unsigned int v10;  // [bp-0x66c]
    int node;  // [bp-0x668], Other Possible Types: unsigned int
    unsigned int v12;  // [bp-0x668]
    unsigned int v13;  // [bp-0x664]
    unsigned int j;  // [bp-0x660]
    unsigned int v15;  // [bp-0x660]
    char v16[192];  // [bp-0x65c]
    char *v17;  // [bp-0x59c]
    unsigned int v18;  // [bp-0x598]
    unsigned int v19;  // [bp-0x594]
    unsigned int v20;  // [bp-0x590]
    unsigned int v21;  // [bp-0x58c]
    unsigned int v22;  // [bp-0x588]
    char v23;  // [bp-0x584]
    char v24;  // [bp-0x544]
    char v25[256];  // [bp-0x504]
    char v26;  // [bp-0x404]
    char v27[516];  // [bp-0x204], Other Possible Types: char

    v13 = g_52b40c + 33008;
    k = g_52b40c + 33064;
    sub_444320("    Opening fight data file %s...", v29, v2, v3, v4, v5);
    v30 = sub_40e7f0(v29, "rt");
    if (v30)
    {
        v31 = sub_432ab0(v30, 0);
        sub_40eab0();
        if (v31)
        {
            sub_444320("OK\n");
            if (*v29)
            {
                v32 = v29;
                v33 = v32;
                do
                {
                    v34 = v33;
                    v35 = v34 + 1;
                    v33 = v35;
                } while (*v34);
                v36 = v35 - (v32 + 1) - 1;
                if ((int)v36 >= 0)
                {
                    do
                    {
                        v37 = v36;
                    } while (v29[v36] != 92 && (v36 = v37, v29[v36] != 47 && (v36 = v37 - 1, (int)(v37 - 1) >= 0)));
                }
                n = v36 + 1;
                if ((int)n > 0)
                    strncpy(v27, v29, n);
                v27[n] = 0;
            }
            else
            {
                v27 = 0;
            }
            sub_444320("    Reading [Files]...");
            v39 = v31 + 32;
            if ((!v39 || !*(char *)v39 || (v1 = "Files", v0 = v39, _snprintf(v25, 0x100, "%s.%s"), sub_4330a0() == 0xffffffff)) && sub_4330a0() == 0xffffffff)
            {
                v1 = "Missing files info";
            }
            else
            {
                v40 = sub_433410(v31, "sff");
                if (!v40)
                {
                    sprintf(g_52b40c + 73820, "Can't read Files|sff");
                    return;
                }
                if (!sub_444150(v40, &v26, 0, &v27))
                    return;
                v41 = sub_41a080(&v26);
                idx1 = v13;
                *(unsigned int *)idx1 = v41;
                if (!v41)
                {
                    v1 = v40;
                    sprintf(g_52b40c + 73820, "Can't load %s");
                    return;
                }
                v43 = sub_433410(v31, "snd");
                if (!v43)
                {
                    v1 = "Can't read Files|snd";
                }
                else
                {
                    if (g_52b370 != 1)
                    {
                        *(unsigned int *)(idx1 + 48) = 0;
                    }
                    else if (!sub_444150(v43, &v26, 0, &v27))
                    {
                        return;
                    }
                    else
                    {
                        v44 = sub_415b20(&v26);
                        *(unsigned int *)(idx1 + 48) = v44;
                        if (!v44)
                        {
                            v1 = v43;
                            sprintf(g_52b40c + 73820, "Can't load %s");
                            return;
                        }
                    }
                    v45 = sub_433410(v31, "fightfx.sff");
                    if (!v45)
                    {
                        v1 = "Can't read Files|fightfx.sff";
                        v0 = g_52b40c + 73820;
LABEL_451d7a:
                        sprintf(v0, v1);
                        return;
                    }
                    if (!sub_444150(v45, &v26, 0, &v27))
                        return;
                    v46 = sub_41a080(&v26);
                    v47 = g_52b40c;
                    *(unsigned int *)&g_52b40c[32996] = v46;
                    if (!v46)
                    {
LABEL_451fef:
                        v1 = v45;
                        sprintf(v47 + 73820, "Can't load %s");
                        return;
                    }
                    v45 = sub_433410(v31, "fightfx.air");
                    if (!v45)
                    {
                        v1 = "Can't read Files|fightfx.air";
                    }
                    else
                    {
                        if (!sub_444150(v45, &v26, 0, &v27))
                            return;
                        v48 = sub_401000();
                        v47 = g_52b40c;
                        *(unsigned int *)&g_52b40c[33000] = v48;
                        if (!v48)
                            goto LABEL_451fef;
                        v45 = sub_433410(v31, "common.snd");
                        if (!v45)
                        {
                            v1 = "Can't read Files|common.snd";
                        }
                        else if (g_52b370 == 1)
                        {
                            if (!sub_444150(v45, &v26, 0, &v27))
                                return;
                            v49 = sub_415b20(&v26);
                            v47 = g_52b40c;
                            *(unsigned int *)&g_52b40c[33060] = v49;
                            if (v49)
                                goto LABEL_45201d;
                            goto LABEL_451fef;
                        }
                        else
                        {
                            *(unsigned int *)&g_52b40c[33060] = 0;
LABEL_45201d:
                            sub_444320("OK\n");
                            sub_444320("    Loading fonts...");
                            v17 = &v27;
                            v18 = g_52b40c + 65208;
                            v19 = "./";
                            v20 = "data";
                            v21 = "font/";
                            v22 = 0;
                            sub_410bc0(v31, &v17);
                            sub_444320("OK\n");
                            *(unsigned int *)(v31 + 24) = 0xffffffff;
                            v50 = sub_433220();
                            if (!sub_40e3a0())
                            {
                                do
                                {
                                    v50 = sub_433220();
                                    if (!v50)
                                        goto LABEL_45213f;
                                } while (!sub_40e3a0());
                            }
                            if (v50)
                            {
                                sub_444320("    Load fight anim...");
                                v51 = sub_401130();
                                *(unsigned int *)(v13 + 4) = v51;
                                if (!v51)
                                {
                                    v52 = g_52b40c + 73819;
                                    do
                                    {
                                        v53 = v52;
                                        iter = v53 + 1;
                                        v52 = iter;
                                    } while (*(char *)(v53 + 1));
                                    v55 = 7;
                                    for (i = "\nError loading anim in Files"; v55; i += 4)
                                    {
                                        v55 -= 1;
                                        *(int *)iter = *(int *)i;
                                        iter += 4;
                                    }
                                    *(char *)iter = *(char *)i;
                                    return;
                                }
                                sub_444320("OK\n");
                            }
LABEL_45213f:
                            *(float *)&g_52b40c[33004] = 1.0f;
                            if (sub_433050() != 0xffffffff)
                                sub_4336d0(g_52b40c + 33004);
                            sub_444320("    Reading [Lifebar]...");
                            if (sub_433050() == 0xffffffff)
                            {
                                v1 = "Missing lifebar info";
                            }
                            else
                            {
                                iter1 = k;
                                v58 = 1;
                                v6 = 1;
                                node = 2;
                                while (1)
                                {
                                    iter2 = iter1 + 52;
                                    l = 5;
                                    do
                                    {
                                        v61 = l;
                                        *(unsigned int *)iter2 = 1;
                                        iter2 += 160;
                                        l = v61 - 1;
                                    } while (l);
                                    sprintf(v16, "p%i.pos", v58);
                                    sub_433790("%i,%i", iter1, iter1 + 4);
                                    sprintf(v16, "p%i.range.x", v58);
                                    sub_433790("%i,%i", iter1 + 8, iter1 + 12);
                                    v9 = 0;
                                    do
                                    {
                                        sprintf(v16, "p%i.bg%i", v6, v9);
                                    } while ((sub_446440(v31), v9 += 1, v9 < 3));
                                    sprintf(v16, "p%i.mid", v6);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.front", v6);
                                    sub_446440(v31);
                                    v6 += 1;
                                    iter1 += 820;
                                    v12 = node - 1;
                                    if (node == 1)
                                        break;
                                    v58 = v6;
                                    node = v12;
                                }
                                sub_444320("[Turns Lifebar]...");
                                if (sub_433050() == 0xffffffff)
                                {
                                    sprintf(g_52b40c + 73820, "Missing turns lifebar info");
                                    return;
                                }
                                v63 = 1;
                                v6 = 1;
                                v64 = k + 4920;
                                node = 2;
                                while (1)
                                {
                                    v65 = v64 + 52;
                                    m = 5;
                                    do
                                    {
                                        v67 = m;
                                        *(unsigned int *)v65 = 1;
                                        v65 += 160;
                                        m = v67 - 1;
                                    } while (m);
                                    sprintf(v16, "p%i.pos", v63);
                                    sub_433790("%i,%i", v64, v64 + 4);
                                    sprintf(v16, "p%i.range.x", v63);
                                    sub_433790("%i,%i", v64 + 8, v64 + 12);
                                    v9 = 0;
                                    do
                                    {
                                        sprintf(v16, "p%i.bg%i", v6, v9);
                                    } while ((sub_446440(v31), v9 += 1, v9 < 3));
                                    sprintf(v16, "p%i.mid", v6);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.front", v6);
                                    sub_446440(v31);
                                    v6 += 1;
                                    v64 += 820;
                                    v12 = node - 1;
                                    if (node == 1)
                                        break;
                                    v63 = v6;
                                    node = v12;
                                }
                                sub_444320("[Simul Lifebar]...");
                                if (sub_433050() == 0xffffffff)
                                {
                                    sprintf(g_52b40c + 73820, "Missing simul lifebar info");
                                    return;
                                }
                                v69 = 1;
                                v6 = 1;
                                v70 = k + 1640;
                                k = 4;
                                while (1)
                                {
                                    v71 = v70 + 52;
                                    n = 5;
                                    do
                                    {
                                        v73 = n;
                                        *(unsigned int *)v71 = 1;
                                        v71 += 160;
                                        n = v73 - 1;
                                    } while (n);
                                    sprintf(v16, "p%i.pos", v69);
                                    sub_433790("%i,%i", v70, v70 + 4);
                                    sprintf(v16, "p%i.range.x", v69);
                                    sub_433790("%i,%i", v70 + 8, v70 + 12);
                                    v9 = 0;
                                    do
                                    {
                                        sprintf(v16, "p%i.bg%i", v6, v9);
                                    } while ((sub_446440(v31), v9 += 1, v9 < 3));
                                    sprintf(v16, "p%i.mid", v6);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.front", v6);
                                    sub_446440(v31);
                                    v6 += 1;
                                    v70 += 820;
                                    v8 = k - 1;
                                    if (k == 1)
                                        break;
                                    v69 = v6;
                                    k = v8;
                                }
                                sub_444320("[Powerbar]...");
                                if (sub_433050() == 0xffffffff)
                                {
                                    sprintf(g_52b40c + 73820, "Missing Powerbar info");
                                    return;
                                }
                                v75 = v13;
                                v76 = v75 + 10216;
                                memset(v76, -0x1, 40);
                                v77 = 1;
                                v78 = v75 + 8260;
                                j = v76;
                                k = v75 + 9896;
                                v6 = v78;
                                node = 2;
                                while (1)
                                {
                                    v79 = v78 + 48;
                                    i0 = 5;
                                    do
                                    {
                                        v81 = i0;
                                        *(unsigned int *)v79 = 1;
                                        v79 += 160;
                                        i0 = v81 - 1;
                                    } while (i0);
                                    sprintf(v16, "p%i.pos", v77);
                                    sub_433790("%i,%i", v78 - 4, v78);
                                    sprintf(v16, "p%i.range.x", v77);
                                    sub_433790("%i,%i", v78 + 4, v78 + 8);
                                    v9 = 0;
                                    v83 = v77;
                                    do
                                    {
                                        sprintf(v16, "p%i.bg%i", v83, v9);
                                    } while ((sub_446440(v31), v9 += 1, v9 < 3));
                                    sprintf(v16, "p%i.mid", v83);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.front", v83);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.counter", v83);
                                    sub_446440(v31);
                                    v6 += 820;
                                    k += 160;
                                    v77 = v83 + 1;
                                    v12 = node - 1;
                                    if (node == 1)
                                        break;
                                    v78 = v6;
                                    node = v12;
                                }
                                v84 = 0;
                                do
                                {
                                    sprintf(&v23, "level%i.snd", v84);
                                } while ((sub_433790("%i,%i", j, j + 40), v84 += 1, j += 4, v84 < 10));
                                sub_444320("\n    [Face]...");
                                if (sub_433050() == 0xffffffff)
                                {
                                    sprintf(g_52b40c + 73820, "Missing face info");
                                    return;
                                }
                                idx2 = v13 + 10296;
                                v86 = idx2 + 0xa00;
                                *(unsigned int *)(idx2 + 2592) = 1;
                                *(unsigned int *)(idx2 + 2596) = 0xffffffff;
                                node = 1;
                                v6 = idx2;
                                k = v86;
                                v9 = 2;
                                while (1)
                                {
                                    sprintf(v16, "p%i.pos", node);
                                    sub_433790("%i,%i", v86, v86 + 16);
                                    v87 = 0;
                                    do
                                    {
                                        sprintf(v16, "p%i.bg%i", node, v87);
                                    } while ((sub_446440(v31), v87 += 1, v87 < 3));
                                    sprintf(v16, "p%i.bg", node);
                                    sprintf(&v23, "%s.spr", v16);
                                    sprintf(&v24, "%s.anim", v16);
                                    if (sub_433410(v31, &v23) || sub_433410(v31, &v24))
                                        sub_446440(v31);
                                    sprintf(v16, "p%i.face", node);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.facing", node);
                                    v88 = sub_433410(v31, v16);
                                    if (v88)
                                        *(int *)(k + 32) = atoi(v88);
                                    k += 4;
                                    v6 += 640;
                                    v10 = v9 - 1;
                                    node += 1;
                                    if (v9 == 1)
                                        break;
                                    v86 = k;
                                    v9 = v10;
                                }
                                sub_444320("[Simul Face]...");
                                if (sub_433050() == 0xffffffff)
                                {
                                    sprintf(g_52b40c + 73820, "Missing simul face info");
                                    return;
                                }
                                v89 = v13 + 12904;
                                v90 = v89 + 0xa00;
                                *(unsigned int *)(v89 + 2600) = 1;
                                *(unsigned int *)(v89 + 2592) = 1;
                                *(unsigned int *)(v89 + 2604) = 0xffffffff;
                                *(unsigned int *)(v89 + 2596) = 0xffffffff;
                                node = 1;
                                v6 = v89;
                                k = v90;
                                v10 = 4;
                                while (1)
                                {
                                    sprintf(v16, "p%i.pos", node);
                                    sub_433790("%i,%i", v90, v90 + 16);
                                    v91 = 0;
                                    do
                                    {
                                        sprintf(v16, "p%i.bg%i", node, v91);
                                    } while ((sub_446440(v31), v91 += 1, v91 < 3));
                                    sprintf(v16, "p%i.bg", node);
                                    sprintf(&v23, "%s.spr", v16);
                                    sprintf(&v24, "%s.anim", v16);
                                    if (sub_433410(v31, &v23) || sub_433410(v31, &v24))
                                        sub_446440(v31);
                                    sprintf(v16, "p%i.face", node);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.facing", node);
                                    v92 = sub_433410(v31, v16);
                                    if (v92)
                                        *(int *)(k + 32) = atoi(v92);
                                    k += 4;
                                    v6 += 640;
                                    v9 = v10 - 1;
                                    node += 1;
                                    if (v10 == 1)
                                        break;
                                    v90 = k;
                                    v10 = v9;
                                }
                                sub_444320("[Turns Face]...");
                                if (sub_433050() == 0xffffffff)
                                {
                                    sprintf(g_52b40c + 73820, "Missing turns face info");
                                    return;
                                }
                                v93 = v13 + 15512;
                                v94 = v93 + 2592;
                                *(unsigned int *)v94 = 1;
                                *(unsigned int *)(v93 + 2596) = 0xffffffff;
                                v95 = 1;
                                k = v93;
                                node = v94;
                                v6 = v13 + 18120;
                                j = 2;
                                do
                                {
                                    v15 = j;
                                    sprintf(v16, "p%i.pos", v95);
                                    sub_433790("%i,%i", node - 32, node - 16);
                                    v9 = 0;
                                    do
                                    {
                                        sprintf(v16, "p%i.bg%i", v95, v9);
                                    } while ((sub_446440(v31), v9 += 1, v9 < 3));
                                    sprintf(v16, "p%i.bg", v95);
                                    sprintf(&v23, "%s.spr", v16);
                                    sprintf(&v24, "%s.anim", v16);
                                    if (sub_433410(v31, &v23) || sub_433410(v31, &v24))
                                        sub_446440(v31);
                                    sprintf(v16, "p%i.face", v95);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.facing", v95);
                                    v96 = sub_433410(v31, v16);
                                    if (v96)
                                        *(int *)node = atoi(v96);
                                    sprintf(v16, "p%i.teammate.pos", v95);
                                    sub_433790("%i,%i", v6 + 800, v6 + 804);
                                    sprintf(v16, "p%i.teammate.spacing", v95);
                                    sub_433790("%i,%i", v6 + 812, v6 + 816);
                                    v9 = 0;
                                    do
                                    {
                                        sprintf(v16, "p%i.teammate.bg%i", v95, v9);
                                    } while ((sub_446440(v31), v9 += 1, v9 < 3));
                                    sprintf(v16, "p%i.teammate.bg", v95);
                                    sprintf(&v23, "%s.spr", v16);
                                    sprintf(&v24, "%s.anim", v16);
                                    if (sub_433410(v31, &v23) || sub_433410(v31, &v24))
                                        sub_446440(v31);
                                    sprintf(v16, "p%i.teammate.face", v95);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.teammate.ko", v95);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.teammate.facing", v95);
                                    v97 = sub_433410(v31, v16);
                                    if (v97)
                                        *(int *)node = atoi(v97);
                                    node += 4;
                                    v6 += 820;
                                    k += 640;
                                    v95 += 1;
                                    j = v15 - 1;
                                } while (j);
                                sub_444320("\n    [Name]...");
                                v98 = v31 + 32;
                                if ((!v98 || !*(char *)v98 || (v1 = "name", v0 = v98, _snprintf(v25, 0x100, "%s.%s"), sub_4330a0() == 0xffffffff)) && sub_4330a0() == 0xffffffff)
                                {
                                    sprintf(g_52b40c + 73820, "Missing name info");
                                    return;
                                }
                                v99 = v13;
                                k = v99 + 19760;
                                node = 1;
                                v6 = k + 0x500;
                                j = 2;
                                do
                                {
                                    v15 = j;
                                    sprintf(v16, "p%i.pos", node);
                                    sub_433790("%i,%i", v6, v6 + 16);
                                    sprintf(v16, "p%i.bg", node);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.name", node);
                                    sub_446440(v31);
                                    v6 += 4;
                                    k += 160;
                                    node += 1;
                                    j = v15 - 1;
                                } while (j);
                                sub_444320("[Simul Name]...");
                                v100 = v31 + 32;
                                if ((!v100 || !*(char *)v100 || (v1 = "simul name", v0 = v100, _snprintf(v25, 0x100, "%s.%s"), sub_4330a0() == 0xffffffff)) && sub_4330a0() == 0xffffffff)
                                {
                                    sprintf(g_52b40c + 73820, "Missing simul name info");
                                    return;
                                }
                                k = v99 + 21072;
                                node = 1;
                                v6 = k + 0x500;
                                j = 4;
                                do
                                {
                                    v15 = j;
                                    sprintf(v16, "p%i.pos", node);
                                    sub_433790("%i,%i", v6, v6 + 16);
                                    sprintf(v16, "p%i.bg", node);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.name", node);
                                    sub_446440(v31);
                                    v6 += 4;
                                    k += 160;
                                    node += 1;
                                    j = v15 - 1;
                                } while (j);
                                sub_444320("[Turns Name]...");
                                v101 = v31 + 32;
                                if ((!v101 || !*(char *)v101 || (v1 = "turns name", v0 = v101, _snprintf(v25, 0x100, "%s.%s"), sub_4330a0() == 0xffffffff)) && sub_4330a0() == 0xffffffff)
                                {
                                    sprintf(g_52b40c + 73820, "Missing turns name info");
                                    return;
                                }
                                k = v99 + 22384;
                                node = 1;
                                v6 = k + 0x500;
                                j = 4;
                                do
                                {
                                    v15 = j;
                                    sprintf(v16, "p%i.pos", node);
                                    sub_433790("%i,%i", v6, v6 + 16);
                                    sprintf(v16, "p%i.bg", node);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.name", node);
                                    sub_446440(v31);
                                    v6 += 4;
                                    k += 160;
                                    node += 1;
                                    j = v15 - 1;
                                } while (j);
                                sub_444320("\n    [Time]...");
                                v102 = v31 + 32;
                                if ((!v102 || !*(char *)v102 || (v1 = "time", v0 = v31 + 32, _snprintf(v25, 0x100, "%s.%s"), sub_4330a0() == 0xffffffff)) && sub_4330a0() == 0xffffffff)
                                {
                                    sprintf(g_52b40c + 73820, "Missing time info");
                                    return;
                                }
                                v103 = v99 + 23696;
                                *(unsigned int *)(v103 + 328) = 60;
                                sub_433790("%i,%i", v103 + 320, v103 + 324);
                                sub_446440(v31);
                                sub_446440(v31);
                                v104 = sub_433410(v31, "framespercount");
                                if (v104)
                                    *(int *)(v99 + 24024) = atoi(v104);
                                sub_444320("[Combo]...");
                                v105 = v31 + 32;
                                if ((!v105 || !*(char *)v105 || (v1 = "combo", v0 = v31 + 32, _snprintf(v25, 0x100, "%s.%s"), sub_4330a0() == 0xffffffff)) && sub_4330a0() == 0xffffffff)
                                {
                                    sprintf(g_52b40c + 73820, "Missing combo info");
                                    return;
                                }
                                v106 = v99 + 24036;
                                if (sub_433410(v31, "pos"))
                                {
                                    v13 = v106;
                                    j = 2;
                                    do
                                    {
                                        v15 = j;
                                        v107 = v13;
                                        v108 = v107 - 4;
                                        v109 = v107 - 8;
                                        *(unsigned int *)v109 = 10;
                                        *(unsigned int *)v108 = 80;
                                        *(unsigned int *)v107 = 0xffffffd8;
                                        *(unsigned int *)(v107 + 4) = 90;
                                        sub_433790("%i,%i", v109, v108);
                                        v110 = sub_433410(v31, "start.x");
                                        if (v110)
                                            *(int *)v107 = atoi(v110);
                                        sub_446440(v31);
                                        v111 = sub_433410(v31, "counter.shake");
                                        if (v111)
                                            *(int *)(v13 + 8) = atoi(v111);
                                        sub_446440(v31);
                                        v112 = sub_433410(v31, "displaytime");
                                        if (v112)
                                        {
                                            index = v13;
                                            *(int *)(index + 4) = atoi(v112);
                                        }
                                        else
                                        {
                                            index = v13;
                                        }
                                        strncpy(index + 396, index + 308, 64);
                                        *(char *)(index + 459) = 0;
                                        strncpy(index + 172, index + 84, 64);
                                        v114 = *(char *)(index + 172);
                                        *(char *)(index + 459) = 0;
                                        if (!(char)v114)
                                        {
                                            *(unsigned short *)(index + 172) = 26917;
                                            *(char *)(index + 174) = 0;
                                        }
                                        j = v15 - 1;
                                        v13 = index + 468;
                                    } while (j);
                                    *(unsigned int *)(v99 + 24528) = 0xffffffff;
                                    *(unsigned int *)(v99 + 24752) = 0xffffffff;
                                    v115 = g_52be10 - *(int *)(v99 + 24504);
                                    v116 = 319 - *(int *)(v99 + 24496);
                                    *(unsigned int *)(v99 + 24060) = 1;
                                    *(unsigned int *)(v99 + 24284) = 1;
                                    *(unsigned int *)(v99 + 24504) = v115 - 1;
                                    *(unsigned int *)(v99 + 24496) = v116;
                                }
                                else
                                {
                                    v9 = 1;
                                    v13 = v106;
                                    j = 2;
                                    do
                                    {
                                        v15 = j;
                                        sprintf(v16, "team%i.pos", v9);
                                        v117 = v13;
                                        sub_433790("%i,%i", v117 - 8, v117 - 4);
                                        sprintf(v16, "team%i.start.x", v9);
                                        v118 = sub_433410(v31, v16);
                                        if (v118)
                                            *(int *)v117 = atoi(v118);
                                        sprintf(v16, "team%i.counter", v9);
                                        sub_446440(v31);
                                        sprintf(v16, "team%i.counter.shake", v9);
                                        v119 = sub_433410(v31, v16);
                                        if (v119)
                                            *(int *)(v13 + 8) = atoi(v119);
                                        sprintf(v16, "team%i.text", v9);
                                        sub_446440(v31);
                                        sprintf(v16, "team%i.displaytime", v9);
                                        v120 = sub_433410(v31, v16);
                                        if (v120)
                                        {
                                            v121 = v13;
                                            *(int *)(v121 + 4) = atoi(v120);
                                        }
                                        else
                                        {
                                            v121 = v13;
                                        }
                                        strncpy(v121 + 396, v121 + 308, 64);
                                        *(char *)(v121 + 459) = 0;
                                        strncpy(v121 + 172, v121 + 84, 64);
                                        v122 = *(char *)(v121 + 172);
                                        *(char *)(v121 + 459) = 0;
                                        if (!(char)v122)
                                        {
                                            *(unsigned short *)(v121 + 172) = 26917;
                                            *(char *)(v121 + 174) = 0;
                                        }
                                        v9 += 1;
                                        j = v15 - 1;
                                        v13 = v121 + 468;
                                    } while (j);
                                }
                                sub_444320("[Round]...");
                                v123 = v31 + 32;
                                if ((!v123 || !*(char *)v123 || (v1 = "round", v0 = v123, _snprintf(v25, 0x100, "%s.%s"), sub_4330a0() == 0xffffffff)) && sub_4330a0() == 0xffffffff)
                                {
                                    sprintf(g_52b40c + 73820, "Missing round info");
                                    return;
                                }
                                *(unsigned int *)(v99 + 24972) = 30;
                                *(unsigned int *)(v99 + 24976) = 70;
                                v124 = (int)g_52b40c[71764] / 2;
                                *(unsigned int *)(v99 + 27560) = v124;
                                *(unsigned int *)(v99 + 27048) = v124;
                                *(unsigned int *)(v99 + 26872) = v124;
                                v125 = (int)g_52b40c[71768] / 2;
                                *(unsigned int *)(v99 + 27564) = v125;
                                *(unsigned int *)(v99 + 27052) = v125;
                                *(unsigned int *)(v99 + 26876) = v125;
                                *(unsigned int *)(v99 + 26908) = 1751607622;
                                *(unsigned short *)(v99 + 26912) = 8564;
                                *(char *)(v99 + 26914) = 0;
                                *(char **)(v99 + 27084) = &g_4f2e4b;
                                *(unsigned int *)(v99 + 26748) = 0;
                                *(unsigned int *)(v99 + 26996) = 0;
                                *(unsigned int *)(v99 + 27492) = 0;
                                *(unsigned int *)(v99 + 28028) = 60;
                                *(unsigned int *)(v99 + 28036) = 60;
                                *(unsigned int *)(v99 + 28040) = 0;
                                *(unsigned int *)(v99 + 28044) = 10;
                                *(unsigned int *)(v99 + 28048) = 45;
                                *(unsigned int *)(v99 + 28052) = 150;
                                *(unsigned int *)(v99 + 24964) = 2;
                                *(unsigned int *)(v99 + 24968) = 1;
                                v126 = sub_433410(v31, "match.wins");
                                if (v126)
                                    *(int *)(v99 + 24964) = atoi(v126);
                                v127 = sub_433410(v31, "match.maxdrawgames");
                                if (v127)
                                    *(int *)(v99 + 24968) = atoi(v127);
                                v128 = sub_433410(v31, "start.waittime");
                                if (v128)
                                    *(int *)(v99 + 24972) = atoi(v128);
                                v129 = sub_433410(v31, "intro.time");
                                if (v129)
                                    *(int *)(v99 + 24976) = atoi(v129);
                                sub_433790("%i,%i", v99 + 24980, v99 + 24984);
                                sub_446440(v31);
                                v130 = 0;
                                v131 = 0;
                                if (*(char *)(v99 + 25060))
                                {
                                    do
                                    {
                                        if (*(char *)(v130 + v99 + 25060) != 37 || *(char *)(v130 + v99 + 25061) != 37)
                                        {
                                            *(char *)(v131 + v99 + 25060) = *(char *)(v130 + v99 + 25060);
                                            v131 += 1;
                                        }
                                    } while ((v130 += 1, *(char *)(v130 + v99 + 25060)));
                                }
                                *(char *)(v131 + v99 + 25060) = 0;
                                v132 = sub_433410(v31, "round.time");
                                if (v132)
                                    *(int *)(v99 + 26748) = atoi(v132);
                                *(int *)(v99 + 26752) = *(int *)(v99 + 26748);
                                v133 = sub_433410(v31, "round.sndtime");
                                if (v133)
                                    *(int *)(v99 + 26752) = atoi(v133);
                                k = 1;
                                node = v99 + 26760;
                                do
                                {
                                    *(unsigned int *)node = 0xffffffff;
                                    sprintf(v16, "round%i.snd", k);
                                    sub_433790("%i,%i", node, node + 40);
                                    sprintf(v16, "round%i", k);
                                    sub_446440(v31);
                                    node += 4;
                                    k += 1;
                                } while (k <= 9);
                                *(unsigned int *)(v99 + 27004) = 0xffffffff;
                                sub_446440(v31);
                                v134 = sub_433410(v31, "fight.time");
                                if (v134)
                                    *(int *)(v99 + 26996) = atoi(v134);
                                *(int *)(v99 + 27000) = *(int *)(v99 + 26996);
                                v135 = sub_433410(v31, "fight.sndtime");
                                if (v135)
                                    *(int *)(v99 + 27000) = atoi(v135);
                                sub_433790("%i,%i", v99 + 27004, v99 + 27008);
                                v136 = sub_433410(v31, "ctrl.time");
                                if (v136)
                                    *(int *)(v99 + 28032) = atoi(v136);
                                *(unsigned int *)(v99 + 27516) = 0xffffffff;
                                *(unsigned int *)(v99 + 27508) = 0xffffffff;
                                *(unsigned int *)(v99 + 27500) = 0xffffffff;
                                sub_446440(v31);
                                sub_446440(v31);
                                sub_446440(v31);
                                v137 = sub_433410(v31, "KO.time");
                                if (v137)
                                    *(int *)(v99 + 27492) = atoi(v137);
                                *(int *)(v99 + 27496) = *(int *)(v99 + 27492);
                                v138 = sub_433410(v31, "KO.sndtime");
                                if (v138)
                                    *(int *)(v99 + 27496) = atoi(v138);
                                sub_433790("%i,%i", v99 + 27500, v99 + 27504);
                                sub_433790("%i,%i", v99 + 27508, v99 + 27512);
                                sub_433790("%i,%i", v99 + 27516, v99 + 27520);
                                v139 = *(int *)(v99 + 28044);
                                *(unsigned int *)(v99 + 27492) = *(int *)(v99 + 27492) + v139;
                                *(unsigned int *)(v99 + 27496) = *(int *)(v99 + 27496) + v139;
                                v140 = sub_433410(v31, "win.time");
                                if (v140)
                                    *(int *)(v99 + 28028) = atoi(v140);
                                sub_446440(v31);
                                sub_446440(v31);
                                sub_446440(v31);
                                v141 = v99 + 28004;
                                *(unsigned int *)v141 = 0xffffffff;
                                v142 = v99 + 28020;
                                *(unsigned int *)v142 = 0xffffffff;
                                *(unsigned int *)(v99 + 28012) = 0xffffffff;
                                sub_433790("%i,%i", v141, v99 + 28008);
                                sub_433790("%i,%i", v99 + 28012, v99 + 28016);
                                sub_433790("%i,%i", v142, v99 + 28024);
                                v143 = sub_433410(v31, "slow.time");
                                if (v143)
                                    *(int *)(v99 + 28036) = atoi(v143);
                                v144 = sub_433410(v31, "over.waittime");
                                if (v144)
                                    *(int *)(v99 + 28040) = atoi(v144);
                                v145 = sub_433410(v31, "over.hittime");
                                if (v145)
                                    *(int *)(v99 + 28044) = atoi(v145);
                                v146 = sub_433410(v31, "over.wintime");
                                if (v146)
                                    *(int *)(v99 + 28048) = atoi(v146);
                                v147 = sub_433410(v31, "over.time");
                                if (v147)
                                    *(int *)(v99 + 28052) = atoi(v147);
                                sub_444320("[WinIcon]...");
                                v148 = v31 + 32;
                                if ((!v148 || !*(char *)v148 || (v1 = "winicon", v0 = v148, _snprintf(v25, 0x100, "%s.%s"), sub_4330a0() == 0xffffffff)) && sub_4330a0() == 0xffffffff)
                                {
                                    sprintf(g_52b40c + 73820, "Missing winicon info");
                                    return;
                                }
                                v149 = v99 + 28056;
                                v6 = v149;
                                k = v149;
                                v150 = 1;
                                v9 = v149 + 800;
                                node = v149 + 3520;
                                j = 2;
                                do
                                {
                                    v15 = j;
                                    sprintf(v16, "p%i.pos", v150);
                                    sub_433790("%i,%i", node, node + 8);
                                    sprintf(v16, "p%i.iconoffset", v150);
                                    sub_433790("%i,%i", node + 16, node + 24);
                                    sprintf(v16, "p%i.bg0", v150);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.counter", v150);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.n", v150);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.s", v150);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.h", v150);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.throw", v150);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.t", v150);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.c", v150);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.suicide", v150);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.teammate", v150);
                                    sub_446440(v31);
                                    sprintf(v16, "p%i.perfect", v150);
                                    sub_446440(v31);
                                    node += 4;
                                    k += 160;
                                    v150 += 1;
                                    v9 += 1440;
                                    j = v15 - 1;
                                } while (j);
                                v151 = sub_433410(v31, "useiconupto");
                                if (v151)
                                {
                                    idx = v6;
                                    *(int *)(idx + 3552) = atoi(v151);
                                }
                                else
                                {
                                    idx = v6;
                                }
                                if (*(int *)(idx + 3552) > 9)
                                    *(int *)(idx + 3552) = 9;
                                sub_444320("OK\n");
                                sub_432980();
                                sub_444320("    Allocating explods...");
                                v153 = sub_4449e0((int)g_52b40c[71880], (int)g_52b40c[32956]);
                                *(unsigned int *)&g_52b40c[64688] = v153;
                                if (!v153)
                                {
                                    sprintf(g_52b40c + 73820, "Can't allocate explods");
                                    return;
                                }
                                v154 = sub_4449e0((int)g_52b40c[71884], 0);
                                *(unsigned int *)&g_52b40c[64692] = v154;
                                if (!v154)
                                    sprintf(g_52b40c + 73820, "Can't allocate sysexplods");
                                else
                                    sub_444320("OK\n");
                                return;
                            }
                        }
                    }
                }
            }
            v0 = g_52b40c + 73820;
            goto LABEL_451d7a;
        }
    }
    v1 = v29;
    sprintf(g_52b40c + 73820, "Can't open %s");
    return;
}
