// hitdef_attr_reader entry=0x47d820
typedef struct st_47d820_0 {
    unsigned int field_0;
    unsigned int field_4;
    char field_8;
    char padding_9[3];
    unsigned int field_c;
    unsigned int field_10;
    char field_14;
    char padding_15[3];
    unsigned int field_18;
    unsigned int field_1c;
    char field_20;
    char padding_21[3];
    unsigned int field_24;
} st_47d820_0;

extern unsigned int g_52b40c;

unsigned int sub_47d820(int a0, st_47d820_0 *a1)
{
    st_47d820_0 *v1;  // edi
    st_47d820_0 *v2;  // esi
    int v3;  // ebx
    int v4;  // ebp
    unsigned int v5;  // eax
    st_47d820_0 *v6;  // esi
    int v7;  // edi
    int v0;  // [bp-0x10]

    v1 = &a1->field_10;
    if (a1->field_18 != 0xffffffff && a1->field_18)
        sub_438c40(v1);
    v2 = &a1->field_4;
    *(unsigned int *)&v1->field_8 = 0xffffffff;
    v1->field_0 = 1;
    *(char *)&v1->field_4 = 0;
    if (*(int *)&v2->field_8 != 0xffffffff && *(int *)&v2->field_8)
        sub_438c40(v2);
    *(unsigned int *)&v2->field_8 = 0xffffffff;
    v2->field_0 = 0xffffffff;
    *(char *)&v2->field_4 = 0;
    v4 = sub_433410(v3, "attr");
    if (v4)
    {
        v5 = sub_489dc0(v4, 0);
        a1->field_0 = v5;
        if (v5 == 0xffffffff)
        {
            v0 = v4;
            sprintf(g_52b40c + 73820, "Invalid attr: %s");
            return 0;
        }
    }
    if (!sub_438b20(sub_433410(v3, "stateno"), v2, a0, 0, 1))
    {
        return 0;
    }
    else if (!sub_438b20(sub_433410(v3, "time"), v1, a0, 0, 1))
    {
        return 0;
    }
    else
    {
        v6 = &a1->field_1c;
        v7 = sub_433410(v3, "forceair");
        if (*(int *)&v6->field_8 != 0xffffffff && *(int *)&v6->field_8)
            sub_438c40(v6);
        *(unsigned int *)&v6->field_8 = 0xffffffff;
        v6->field_0 = 0;
        *(char *)&v6->field_4 = 0;
        if (!v7)
        {
            return 1;
        }
        else if (!sub_438b20(v7, v6, a0, 0, 1))
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
}
