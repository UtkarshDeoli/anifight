// pass9 drawwrap entry=0x4654d0 (asked 0x4654d0)
typedef struct st_4654d0_0 {
    char padding_0[432];
    unsigned int field_1b0;
    char padding_1b4[180];
    struct st_4654d0_1 *field_268;
    char padding_26c[3056];
    unsigned int field_e5c;
} st_4654d0_0;

typedef struct st_4654d0_1 {
    struct st_4654d0_2 *field_0;
} st_4654d0_1;

typedef struct st_4654d0_5 {
    char padding_0[5256];
    unsigned int field_1488;
} st_4654d0_5;

typedef struct st_4654d0_4 {
    char padding_0[70192];
    struct st_4654d0_0 *field_11230;
} st_4654d0_4;

typedef struct st_4654d0_2 {
    char padding_0[8];
    int field_8;
} st_4654d0_2;

typedef struct st_4654d0_3 {
    char padding_0[59756];
    unsigned int field_e96c;
    char padding_e970[9528];
    int field_10ea8;
    char padding_10eac[64];
    unsigned int field_10eec;
    unsigned int field_10ef0;
    unsigned int field_10ef4;
    char padding_10ef8[72];
    unsigned int field_10f40;
    char padding_10f44[1260];
    int field_11430;
    char padding_11434[728];
    unsigned int field_1170c;
    char padding_11710[12];
    unsigned int field_1171c;
    char padding_11720[8];
    unsigned int field_11728;
    char padding_1172c[300];
    unsigned int field_11858;
} st_4654d0_3;

extern st_4654d0_3 *g_52b40c;

