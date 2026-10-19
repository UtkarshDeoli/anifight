// hitdef_schema_parse entry=0x47da40
typedef struct st_47da40_1 {
    char padding_0[164];
    float field_a4;
} st_47da40_1;

typedef struct st_47da40_0 {
    char padding_0[1032];
    unsigned int field_408;
    char padding_40c[68408];
    float field_10f44;
    float field_10f48;
    char padding_10f4c[4368];
    char field_1205c;
    char padding_1205d[3];
    char field_12060;
} st_47da40_0;

extern st_47da40_0 *g_52b40c;

unsigned int sub_47da40(int a0, void* a1)
{
    void* v7;  // esi
    void* v8;  // esi
    unsigned int v17;  // ecx
    void* i;  // esi
    void* v19;  // esi
    void* v20;  // esi
    void* v21;  // esi
    void* v22;  // esi
    st_47da40_1 *v23;  // ebx
    int v24;  // esi
    int v25;  // eax
    void* v26;  // esi
    void* v9;  // esi
    char *v27;  // edi
    unsigned int v28;  // eax
    char *v29;  // esi
    unsigned int v30;  // eax
    int v31;  // edi
    int v32;  // eax
    int v33;  // eax
    int v34;  // eax
    void* v35;  // ecx
    void* v36;  // esi
    int v10;  // esi
    unsigned int v37;  // eax
    int v38;  // eax
    int v39;  // eax
    void* v40;  // esi
    unsigned int v41;  // eax
    void* v42;  // esi
    int v43;  // eax
    void* v44;  // esi
    unsigned int v45;  // eax
    void* v46;  // esi
    int v11;  // eax
    int v47;  // eax
    unsigned int v48;  // eax
    int v49;  // esi
    int v50;  // edi
    int v51;  // eax
    void* v52;  // esi
    void* v53;  // ecx
    int v55;  // eax
    void* v56;  // ecx
    unsigned int v12;  // eax
    int v58;  // eax
    void* v59;  // ecx
    int v61;  // eax
    int v62;  // eax
    unsigned int v64;  // ecx
    int v65;  // eax
    void* v66;  // esi
    char *v13;  // eax
    unsigned int v67;  // eax
    int v68;  // eax
    void* v69;  // esi
    int v70;  // eax
    int v71;  // eax
    void* v72;  // ecx
    int v73;  // eax
    int v74;  // eax
    void* v75;  // esi
    int v76;  // eax
    unsigned int v14;  // eax
    int v77;  // eax
    int v78;  // eax
    int v79;  // eax
    int v80;  // eax
    void* v81;  // esi
    unsigned int v82;  // eax
    int v83;  // eax
    void* v84;  // esi
    int v85;  // eax
    unsigned int v86;  // eax
    unsigned int v15;  // eax
    unsigned int v87;  // eax
    unsigned int iter;  // edi
    void* v0;  // [bp-0x24]
    st_47da40_1 *v1;  // [bp-0x20]
    unsigned int v2;  // [bp-0x1c]
    void* v3;  // [bp-0x18], Other Possible Types: unsigned int
    int v4;  // [bp-0x4]
    float v5;  // [bp-0x4]
    void* node;  // [bp+0x8]

    v7 = a1 + 224;
    if ((int)a1[232] != 0xffffffff && (int)a1[232])
        sub_438c40(v7);
    *(unsigned int *)&v7[8] = 0xffffffff;
    *(unsigned int *)v7 = 0xffffffff;
    *(char *)&v7[4] = 0;
    v8 = a1 + 252;
    if ((int)a1[260] != 0xffffffff && (int)a1[260])
        sub_438c40(v8);
    *(unsigned int *)&v8[8] = 0xffffffff;
    *(unsigned int *)v8 = 0xffffffff;
    *(char *)&v8[4] = 0;
    v9 = a1 + 932;
    *(unsigned int *)&a1[276] = 1;
    *(unsigned int *)&a1[248] = 1;
    if ((int)v9[8] != 0xffffffff && (int)v9[8])
        sub_438c40(v9);
    *(unsigned int *)&v9[8] = 0xffffffff;
    *(unsigned int *)v9 = 0xffffffff;
    *(char *)&v9[4] = 0;
    if ((int)a1[28] != 0xffffffff && (int)a1[28])
        sub_438c40(a1 + 20);
    *(unsigned int *)&a1[28] = 0xffffffff;
    *(unsigned int *)&a1[20] = 0xffffffff;
    *(char *)&a1[24] = 0;
    if ((int)a1[40] != 0xffffffff && (int)a1[40])
        sub_438c40(a1 + 32);
    *(unsigned int *)&a1[40] = 0xffffffff;
    *(unsigned int *)&a1[32] = 0xffffffff;
    *(char *)&a1[36] = 0;
    if ((int)a1[52] != 0xffffffff && (int)a1[52])
        sub_438c40(a1 + 44);
    v10 = a0;
    *(unsigned int *)&a1[52] = 0xffffffff;
    *(unsigned int *)&a1[44] = 0xffffffff;
    *(char *)&a1[48] = 0;
    *(unsigned int *)a1 = 2;
    v11 = sub_433410(v10, "attr");
    if (v11)
    {
        v12 = sub_489dc0(v11, 0);
        *(unsigned int *)&a1[4] = v12;
        if (v12 == 0xffffffff)
            return 0;
    }
    v13 = sub_433410(v10, "affectteam");
    if (v13)
    {
        v14 = *v13;
        if (v14 - 1 <= 126)
            v14 = tolower(v14);
        v15 = v14 - 98;
        if (v14 == 98)
        {
            *(unsigned int *)a1 = 3;
        }
        else if (v15 != 3)
        {
            if (v15 != 4)
            {
                iter = &g_52b40c->field_1205c;
                v17 = 6;
                for (i = "Invalid affectteam value"; v17; i += 4)
                {
                    v17 -= 1;
                    *(int *)iter = *(int *)i;
                    iter += 4;
                }
                *(char *)iter = *(char *)i;
                return 0;
            }
            *(unsigned int *)a1 = 1;
        }
        else
        {
            *(unsigned int *)a1 = 2;
        }
    }
    v19 = a1 + 104;
    if ((int)a1[112] != 0xffffffff && (int)a1[112])
        sub_438c40(v19);
    *(unsigned int *)&v19[8] = 0xffffffff;
    *(unsigned int *)v19 = 0xffffffff;
    *(char *)&v19[4] = 0;
    v20 = a1 + 212;
    if ((int)a1[220] != 0xffffffff && (int)a1[220])
        sub_438c40(v20);
    *(unsigned int *)&v20[8] = 0xffffffff;
    *(unsigned int *)v20 = 1;
    *(char *)&v20[4] = 0;
    v21 = a1 + 380;
    if ((int)a1[388] != 0xffffffff && (int)a1[388])
        sub_438c40(v21);
    *(unsigned int *)&v21[8] = 0xffffffff;
    *(unsigned int *)v21 = 0;
    *(char *)&v21[4] = 0;
    v22 = a1 + 392;
    if ((int)a1[400] != 0xffffffff && (int)a1[400])
        sub_438c40(v22);
    *(unsigned int *)&v22[8] = 0xffffffff;
    *(unsigned int *)v22 = 0;
    *(char *)&v22[4] = 0;
    if (!sub_438b20(sub_433410(a0, "id"), a1 + 8, v23, 0, 1))
        return 0;
    v24 = a0;
    if (!sub_438b20(sub_433410(v24, "chainid"), a1 + 20, v23, 0, 1))
        return 0;
    v25 = sub_433410(v24, "nochainid");
    if (v25)
    {
        if (!sub_488ff0(v25, "ii", v23, &a1, a1 + 32, a1 + 44))
        {
            return 0;
        }
        else if (*(char *)a1)
        {
            return 0;
        }
    }
    v26 = a1 + 552;
    if ((int)a1[560] != 0xffffffff && (int)a1[560])
        sub_438c40(v26);
    *(unsigned int *)&v26[8] = 0xffffffff;
    *(unsigned int *)v26 = 0xfffffffe;
    *(char *)&v26[4] = 0;
    v27 = sub_433410(a0, "sparkno");
    if (v27)
    {
        v28 = *v27;
        if (v28 - 1 <= 126)
            v28 = tolower(v28);
        v3 = 1;
        v2 = 0;
        v1 = v23;
        v0 = v26;
        if (v28 == 115)
        {
            if (!sub_438b20(v27 + 1))
                return 0;
            *(unsigned int *)&a1[564] = 1;
        }
        else if (!sub_438b20(v27))
        {
            return 0;
        }
    }
    sub_438d20(-0x2);
    v29 = sub_433410(a0, "guard.sparkno");
    if (v29)
    {
        v30 = *v29;
        if (v30 - 1 <= 126)
            v30 = tolower(v30);
        v3 = 1;
        v2 = 0;
        v1 = v23;
        v0 = a1 + 568;
        if (v30 == 115)
        {
            if (!sub_438b20(v29 + 1))
                return 0;
            *(unsigned int *)&a1[580] = 1;
        }
        else if (!sub_438b20(v29))
        {
            return 0;
        }
    }
    v31 = a0;
    v32 = sub_433410(v31, "sparkxy");
    if (v32)
    {
        if (!sub_488ff0(v32, "ii", v23, &a1, a1 + 584, a1 + 596))
        {
            return 0;
        }
        else if (*(char *)a1)
        {
            return 0;
        }
    }
    v33 = sub_433410(v31, "pausetime");
    if (v33)
    {
        if (!sub_488ff0(v33, "ii", v23, &a1, a1 + 200, a1 + 188))
        {
            return 0;
        }
        else if (*(char *)a1)
        {
            return 0;
        }
    }
    v34 = sub_433410(v31, "guard.pausetime");
    v35 = a1 + 956;
    if (v34)
    {
        v36 = a1 + 944;
        v37 = sub_488ff0(v34, "ii", v23, &a1, v35, v36);
        if (!v37)
        {
            return 0;
        }
        else if (*(char *)a1)
        {
            return 0;
        }
        else if (v37 == 1)
        {
            sub_438e30(v36, a1 + 188);
        }
    }
    else
    {
        sub_438e30(v35, a1 + 200);
        sub_438e30(a1 + 944, a1 + 188);
    }
    if (!sub_488f40(v23, 1))
    {
        return 0;
    }
    else if (!sub_488f40(v23, 1))
    {
        return 0;
    }
    else if (!sub_488f40(v23, 1))
    {
        return 0;
    }
    else if (!sub_488f40(v23, -(-(0 < (unsigned int)((int)a1[4] & 448)))))
    {
        return 0;
    }
    else if (!sub_438b20(sub_433410(v31, "numhits"), a1 + 212, v23, 0, 1))
    {
        return 0;
    }
    else
    {
        v38 = sub_433410(v31, "damage");
        if (v38)
        {
            if (!sub_488ff0(v38, "ii", v23, &a1, a1 + 116, a1 + 128))
            {
                return 0;
            }
            else if (*(char *)a1)
            {
                return 0;
            }
        }
        v39 = sub_433410(v31, "getpower");
        if (v39)
        {
            v40 = a1 + 152;
            v41 = sub_488ff0(v39, "ii", v23, &a1, a1 + 140, v40);
            if (!v41)
                return 0;
            if (*(char *)a1)
                return 0;
            if (v41 != 1)
                goto LABEL_47e0b8;
            sub_438e30(v40, a1 + 140);
        }
        else
        {
            v42 = a1 + 140;
            sub_438e30(v42, a1 + 116);
            sub_438ed0(g_52b40c->field_10f44);
            sub_438e30(a1 + 152, v42);
        }
        sub_438f70();
LABEL_47e0b8:
        v43 = sub_433410(v31, "givepower");
        if (v43)
        {
            v44 = a1 + 176;
            v45 = sub_488ff0(v43, "ii", v23, &a1, a1 + 164, v44);
            if (!v45)
                return 0;
            if (*(char *)a1)
                return 0;
            if (v45 != 1)
                goto LABEL_47e159;
            sub_438e30(v44, a1 + 164);
        }
        else
        {
            v46 = a1 + 164;
            sub_438e30(v46, a1 + 116);
            sub_438ed0(g_52b40c->field_10f48);
            sub_438e30(a1 + 176, v46);
        }
        sub_438f70();
LABEL_47e159:
        sub_438d20(4);
        *(unsigned int *)&a1[300] = 0;
        v47 = sub_433410(v31, "priority");
        node = a1;
        if (v47)
        {
            if (!sub_488ff0(v47, "i:,", v23, &node, a1 + 288))
                return 0;
            node = a1;
            if (*(char *)node)
            {
                node = a1 + 1;
                if (sub_40e340())
                {
                    do
                    {
                        node += 1;
                    } while (sub_40e340());
                }
                v48 = sub_40e380();
                if (v48 == 100)
                {
                    *(unsigned int *)&a1[300] = 2;
                }
                else if (v48 != 104)
                {
                    if (v48 != 109)
                        return 0;
                    *(unsigned int *)&a1[300] = 1;
                }
                else
                {
                    *(unsigned int *)&a1[300] = 0;
                }
            }
        }
        v49 = v31;
        sub_433410(v49, "hitsound");
        if (!sub_489bc0(a1 + 224, a1 + 236, v23))
            return 0;
        sub_433410(v49, "guardsound");
        if (!sub_489bc0(a1 + 252, a1 + 264, v23))
        {
            return 0;
        }
        else if (!sub_438b20(sub_433410(v49, "ground.slidetime"), a1 + 708, v23, 0, 1))
        {
            return 0;
        }
        else if (!sub_438b20(sub_433410(v49, "ground.hittime"), a1 + 696, v23, 0, 1))
        {
            return 0;
        }
        else
        {
            sub_433410(v49, "ground.velocity");
            if (!sub_489c40(a1 + 732, v23))
                return 0;
            sub_433410(v49, "air.velocity");
            if (!sub_489c40(a1 + 800, v23))
            {
                return 0;
            }
            else if (!sub_438b20(sub_433410(v49, "air.juggle"), a1 + 104, v23, 0, 1))
            {
                return 0;
            }
            else
            {
                sub_438d20(20);
                v50 = a0;
                if (!sub_438b20(sub_433410(v50, "air.hittime"), a1 + 764, v23, 0, 1))
                    return 0;
                sub_438d20(-0x401);
                sub_438d20(-0x401);
                sub_438d20(-0x401);
                sub_438d20(-0x401);
                v51 = sub_433410(v50, "snap");
                if (v51)
                {
                    v3 = a1 + 340;
                    v52 = a1 + 632;
                    v53 = a1 + 644;
                    if (!((unsigned int)*(int *)&v23[1].padding_0[16] < g_52b40c->field_408 ? sub_488ff0(v51, "iiii", v23, &node, v52, v53, 0) : sub_488ff0(v51, "iii", v23, &node, v52, v53)))
                        return 0;
                    if (*(char *)node)
                        return 0;
                    sub_438e30(a1 + 656, v52);
                    sub_438e30(a1 + 668, a1 + 644);
                }
                else
                {
                    v55 = sub_433410(v50, "mindist");
                    if (v55)
                    {
                        v56 = a1 + 644;
                        if (!((unsigned int)*(int *)&v23[1].padding_0[16] < g_52b40c->field_408 ? sub_488ff0(v55, "iii", v23, &node, a1 + 632, v56, 0) : sub_488ff0(v55, "ii", v23, &a1, a1 + 632, v56)))
                        {
                            return 0;
                        }
                        else if (*(char *)node)
                        {
                            return 0;
                        }
                    }
                    v58 = sub_433410(v50, "maxdist");
                    if (v58)
                    {
                        v59 = a1 + 668;
                        if (!((unsigned int)*(int *)&v23[1].padding_0[16] < g_52b40c->field_408 ? sub_488ff0(v58, "iii", v23, &node, a1 + 656, v59, 0) : sub_488ff0(v58, "ii", v23, &a1, a1 + 656, v59)))
                        {
                            return 0;
                        }
                        else if (*(char *)node)
                        {
                            return 0;
                        }
                    }
                }
                sub_438d20(1);
                v61 = sub_433410(v50, "p1sprpriority");
                if (!v61)
                    v61 = sub_433410(v50, "sprpriority");
                if (!sub_438b20(v61, a1 + 352, v23, 0, 1))
                {
                    return 0;
                }
                else if (!sub_488f40(v23, 0))
                {
                    return 0;
                }
                else
                {
                    sub_438d20(0);
                    if (!sub_438b20(sub_433410(v50, "p1facing"), a1 + 608, v23, 0, 1))
                    {
                        return 0;
                    }
                    else if (!sub_438b20(sub_433410(v50, "p1getp2facing"), a1 + 620, v23, 0, 1))
                    {
                        return 0;
                    }
                    else if (!sub_438b20(sub_433410(v50, "p2facing"), a1 + 680, v23, 0, 1))
                    {
                        return 0;
                    }
                    else
                    {
                        sub_438d20(-0x1);
                        sub_438d20(-0x1);
                        sub_438d20(0);
                        if (!sub_438b20(sub_433410(v50, "p1stateno"), a1 + 304, v23, 0, 1))
                            return 0;
                        v4 = sub_433410(v50, "p2stateno");
                        if (v4)
                        {
                            sub_438d20(1);
                            if (!sub_438b20(v4, a1 + 316, v23, 0, 1))
                                return 0;
                        }
                        if (!sub_438b20(sub_433410(v50, "p2getp1state"), a1 + 328, v23, 0, 1))
                            return 0;
                        v62 = sub_433410(v50, "forcestand");
                        if (v62)
                        {
                            *(unsigned int *)&a1[404] = 1;
                            if (!sub_438b20(v62, a1 + 380, v23, 0, 1))
                                return 0;
                        }
                        if (!sub_438b20(sub_433410(v50, "forcenofall"), a1 + 392, v23, 0, 1))
                            return 0;
                        sub_438d20(0);
                        sub_438d50(-3.4028234663852886e+38f);
                        sub_456e40(-4.5f);
                        v5 = (float)(-4.5 / v23->field_a4);
                        sub_438d50(v5);
                        sub_438d20(1);
                        sub_438d20(4);
                        if (!sub_438b20(sub_433410(v50, "fall.damage"), a1 + 412, v23, 0, 1))
                        {
                            return 0;
                        }
                        else if (!sub_438b20(sub_433410(v50, "fall.xvelocity"), a1 + 428, v23, 0, 2))
                        {
                            return 0;
                        }
                        else if (!sub_438b20(sub_433410(v50, "fall.yvelocity"), a1 + 440, v23, 0, 2))
                        {
                            return 0;
                        }
                        else if (!sub_438b20(sub_433410(v50, "fall.recover"), a1 + 452, v23, 0, 1))
                        {
                            return 0;
                        }
                        else if (!sub_438b20(sub_433410(v50, "fall.recovertime"), a1 + 464, v23, 0, 1))
                        {
                            return 0;
                        }
                        else
                        {
                            v3 = v64;
                            sub_438d50(-3.4028234663852886e+38f);
                            if (!sub_438b20(sub_433410(v50, "yaccel"), a1 + 0x404, v23, 0, 2))
                                return 0;
                            sub_438d20(0);
                            sub_438d20(20);
                            v65 = sub_433410(v50, "down.velocity");
                            if (v65)
                            {
                                v66 = a1 + 860;
                                v67 = sub_488ff0(v65, "ff", v23, &node, a1 + 848, v66);
                                if (!v67)
                                {
                                    return 0;
                                }
                                else if (*(char *)node)
                                {
                                    return 0;
                                }
                                else if (v67 == 1)
                                {
                                    sub_438e30(v66, a1 + 800);
                                }
                            }
                            else
                            {
                                sub_438e30(a1 + 848, a1 + 788);
                                sub_438e30(a1 + 860, a1 + 800);
                            }
                            if (!sub_438b20(sub_433410(v50, "down.bounce"), a1 + 824, v23, 0, 1))
                            {
                                return 0;
                            }
                            else if (!sub_438b20(sub_433410(v50, "down.hittime"), a1 + 836, v23, 0, 1))
                            {
                                return 0;
                            }
                            else
                            {
                                v68 = sub_433410(v50, "guard.slidetime");
                                v69 = a1 + 908;
                                if (!v68)
                                {
                                    sub_438e30(v69, a1 + 708);
                                }
                                else if (!sub_438b20(v68, v69, v23, 0, 1))
                                {
                                    return 0;
                                }
                                v70 = sub_433410(v50, "guard.hittime");
                                if (!v70)
                                {
                                    sub_438e30(a1 + 896, a1 + 696);
                                }
                                else if (!sub_438b20(v70, a1 + 896, v23, 0, 1))
                                {
                                    return 0;
                                }
                                v71 = sub_433410(v50, "guard.velocity");
                                v72 = a1 + 884;
                                if (!v71)
                                {
                                    sub_438e30(v72, a1 + 720);
                                }
                                else if (!sub_438b20(v71, v72, v23, 0, 2))
                                {
                                    return 0;
                                }
                                v73 = sub_433410(v50, "guard.ctrltime");
                                if (!v73)
                                {
                                    sub_438e30(a1 + 920, v69);
                                }
                                else if (!sub_438b20(v73, a1 + 920, v23, 0, 1))
                                {
                                    return 0;
                                }
                                if (!sub_438b20(sub_433410(v50, "guard.dist"), a1 + 932, v23, 0, 1))
                                    return 0;
                                v74 = sub_433410(v50, "ground.cornerpush.veloff");
                                v75 = a1 + 744;
                                if (v74)
                                {
                                    if (!sub_438b20(v74, v75, v23, 0, 2))
                                        return 0;
                                }
                                else
                                {
                                    if ((char)a1[4] & 4)
                                    {
                                        v3 = v64;
                                        sub_438d50(0.0f);
                                    }
                                    else
                                    {
                                        sub_438e30(v75, a1 + 884);
                                        sub_438ed0(1.2999999523162842f);
                                    }
                                }
                                v76 = sub_433410(v50, "air.cornerpush.veloff");
                                if (!v76)
                                {
                                    sub_438e30(a1 + 812, v75);
                                }
                                else if (!sub_438b20(v76, a1 + 812, v23, 0, 2))
                                {
                                    return 0;
                                }
                                v77 = sub_433410(v50, "down.cornerpush.veloff");
                                if (!v77)
                                {
                                    sub_438e30(a1 + 872, v75);
                                }
                                else if (!sub_438b20(v77, a1 + 872, v23, 0, 2))
                                {
                                    return 0;
                                }
                                v78 = sub_433410(v50, "guard.cornerpush.veloff");
                                if (!v78)
                                {
                                    sub_438e30(a1 + 968, v75);
                                }
                                else if (!sub_438b20(v78, a1 + 968, v23, 0, 2))
                                {
                                    return 0;
                                }
                                v79 = sub_433410(v50, "airguard.cornerpush.veloff");
                                if (!v79)
                                {
                                    sub_438e30(a1 + 1016, a1 + 968);
                                }
                                else if (!sub_438b20(v79, a1 + 1016, v23, 0, 2))
                                {
                                    return 0;
                                }
                                v80 = sub_433410(v50, "airguard.velocity");
                                if (v80)
                                {
                                    v81 = a1 + 992;
                                    v82 = sub_488ff0(v80, "ff", v23, &node, a1 + 980, v81);
                                    if (!v82)
                                        return 0;
                                    if (*(char *)node)
                                        return 0;
                                    if (v82 != 1)
                                        goto LABEL_47ed15;
                                }
                                else
                                {
                                    sub_438e30(a1 + 980, a1 + 788);
                                    sub_438ed0(1.5f);
                                    v81 = a1 + 992;
                                }
                                sub_438e30(v81, a1 + 800);
                                sub_438ed0(0.5f);
LABEL_47ed15:
                                v83 = sub_433410(v50, "airguard.ctrltime");
                                if (!v83)
                                {
                                    sub_438e30(a1 + 1004, a1 + 920);
                                }
                                else if (!sub_438b20(v83, a1 + 1004, v23, 0, 1))
                                {
                                    return 0;
                                }
                                v84 = a1 + 776;
                                sub_438d20(0);
                                v85 = sub_433410(v50, "fall");
                                if (v85)
                                {
                                    if (!sub_438b20(v85, a1 + 540, v23, 0, 1))
                                        return 0;
                                    sub_438e30(v84, a1 + 540);
                                }
                                v4 = sub_433410(v50, "air.fall");
                                if (v4)
                                {
                                    sub_438c40(v84);
                                    if (!sub_438b20(v4, v84, v23, 0, 1))
                                        return 0;
                                }
                                if (sub_433410(v50, "guardflag"))
                                {
                                    v86 = sub_48a2e0();
                                    *(unsigned int *)&a1[280] = (int)a1[280] | v86;
                                }
                                if (sub_433410(v50, "hitflag"))
                                {
                                    v87 = sub_48a2e0();
                                    *(unsigned int *)&a1[284] = (int)a1[284] | v87;
                                }
                                else
                                {
                                    *(unsigned int *)&a1[284] = (int)a1[284] | 62;
                                }
                                *(unsigned int *)&a1[376] = 0;
                                if (sub_433410(v50, "animtype"))
                                {
                                    sub_40e380();
                                    switch (sub_40e380())
                                    {
                                    case 98:
                                        *(unsigned int *)&a1[376] = 3;
                                        break;
                                    case 100:
                                        *(unsigned int *)&a1[376] = 5;
                                        break;
                                    case 104:
                                        *(unsigned int *)&a1[376] = 2;
                                        break;
                                    case 108:
                                        *(unsigned int *)&a1[376] = 0;
                                        break;
                                    case 109:
                                        *(unsigned int *)&a1[376] = 1;
                                        break;
                                    case 117:
                                        *(unsigned int *)&a1[376] = 4;
                                        break;
                                    default:
                                        *(unsigned int *)&g_52b40c->field_1205c = 1635151433;
                                        *(unsigned int *)&g_52b40c->field_12060 = 543451500;
                                        *(unsigned int *)&g_52b40c[1].padding_0[3] = 1835626081;
                                        *(unsigned int *)&g_52b40c[1].padding_0[7] = 1701869940;
                                        g_52b40c[1].padding_0[11] = 0;
                                        return 0;
                                    }
                                }
                                *(int *)&a1[760] = (int)a1[376];
                                if (sub_433410(v50, "air.animtype"))
                                {
                                    sub_40e380();
                                    switch (sub_40e380())
                                    {
                                    case 98:
                                        *(unsigned int *)&a1[760] = 3;
                                        break;
                                    case 100:
                                        *(unsigned int *)&a1[760] = 5;
                                        break;
                                    case 104:
                                        *(unsigned int *)&a1[760] = 2;
                                        break;
                                    case 108:
                                        *(unsigned int *)&a1[760] = 0;
                                        break;
                                    case 109:
                                        *(unsigned int *)&a1[760] = 1;
                                        break;
                                    case 117:
                                        *(unsigned int *)&a1[760] = 4;
                                        break;
                                    default:
                                        *(unsigned int *)&g_52b40c->field_1205c = 1635151433;
                                        *(unsigned int *)&g_52b40c->field_12060 = 543451500;
                                        *(unsigned int *)&g_52b40c[1].padding_0[3] = 779250017;
                                        *(unsigned int *)&g_52b40c[1].padding_0[7] = 1835626081;
                                        *(unsigned int *)&g_52b40c[1].padding_0[11] = 1701869940;
                                        g_52b40c[1].padding_0[15] = 0;
                                        return 0;
                                    }
                                }
                                if ((int)a1[760] == 4)
                                {
                                    *(unsigned int *)&a1[424] = 4;
                                }
                                else if ((int)a1[760] == 5)
                                {
                                    *(unsigned int *)&a1[424] = 5;
                                }
                                else
                                {
                                    *(unsigned int *)&a1[424] = 3;
                                }
                                if (sub_433410(v50, "fall.animtype"))
                                {
                                    sub_40e380();
                                    switch (sub_40e380())
                                    {
                                    case 98:
                                        *(unsigned int *)&a1[424] = 3;
                                        break;
                                    case 100:
                                        *(unsigned int *)&a1[424] = 5;
                                        break;
                                    case 104:
                                        *(unsigned int *)&a1[424] = 2;
                                        break;
                                    case 108:
                                        *(unsigned int *)&a1[424] = 0;
                                        break;
                                    case 109:
                                        *(unsigned int *)&a1[424] = 1;
                                        break;
                                    case 117:
                                        *(unsigned int *)&a1[424] = 4;
                                        break;
                                    default:
                                        *(unsigned int *)&g_52b40c->field_1205c = 1635151433;
                                        *(unsigned int *)&g_52b40c->field_12060 = 543451500;
                                        *(unsigned int *)&g_52b40c[1].padding_0[3] = 1819042150;
                                        *(unsigned int *)&g_52b40c[1].padding_0[7] = 1768841518;
                                        *(unsigned int *)&g_52b40c[1].padding_0[11] = 1887007853;
                                        *(unsigned short *)&g_52b40c[1].padding_0[15] = 101;
                                        return 0;
                                    }
                                }
                                *(unsigned int *)&a1[692] = 1;
                                if (sub_433410(v50, "ground.type"))
                                {
                                    sub_40e380();
                                    switch (sub_40e380())
                                    {
                                    case 104:
                                        *(unsigned int *)&a1[692] = 1;
                                        break;
                                    case 108:
                                        *(unsigned int *)&a1[692] = 2;
                                        break;
                                    case 110:
                                        *(unsigned int *)&a1[692] = 0;
                                        break;
                                    case 116:
                                        *(unsigned int *)&a1[692] = 3;
                                        break;
                                    default:
                                        *(unsigned int *)&g_52b40c->field_1205c = 1635151433;
                                        *(unsigned int *)&g_52b40c->field_12060 = 543451500;
                                        *(unsigned int *)&g_52b40c[1].padding_0[3] = 1970238055;
                                        *(unsigned int *)&g_52b40c[1].padding_0[7] = 1949197422;
                                        *(unsigned int *)&g_52b40c[1].padding_0[11] = 6647929;
                                        return 0;
                                    }
                                }
                                *(int *)&a1[756] = (int)a1[692];
                                if (sub_433410(v50, "air.type"))
                                {
                                    sub_40e380();
                                    switch (sub_40e380())
                                    {
                                    case 104:
                                        *(unsigned int *)&a1[756] = 1;
                                        break;
                                    case 108:
                                        *(unsigned int *)&a1[756] = 2;
                                        break;
                                    case 110:
                                        *(unsigned int *)&a1[756] = 0;
                                        break;
                                    case 116:
                                        *(unsigned int *)&a1[756] = 3;
                                        break;
                                    default:
                                        *(unsigned int *)&g_52b40c->field_1205c = 1635151433;
                                        *(unsigned int *)&g_52b40c->field_12060 = 543451500;
                                        *(unsigned int *)&g_52b40c[1].padding_0[3] = 779250017;
                                        *(unsigned int *)&g_52b40c[1].padding_0[7] = 1701869940;
                                        g_52b40c[1].padding_0[11] = 0;
                                        return 0;
                                    }
                                }
                                if (!sub_47cdb0(v50, v23, 1))
                                    return 0;
                                *(unsigned int *)&a1[1040] = 0;
                                if (sub_47c1e0(a0, v23))
                                    return -(-(0 < sub_47c1e0(a0, v23)));
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }
}
