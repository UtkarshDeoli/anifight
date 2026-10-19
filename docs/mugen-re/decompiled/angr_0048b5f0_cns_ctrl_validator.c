// cns_ctrl_validator entry=0x48b5f0
typedef struct st_48b5f0_2 {
    char padding_0[4];
    int field_4;
    int field_8;
    unsigned int field_c;
} st_48b5f0_2;

typedef struct st_48b5f0_5 {
    char padding_0[32];
    unsigned int field_20;
} st_48b5f0_5;

typedef struct st_48b5f0_1 {
    char padding_0[4];
    unsigned int field_4;
    char padding_8[16];
    unsigned int field_18;
    unsigned int field_1c;
    unsigned int field_20;
    unsigned int field_24;
} st_48b5f0_1;

typedef struct st_48b5f0_4 {
    unsigned int field_0;
} st_48b5f0_4;

typedef struct st_48b5f0_3 {
    char padding_0[456];
    struct st_48b5f0_4 *field_1c8;
} st_48b5f0_3;

typedef struct st_48b5f0_0 {
    struct st_48b5f0_1 *field_0;
    char padding_4[72];
    unsigned int field_4c;
} st_48b5f0_0;

extern unsigned int g_52b40c;

int sub_48b5f0(float idx)
{
    unsigned int v10;  // eax
    unsigned int v11;  // eax
    int v20;  // esi
    unsigned int v21;  // eax
    st_48b5f0_5 *v22;  // eax
    unsigned int v23;  // esi
    unsigned int v24;  // 4102
    st_48b5f0_2 *v25;  // esi
    float iter;  // edi
    char v27;  // al
    char v28;  // 4098
    unsigned int v29;  // 4107
    st_48b5f0_0 *v12;  // ebx
    st_48b5f0_5 *v30;  // edx
    unsigned int v31;  // ecx
    unsigned int *v32;  // eax
    unsigned int v13;  // edi
    int i;  // eax
    double v15;  // mm7
    int v16;  // eax
    double v17;  // mm7
    unsigned int v18;  // ecx
    int v19;  // eax
    unsigned int v0;  // [bp-0x30]
    unsigned int v1;  // [bp-0x2c]
    int v2;  // [bp-0x28]
    int v3;  // [bp-0x24]
    int v4;  // [bp-0x20]
    int v5;  // [bp-0x1c]
    int v6;  // [bp-0x18]
    unsigned int v7;  // [bp-0x14]
    st_48b5f0_3 *v8;  // [bp-0x10]
    float v9;  // [bp+0x4]

    v6 = 0;
    sub_48a840(v2, v3, v4, v5, v6);
    v10 = *(int *)(*(unsigned int *)&idx + 3132);
    if (!*(int *)(*(unsigned int *)&idx + 3132))
        v10 = *(int *)(*(unsigned int *)&idx + 3124);
    v11 = sub_4032f0();
    v12 = (v11 == 0xffffffff ? NULL : v11 * *(int *)(*(int *)v10 + 4) + *(int *)(*(int *)v10 + 24));
    if (*(int *)(*(unsigned int *)&idx + 3696) <= 0)
    {
        if (!v12)
        {
            *(unsigned int *)(*(unsigned int *)&idx + 3736) = 0;
            if (*(int *)(*(unsigned int *)&idx + 3668))
                return 0;
            *(unsigned int *)(*(unsigned int *)&idx + 0x13fc) = 0;
            *(unsigned int *)(*(unsigned int *)&idx + 5112) = 0;
            *(unsigned int *)(*(unsigned int *)&idx + 3724) = 0;
            *(unsigned int *)(*(unsigned int *)&idx + 3728) = 0;
            *(unsigned int *)(*(unsigned int *)&idx + 4540) = 0;
            return 0;
        }
        else
        {
            *(unsigned int *)(*(unsigned int *)&idx + 3736) = 1;
            if (!*(int *)(*(unsigned int *)&idx + 3668))
            {
                if (!sub_439260(1))
                {
                    *(unsigned int *)(*(unsigned int *)&idx + 5112) = 0;
                    *(unsigned int *)(*(unsigned int *)&idx + 0x13fc) = 0;
                }
                if (!sub_439260(1))
                {
                    *(unsigned int *)(*(unsigned int *)&idx + 3724) = 0;
                    *(unsigned int *)(*(unsigned int *)&idx + 3728) = 0;
                }
                v13 = sub_439260(1);
                if (!v13)
                {
                    *(unsigned int *)(*(unsigned int *)&idx + 4540) = 0;
                    *(unsigned int *)(*(unsigned int *)&idx + 4528) = 0;
                }
                if (v12->field_4c)
                    *(unsigned int *)(*(unsigned int *)&idx + 536) = sub_439260(1);
                i = 0;
                if (*(int *)&v12->field_0->padding_8[0] > 0)
                {
                    do
                    {
                        *(char *)(i + *(unsigned int *)&idx + 3156) = 0;
                        i += 1;
                    } while (i < *(int *)&v12->field_0->padding_8[0]);
                }
                if (*(int *)&v12->field_0->padding_8[0] > 0x200)
                {
                    v1 = 0x200;
                    v0 = *(int *)(*(unsigned int *)&idx + 3148);
                    sprintf(g_52b40c + 73820, "state %i can only have max of %i controllers");
                    sub_443e80(); /* do not return */
                }
                if (*(int *)&v12->padding_4[0] != -0x1)
                    *(int *)(*(unsigned int *)&idx + 3672) = *(int *)&v12->padding_4[0];
                if (*(int *)&v12->padding_4[4] != -0x1)
                    *(int *)(*(unsigned int *)&idx + 3676) = *(int *)&v12->padding_4[4];
                if (*(int *)&v12->padding_4[8] != -0x1)
                    *(int *)(*(unsigned int *)&idx + 3680) = *(int *)&v12->padding_4[8];
                if (v7 && sub_439260(1))
                {
                    sub_464cd0();
                    if (v15 < 0.0)
                        sub_46b410();
                }
                v16 = sub_439260(1);
                if (v16 < -0x1)
                {
                    sub_443cd0();
                }
                else if (v16 != -0x1)
                {
                    *(int *)(*(unsigned int *)&idx + 4580) = v16;
                }
                if (!v13)
                    *(int *)(*(unsigned int *)&idx + 4908) = *(int *)(*(unsigned int *)&idx + 104);
                sub_439330();
                v9 = (float)v15;
                v17 = v9;
                if (v17 != -3.4028234663852886e+38)
                {
                    v1 = v18;
                    sub_465250((float)v17);
                    v17 *= (double)*(int *)(*(unsigned int *)&idx + 488);
                    *(float *)(*(unsigned int *)&idx + 516) = (float)v17;
                }
                sub_439330();
                v9 = (float)v17;
                if (v9 != -3.4028234663852886e+38f)
                {
                    v1 = v18;
                    sub_465250(v9);
                    *(float *)(*(unsigned int *)&idx + 520) = v9;
                }
                v19 = sub_439260(1);
                if (v19 < -0x1)
                {
                    sub_443cd0();
                }
                else if (v19 != -0x1)
                {
                    *(int *)*(int *)(*(unsigned int *)&idx + 5144) = *(int *)(*(int *)idx + 1080);
                    *(unsigned int *)(*(unsigned int *)&idx + 3144) = 0;
                    sub_46b0c0();
                }
                v20 = sub_439260(1);
                if (v20 < -0x1)
                {
                    sub_443cd0();
                }
                else
                {
                    if (v20 > 1)
                    {
                        v20 = 1;
                        sub_443cd0();
                    }
                    else if (v20 == -0x1)
                    {
                        goto LABEL_48b907;
                    }
                    *(int *)(*(unsigned int *)&idx + 3684) = v20;
                }
LABEL_48b907:
                v21 = sub_439260(1);
                sub_46b070(*(int *)v8->field_1c8 + v21);
            }
        }
    }
    if (!v12)
        return 0;
    v22 = v12->field_0;
    if (!*(int *)&v22->padding_0[8])
        return v6;
    v23 = *(int *)&v22[1].padding_0[0] * *(int *)&v22->padding_0[4];
    v24 = *(int *)&v22->padding_0[24];
    v25 = v23 + *(int *)&v22->padding_0[24];
    v22->field_20 = *(int *)&v22[1].padding_0[0];
    if (!(v23 + v24))
        return v6;
    iter = __int_as_float(*(unsigned int *)&idx + 3156);
    v9 = iter;
    while (1)
    {
        if (!*(char *)iter && (!*(int *)(*(unsigned int *)&idx + 3696) || v25->field_c))
        {
            *(unsigned int *)(*(unsigned int *)&idx + 8728) = 0xffffffff;
            if (sub_486040(idx, v25->field_4))
            {
                if (v25->field_8 > 0)
                    *(char *)v9 = v25->field_8;
                else
                    *(char *)v9 = 1;
                v27 = sub_4781d0(idx, v25, &v7);
                if (v27 & 4)
                    v6 = 1;
                if (v27 & 3)
                    return v6;
            }
            iter = v9;
        }
        if (v25->field_8 > 0 && (!*(int *)(*(unsigned int *)&idx + 3696) || v25->field_c))
        {
            v28 = *(char *)iter;
            *(char *)iter = *(char *)iter - 1;
            v29 = _ccall(8, 1, v28, 0xff, 0);
            if (v29 & 1)
                *(char *)iter = 0;
        }
        v30 = v12->field_0;
        v31 = v30->field_20;
        if ((int)v31 >= *(int *)&v30[1].padding_0[4])
        {
            v25 = NULL;
        }
        else
        {
            v32 = v31 * 16 + *(int *)&v30->padding_0[28];
            do
            {
                v32 += 4;
                v31 += 1;
            } while (!*v32);
            v25 = v31 * *(int *)&v30->padding_0[4] + *(int *)&v30->padding_0[24];
            v30->field_20 = v31;
        }
        iter = __int_as_float(__float_as_int(iter) + 1);
        v9 = iter;
        if (!v25)
            return v6;
    }
}
