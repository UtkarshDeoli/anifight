// contains 0x472200; entry=0x472200
typedef struct st_472200_3 {
    struct st_472200_2 *field_0;
    unsigned int field_4;
    unsigned int field_8;
    unsigned int field_c;
    unsigned int field_10;
    unsigned int field_14;
} st_472200_3;

typedef struct st_472200_2 {
    char padding_0[44];
    unsigned int field_2c;
    char padding_30[36];
    unsigned int field_54;
} st_472200_2;

typedef struct st_472200_0 {
    unsigned int field_0;
    char padding_4[4];
    int field_8;
} st_472200_0;

typedef struct st_472200_1 {
    char padding_0[32960];
    struct st_472200_0 *field_80c0;
} st_472200_1;

extern st_472200_1 *g_52b40c;

void sub_472200(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3, unsigned int a4, unsigned int a5)
{
    st_472200_0 *v9;  // edx
    unsigned int v10;  // edi
    int i;  // edi
    unsigned int v12;  // esi
    st_472200_3 *v13;  // eax
    int v14;  // ecx
    st_472200_2 *v15;  // eax
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
    if (!a5)
        return;
    v0 = v10;
    i = 0;
    if (v9->field_8 <= 0)
        return;
    v12 = 0;
    do
    {
        v5 = CONCAT(a1, a0);
        v6 = a2;
        v7 = a3;
        v13 = v9->field_0 + v12;
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
                v15->field_2c = !v15->field_2c;
                v15->field_54 = v15->field_54 & 0xfffffffb | 1;
                break;
            }
        } while ((v14 += 4, v14 < 20));
        i += 1;
        v12 += 24;
    } while (i < v9->field_8);
    return;
}
