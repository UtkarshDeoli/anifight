// pass9 10ebc_writer_b entry=0x458bb0 (asked 0x458bb0)
typedef struct st_458bb0_2 {
    char padding_0[2276];
    unsigned int field_8e4;
} st_458bb0_2;

typedef struct st_458bb0_0 {
    char padding_0[7776];
    unsigned int field_1e60;
    unsigned int field_1e64;
    char padding_1e68[10128];
    char field_45f8;
    char padding_45f9[51395];
    unsigned int field_10ebc;
    char padding_10ec0[2336];
    unsigned int field_117e0;
    unsigned int field_117e4;
    unsigned int field_117e8;
    char padding_117ec[28];
    unsigned int field_11808;
} st_458bb0_0;

extern st_458bb0_0 *g_52b40c;
extern char g_52b950;

st_458bb0_0 * sub_458bb0(void)
{
    st_458bb0_0 *v3;  // eax
    unsigned int v4;  // esi
    int v5;  // ecx
    st_458bb0_2 *v6;  // edi
    unsigned int v7;  // esi
    unsigned int v8;  // eax
    int v9;  // ecx
    int v0;  // [bp-0x8]
    int v1;  // [bp-0x4]

    sub_445540(v0, v1);
    sub_445540(v0, v1);
    v3 = g_52b40c;
    g_52b40c->field_10ebc = 0;
    g_52b950 = 0;
    if (g_52b40c->field_117e8)
    {
        if (!g_52b40c->field_45f8)
        {
            v4 = &g_52b40c->padding_0[6876 * g_52b40c->field_117e0 + 12108];
            v5 = *(int *)(g_52b40c->field_117e4 * 92 + v4 + 28);
            if (v5 != -0x1 && v5 >= 0 && v5 < (int)g_52b40c->field_1e64)
            {
                v6 = *(int *)(g_52b40c->field_1e60 + v5 * 3684 + 8);
                if (v6 && (int)v6->field_8e4 > 0)
                {
                    v7 = *(int *)(g_52b40c->field_117e4 * 1616 + v4 + 780);
                    if (v7 > 99 || !*(int *)&v6->padding_0[1876 + 4 * v7] || v7 == -0x1)
                    {
                        v7 = 0;
                        v8 = sub_4599c0();
                        v9 = v8 % v6->field_8e4 + 1;
                        do
                        {
                            v7 += 1;
                            if (!*(int *)&v6->padding_0[1876 + 4 * v7])
                            {
                                do
                                {
                                    v7 = (v7 + 1) % 100;
                                } while (!*(int *)&v6->padding_0[1876 + 4 * v7]);
                            }
                        } while ((v9 -= 1, v9 > 0));
                    }
                    strncpy(&g_52b40c->field_45f8, *(int *)&v6->padding_0[1876 + 4 * v7], 0x3ff);
                    v3 = g_52b40c;
                }
            }
        }
    }
    v3->field_11808 = 0;
    return v3;
}
