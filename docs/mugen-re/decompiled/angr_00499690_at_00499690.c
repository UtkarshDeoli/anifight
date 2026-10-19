// contains 0x499690; entry=0x499690
extern unsigned int g_52b414;

int sub_499690(double a0)
{
    double v4;  // mm7
    int v5;  // edx
    double v6;  // mm0
    float v0;  // [bp-0x24]
    long long v1;  // [bp-0x14]
    double v2;  // [bp-0xc], Other Possible Types: float

    if (g_52b414)
    {
        v2 = a0;
        return (int)v2;
    }
    v2 = (float)a0;
    v1 = (long long)a0;
    v4 = (double)v1;
    v5 = *(int *)&v2;
    if (!(unsigned int)v1)
    {
        v5 = *(unsigned int *)((void*)&v1 + 4);
        if (!(v5 & 0x7fffffff))
        {
            v2 = (float)v4;
            v2 = (float)a0;
            return v1;
        }
    }
    v6 = a0 - v4;
    if (v5 >= 0)
    {
        v0 = (float)v6;
        return (unsigned int)v1 - (*(unsigned int *)&v0 + 0x7fffffff < *(unsigned int *)&v0);
    }
    v0 = (float)v6;
    return (unsigned int)v1 + ((*(unsigned int *)&v0 ^ 0x80000000) + 0x7fffffff < (*(unsigned int *)&v0 ^ 0x80000000));
}
