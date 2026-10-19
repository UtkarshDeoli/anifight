// cns_statedef_scan entry=0x48a900
typedef struct st_48a900_0 {
    char padding_0[4];
    unsigned int field_4;
    char padding_8[4];
    unsigned int field_c;
    char padding_10[4];
    unsigned int field_14;
    int field_18;
    char padding_1c[16];
    unsigned int field_2c;
} st_48a900_0;

extern unsigned int g_52b0d8;
extern void* g_52b40c;

int sub_48a900(void)
{
    unsigned int v8;  // eax
    st_48a900_0 *v9;  // eax
    int num;  // ebp
    unsigned int v19;  // edi
    unsigned int v20;  // eax
    unsigned int v21;  // eax
    unsigned int idx;  // eax
    void* node;  // ecx
    void* v24;  // eax
    void* v25;  // eax
    unsigned int v26;  // eax
    unsigned int v27;  // edi
    int v10;  // ebx
    unsigned int v28;  // edi
    unsigned int iter;  // edi
    unsigned int v30;  // ecx
    unsigned int v31;  // ecx
    void* v11;  // esi
    unsigned int *v12;  // ecx
    unsigned int v13;  // eax
    unsigned int index;  // esi
    void* *v15;  // eax
    unsigned int v16;  // ecx
    unsigned int v17;  // 4114
    int v0;  // [bp-0xb0]
    int v1;  // [bp-0xac]
    int v2;  // [bp-0xa8]
    int v3;  // [bp-0xa4]
    unsigned int *ptr;  // [bp-0xa0]
    int v5;  // [bp-0x9c]
    char v6;  // [bp-0x98]
    int v7;  // [bp+0x4]

    v5 = sub_433280("statedef ", v0, v1, v2, v3);
    if (!v5)
    {
        sprintf(g_52b40c + 73820, "Need at least one statedef");
        return;
    }
    ptr = calloc(1, 4);
    if (!ptr)
    {
        g_52b0d8 = 1;
        return;
    }
    v8 = sub_402f10(152);
    *ptr = v8;
    if (!v8)
        return;
    v9->field_18 = -0x1;
    v10 = 0;
    v11 = sub_433220();
    if (v5 <= 0)
        return;
    while (1)
    {
        if (!sub_40e3a0())
        {
            do
            {
                v9->field_18 = v9->field_18 + 1;
                if (v9->field_18 >= v9[1].padding_0)
                {
                    v11 = NULL;
                }
                else
                {
                    v12 = v9->field_2c + v9->field_18 * 4;
                    v13 = *v12 + 1;
                    v9->field_c = v13;
                    index = v9->field_c;
                    v9->field_14 = v13;
                    v15 = *(int *)&v9->padding_1c[8];
                    *(void* *)v9->padding_10 = v15[index] - v9->field_4;
                    v11 = v15[*v12];
                    v16 = v9->field_14;
                    v17 = *(int *)&v9->padding_1c[12];
                    *(unsigned int *)&v9->padding_1c[0] = 0;
                    if ((int)v16 >= (int)v17 || *(char *)v15[v16] == 91)
                        *(unsigned int *)&v9->padding_1c[0] = 1;
                }
            } while (!sub_40e3a0());
        }
        num = atoi(v11 + 10);
        if (sub_48acc0(&v6, v11, v7))
        {
            sub_4030f0(&v6, num, 0);
            v10 += 1;
            v11 = sub_433220();
            if (v10 >= v5)
                return;
        }
        else
        {
            v19 = g_52b40c + 73820;
            v20 = v19 - 1;
            do
            {
                v21 = v20;
                idx = v21 + 1;
                v20 = idx;
            } while (*(char *)(v21 + 1));
            *(unsigned int *)idx = 1920091402;
            *(unsigned int *)(idx + 4) = 1763734127;
            *(unsigned short *)(idx + 8) = 8302;
            *(char *)(idx + 10) = 0;
            node = v11;
            do
            {
                v24 = v11;
                v25 = v24 + 1;
                v11 = v25;
            } while (*(char *)v24);
            v26 = v25 - node;
            v27 = v19 - 1;
            do
            {
                v28 = v27;
                iter = v28 + 1;
                v27 = iter;
            } while (*(char *)(v28 + 1));
            for (v30 = v26 >> 2; v30; node += 4)
            {
                v30 -= 1;
                *(int *)iter = *(int *)node;
                iter += 4;
            }
            for (v31 = v26 & 3; v31; node += 1)
            {
                v31 -= 1;
                *(char *)iter = *(char *)node;
                iter += 1;
            }
            return;
        }
    }
}
