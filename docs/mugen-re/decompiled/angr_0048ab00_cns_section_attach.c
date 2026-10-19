// cns_section_attach entry=0x48ab00
typedef struct st_48ab00_0 {
    char padding_0[4];
    unsigned int field_4;
    char padding_8[4];
    unsigned int field_c;
    char padding_10[4];
    unsigned int field_14;
    int field_18;
    unsigned int field_1c;
    char padding_20[12];
    unsigned int field_2c;
} st_48ab00_0;

extern void* g_52b40c;

int sub_48ab00(void)
{
    st_48ab00_0 *v9;  // eax
    void* v10;  // esi
    unsigned int v19;  // eax
    unsigned int v20;  // eax
    unsigned int idx;  // eax
    void* node;  // ecx
    void* v23;  // eax
    void* v24;  // eax
    unsigned int v25;  // eax
    unsigned int v26;  // edi
    unsigned int v27;  // edi
    unsigned int iter;  // edi
    unsigned int *v11;  // ecx
    unsigned int v29;  // ecx
    unsigned int v30;  // ecx
    unsigned int v12;  // eax
    unsigned int index;  // esi
    void* *v14;  // eax
    unsigned int v15;  // ecx
    unsigned int v16;  // 4114
    int num;  // ebx
    unsigned int v18;  // edi
    int v0;  // [bp-0xb4]
    int v1;  // [bp-0xb0]
    int v2;  // [bp-0xac]
    int v3;  // [bp-0xa8]
    int v4;  // [bp-0xa4]
    int v5;  // [bp-0xa0]
    char v6;  // [bp-0x9c]
    int v8;  // [bp+0x8]

    v4 = sub_433280("statedef ", v0, v1, v2, v3);
    if (!v4)
    {
        return;
    }
    else if (sub_4031f0())
    {
        v9->field_18 = -0x1;
        v10 = sub_433220();
        v5 = 0;
        if (v4 <= 0)
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
                        v10 = NULL;
                    }
                    else
                    {
                        v11 = v9->field_2c + v9->field_18 * 4;
                        v12 = *v11 + 1;
                        v9->field_c = v12;
                        index = v9->field_c;
                        v9->field_14 = v12;
                        v14 = *(int *)&v9->padding_20[4];
                        *(void* *)v9->padding_10 = v14[index] - v9->field_4;
                        v10 = v14[*v11];
                        v15 = v9->field_14;
                        v16 = *(int *)&v9->padding_20[8];
                        v9->field_1c = 0;
                        if ((int)v15 >= (int)v16 || *(char *)v14[v15] == 91)
                            v9->field_1c = 1;
                    }
                } while (!sub_40e3a0());
            }
            num = atoi(v10 + 10);
            if (sub_48acc0(&v6, v10, v8))
            {
                sub_4030f0(&v6, num, 0);
                v10 = sub_433220();
                v5 += 1;
                if (v5 >= v4)
                    return;
            }
            else
            {
                v18 = g_52b40c + 73820;
                v19 = v18 - 1;
                do
                {
                    v20 = v19;
                    idx = v20 + 1;
                    v19 = idx;
                } while (*(char *)(v20 + 1));
                *(unsigned int *)idx = 1920091402;
                *(unsigned int *)(idx + 4) = 1763734127;
                *(unsigned short *)(idx + 8) = 8302;
                *(char *)(idx + 10) = 0;
                node = v10;
                do
                {
                    v23 = v10;
                    v24 = v23 + 1;
                    v10 = v24;
                } while (*(char *)v23);
                v25 = v24 - node;
                v26 = v18 - 1;
                do
                {
                    v27 = v26;
                    iter = v27 + 1;
                    v26 = iter;
                } while (*(char *)(v27 + 1));
                for (v29 = v25 >> 2; v29; node += 4)
                {
                    v29 -= 1;
                    *(int *)iter = *(int *)node;
                    iter += 4;
                }
                for (v30 = v25 & 3; v30; node += 1)
                {
                    v30 -= 1;
                    *(char *)iter = *(char *)node;
                    iter += 1;
                }
                return;
            }
        }
    }
    else
    {
        return;
    }
}
