// contains 0x4723a0; entry=0x4723a0
typedef struct st_4723a0_2 {
    void* field_0;
    unsigned int field_4;
    unsigned int field_8;
    unsigned int field_c;
    unsigned int field_10;
    unsigned int field_14;
} st_4723a0_2;

typedef struct st_4723a0_0 {
    unsigned int field_0;
    char padding_4[4];
    int field_8;
} st_4723a0_0;

typedef struct st_4723a0_1 {
    char padding_0[32960];
    struct st_4723a0_0 *field_80c0;
} st_4723a0_1;

extern st_4723a0_1 *g_52b40c;

void sub_4723a0(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3, unsigned int a4)
{
    st_4723a0_0 *v10;  // esi
    int i;  // ebp
    unsigned int v12;  // ebx
    unsigned int v13;  // edi
    unsigned int v14;  // edi
    st_4723a0_2 *v15;  // eax
    int v16;  // ecx
    void* v17;  // eax
    unsigned int v0;  // [bp-0x38]
    unsigned int v1;  // [bp-0x34]
    unsigned long long v2;  // [bp-0x28]
    unsigned int v3;  // [bp-0x20]
    unsigned int v4;  // [bp-0x1c]
    unsigned int v5;  // [bp-0x18]
    unsigned long long v6;  // [bp-0x14]
    unsigned int v7;  // [bp-0xc]
    unsigned int v8;  // [bp-0x8]
    unsigned int v9;  // [bp-0x4]

    v10 = g_52b40c->field_80c0;
    i = 0;
    if (v10->field_8 <= 0)
        return;
    v1 = v12;
    v0 = v13;
    v14 = 0;
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
                if (!((char)v17[84] & 4))
                {
                    *(unsigned int *)&v17[16] = 0;
                    *(unsigned int *)&v17[20] = 0;
                    *(unsigned int *)&v17[24] = 0;
                    *(unsigned int *)&v17[40] = 0;
                    *(unsigned int *)&v17[44] = 0;
                    *(unsigned int *)&v17[36] = 0x100;
                    *(unsigned int *)&v17[32] = 0x100;
                    *(unsigned int *)&v17[28] = 0x100;
                    *(unsigned int *)&v17[12] = 0x100;
                    *(unsigned int *)&v17[84] = (int)v17[84] | 5;
                    break;
                }
            }
        } while ((v16 += 4, v16 < 20));
        i += 1;
        v14 += 24;
    } while (i < v10->field_8);
    return;
}
