// contains 0x414320; entry=0x414320
typedef struct st_414320_0 {
    int field_0;
    unsigned int field_4;
    unsigned int field_8;
    unsigned int field_c;
    unsigned int field_10;
    unsigned int field_14;
    unsigned int field_18;
    char padding_1c[4];
    unsigned int field_20;
} st_414320_0;

int sub_414320(void)
{
    st_414320_0 *v12;  // eax
    int v13;  // ebp
    int v22;  // esi
    char *v23;  // edx
    int v24;  // ecx
    char *v26;  // esi
    unsigned short *node;  // ebx
    unsigned int v28;  // edx
    unsigned int v29;  // eax
    unsigned int v30;  // ecx
    int v31;  // eax
    unsigned int v14;  // edx
    int v32;  // edx
    int v33;  // ecx
    char *v34;  // edx
    char *v35;  // ebx
    unsigned int v36;  // eax
    unsigned int v37;  // ecx
    unsigned int v38;  // edx
    unsigned int v39;  // esi
    int v40;  // eax
    int v41;  // ecx
    unsigned int v15;  // ecx
    int v42;  // edx
    char *v43;  // esi
    char *v44;  // esi
    int v46;  // eax
    char *v47;  // esi
    int v48;  // edx
    int v49;  // ecx
    char *v16;  // ecx
    unsigned int v17;  // edi
    char *i;  // edx
    int v20;  // eax
    char *v21;  // edx
    unsigned int v0;  // [bp-0x24]
    unsigned int v1;  // [bp-0x20]
    unsigned int v2;  // [bp-0x1c]
    unsigned int v3;  // [bp-0x18]
    unsigned int v4;  // [bp-0x14]
    unsigned int v5;  // [bp-0x10]
    char *k;  // [bp-0xc], Other Possible Types: unsigned int
    unsigned int v7;  // [bp-0xc]
    unsigned int v8;  // [bp-0x8]
    int v9;  // [bp-0x4]
    unsigned short *iter;  // [bp+0x4], Other Possible Types: unsigned int
    unsigned int v11;  // [bp+0x4]

    v13 = v12->field_0;
    v0 = v12->field_4;
    v1 = v12->field_8;
    v2 = v12->field_c;
    v14 = v12->field_10;
    v16 = v15 * 4 + 4;
    v17 = (0 < v12->field_20 ? 0xff : 0);
    k = v16;
    v3 = v14;
    v4 = v12->field_14;
    v5 = v12->field_18;
    if (v14 == 0x100 && v12->field_14 == 0x100 && v12->field_18 == 0x100 && v13 == 0x100)
    {
        i = NULL;
        if (v16 <= 0)
            return;
        do
        {
            v20 = (*(char *)(i + v34) ^ v17) + v0;
            v21 = i + 1;
            if (v20 < 0)
            {
                v20 = 0;
            }
            else if (v20 > 0xff)
            {
                v20 = 0xff;
            }
            v22 = (*(char *)(v21 + v34) ^ v17) + v1;
            v23 = v21 + 1;
            if (v22 < 0)
            {
                v22 = 0;
            }
            else if (v22 > 0xff)
            {
                v22 = 0xff;
            }
            v24 = (*(char *)(v23 + v34) ^ v17) + v2;
            if (v24 < 0)
            {
                v24 = 0;
            }
            else if (v24 > 0xff)
            {
                v24 = 0xff;
            }
            *iter = (((v20 & 0xfff8) * 32 | v22) & 0xfffc) * 8 | v24 >> 3;
            i = v23 + 2;
            iter += 1;
        } while (i < k);
        return;
    }
    if (!v13)
    {
        if (v16 > 0)
        {
            v26 = v34 + 1;
            node = iter;
            iter = (v16 - 1 >> 2) + 1;
            do
            {
                v11 = iter;
                v28 = *(v26 - 1);
                v29 = *v26;
                v30 = (int)((v26[1] + v28 + v29 * 2) * 4 - v29 + v28) >> 4 ^ v17;
                v31 = (int)(v3 * (v30 + v0)) >> 8;
                if (v31 < 0)
                {
                    v31 = 0;
                }
                else if (v31 > 0xff)
                {
                    v31 = 0xff;
                }
                v32 = (int)(v4 * (v1 + v30)) >> 8;
                if (v32 < 0)
                {
                    v32 = 0;
                }
                else if (v32 > 0xff)
                {
                    v32 = 0xff;
                }
                v33 = (int)(v5 * (v30 + v2)) >> 8;
                if (v33 < 0)
                {
                    v33 = 0;
                }
                else if (v33 > 0xff)
                {
                    v33 = 0xff;
                }
                *node = (((v31 & 0xfff8) * 32 | v32) & 0xfffc) * 8 | v33 >> 3;
                node += 1;
                v26 += 4;
                iter = v11 - 1;
            } while (iter);
            return;
        }
        else
        {
            return;
        }
    }
    else
    {
        if (v13 < 0x100)
        {
            v8 = 0x100 - v13;
            if (v16 <= 0)
                return;
            v35 = v34 + 1;
            k = (v16 - 1 >> 2) + 1;
            do
            {
                v7 = k;
                v36 = *(v35 - 1);
                v37 = *v35;
                v38 = v35[1];
                v39 = v8 * ((int)((v36 + v37 * 2 + v38) * 4 - v37 + v36) >> 4);
                v40 = (int)(v3 * (((int)(v13 * v36 + v39) >> 8 ^ v17) + v0)) >> 8;
                if (v40 < 0)
                {
                    v40 = 0;
                }
                else if (v40 > 0xff)
                {
                    v40 = 0xff;
                }
                v41 = (int)(v4 * (((int)(v13 * *v35 + v39) >> 8 ^ v17) + v1)) >> 8;
                if (v41 < 0)
                {
                    v41 = 0;
                }
                else if (v41 > 0xff)
                {
                    v41 = 0xff;
                }
                v13 = v9;
                v42 = (int)(v5 * (((int)(v13 * v38 + v39) >> 8 ^ v17) + v2)) >> 8;
                if (v42 < 0)
                {
                    v42 = 0;
                }
                else if (v42 > 0xff)
                {
                    v42 = 0xff;
                }
                *iter = (((v40 & 0xfff8) * 32 | v41) & 0xfffc) * 8 | v42 >> 3;
                v35 += 4;
                k = v7 - 1;
                iter += 1;
            } while (k);
            return;
        }
        else
        {
            v43 = NULL;
            if (v16 <= 0)
                return;
            while (1)
            {
                v44 = v43 + 1;
                v46 = (int)(v14 * ((*(char *)(v43 + v34) ^ v17) + v0)) >> 8;
                if (v46 < 0)
                {
                    v46 = 0;
                }
                else if (v46 > 0xff)
                {
                    v46 = 0xff;
                }
                v47 = v44 + 1;
                v48 = (int)(v4 * ((*(char *)(v44 + v34) ^ v17) + v1)) >> 8;
                if (v48 < 0)
                {
                    v48 = 0;
                }
                else if (v48 > 0xff)
                {
                    v48 = 0xff;
                }
                v49 = (int)(v5 * ((*(char *)(v47 + v34) ^ v17) + v2)) >> 8;
                if (v49 < 0)
                {
                    v49 = 0;
                }
                else if (v49 > 0xff)
                {
                    v49 = 0xff;
                }
                *iter = (((v46 & 0xfff8) * 32 | v48) & 0xfffc) * 8 | v49 >> 3;
                v43 = v47 + 2;
                iter += 1;
                if (v43 >= k)
                    break;
                v14 = v3;
            }
            return;
        }
    }
}
