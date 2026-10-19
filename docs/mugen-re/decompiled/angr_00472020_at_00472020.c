// contains 0x472020; entry=0x472020
typedef struct st_472020_3 {
    struct st_472020_2 *field_0;
    unsigned int field_4;
    unsigned int field_8;
    unsigned int field_c;
    unsigned int field_10;
    unsigned int field_14;
} st_472020_3;

typedef struct st_472020_2 {
    char padding_0[16];
    unsigned int field_10;
    unsigned int field_14;
    unsigned int field_18;
    char padding_1c[56];
    unsigned int field_54;
} st_472020_2;

typedef struct st_472020_0 {
    unsigned int field_0;
    char padding_4[4];
    int field_8;
} st_472020_0;

typedef struct st_472020_1 {
    char padding_0[32960];
    struct st_472020_0 *field_80c0;
} st_472020_1;

extern st_472020_1 *g_52b40c;

void sub_472020(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3, unsigned int a4, unsigned int a5)
{
    st_472020_0 *v9;  // edx
    unsigned int v10;  // esi
    unsigned int v11;  // edi
    unsigned int v12;  // ebx
    st_472020_3 *v13;  // eax
    int v14;  // ecx
    st_472020_2 *v15;  // eax
    int i;  // [bp-0x2c]
    unsigned long long v1;  // [bp-0x28]
    unsigned int v2;  // [bp-0x20]
    unsigned int v3;  // [bp-0x1c]
    unsigned int v4;  // [bp-0x18]
    unsigned long long v5;  // [bp-0x14]
    unsigned int v6;  // [bp-0xc]
    unsigned int v7;  // [bp-0x8]
    unsigned int v8;  // [bp-0x4]

    v9 = g_52b40c->field_80c0;
    v10 = 0;
    if (!v11 && !a5 && !v12)
        return;
    i = 0;
    if (v9->field_8 <= 0)
        return;
    do
    {
        v5 = CONCAT(a1, a0);
        v6 = a2;
        v7 = a3;
        v13 = v9->field_0 + v10;
        v8 = a4;
        v1 = CONCAT(v13->field_8, v13->field_4);
        v2 = v13->field_c;
        v3 = v13->field_10;
        v4 = v13->field_14;
        v14 = 0;
        do
        {
            if (*(int *)((char *)&v1 + v14) & *(int *)((char *)&v5 + v14))
            {
                v15 = v13->field_0;
                if (v11 || a5 || v12)
                {
                    v15->field_14 = v15->field_14 + a5;
                    v15->field_10 = v15->field_10 + v11;
                    v15->field_18 = v15->field_18 + v12;
                    v15->field_54 = v15->field_54 & 0xfffffffb | 1;
                    break;
                }
            }
        } while ((v14 += 4, v14 < 20));
        v10 += 24;
        i += 1;
    } while (i < v9->field_8);
    return;
}
