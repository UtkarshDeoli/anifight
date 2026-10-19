// superpause_params entry=0x4824b0
typedef struct st_4824b0_0 {
    unsigned int field_0;
    char field_4;
    char padding_5[3];
    unsigned int field_8;
    unsigned int field_c;
    char field_10;
    char padding_11[3];
    unsigned int field_14;
    unsigned int field_18;
    char field_1c;
    char padding_1d[3];
    unsigned int field_20;
    unsigned int field_24;
    char field_28;
    char padding_29[3];
    unsigned int field_2c;
} st_4824b0_0;

int sub_4824b0(void)
{
    int v5;  // ebx
    int v6;  // edi
    st_4824b0_0 *v7;  // eax
    st_4824b0_0 *v8;  // edi
    int v9;  // ebp
    st_4824b0_0 *v10;  // edi
    int v11;  // ebp
    st_4824b0_0 *v12;  // esi
    int v13;  // edi
    int v0;  // [bp-0x10]
    int v1;  // [bp-0xc]
    int v2;  // [bp-0x8]
    int v3;  // [bp-0x4]
    int v4;  // [bp+0x4]

    v6 = sub_433410(v5, "time", v0, v1, v2, v3);
    if (v7->field_8 != 0xffffffff && v7->field_8)
        sub_438c40(v7);
    v7->field_8 = 0xffffffff;
    v7->field_0 = 0;
    v7->field_4 = 0;
    if (v6 && !sub_438b20(v6, v7, v4, 0, 1))
        return;
    v8 = &v7->field_c;
    v9 = sub_433410(v5, "movetime");
    if (v8->field_8 != 0xffffffff && v8->field_8)
        sub_438c40(v8);
    v8->field_8 = 0xffffffff;
    v8->field_0 = 0;
    v8->field_4 = 0;
    if (v9 && !sub_438b20(v9, v8, v4, 0, 1))
        return;
    v10 = &v7->field_18;
    v11 = sub_433410(v5, "endcmdbuftime");
    if (v10->field_8 != 0xffffffff && v10->field_8)
        sub_438c40(v10);
    v10->field_8 = 0xffffffff;
    v10->field_0 = 0;
    v10->field_4 = 0;
    if (v11 && !sub_438b20(v11, v10, v4, 0, 1))
        return;
    v12 = &v7->field_24;
    v13 = sub_433410(v5, "pausebg");
    if (v12->field_8 != 0xffffffff && v12->field_8)
        sub_438c40(v12);
    v12->field_8 = 0xffffffff;
    v12->field_0 = 1;
    v12->field_4 = 0;
    if (!v13)
        return;
    return;
}
