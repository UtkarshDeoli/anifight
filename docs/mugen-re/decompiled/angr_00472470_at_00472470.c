// contains 0x472470; entry=0x472470
typedef struct st_472470_0 {
    int field_0;
    char padding_4[4];
    unsigned int field_8;
    unsigned int field_c;
    unsigned int field_10;
    unsigned int field_14;
    unsigned int field_18;
    int field_1c;
    unsigned int field_20;
    char padding_24[12];
    int field_30;
    unsigned int field_34;
    unsigned int field_38;
} st_472470_0;

typedef struct st_472470_2 {
    char padding_0[69308];
    unsigned int field_10ebc;
    char padding_10ec0[2112];
    unsigned int field_11700;
} st_472470_2;

typedef struct st_472470_1 {
    unsigned int field_0;
    unsigned int field_4;
    unsigned int field_8;
    unsigned int field_c;
    unsigned int field_10;
    unsigned int field_14;
    unsigned int field_18;
    unsigned int field_1c;
    int field_20;
    char padding_24[36];
    unsigned int field_48;
    char padding_4c[4];
    unsigned int field_50;
} st_472470_1;

extern st_472470_1 *g_52b34c;
extern st_472470_2 *g_52b40c;

void sub_472470(st_472470_0 *ptr, int a1, int a2, int a3, int a4, int a5, unsigned int a6, unsigned int a7)
{
    int v5;  // eax
    unsigned int v6;  // esi
    double v7;  // mm6
    unsigned int v0;  // [bp-0x10]
    unsigned int v1;  // [bp-0xc]
    unsigned int v2;  // [bp-0x8]
    unsigned int v3;  // [bp-0x4]
    float v4;  // [bp+0x4], Other Possible Types: int

    if (ptr->field_38)
    {
        if (a6)
        {
            g_52b34c->field_4 = 0;
            g_52b34c->field_8 = 0;
            g_52b34c->field_c = 0;
            g_52b34c->field_1c = 0;
            g_52b34c->field_20 = 0;
            g_52b34c->field_18 = 0x100;
            g_52b34c->field_14 = 0x100;
            g_52b34c->field_10 = 0x100;
            g_52b34c->field_0 = 0x100;
            g_52b34c->field_48 = 1;
            g_52b34c->field_50 = 1;
        }
        else
        {
            sub_4723a0(a1, a2, a3, a4, a5);
        }
        ptr->field_38 = 0;
    }
    v5 = (int)ptr->padding_4;
    if (!v5)
        return;
    if (g_52b40c->field_11700)
        a7 = 0;
    if (g_52b40c->field_10ebc)
        return;
    v4 = ptr->field_0;
    if (ptr->field_0 >= v5 && v5 != -0x1)
    {
        memset(ptr, 0, 60);
        ptr->field_20 = 0x100;
        ptr->field_1c = 0x100;
        ptr->field_18 = 0x100;
        ptr->field_8 = 0x100;
        ptr->field_38 = 1;
        return;
    }
    v6 = ptr->field_10;
    v1 = ptr->field_c;
    v2 = ptr->field_10;
    v3 = ptr->field_14;
    v0 = ptr->field_34;
    if (ptr->field_34)
    {
        _CIsin();
        v4 = (float)v7;
        sub_499690();
        v6 = sub_499690();
        sub_499690();
    }
    if (a6)
    {
        sub_413d50();
        sub_413d80();
        if (g_52b34c->field_20 != ptr->field_30)
        {
            g_52b34c->field_20 = ptr->field_30;
            g_52b34c->field_48 = 1;
            g_52b34c->field_50 = 0;
        }
        if (g_52b34c->field_0 != ptr->field_8)
        {
            g_52b34c->field_0 = ptr->field_8;
            g_52b34c->field_48 = 1;
            g_52b34c->field_50 = 0;
        }
    }
    else
    {
        sub_472020(a1, a2, a3, a4, a5, v6);
        sub_472100(a1, a2, a3, a4, a5, ptr->field_1c);
        sub_472200(a1, a2, a3, a4, a5, ptr->field_30);
        sub_4722d0(a1, a2, a3, a4, a5);
    }
    if (!a7)
        return;
    ptr->field_0 = ptr->field_0 + 1;
    return;
}
