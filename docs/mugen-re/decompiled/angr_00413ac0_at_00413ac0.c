// contains 0x413ac0; entry=0x413ac0
typedef struct st_413ac0_0 {
    char padding_0[12];
    unsigned int field_c;
    char padding_10[32];
    unsigned int field_30;
    char padding_34[32];
    unsigned int field_54;
    unsigned int field_58;
} st_413ac0_0;

extern unsigned int g_52b2fc;
extern unsigned int g_52b34c;
extern unsigned int g_52b350;

unsigned int sub_413ac0(void)
{
    st_413ac0_0 *v1;  // ebx
    unsigned int v2;  // edx
    unsigned int v3;  // eax
    st_413ac0_0 *i;  // esi
    st_413ac0_0 *iter;  // edi
    unsigned int v6;  // ecx
    unsigned int v7;  // eax

    if ((!g_52b2fc || g_52b2fc == 2) && v1->field_58 != *(int *)(g_52b34c + 76))
        v2 = 1;
    else
        v2 = 0;
    v3 = v1->field_54 & 2;
    if (!v2)
    {
        if (!((char)v1->field_54 & 1))
        {
            if (!v3)
                return v3;
        }
        else
        {
            if (!v3 && sub_413be0())
                goto LABEL_413b34;
        }
        i = &v1->field_c;
    }
    else
    {
        i = &v1->field_c;
        sub_413b70();
    }
    sub_414320(v1 + 1);
    iter = &v1->field_30;
    v6 = 9;
    for (g_52b350 = g_52b350 + 1; v6; i = &i->padding_0[4])
    {
        v6 -= 1;
        *(int *)&iter->padding_0[0] = *(int *)&i->padding_0[0];
        iter = &iter->padding_0[4];
    }
LABEL_413b34:
    v1->field_54 = v1->field_54 & 0xfffffffc;
    v7 = *(int *)(g_52b34c + 76);
    v1->field_58 = v7;
    return v7;
}
