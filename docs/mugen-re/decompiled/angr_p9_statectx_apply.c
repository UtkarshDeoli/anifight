// pass9 statectx_apply entry=0x48bb50 (asked 0x48bb50)
void sub_48bb50(void)
{
    unsigned int *v1;  // esi
    unsigned int *v2;  // edi
    unsigned int v3;  // ecx

    v1[783] = v2[1];
    v1[782] = *v2;
    v3 = v1[620];
    v1[785] = v2[3];
    v1[784] = v2[2];
    if (v3 != v2[2])
    {
        memset(v1 + 621, 0, 128);
        v1[620] = v2[2];
    }
    return;
}
