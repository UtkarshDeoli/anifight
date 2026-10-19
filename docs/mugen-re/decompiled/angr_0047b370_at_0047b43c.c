// contains 0x47b43c; entry=0x47b370
typedef struct st_47b370_0 {
    char padding_0[16];
    int field_10;
    char padding_14[76];
    void* field_60;
} st_47b370_0;


int sub_47b370(void)
{
    st_47b370_0 *v3;  // eax
    int v4;  // eax
    void* v5;  // esi
    int v0;  // [bp-0x8]
    int v1;  // [bp-0x4]

    sub_438ca0(v0, v1);
    sub_438c40(&v3->padding_14[4]);
    sub_438c40(&v3->padding_14[16]);
    sub_438c40(&v3->padding_14[28]);
    sub_438c40(&v3->padding_14[40]);
    sub_438c40(&v3->padding_14[52]);
    sub_438c40(&v3->padding_14[64]);
    v4 = v3->field_10;
    if (v4 <= 300)
    {
        if (v4 != 300)
        {
            switch (v4)
            {
            case 34:
                sub_47d7c0();
                break;
            case 37: case 38:
                sub_480350();
                break;
            case 39:
                sub_481d30();
                break;
            case 120:
                v5 = v3->field_60;
                sub_438c40(v5 + 4);
                sub_438c40(v5 + 16);
                sub_438c40(v5 + 28);
                break;
            case 200:
                sub_47c590();
                break;
            case 210:
                sub_47c0e0();
                break;
            case 212:
                sub_4827a0();
                break;
            case 214:
                sub_482a10();
                break;
            case 216:
                sub_47d260();
                break;
            case 220: case 223:
                sub_47cc60();
                break;
            }
        }
        else
        {
            sub_4823a0();
        }
    }
    if (v3->field_60)
        free(v3->field_60);
    return;
}
