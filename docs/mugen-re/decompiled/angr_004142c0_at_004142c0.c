// contains 0x4142c0; entry=0x4142c0
typedef struct st_4142c0_0 {
    char field_0;
    char field_1;
    char field_2;
} st_4142c0_0;

int sub_4142c0(void)
{
    unsigned int v3;  // ecx
    unsigned int i;  // ecx
    unsigned int v5;  // ebx
    unsigned int v6;  // edi
    unsigned short *iter;  // esi
    st_4142c0_0 *v8;  // eax
    unsigned int v0;  // [bp-0xc]
    unsigned int v1;  // [bp-0x8]

    i = v3 + 1;
    if (v3 == 0xffffffff)
        return;
    v1 = v5;
    v0 = v6;
    do
    {
        *iter = (((v8->field_0 & 0xfff8) * 32 | v8->field_1) & 0xfffc) * 8 | v8->field_2 >> 3;
        i -= 1;
        v8 = &v8[1].field_1;
        iter += 1;
    } while (i);
    return;
}
