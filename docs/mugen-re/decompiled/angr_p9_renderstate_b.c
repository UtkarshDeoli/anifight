// pass9 renderstate_b entry=0x413d80 (asked 0x413d80)
typedef struct st_413d80_0 {
    char padding_0[16];
    unsigned int field_10;
    unsigned int field_14;
    unsigned int field_18;
    char padding_1c[44];
    unsigned int field_48;
    char padding_4c[4];
    unsigned int field_50;
} st_413d80_0;

extern st_413d80_0 *g_52b34c;

void sub_413d80(unsigned int a0, unsigned int a1)
{
    unsigned int v1;  // esi

    if (g_52b34c->field_10 == a0 && g_52b34c->field_14 == v1 && g_52b34c->field_18 == a1)
        return;
    g_52b34c->field_10 = a0;
    g_52b34c->field_14 = v1;
    g_52b34c->field_18 = a1;
    g_52b34c->field_48 = 1;
    g_52b34c->field_50 = 0;
    return;
}
