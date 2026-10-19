// contains 0x4722d0; entry=0x4722d0
typedef struct st_4722d0_3 {
    struct st_4722d0_2 *field_0;
    unsigned int field_4;
    unsigned int field_8;
    unsigned int field_c;
    unsigned int field_10;
    unsigned int field_14;
} st_4722d0_3;

typedef struct st_4722d0_2 {
    char padding_0[12];
    int field_c;
    char padding_10[68];
    unsigned int field_54;
} st_4722d0_2;

typedef struct st_4722d0_0 {
    unsigned int field_0;
    char padding_4[4];
    int field_8;
} st_4722d0_0;

typedef struct st_4722d0_1 {
    char padding_0[32960];
    struct st_4722d0_0 *field_80c0;
} st_4722d0_1;

extern st_4722d0_1 *g_52b40c;

void sub_4722d0(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3, unsigned int a4)
{
    st_4722d0_0 *v9;  // edx
    int v10;  // edi
    unsigned int v11;  // ebx
    int i;  // ebx
    unsigned int v13;  // esi
    st_4722d0_3 *v14;  // eax
    int v15;  // ecx
    st_4722d0_2 *v16;  // eax
    unsigned int v0;  // [bp-0x2c]
    unsigned long long v1;  // [bp-0x28]
    unsigned int v2;  // [bp-0x20]
    unsigned int v3;  // [bp-0x1c]
    unsigned int v4;  // [bp-0x18]
    unsigned long long v5;  // [bp-0x14]
    unsigned int v6;  // [bp-0xc]
    unsigned int v7;  // [bp-0x8]
    unsigned int v8;  // [bp-0x4]

    v9 = g_52b40c->field_80c0;
    if (v10 == 0x100)
        return;
    v0 = v11;
    i = 0;
    if (v9->field_8 <= 0)
        return;
    v13 = 0;
    do
    {
        v5 = CONCAT(a1, a0);
        v6 = a2;
        v7 = a3;
        v14 = v9->field_0 + v13;
        v8 = a4;
        v1 = CONCAT(v14->field_8, v14->field_4);
        v2 = v14->field_c;
        v3 = v14->field_10;
        v4 = v14->field_14;
        v15 = 0;
        do
        {
            if (*(int *)((char *)&v1 + v15) & *(int *)((char *)&v5 + v15))
            {
                v16 = v14->field_0;
                v16->field_c = v10 * v16->field_c >> 8;
                v16->field_54 = v16->field_54 & 0xfffffffb | 1;
                break;
            }
        } while ((v15 += 4, v15 < 20));
        i += 1;
        v13 += 24;
    } while (i < v9->field_8);
    return;
}
