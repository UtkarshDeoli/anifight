// contains 0x47d050; entry=0x47d050
typedef struct st_47d050_0 {
    char padding_0[69140];
    char field_10e14;
    char padding_10e15[3];
    int field_10e18;
    int field_10e1c;
    unsigned int field_10e20;
    unsigned int field_10e24;
    unsigned int field_10e28;
    int field_10e2c;
    int field_10e30;
    int field_10e34;
    unsigned int field_10e38;
    unsigned int field_10e3c;
    unsigned int field_10e40;
    unsigned int field_10e44;
    unsigned int field_10e48;
    char padding_10e4c[4];
    char field_10e50;
    char padding_10e51[87];
    int field_10ea8;
} st_47d050_0;

extern st_47d050_0 *g_52b40c;

int sub_47d050(void)
{
    unsigned int ptr;  // ebx
    int v2;  // eax
    int *v3;  // edi
    int v4;  // eax
    int v5;  // eax
    int v6;  // 4101
    unsigned int *v0;  // [bp+0x4]

    if (*v0 == 1)
    {
        ptr = &g_52b40c->field_10e14;
    }
    else if (*v0 == 2)
    {
        ptr = &g_52b40c->field_10e50;
    }
    memset(ptr, 0, 60);
    v2 = sub_439260(1);
    *(int *)(ptr + 4) = v2;
    if (v2 < -0x1)
    {
        *(int *)(ptr + 4) = 0;
        if (v3)
            sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, v3 + 8, v3[1], v3[787], "invalid PalFX time");
    }
    v4 = sub_439260(1);
    *(int *)(ptr + 8) = v4;
    if ((v4 < 0 || v4 > 0x100) && v3)
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, v3 + 8, v3[1], v3[787], "invalid PalFX color value");
    *(unsigned int *)(ptr + 12) = sub_439260(1);
    *(unsigned int *)(ptr + 16) = sub_439260(1);
    *(unsigned int *)(ptr + 20) = sub_439260(1);
    *(unsigned int *)(ptr + 24) = sub_439260(1);
    *(unsigned int *)(ptr + 28) = sub_439260(1);
    v5 = sub_439260(1);
    v6 = *(int *)(ptr + 24);
    *(int *)(ptr + 32) = v5;
    if ((v6 < 0 || *(int *)(ptr + 28) < 0 || v5 < 0) && v3)
        sub_40acd0("%-4i: Warning: Player %s (%i) in state %i: %s\n", g_52b40c->field_10ea8, v3 + 8, v3[1], v3[787], "invalid PalFX mul value");
    *(unsigned int *)(ptr + 36) = sub_439260(1);
    *(unsigned int *)(ptr + 40) = sub_439260(1);
    *(unsigned int *)(ptr + 44) = sub_439260(1);
    *(unsigned int *)(ptr + 52) = sub_439260(1);
    *(unsigned int *)(ptr + 48) = sub_439260(1);
    return;
}
