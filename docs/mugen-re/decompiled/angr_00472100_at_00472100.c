// contains 0x472100; entry=0x472100
typedef struct st_472100_3 {
    struct st_472100_2 *field_0;
    unsigned int field_4;
    unsigned int field_8;
    unsigned int field_c;
    unsigned int field_10;
    unsigned int field_14;
} st_472100_3;

typedef struct st_472100_2 {
    char padding_0[28];
    int field_1c;
    int field_20;
    int field_24;
    char padding_28[44];
    unsigned int field_54;
} st_472100_2;

typedef struct st_472100_0 {
    unsigned int field_0;
    char padding_4[4];
    int field_8;
} st_472100_0;

typedef struct st_472100_1 {
    char padding_0[32960];
    struct st_472100_0 *field_80c0;
} st_472100_1;

extern st_472100_1 *g_52b40c;

void sub_472100(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3, unsigned int a4, int a5)
{
    st_472100_0 *v10;  // edx
    int v11;  // edi
    int v12;  // ebx
    unsigned int v13;  // esi
    unsigned int v14;  // esi
    st_472100_3 *v15;  // eax
    int v16;  // ecx
    st_472100_2 *v17;  // eax
    unsigned int v0;  // [bp-0x30]
    int i;  // [bp-0x2c]
    unsigned long long v2;  // [bp-0x28]
    unsigned int v3;  // [bp-0x20]
    unsigned int v4;  // [bp-0x1c]
    unsigned int v5;  // [bp-0x18]
    unsigned long long v6;  // [bp-0x14]
    unsigned int v7;  // [bp-0xc]
    unsigned int v8;  // [bp-0x8]
    unsigned int v9;  // [bp-0x4]

    v10 = g_52b40c->field_80c0;
    if (v11 == 0x100 && a5 == 0x100 && v12 == 0x100)
        return;
    v0 = v13;
    v14 = 0;
    i = 0;
    if (v10->field_8 <= 0)
        return;
    do
    {
        v6 = CONCAT(a1, a0);
        v7 = a2;
        v8 = a3;
        v15 = v10->field_0 + v14;
        v9 = a4;
        v2 = CONCAT(v15->field_8, v15->field_4);
        v3 = v15->field_c;
        v4 = v15->field_10;
        v5 = v15->field_14;
        v16 = 0;
        do
        {
            if (*(int *)((char *)&v2 + v16) & *(int *)((char *)&v6 + v16))
            {
                v17 = v15->field_0;
                if (v11 != 0x100 || a5 != 0x100 || v12 != 0x100)
                {
                    v17->field_1c = v11 * v17->field_1c >> 8;
                    v17->field_20 = a5 * v17->field_20 >> 8;
                    v17->field_24 = v12 * v17->field_24 >> 8;
                    v17->field_54 = v17->field_54 & 0xfffffffb | 1;
                    break;
                }
            }
        } while ((v16 += 4, v16 < 20));
        v14 += 24;
        i += 1;
    } while (i < v10->field_8);
    return;
}