unsigned int sub_4654d0(float idx)
{
    unsigned int idx1;  // eax
    float index;  // ebx
    int v15;  // edx
    int v16;  // ecx
    char v17;  // al
    st_4654d0_5 *v18;  // eax
    unsigned int v19;  // edx
    float v20;  // ebx
    unsigned int i;  // esi
    unsigned int v22;  // ecx
    float iter;  // edi
    float v24;  // esi
    unsigned int v7;  // ecx
    unsigned int v25;  // ebx
    unsigned int v26;  // ecx
    unsigned int v27;  // eax
    int v28;  // ebx
    st_4654d0_4 *node;  // edi
    st_4654d0_0 *v30;  // eax
    int v31;  // esi
    int v32;  // esi
    unsigned int v33;  // eax
    unsigned int v34;  // eax
    unsigned int v8;  // esi
    unsigned int v35;  // eax
    unsigned int v9;  // ecx
    int v10;  // edx
    int v11;  // 4132
    unsigned int v12;  // eax
    unsigned int v13;  // eax
    unsigned int v14;  // eax
    unsigned int v0;  // [bp-0x10]
    unsigned int v1;  // [bp-0xc]
    unsigned int v2;  // [bp-0x8]
    unsigned int v3;  // [bp-0x4]
    int v4;  // [bp+0x4]

    idx1 = *(int *)(*(unsigned int *)&idx + 832);
    index = __int_as_float(*(unsigned int *)&idx + 660);
    v1 = 0;
    if (idx1 == *(int *)(__float_as_int(index) + 176))
    {
        v7 = 0;
    }
    else
    {
        v7 = *(int *)(__float_as_int(index) + idx1 * 4);
        idx1 += 1;
        *(unsigned int *)(__float_as_int(index) + 172) = idx1;
        if ((int)idx1 >= 30)
            *(unsigned int *)(__float_as_int(index) + 172) = 0;
    }
    v0 = v7;
    if (!*(int *)(*(unsigned int *)&idx + 5256))
    {
        v8 = *(int *)&g_52b40c->padding_0[18980];
        if (v8 != 6)
        {
            if (v8 != 7 && v8 != 8)
            {
                idx1 = *(int *)&g_52b40c->padding_11710[0];
                if (idx1 == 1 || !idx1 && (int)g_52b40c->field_1171c > 2)
                {
                    idx1 = v7 - 5;
                    if (idx1 <= 5)
                    {
                        idx1 = g_52b40c->field_e96c + 2;
                        if (*(int *)&g_52b40c->padding_11434[720] <= (int)idx1 && !g_52b40c->field_11728)
                        {
                            v9 = g_52b40c->field_11858;
                            g_52b40c->field_11728 = 1;
                            g_52b40c->field_10eec = 1;
                            g_52b40c->field_10ef0 = 0;
                            v10 = (int)((unsigned int)(2290649225 * (v9 * 10) >> 32) + v9 * 10) >> 7;
                            idx1 = ((unsigned int)(v10) >> 31) + v10;
                            v11 = *(int *)&g_52b40c->padding_11434[688];
                            g_52b40c->field_10ef4 = idx1;
                            if (v11 > 1)
                                g_52b40c->field_10ef4 = idx1 * 2;
                            v7 = v0;
                        }
                    }
                }
            }
            if (v8 != 6 && v8 != 7)
                goto LABEL_4655fe;
        }
        if (*(int *)&g_52b40c->padding_1172c[164] == 1 && v7 - 5 <= 5)
        {
            *(unsigned int *)&g_52b40c->padding_1172c[164] = 2;
            *(unsigned int *)&g_52b40c->padding_1172c[168] = 0;
        }
LABEL_4655fe:
        if (v8 != 6 && v8 != 7 && v8 != 8 && *(int *)&g_52b40c->padding_11710[0] == 4)
        {
            idx1 = g_52b40c->field_1171c;
            if ((int)idx1 > *(int *)&g_52b40c->padding_e970[1296] && (int)idx1 < (int)(g_52b40c->field_1170c - 0x11) && v7 - 5 <= 5)
            {
                *(unsigned int *)&g_52b40c->padding_11710[0] = 4;
                sub_413d80();
                idx1 = g_52b40c->field_1170c - 0x11;
                g_52b40c->field_1171c = idx1;
                *(unsigned int *)&g_52b40c->padding_1172c[0] = 1;
            }
        }
    }
    if (*(int *)(*(unsigned int *)&idx + 436))
        return v13;
    if (*(int *)(*(unsigned int *)&idx + 5248) && *(int *)(*(unsigned int *)&idx + 648))
        return v12;
    if (*(int *)(*(unsigned int *)&idx + 648))
    {
        v0 = 0;
        idx1 = sub_411570();
        *(unsigned int *)(*(unsigned int *)&idx + 652) = 1;
    }
    else if (*(int *)(*(unsigned int *)&idx + 652))
    {
        idx1 = sub_4115c0(*(int *)(*(unsigned int *)&idx + 2476));
        *(unsigned int *)(*(unsigned int *)&idx + 652) = 0;
    }
    v13 = idx1;
    if (!*(int *)&g_52b40c->padding_11434[532])
        return v13;
    v4 = -0x1;
    if (*(int *)(*(unsigned int *)&idx + 3696) > 0)
        *(unsigned int *)(*(unsigned int *)&idx + 2460) = 1;
    if (!*(int *)(*(unsigned int *)&idx + 648))
        *(unsigned int *)(*(unsigned int *)&idx + 2464) = sub_40b770();
    else
        *(unsigned int *)(*(unsigned int *)&idx + 2464) = 0;
    v14 = *(int *)&g_52b40c->padding_11434[612];
    v15 = 0;
    if (v14 != 0xffffffff && !*(int *)(*(unsigned int *)&idx + v14 * 4 + 552) && *(int *)&g_52b40c->padding_11434[640 + 36 * v14] < *(int *)&g_52b40c->padding_11434[648 + 36 * v14])
        v15 = 2;
    if (!*(int *)(*(unsigned int *)&idx + 2460))
    {
        v16 = 0;
        do
        {
            v17 = *(char *)(v16 + *(unsigned int *)&idx + 2484);
            if ((char)*(char *)(v16 + *(unsigned int *)&idx + 2484) > 0)
            {
                *(char *)(v16 + *(unsigned int *)&idx + 2484) = v17 - 1;
                if (v17 - 1 < v15)
                    *(char *)(v16 + *(unsigned int *)&idx + 2484) = v15;
            }
        } while ((v16 += 1, v16 < 128));
    }
    v18 = *(int *)(*(unsigned int *)&idx + 5248);
    if (v18 && v18->field_1488)
    {
        v19 = *(int *)(*(unsigned int *)&idx + 2460);
        v20 = __int_as_float(*(unsigned int *)&idx + 848);
        i = &v18->padding_0[848];
        v22 = 407;
        for (iter = v20; v22; i += 4)
        {
            v22 -= 1;
            *(int *)iter = *(int *)i;
            iter = __int_as_float(__float_as_int(iter) + 4);
        }
        *(unsigned int *)(*(unsigned int *)&idx + 2460) = v19;
    }
    else
    {
        v20 = __int_as_float(*(unsigned int *)&idx + 848);
        sub_40b480(*(unsigned int *)&idx + 660, g_52b40c->field_10ea8);
    }
    v24 = __int_as_float(*(unsigned int *)&idx + 2484);
    v2 = *(int *)(*(unsigned int *)&idx + 2392);
    sub_40b650(v24, v20, g_52b40c->field_10ea8, 1);
    *(unsigned int *)(*(unsigned int *)&idx + 2392) = v2;
    sub_40b650(v24, v20, g_52b40c->field_10ea8, 2);
    sub_4601b0();
    if (!*(int *)(*(unsigned int *)&idx + 3696) && *(int *)&g_52b40c->padding_11434[272 + 4 * *(int *)(*(unsigned int *)&idx + 8)] && *(int *)(*(unsigned int *)&idx + 2460) == 1)
        *(unsigned int *)(*(unsigned int *)&idx + 2460) = 0;
    v25 = *(int *)(*(unsigned int *)&idx + 3672);
    if (v25 == 1)
    {
        v26 = *(int *)(*(unsigned int *)&idx + 788);
        if (v26 && !*(int *)(*(unsigned int *)&idx + 796))
            v4 = 1;
        if (*(int *)(*(unsigned int *)&idx + 796) && !v26)
            v4 = 1;
        if (*(int *)(*(unsigned int *)&idx + 792))
            v4 = 11;
        if (*(int *)(*(unsigned int *)&idx + 784))
        {
            v4 = 7;
            if (v26)
                v4 = 7;
            if (*(int *)(*(unsigned int *)&idx + 784) && *(int *)(*(unsigned int *)&idx + 796))
                v4 = 7;
        }
    }
    if (v25 == 2 && *(int *)(*(unsigned int *)&idx + 792))
        v1 = 1;
    if (v25 == 3 && v0 == 1)
        v4 = 10;
    *(unsigned int *)(*(unsigned int *)&idx + 3688) = 0;
    if (!g_52b40c->field_10f40 && *(int *)(*(unsigned int *)&idx + 796) && (!*(int *)(*(unsigned int *)&idx + 3676) && (*(int *)(*(unsigned int *)&idx + 3684) || *(int *)(*(unsigned int *)&idx + 3148) == 52) || (v27 = *(int *)(*(unsigned int *)&idx + 3148), *(int *)(*(unsigned int *)&idx + 3148) == 151 || *(int *)(*(unsigned int *)&idx + 3148) == 0x99 || *(int *)(*(unsigned int *)&idx + 3148) == 155)) && (v25 != 1 || !*(char *)(*(unsigned int *)&idx + 592)) && (v25 != 2 || !*(char *)(*(unsigned int *)&idx + 593)) && (v25 != 3 || !*(char *)(*(unsigned int *)&idx + 594)) && !*(int *)(*(unsigned int *)&idx + 784))
    {
        v28 = g_52b40c->field_11430;
        if (v28 <= *(int *)&g_52b40c->padding_11434[0])
        {
            node = &g_52b40c->padding_0[4 * v28 + 70192];
            do
            {
                v30 = *(int *)&node->padding_0[0];
                if (*(unsigned int *)&idx != v30 && v30 && v30->field_1b0 && *(int *)&v30->padding_0[12] != *(int *)(*(unsigned int *)&idx + 12) && *(int *)&v30->padding_0[12] && (v30->field_e5c == 1 || *(int *)(*(int *)v30->field_268 + 8) > 0))
                {
                    *(unsigned int *)(*(unsigned int *)&idx + 3688) = 1;
                    if (sub_464f10(idx))
                        v4 = 40;
                }
            } while ((v28 += 1, node = &node->padding_0[4], v28 <= *(int *)&g_52b40c->padding_11434[0]));
        }
    }
    if (*(int *)(*(unsigned int *)&idx + 788) && *(int *)(*(unsigned int *)&idx + 796))
        v31 = -0x1;
    else
        v31 = v4;
    if (*(int *)(*(unsigned int *)&idx + 656))
    {
        v31 = -0x1;
        memset(*(unsigned int *)&idx + 2484, 0, 128);
        v1 = 0;
LABEL_465a78:
        v32 = v31 - 1;
        if (v31 != 1)
        {
            if (v32 != 6)
            {
                if (v32 != 9)
                    goto LABEL_465ba1;
                if (*(int *)(*(unsigned int *)&idx + 3684) && *(int *)(*(unsigned int *)&idx + 540) > 0)
                {
                    v3 = *(int *)(*(unsigned int *)&idx + 504);
                    v2 = *(int *)(*(unsigned int *)&idx + 500);
                    if ((double)*(int *)(*(unsigned int *)&idx + 272) < -__double_as_longlong((double)*(float *)&v3))
                    {
                        sub_46af10();
                        *(unsigned int *)(*(unsigned int *)&idx + 540) = *(int *)(*(unsigned int *)&idx + 540) - 1;
                    }
                }
            }
            else if (*(int *)(*(unsigned int *)&idx + 3684) && *(int *)&g_52b40c->padding_11710[0] != 3)
            {
                sub_46af10();
            }
        }
        else
        {
            if (*(int *)(*(unsigned int *)&idx + 3684) && !*(char *)(*(unsigned int *)&idx + 598) && *(int *)(*(unsigned int *)&idx + 3148) != 20)
                sub_46af10();
        }
    }
    else
    {
        if (v31 > 11)
        {
            if (v31 != 40)
            {
                if (v31 != 900)
                {
LABEL_465ba1:
                    if (*(int *)(*(unsigned int *)&idx + 3148) == 20)
                    {
                        *(unsigned int *)(*(unsigned int *)&idx + 3152) = 20;
                        *(unsigned int *)(*(unsigned int *)&idx + 3148) = 0;
                        *(unsigned int *)(*(unsigned int *)&idx + 3668) = 0;
                    }
                    if (*(int *)(*(unsigned int *)&idx + 3672) == 2 && *(int *)(*(unsigned int *)&idx + 3684) && !v1)
                    {
                        sub_46af10();
                        goto LABEL_465c86;
                    }
                }
                else
                {
                    sub_46b410();
                    if (*(int *)(*(int *)(*(int *)*(int *)*(int *)(*(unsigned int *)&idx + 5144) + 28) + *(int *)(*(int *)(*(unsigned int *)&idx + 5144) + 12) * 16 + 12) != 5)
                    {
                        sub_46b1b0(5);
                        goto LABEL_465c86;
                    }
                }
            }
            else
            {
                if (!*(int *)(*(unsigned int *)&idx + 3676) && *(int *)(*(unsigned int *)&idx + 3684))
                {
                    v34 = *(int *)(*(unsigned int *)&idx + 3148);
                    if (v34 != 130 && v34 != 131 && v34 != 132 && v34 != 120 && v34 != 140)
                    {
                        if (*(int *)(*(unsigned int *)&idx + 3672) != 3 && v34 != 100)
                            *(float *)(*(unsigned int *)&idx + 516) = 0.0f;
                        sub_46af10();
                    }
                    *(unsigned int *)(*(unsigned int *)&idx + 3688) = 1;
                    goto LABEL_465c86;
                }
            }
        }
        if (v31 != 11)
            goto LABEL_465a78;
        if (*(int *)(*(unsigned int *)&idx + 3684))
        {
            v33 = *(int *)(*(unsigned int *)&idx + 3148);
            if (v33 != 100)
                *(float *)(*(unsigned int *)&idx + 516) = 0.0f;
            if (v33 != 10 && v33 != 11)
                sub_46af10();
        }
    }
LABEL_465c86:
    if (!v0)
    {
        return v13;
    }
    else if (*(int *)(*(unsigned int *)&idx + 3676) != 2)
    {
        return v13;
    }
    else if (*(int *)(*(unsigned int *)&idx + 3672) == 4)
    {
        v35 = sub_499690();
        *(unsigned int *)(*(unsigned int *)&idx + 4256) = v35;
        return v35;
    }
    else
    {
        if (*(int *)(*(unsigned int *)&idx + 4236) <= 0)
            return v13;
        *(unsigned int *)(*(unsigned int *)&idx + 4256) = sub_499690();
        return v13;
    }
}
