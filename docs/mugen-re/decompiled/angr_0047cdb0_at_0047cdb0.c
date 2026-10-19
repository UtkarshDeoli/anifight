// contains 0x47cdb0; entry=0x47cdb0
typedef struct st_47cdb0_0 {
    char padding_0[16];
    unsigned int field_10;
    char field_14;
    char padding_15[3];
    unsigned int field_18;
    char padding_1c[36];
    unsigned int field_40;
    char field_44;
    char padding_45[3];
    unsigned int field_48;
    unsigned int field_4c;
    char field_50;
    char padding_51[3];
    unsigned int field_54;
    unsigned int field_58;
    char field_5c;
    char padding_5d[3];
    unsigned int field_60;
    char padding_64[36];
    unsigned int field_88;
    char field_8c;
    char padding_8d[3];
    unsigned int field_90;
} st_47cdb0_0;

int sub_47cdb0(void)
{
    st_47cdb0_0 *v4;  // eax
    st_47cdb0_0 *v5;  // ebx
    int v14;  // eax
    int v15;  // eax
    int v16;  // eax
    int v17;  // eax
    st_47cdb0_0 *v18;  // edi
    int v19;  // ebx
    int v20;  // eax
    st_47cdb0_0 *v21;  // esi
    int v22;  // edi
    st_47cdb0_0 *v6;  // edi
    st_47cdb0_0 *v7;  // ebp
    int v8;  // eax
    int v9;  // eax
    int v10;  // eax
    int v11;  // edi
    int v12;  // eax
    int v13;  // ebp
    char v0;  // [bp-0x4], Other Possible Types: unsigned int
    int v1;  // [bp+0x4]
    int v2;  // [bp+0x8]
    unsigned int v3;  // [bp+0xc]

    v5 = &v4->field_40;
    if (v4->field_48 != 0xffffffff && v4->field_48)
        sub_438c40(v5);
    v6 = &v4->field_4c;
    *(unsigned int *)&v5->padding_0[8] = 0xffffffff;
    *(unsigned int *)&v5->padding_0[0] = 0x100;
    v5->padding_0[4] = 0;
    if (*(int *)&v6->padding_0[8] != 0xffffffff && *(int *)&v6->padding_0[8])
        sub_438c40(v6);
    v7 = &v4->field_58;
    *(unsigned int *)&v6->padding_0[8] = 0xffffffff;
    *(unsigned int *)&v6->padding_0[0] = 0x100;
    v6->padding_0[4] = 0;
    if (*(int *)&v7->padding_0[8] != 0xffffffff && *(int *)&v7->padding_0[8])
        sub_438c40(v7);
    *(unsigned int *)&v7->padding_0[8] = 0xffffffff;
    *(unsigned int *)&v7->padding_0[0] = 0x100;
    v7->padding_0[4] = 0;
    v8 = "palfx.time";
    if (!v3)
        v8 = "time";
    if (!sub_438b20(sub_433410(v1, v8), &v4->padding_0[4], v2, 0, 1))
        return;
    v9 = "palfx.mul";
    if (!v3)
        v9 = "mul";
    v10 = sub_433410(v1, v9);
    if (v10)
    {
        v11 = v2;
        if (sub_488ff0(v10, "iii", v11, &v0, v5, v6, v7) != 3)
        {
            return;
        }
        else if (*(char *)v0)
        {
            return;
        }
    }
    else
    {
        v11 = v2;
    }
    v12 = "palfx.add";
    if (!v3)
        v12 = "add";
    v13 = v1;
    v14 = sub_433410(v13, v12);
    if (v14)
    {
        if (sub_488ff0(v14, "iii", v11, &v0, v4->padding_1c, &v4->padding_1c[12], &v4->padding_1c[24]) != 3)
        {
            return;
        }
        else if (*(char *)v0)
        {
            return;
        }
    }
    v15 = "palfx.sinadd";
    if (!v3)
        v15 = "sinadd";
    v16 = sub_433410(v13, v15);
    if (v16)
    {
        if (sub_488ff0(v16, "iiii", v11, &v0, v4->padding_64, &v4->padding_64[12], &v4->padding_64[24], v4 + 1) - 3 > 1)
        {
            return;
        }
        else if (*(char *)v0)
        {
            return;
        }
    }
    v17 = "palfx.invertall";
    if (!v3)
        v17 = "invertall";
    v18 = &v4->field_88;
    v19 = sub_433410(v13, v17);
    if (*(int *)&v18->padding_0[8] != 0xffffffff && *(int *)&v18->padding_0[8])
        sub_438c40(v18);
    *(unsigned int *)&v18->padding_0[8] = 0xffffffff;
    *(unsigned int *)&v18->padding_0[0] = 0;
    v18->padding_0[4] = 0;
    if (v19 && !sub_438b20(v19, v18, v2, 0, 1))
        return;
    v20 = "palfx.color";
    if (!v3)
        v20 = "color";
    v21 = &v4->field_10;
    v22 = sub_433410(v13, v20);
    if (*(int *)&v21->padding_0[8] != 0xffffffff && *(int *)&v21->padding_0[8])
        sub_438c40(v21);
    *(unsigned int *)&v21->padding_0[8] = 0xffffffff;
    *(unsigned int *)&v21->padding_0[0] = 0x100;
    v21->padding_0[4] = 0;
    if (!v22)
        return;
    return;
}
