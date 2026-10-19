// helper_params entry=0x481ec0
extern unsigned int g_52b40c;

int sub_481ec0(void)
{
    int v9;  // ebp
    int v10;  // esi
    unsigned int *v16;  // esi
    int v17;  // eax
    unsigned int *v18;  // esi
    int v19;  // eax
    unsigned int v21;  // ecx
    int v23;  // eax
    int v24;  // eax
    unsigned int *v11;  // eax
    int v12;  // ebx
    int v13;  // eax
    unsigned int v14;  // esi
    unsigned int v15;  // eax
    int v0;  // [bp-0x20], Other Possible Types: unsigned int
    int v1;  // [bp-0x1c]
    int v2;  // [bp-0x18]
    int v3;  // [bp-0x14]
    int v4;  // [bp-0x10]
    char v5;  // [bp-0x8], Other Possible Types: unsigned int
    int v6;  // [bp-0x4]
    int v7;  // [bp+0x4]
    int v8;  // [bp+0x8]

    v9 = v7;
    v10 = sub_433410(v9, "helpertype", v1, v2, v3, v4);
    if (v10)
    {
        if (!_stricmp(v10, "normal"))
        {
            v11[15] = 0;
        }
        else if (!_stricmp(v10, "player"))
        {
            v11[15] = 1;
        }
        else if (sub_40e400(4))
        {
            v11[15] = 2;
        }
        else
        {
            v0 = v10;
            sprintf(g_52b40c + 73820, "Illegal helper type: %s");
            return;
        }
    }
    if (sub_433410(v9, "name") && !sub_4442b0())
        return;
    v12 = v8;
    if (!sub_438b20(sub_433410(v9, "id"), v11 + 12, v12, 0, 1))
        return;
    v13 = sub_433410(v9, "pos");
    if (v13)
    {
        if (!sub_488ff0(v13, "ii", v12, &v5, v11 + 16, v11 + 19))
        {
            return;
        }
        else if (*(char *)v5)
        {
            return;
        }
    }
    v14 = sub_433410(v9, "postype");
    if (v14)
    {
        v15 = sub_489510();
        v11[22] = v15;
        if (v15 == 0xffffffff)
        {
            v0 = v14;
            sprintf(g_52b40c + 73820, "Illegal postype: %s");
            return;
        }
    }
    v16 = v11 + 23;
    v17 = sub_433410(v9, "facing");
    v6 = v17;
    if (v16[2] != 0xffffffff && v16[2])
    {
        sub_438c40(v16);
        v17 = v6;
    }
    v16[2] = 0xffffffff;
    *v16 = 1;
    *(char *)&v16[1] = 0;
    if (v17 && !sub_438b20(v17, v16, v12, 0, 1))
        return;
    v18 = v11 + 26;
    v19 = sub_433410(v9, "stateno");
    v6 = v19;
    if (v18[2] != 0xffffffff && v18[2])
    {
        sub_438c40(v18);
        v19 = v6;
    }
    v18[2] = 0xffffffff;
    *v18 = 0;
    *(char *)&v18[1] = 0;
    if (v19 && !sub_438b20(v19, v18, v12, 0, 1))
        return;
    if (!sub_488f40(v12, 0))
    {
        return;
    }
    else if (!sub_488f40(v12, 0))
    {
        return;
    }
    else if (!sub_488f40(v12, 0))
    {
        return;
    }
    else if (!sub_488f40(v12, 0))
    {
        return;
    }
    else
    {
        v0 = v21;
        if (!sub_488fa0(v12, -3.4028234663852886e+38f))
            return;
        v0 = v21;
        if (!sub_488fa0(v12, -3.4028234663852886e+38f))
        {
            return;
        }
        else if (!sub_488f40(v12, -0x80000000))
        {
            return;
        }
        else if (!sub_488f40(v12, -0x80000000))
        {
            return;
        }
        else if (!sub_488f40(v12, -0x80000000))
        {
            return;
        }
        else if (!sub_488f40(v12, -0x80000000))
        {
            return;
        }
        else if (!sub_488f40(v12, -0x80000000))
        {
            return;
        }
        else if (!sub_488f40(v12, -0x80000000))
        {
            return;
        }
        else
        {
            sub_438d20(-0x80000000);
            sub_438d20(-0x80000000);
            sub_438d20(-0x80000000);
            sub_438d20(-0x80000000);
            v23 = sub_433410(v9, "size.head.pos");
            if (v23)
            {
                if (!sub_488ff0(v23, "ii", v12, &v5, v11 + 65, v11 + 0x44))
                {
                    return;
                }
                else if (*(char *)v5)
                {
                    return;
                }
            }
            v24 = sub_433410(v9, "size.mid.pos");
            if (v24)
            {
                if (!sub_488ff0(v24, "ii", v12, &v5, v11 + 71, v11 + 74))
                {
                    return;
                }
                else if (*(char *)v5)
                {
                    return;
                }
            }
            sub_488f40(v12, -0x80000000);
            return;
        }
    }
}
