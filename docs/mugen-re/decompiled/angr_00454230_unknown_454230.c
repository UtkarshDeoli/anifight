// unknown_454230 entry=0x454230
typedef struct st_454230_1 {
    char padding_0[32936];
    struct st_454230_0 *field_80a8;
    char padding_80ac[60];
    int field_80e8;
    char padding_80ec[8];
    unsigned int field_80f4;
    char padding_80f8[40];
    unsigned int field_8120;
    int field_8124;
} st_454230_1;

typedef struct st_454230_0 {
    char padding_0[36];
    unsigned int field_24;
    int field_28;
} st_454230_0;

extern st_454230_1 *g_52b40c;

void sub_454230(void)
{
    st_454230_0 **v3;  // edi
    st_454230_1 *v4;  // eax
    unsigned int *v5;  // esi
    unsigned int v0;  // [bp-0x1c]
    unsigned int v1;  // [bp-0x14]

    v3 = 0x80f8;
    do
    {
        v4 = g_52b40c;
        v5 = *(int *)(v3 + g_52b40c);
        if (v5 != g_52b40c->field_80a8 && v5)
        {
            if (v5[9])
                sub_417520();
            if (v5[10])
                TTF_CloseFont(v5[10]);
            free(v5);
            v4 = g_52b40c;
        }
    } while ((v3 += 1, v3 < 0x8120));
    v1 = v4->field_80f4;
    sub_401a10(v1);
    sub_417520(v1);
    sub_415470(g_52b40c->field_8124);
    v0 = g_52b40c->field_8120;
    sub_415470(v0);
    sub_417520(v0);
    sub_401a10(g_52b40c->field_80e8);
    sub_444b40();
    sub_444b40();
    return;
}
