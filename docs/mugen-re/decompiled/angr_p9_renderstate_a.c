// pass9 renderstate_a entry=0x413d50 (asked 0x413d50)
typedef struct st_413d50_0 {
    char padding_0[4];
    unsigned int field_4;
    unsigned int field_8;
    unsigned int field_c;
    char padding_10[56];
    unsigned int field_48;
    char padding_4c[4];
    unsigned int field_50;
} st_413d50_0;

extern st_413d50_0 *g_52b34c;

void sub_413d50(unsigned int a0, unsigned int a1)
{
    unsigned int v1;  // esi

    if (g_52b34c->field_4 == a0 && g_52b34c->field_8 == v1 && g_52b34c->field_c == a1)
        return;
    g_52b34c->field_4 = a0;
    g_52b34c->field_8 = v1;
    g_52b34c->field_c = a1;
    g_52b34c->field_48 = 1;
    g_52b34c->field_50 = 0;
    return;
}
