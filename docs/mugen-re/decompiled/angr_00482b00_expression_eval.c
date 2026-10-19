// expression_eval entry=0x482b00
typedef struct st_482b00_1 {
    char field_0;
    char field_1;
    char field_2;
} st_482b00_1;

typedef struct st_482b00_0 {
    unsigned int field_0;
} st_482b00_0;

typedef struct st_482b00_2 {
    char field_0;
} st_482b00_2;

extern unsigned int g_49d25c[4];
extern unsigned int g_49d268[4];
extern st_482b00_1 *g_49d278;
extern unsigned int g_49d27c[4];
extern st_482b00_1 *g_49d698;
extern unsigned int g_52b0d8;
extern unsigned int g_52b40c;
extern char g_52be60;

int sub_482b00(char *a0, unsigned int a1, st_482b00_0 **a2, int a3, unsigned int a4, st_482b00_2 **a5)
{
    unsigned int v34;  // ebx
    char *v35;  // esi
    unsigned int v44;  // eax
    char v134;  // 4101
    unsigned int v135;  // cc_dep1
    unsigned int v136;  // cc_dep2
    char v137;  // 4103
    unsigned int v138;  // eax
    unsigned int v139;  // 4111
    char *v140;  // ecx
    char *node;  // eax
    char v142;  // 4101
    unsigned int v143;  // cc_dep1
    unsigned int v45;  // eax
    unsigned int v144;  // cc_dep2
    char v145;  // 4103
    unsigned int v146;  // eax
    unsigned int v147;  // 4111
    char *v148;  // ecx
    char *iter;  // eax
    char v150;  // 4101
    unsigned int v151;  // cc_dep1
    unsigned int v152;  // cc_dep2
    char v153;  // 4103
    unsigned int v46;  // ebp
    unsigned int v154;  // eax
    unsigned int v155;  // 4111
    char *v156;  // ecx
    char *iter1;  // eax
    char v158;  // 4101
    unsigned int v159;  // cc_dep1
    unsigned int v160;  // cc_dep2
    char v161;  // 4103
    unsigned int v162;  // eax
    unsigned int v163;  // 4111
    char *v47;  // esi
    char *v164;  // ecx
    char *iter2;  // eax
    char v166;  // 4101
    unsigned int v167;  // cc_dep1
    unsigned int v168;  // cc_dep2
    char v169;  // 4103
    unsigned int v170;  // eax
    unsigned int v171;  // 4111
    char *v172;  // ecx
    char *v173;  // eax
    char *v48;  // esi
    char v174;  // 4101
    unsigned int v175;  // cc_dep1
    unsigned int v176;  // cc_dep2
    char v177;  // 4103
    unsigned int v178;  // eax
    unsigned int v179;  // 4111
    char *v180;  // ecx
    char *v181;  // eax
    char v182;  // 4101
    unsigned int v183;  // cc_dep1
    unsigned int *v49;  // eax
    unsigned int v184;  // cc_dep2
    char v185;  // 4103
    unsigned int v186;  // eax
    unsigned int v187;  // 4111
    char *v188;  // ecx
    char *v189;  // eax
    char v190;  // 4101
    unsigned int v191;  // cc_dep1
    unsigned int v192;  // cc_dep2
    char v193;  // 4103
    unsigned int v50;  // esi
    unsigned int v194;  // eax
    unsigned int v195;  // 4111
    char *v196;  // ecx
    char *v197;  // eax
    char v198;  // 4101
    unsigned int v199;  // cc_dep1
    unsigned int v200;  // cc_dep2
    char v201;  // 4103
    unsigned int v202;  // eax
    unsigned int v203;  // 4111
    unsigned int idx;  // edx
    char *v204;  // ecx
    char *v205;  // eax
    char v206;  // 4101
    unsigned int v207;  // cc_dep1
    unsigned int v208;  // cc_dep2
    char v209;  // 4103
    unsigned int v210;  // eax
    unsigned int v211;  // 4111
    char *v212;  // ecx
    char *v213;  // eax
    unsigned int *v52;  // ecx
    char v214;  // 4101
    unsigned int v215;  // cc_dep1
    unsigned int v216;  // cc_dep2
    char v217;  // 4103
    unsigned int v218;  // eax
    unsigned int v219;  // 4111
    char *v220;  // ecx
    char *v221;  // eax
    char v222;  // 4101
    unsigned int v223;  // cc_dep1
    unsigned int index;  // edx
    unsigned int v224;  // cc_dep2
    char v225;  // 4103
    unsigned int v226;  // eax
    unsigned int v227;  // 4111
    char *v228;  // ecx
    char *v229;  // eax
    char v230;  // 4101
    unsigned int v231;  // cc_dep1
    unsigned int v232;  // cc_dep2
    char v233;  // 4103
    char *v36;  // ebp
    unsigned int *v54;  // ecx
    unsigned int v234;  // eax
    unsigned int v235;  // 4111
    char *v236;  // ecx
    char *v237;  // eax
    char v238;  // 4101
    unsigned int v239;  // cc_dep1
    unsigned int v240;  // cc_dep2
    char v241;  // 4103
    unsigned int v242;  // eax
    unsigned int v243;  // 4111
    unsigned int v55;  // eax
    char *v244;  // ecx
    char *v245;  // eax
    char v246;  // 4101
    unsigned int v247;  // cc_dep1
    unsigned int v248;  // cc_dep2
    char v249;  // 4103
    unsigned int v250;  // eax
    unsigned int v251;  // 4111
    char *v252;  // ecx
    char *v253;  // eax
    unsigned int *v56;  // eax
    char v254;  // 4101
    unsigned int v255;  // cc_dep1
    unsigned int v256;  // cc_dep2
    char v257;  // 4103
    unsigned int v258;  // eax
    unsigned int v259;  // 4111
    char *v260;  // ecx
    char *v261;  // eax
    char v262;  // 4101
    unsigned int v263;  // cc_dep1
    unsigned int v57;  // ebx
    unsigned int v264;  // cc_dep2
    char v265;  // 4103
    unsigned int v266;  // eax
    unsigned int v267;  // 4111
    char *v268;  // ecx
    char *v269;  // eax
    char v270;  // 4101
    unsigned int v271;  // cc_dep1
    unsigned int v272;  // cc_dep2
    char v273;  // 4103
    char *v58;  // ecx
    unsigned int v274;  // eax
    unsigned int v275;  // 4111
    char *v276;  // ecx
    char *v277;  // eax
    char v278;  // 4101
    unsigned int v279;  // cc_dep1
    unsigned int v280;  // cc_dep2
    char v281;  // 4103
    unsigned int v282;  // eax
    unsigned int v283;  // 4111
    unsigned int v59;  // edi
    char *v284;  // ecx
    char *v285;  // eax
    char v286;  // 4101
    unsigned int v287;  // cc_dep1
    unsigned int v288;  // cc_dep2
    char v289;  // 4103
    unsigned int v290;  // eax
    unsigned int v291;  // 4111
    char *v292;  // ecx
    char *v293;  // eax
    unsigned int v60;  // esi
    char v294;  // 4101
    unsigned int v295;  // cc_dep1
    unsigned int v296;  // cc_dep2
    char v297;  // 4103
    unsigned int v298;  // eax
    unsigned int v299;  // 4111
    char *v300;  // ecx
    char *v301;  // eax
    char v302;  // 4101
    unsigned int v303;  // cc_dep1
    unsigned int v61;  // eax
    unsigned int v304;  // cc_dep2
    char v305;  // 4103
    unsigned int v306;  // eax
    unsigned int v307;  // 4111
    char *v308;  // ecx
    char *v309;  // eax
    char v310;  // 4101
    unsigned int v311;  // cc_dep1
    unsigned int v312;  // cc_dep2
    char v313;  // 4103
    unsigned int v62;  // eax
    unsigned int v314;  // eax
    unsigned int v315;  // 4111
    char *v316;  // ecx
    char *v317;  // eax
    char v318;  // 4101
    unsigned int v319;  // cc_dep1
    unsigned int v320;  // cc_dep2
    char v321;  // 4103
    unsigned int v322;  // eax
    unsigned int v323;  // 4111
    unsigned int v63;  // eax
    char *v324;  // ecx
    char *v325;  // eax
    char v326;  // 4101
    unsigned int v327;  // cc_dep1
    unsigned int v328;  // cc_dep2
    char v329;  // 4103
    unsigned int v330;  // eax
    unsigned int v331;  // 4111
    char *v332;  // ecx
    char *v333;  // eax
    char *v37;  // esi
    int v64;  // edi
    char v334;  // 4101
    unsigned int v335;  // cc_dep1
    unsigned int v336;  // cc_dep2
    char v337;  // 4103
    unsigned int v338;  // eax
    unsigned int v339;  // 4111
    char *v340;  // ecx
    char *v341;  // eax
    char v342;  // 4101
    unsigned int v343;  // cc_dep1
    char *v65;  // ebp
    unsigned int v344;  // cc_dep2
    char v345;  // 4103
    unsigned int v346;  // eax
    unsigned int v347;  // 4111
    char *v348;  // ecx
    char *v349;  // eax
    char v350;  // 4101
    unsigned int v351;  // cc_dep1
    unsigned int v352;  // cc_dep2
    char v353;  // 4103
    int v66;  // ebx
    unsigned int v354;  // eax
    unsigned int v355;  // 4111
    char *v356;  // ecx
    char *v357;  // eax
    char v358;  // 4101
    unsigned int v359;  // cc_dep1
    unsigned int v360;  // cc_dep2
    char v361;  // 4103
    unsigned int v362;  // eax
    unsigned int v363;  // 4111
    char *v67;  // esi
    char *v364;  // esi
    unsigned int v365;  // eax
    char *v366;  // edi
    unsigned int i;  // esi
    unsigned int v368;  // eax
    unsigned int v369;  // edi
    st_482b00_1 **v370;  // esi
    char *v371;  // ecx
    char *v372;  // eax
    char v373;  // 4101
    int v68;  // ebx
    unsigned int v374;  // cc_dep1
    unsigned int v375;  // cc_dep2
    char v376;  // 4103
    unsigned int v377;  // eax
    unsigned int v378;  // 4111
    char *v379;  // esi
    unsigned int v380;  // eax
    char *v381;  // edi
    unsigned int j;  // esi
    unsigned int v383;  // eax
    char *v69;  // esi
    char *v384;  // eax
    char *v385;  // ecx
    char v386;  // 4101
    unsigned int v387;  // cc_dep1
    unsigned int v388;  // cc_dep2
    char v389;  // 4103
    unsigned int v390;  // eax
    unsigned int v391;  // 4111
    char *v392;  // ecx
    void* v393;  // eax
    unsigned int v70;  // eax
    char v394;  // 4101
    unsigned int v395;  // cc_dep1
    unsigned int v396;  // cc_dep2
    char v397;  // 4103
    unsigned int v398;  // eax
    unsigned int v399;  // 4111
    char *v400;  // ecx
    void* v401;  // eax
    char v402;  // 4101
    unsigned int v403;  // cc_dep1
    char *v71;  // edi
    unsigned int v404;  // cc_dep2
    char v405;  // 4103
    unsigned int v406;  // eax
    unsigned int v407;  // 4111
    char *v408;  // edi
    char v409;  // al
    char *v410;  // ebp
    unsigned int v411;  // eax
    int num;  // eax
    unsigned int *v413;  // ecx
    unsigned int v72;  // esi
    unsigned int v414;  // eax
    unsigned int v415;  // eax
    unsigned int v416;  // eax
    char v417;  // al
    char *v418;  // ebp
    unsigned int v419;  // eax
    unsigned int v420;  // eax
    unsigned int v421;  // eax
    char *v422;  // esi
    unsigned int *v423;  // eax
    unsigned int v73;  // eax
    char *v424;  // edi
    char v425;  // al
    char *v426;  // ebp
    unsigned int v427;  // eax
    char v428;  // al
    char *v429;  // ebp
    unsigned int v430;  // eax
    char *v431;  // ecx
    char *v432;  // ebp
    char v433;  // al
    unsigned int v38;  // eax
    unsigned int v74;  // eax
    char *v434;  // esi
    char *v435;  // esi
    unsigned int n;  // edi
    char *v437;  // ecx
    char v438;  // al
    char *v439;  // ebp
    char *v440;  // ebx
    char *v441;  // edi
    char *v442;  // esi
    unsigned int n1;  // esi
    char *v75;  // ecx
    char *v444;  // ecx
    char *v445;  // eax
    char v446;  // 4101
    unsigned int v447;  // cc_dep1
    unsigned int v448;  // cc_dep2
    char v449;  // 4103
    unsigned int v450;  // eax
    unsigned int v451;  // 4111
    char *v452;  // ecx
    char *v453;  // eax
    char *v76;  // eax
    char v454;  // 4101
    unsigned int v455;  // cc_dep1
    unsigned int v456;  // cc_dep2
    char v457;  // 4103
    unsigned int v458;  // eax
    unsigned int v459;  // 4111
    char *v460;  // ecx
    char *v461;  // eax
    char v462;  // 4101
    unsigned int v463;  // cc_dep1
    char v77;  // 4101
    unsigned int v464;  // cc_dep2
    char v465;  // 4103
    unsigned int v466;  // eax
    unsigned int v467;  // 4111
    char v468;  // al
    char *v469;  // ebp
    char v470;  // al
    char *v471;  // ebp
    unsigned int v472;  // eax
    unsigned int v473;  // eax
    unsigned int v78;  // cc_dep1
    unsigned int v474;  // eax
    char v475;  // al
    char *v476;  // ebp
    unsigned int v477;  // eax
    unsigned int v478;  // eax
    unsigned int n2;  // ebp
    unsigned int *ptr;  // eax
    unsigned int v481;  // ecx
    unsigned int *k;  // esi
    unsigned int *v483;  // edi
    unsigned int v79;  // cc_dep2
    char v80;  // 4103
    unsigned int v81;  // eax
    unsigned int v82;  // 4111
    unsigned int v83;  // edi
    char *v39;  // ebx
    char *v84;  // ecx
    char *v85;  // eax
    char v86;  // 4101
    unsigned int v87;  // cc_dep1
    unsigned int v88;  // cc_dep2
    char v89;  // 4103
    unsigned int v90;  // eax
    unsigned int v91;  // 4111
    char *v92;  // ecx
    char *v93;  // eax
    unsigned int v40;  // eax
    char v94;  // 4101
    unsigned int v95;  // cc_dep1
    unsigned int v96;  // cc_dep2
    char v97;  // 4103
    unsigned int v98;  // eax
    unsigned int v99;  // 4111
    char *v100;  // ecx
    char *v101;  // eax
    char v102;  // 4101
    unsigned int v103;  // cc_dep1
    unsigned int v41;  // eax
    unsigned int v104;  // cc_dep2
    char v105;  // 4103
    unsigned int v106;  // eax
    unsigned int v107;  // 4111
    char *v108;  // ecx
    char *v109;  // eax
    char v110;  // 4101
    unsigned int v111;  // cc_dep1
    unsigned int v112;  // cc_dep2
    char v113;  // 4103
    unsigned int v114;  // eax
    unsigned int v115;  // 4111
    char *v116;  // ecx
    char *v117;  // eax
    char v118;  // 4101
    unsigned int v119;  // cc_dep1
    unsigned int v120;  // cc_dep2
    char v121;  // 4103
    unsigned int v122;  // eax
    unsigned int v123;  // 4111
    char *v43;  // ebx
    char *v124;  // ecx
    char *v125;  // eax
    char v126;  // 4101
    unsigned int v127;  // cc_dep1
    unsigned int v128;  // cc_dep2
    char v129;  // 4103
    unsigned int v130;  // eax
    unsigned int v131;  // 4111
    char *v132;  // ecx
    char *v133;  // eax
    unsigned int v0;  // [bp-0x6f0]
    char *v1;  // [bp-0x6ec]
    st_482b00_2 **v2;  // [bp-0x6e8], Other Possible Types: char *, int
    int v3;  // [bp-0x6e4]
    int v4;  // [bp-0x6e0]
    int v5;  // [bp-0x6dc]
    int v6;  // [bp-0x6d8]
    char v7;  // [bp-0x6d0], Other Possible Types: unsigned long long
    unsigned int v8;  // [bp-0x6c8]
    unsigned int *v9;  // [bp-0x6c4]
    unsigned int v10;  // [bp-0x6c0]
    unsigned int v11;  // [bp-0x6bc]
    unsigned int v12;  // [bp-0x6b8]
    unsigned int v13;  // [bp-0x6b4]
    unsigned int v14;  // [bp-0x6b0]
    int num2;  // [bp-0x6ac]
    unsigned int num1;  // [bp-0x6a8]
    char *ptr1;  // [bp-0x69c]
    char v18;  // [bp-0x698]
    char v19;  // [bp-0x694], Other Possible Types: unsigned int
    unsigned int v20;  // [bp-0x690]
    char *v21;  // [bp-0x68c], Other Possible Types: unsigned int
    unsigned int v22;  // [bp-0x688]
    char v23;  // [bp-0x684]
    char v24;  // [bp-0x681]
    char v25;  // [bp-0x680]
    char v26;  // [bp-0x67d]
    char v27;  // [bp-0x679]
    unsigned short v28;  // [bp-0x584]
    char v29[256];  // [bp-0x485]
    char v30[256];  // [bp-0x484]
    char v31;  // [bp-0x384]
    unsigned short v32;  // [bp-0x304]
    char v33;  // [bp-0x204]

    v34 = 0;
    v22 = 0;
    memset(&v7, 0, 56, v3, v4, v5, v6);
    v35 = a0;
    v36 = NULL;
    v20 = 0;
    v37 = v35;
    while (1)
    {
        v18 = *v37;
        if (!isalnum(*v37) && v18 != 95)
            break;
        *(&v23 - v35 + v37) = *v37;
        v34 += 1;
        v36 += 1;
        v37 += 1;
    }
    v20 = v34;
    (&v23)[v34] = 0;
    while (1)
    {
        v38 = *(char *)(a0 + v36);
        if (v38 - 1 > 126 || !isspace(v38))
            break;
        v36 += 1;
    }
    v10 = 0;
    if (!_stricmp(&v23, "parent"))
    {
        v39 = a0;
        v10 = 1;
        goto LABEL_482d92;
    }
    if (!_stricmp(&v23, "root"))
    {
        v39 = a0;
        v10 = 2;
        goto LABEL_482d92;
    }
    else
    {
        if (!_stricmp(&v23, "helper"))
        {
            v40 = v8;
            v10 = 3;
            if (v40 != 0xffffffff && v40)
            {
                sub_438c40(&v7);
                goto LABEL_482c25;
            }
        }
        if (!_stricmp(&v23, "target"))
        {
            v41 = v8;
            v10 = 4;
            if (v41 != 0xffffffff && v41)
                sub_438c40(&v7);
            v8 = 0xffffffff;
            *(unsigned int *)&v7 = 0xffffffff;
            *((char *)&v7 + 4) = 0;
        }
        else if (!_stricmp(&v23, "partner"))
        {
            v10 = 5;
            if (v8 != 0xffffffff && v8)
            {
                sub_438c40(&v7);
                v8 = 0xffffffff;
                *(unsigned int *)&v7 = 0;
                *((char *)&v7 + 4) = 0;
                goto LABEL_482d1f;
            }
LABEL_482c25:
            v8 = 0xffffffff;
            *(unsigned int *)&v7 = 0;
            *((char *)&v7 + 4) = 0;
        }
        else
        {
            if (!_stricmp(&v23, "enemy"))
            {
                v10 = 6;
                v2 = 0;
            }
            else if (!_stricmp(&v23, "enemynear"))
            {
                v10 = 7;
                v2 = 0;
            }
            else if (!_stricmp(&v23, "playerid"))
            {
                v10 = 8;
                v2 = -0x1;
            }
            else
            {
                v39 = a0;
LABEL_482d87:
                if (!v10)
                    goto LABEL_482e30;
LABEL_482d92:
                v43 = v39 + v36;
                while (1)
                {
                    v44 = *v43;
                    if (v44 - 1 > 126 || !isspace(v44))
                        break;
                    v43 += 1;
                }
                if (*v43 != 44)
                    return 0;
                do
                {
                    v45 = v43[1];
                    v43 += 1;
                } while (v45 - 1 <= 126 && isspace(v45));
                a0 = v43;
                v46 = 0;
                v47 = v43;
                v21 = NULL;
                v20 = 0;
                v48 = v47;
                while (1)
                {
                    v18 = *v48;
                    if (!isalnum(*v48) && v18 != 95)
                        break;
                    v21 += 1;
                    *(v48 + &v23 - v47) = *v48;
                    v46 += 1;
                    v48 += 1;
                }
                v39 = a0;
                v20 = v46;
                (&v23)[v46] = 0;
                v36 = v21;
LABEL_482e30:
                v49 = _stricmp(&v23, "time");
                if (v49 && (v49 = _stricmp(&v23, "statetime"), v49))
                {
                    if (!_stricmp(&v23, "animtime"))
                    {
                        v49 = 0x1;
                    }
                    else if (!_stricmp(&v23, "animelem"))
                    {
                        v49 = 0x2;
                    }
                    else if (!_stricmp(&v23, "anim"))
                    {
                        v49 = 0x3;
                    }
                    else if (!_stricmp(&v23, "vel"))
                    {
                        v49 = 0x6;
                    }
                    else if (!_stricmp(&v23, "pos"))
                    {
                        v49 = 0x4;
                    }
                    else if (!_stricmp(&v23, "screenpos"))
                    {
                        v49 = 0x5;
                    }
                    else if (!_stricmp(&v23, "timemod"))
                    {
                        v49 = 0x7;
                    }
                    else if (!_stricmp(&v23, "p2dist"))
                    {
                        v49 = 0x8;
                    }
                    else if (!_stricmp(&v23, "p2bodydist"))
                    {
                        v49 = 0x9;
                    }
                    else if (!_stricmp(&v23, "p2life"))
                    {
                        v49 = 0xa;
                    }
                    else if (!_stricmp(&v23, "winperfect"))
                    {
                        v49 = 0x6c;
                    }
                    else if (sub_40e400(3))
                    {
                        v49 = 0x6d;
                    }
                    else if (sub_40e400(4))
                    {
                        v49 = 0x6e;
                    }
                    else if (!_stricmp(&v23, "drawgame"))
                    {
                        v49 = 0x6f;
                    }
                    else if (!_stricmp(&v23, "matchover"))
                    {
                        v49 = 0x70;
                    }
                    else if (!_stricmp(&v23, "gametime"))
                    {
                        v49 = 0x71;
                    }
                    else if (!_stricmp(&v23, "roundno"))
                    {
                        v49 = 0x72;
                    }
                    else if (!_stricmp(&v23, "roundstate"))
                    {
                        v49 = 0x73;
                    }
                    else if (!_stricmp(&v23, "matchno"))
                    {
                        v49 = 0x74;
                    }
                    else if (!_stricmp(&v23, "roundsexisted"))
                    {
                        v49 = 0x75;
                    }
                    else if (!_stricmp(&v23, "teamside"))
                    {
                        v49 = 0x78;
                    }
                    else if (!_stricmp(&v23, "teammode"))
                    {
                        v49 = 0x79;
                    }
                    else if (!_stricmp(&v23, "ishometeam"))
                    {
                        v49 = 0x7a;
                    }
                    else if (!_stricmp(&v23, "frontedgedist"))
                    {
                        v49 = 0xb;
                    }
                    else if (!_stricmp(&v23, "backedgedist"))
                    {
                        v49 = 0xc;
                    }
                    else if (!_stricmp(&v23, "frontedgebodydist"))
                    {
                        v49 = 0xb;
                        num2 = 1;
                    }
                    else if (!_stricmp(&v23, "backedgebodydist"))
                    {
                        v49 = 0xc;
                        num2 = 1;
                    }
                    else if (!_stricmp(&v23, "life"))
                    {
                        v49 = 0xf;
                    }
                    else if (!_stricmp(&v23, "lifemax"))
                    {
                        v49 = 0x10;
                    }
                    else if (!_stricmp(&v23, "power"))
                    {
                        v49 = 0x11;
                    }
                    else if (!_stricmp(&v23, "powermax"))
                    {
                        v49 = 0x12;
                    }
                    else if (!_stricmp(&v23, "alive"))
                    {
                        v49 = 0x13;
                    }
                    else if (!_stricmp(&v23, "command"))
                    {
                        v49 = 0x14;
                    }
                    else if (!_stricmp(&v23, "ailevel"))
                    {
                        v49 = 0x19;
                    }
                    else if (!_stricmp(&v23, "hitcount"))
                    {
                        v49 = 0x1e;
                    }
                    else if (!_stricmp(&v23, "uniqhitcount"))
                    {
                        v49 = 0x1f;
                    }
                    else if (!_stricmp(&v23, "movehit"))
                    {
                        v49 = 0x20;
                    }
                    else if (!_stricmp(&v23, "moveguarded"))
                    {
                        v49 = 0x21;
                    }
                    else if (!_stricmp(&v23, "movecontact"))
                    {
                        v49 = 0x22;
                    }
                    else if (!_stricmp(&v23, "movereversed"))
                    {
                        v49 = 0x23;
                    }
                    else if (!_stricmp(&v23, "inguarddist"))
                    {
                        v49 = 0x24;
                    }
                    else if (!_stricmp(&v23, "stateno"))
                    {
                        v49 = 0x28;
                    }
                    else if (!_stricmp(&v23, "prevstateno"))
                    {
                        v49 = 0x29;
                    }
                    else if (!_stricmp(&v23, "p2stateno"))
                    {
                        v49 = 0x2a;
                    }
                    else if (!_stricmp(&v23, "statetype"))
                    {
                        v49 = 0x2b;
                    }
                    else if (!_stricmp(&v23, "p2statetype"))
                    {
                        v49 = 0x2c;
                    }
                    else if (!_stricmp(&v23, "movetype"))
                    {
                        v49 = 0x2d;
                    }
                    else if (!_stricmp(&v23, "p2movetype"))
                    {
                        v49 = 0x2e;
                    }
                    else if (!_stricmp(&v23, "ctrl"))
                    {
                        v49 = 0x2f;
                    }
                    else if (!_stricmp(&v23, "hitdefattr"))
                    {
                        v49 = 0x30;
                    }
                    else if (!_stricmp(&v23, "var"))
                    {
                        v49 = 0x157;
                    }
                    else if (!_stricmp(&v23, "fvar"))
                    {
                        v49 = 0x158;
                    }
                    else if (!_stricmp(&v23, "sysvar"))
                    {
                        v49 = 0x159;
                    }
                    else if (!_stricmp(&v23, "sysfvar"))
                    {
                        v49 = 0x15a;
                    }
                    else if (!_stricmp(&v23, "random"))
                    {
                        v49 = 0x33;
                    }
                    else if (!_stricmp(&v23, "numproj"))
                    {
                        v49 = 0x3c;
                    }
                    else if (!_stricmp(&v23, "numprojid"))
                    {
                        v49 = 0x154;
                    }
                    else if (!_stricmp(&v23, "projcanceltime"))
                    {
                        v49 = 0x15b;
                    }
                    else if (!_stricmp(&v23, "projhittime"))
                    {
                        v49 = 0x151;
                    }
                    else if (sub_40e400(7))
                    {
                        v49 = 0x3e;
                    }
                    else if (!_stricmp(&v23, "projguardedtime"))
                    {
                        v49 = 0x152;
                    }
                    else if (sub_40e400(11))
                    {
                        v49 = 0x3f;
                    }
                    else if (!_stricmp(&v23, "projcontacttime"))
                    {
                        v49 = 0x153;
                    }
                    else if (sub_40e400(11))
                    {
                        v49 = 0x40;
                    }
                    else if (!_stricmp(&v23, "p1name"))
                    {
                        v49 = 0x5a;
                    }
                    else if (!_stricmp(&v23, "name"))
                    {
                        v49 = 0x5a;
                    }
                    else if (!_stricmp(&v23, "p2name"))
                    {
                        v49 = 0x5b;
                    }
                    else if (!_stricmp(&v23, "p3name"))
                    {
                        v49 = 0x5c;
                    }
                    else if (!_stricmp(&v23, "p4name"))
                    {
                        v49 = 0x5d;
                    }
                    else if (!_stricmp(&v23, "authorname"))
                    {
                        v49 = 0x5e;
                    }
                    else if (!_stricmp(&v23, "numpartner"))
                    {
                        v49 = 0x5f;
                    }
                    else if (!_stricmp(&v23, "numenemy"))
                    {
                        v49 = 0x60;
                    }
                    else if (!_stricmp(&v23, "id"))
                    {
                        v49 = 0x61;
                    }
                    else if (!_stricmp(&v23, "facing"))
                    {
                        v49 = 0x63;
                    }
                    else if (!_stricmp(&v23, "hitfall"))
                    {
                        v49 = 0x64;
                    }
                    else if (!_stricmp(&v23, "hitshakeover"))
                    {
                        v49 = 0x65;
                    }
                    else if (!_stricmp(&v23, "hitover"))
                    {
                        v49 = 0x66;
                    }
                    else if (!_stricmp(&v23, "hitvel"))
                    {
                        v49 = 0x67;
                    }
                    else if (!_stricmp(&v23, "hitpausetime"))
                    {
                        v49 = 0x68;
                    }
                    else if (!_stricmp(&v23, "canrecover"))
                    {
                        v49 = 0x69;
                    }
                    else if (!_stricmp(&v23, "animelemno"))
                    {
                        v49 = 0x15c;
                    }
                    else if (!_stricmp(&v23, "animexist"))
                    {
                        v49 = 0x14c;
                    }
                    else if (!_stricmp(&v23, "selfanimexist"))
                    {
                        v49 = 0x14d;
                    }
                    else if (!_stricmp(&v23, "playeridexist"))
                    {
                        v49 = 0x14e;
                    }
                    else if (!_stricmp(&v23, "palno"))
                    {
                        v49 = 0x6b;
                    }
                    else if (!_stricmp(&v23, "numexplod"))
                    {
                        v49 = 0x96;
                    }
                    else if (!_stricmp(&v23, "numtarget"))
                    {
                        v49 = 0xa0;
                    }
                    else if (!_stricmp(&v23, "gethitvar"))
                    {
                        v49 = 0xb4;
                    }
                    else if (!_stricmp(&v23, "const"))
                    {
                        v49 = 0xb5;
                    }
                    else if (!_stricmp(&v23, "const240p"))
                    {
                        v49 = 0x15d;
                    }
                    else if (!_stricmp(&v23, "const480p"))
                    {
                        v49 = 0x15e;
                    }
                    else if (!_stricmp(&v23, "const720p"))
                    {
                        v49 = 0x15f;
                    }
                    else if (!_stricmp(&v23, "stagevar"))
                    {
                        v49 = 0xb6;
                    }
                    else if (!_stricmp(&v23, "tickspersecond"))
                    {
                        v49 = 0xbe;
                    }
                    else if (!_stricmp(&v23, "ishelper"))
                    {
                        v49 = 0xc9;
                    }
                    else if (!_stricmp(&v23, "numhelper"))
                    {
                        v49 = 0xca;
                    }
                    else if (!_stricmp(&v23, "parentdist"))
                    {
                        v49 = 0xcb;
                    }
                    else if (!_stricmp(&v23, "rootdist"))
                    {
                        v49 = 0xcc;
                    }
                    else if (!_stricmp(&v23, "standby"))
                    {
                        v49 = 0xcd;
                    }
                    else if (!_stricmp(&v23, "gamewidth"))
                    {
                        v49 = 0xd2;
                    }
                    else if (!_stricmp(&v23, "gameheight"))
                    {
                        v49 = 0xd3;
                    }
                    else if (!_stricmp(&v23, "pi"))
                    {
                        v49 = 0x12c;
                    }
                    else if (!_stricmp(&v23, "e"))
                    {
                        v49 = 0x12d;
                    }
                    else if (!_stricmp(&v23, "ifelse"))
                    {
                        v49 = 0x131;
                    }
                    else if (!_stricmp(&v23, "log"))
                    {
                        v49 = 0x136;
                    }
                    else if (!_stricmp(&v23, "sin"))
                    {
                        v49 = 0x140;
                    }
                    else if (!_stricmp(&v23, "cos"))
                    {
                        v49 = 0x141;
                    }
                    else if (!_stricmp(&v23, "tan"))
                    {
                        v49 = 0x142;
                    }
                    else if (!_stricmp(&v23, "asin"))
                    {
                        v49 = 0x143;
                    }
                    else if (!_stricmp(&v23, "acos"))
                    {
                        v49 = 0x144;
                    }
                    else if (!_stricmp(&v23, "atan"))
                    {
                        v49 = 0x145;
                    }
                    else if (!_stricmp(&v23, "exp"))
                    {
                        v49 = 0x146;
                    }
                    else if (!_stricmp(&v23, "ln"))
                    {
                        v49 = 0x147;
                    }
                    else if (!_stricmp(&v23, "abs"))
                    {
                        v49 = 0x148;
                    }
                    else if (!_stricmp(&v23, "floor"))
                    {
                        v49 = 0x149;
                    }
                    else if (!_stricmp(&v23, "ceil"))
                    {
                        v49 = 0x14a;
                    }
                    else if (!_stricmp(&v23, "animelemtime"))
                    {
                        v49 = 0x14b;
                    }
                    else if (!_stricmp(&v23, "cond"))
                    {
                        v49 = 0x190;
                    }
                    else
                    {
                        v2 = &v23;
                        v1 = "Invalid trigger: %s";
LABEL_4859cb:
                        v0 = g_52b40c + 73820;
LABEL_4859d8:
                        sprintf(v0, v1);
                        return 0;
                    }
                }
                v50 = 0;
                v9 = v49;
                idx = 0;
                v52 = 0x19;
                do
                {
                    if (v49 == v52)
                    {
                        v50 = 0x1000000;
                        break;
                    }
                } while ((v52 = g_49d25c[idx], idx += 1, v52 != 0xffffffff));
                index = 0;
                v54 = 0x15;
                do
                {
                    if (v49 == v54)
                    {
                        v50 = 0x1010000;
                        break;
                    }
                } while ((v54 = g_49d268[index], index += 1, v54 != 0xffffffff));
                if ((unsigned int)*(int *)(a1 + 184) < v50)
                {
                    v2 = &v23;
                    v1 = "Trigger not valid for mugenversion of character: %s";
                    goto LABEL_4859cb;
                }
                while (1)
                {
                    v55 = *(char *)(v39 + v36);
                    if (v55 - 1 > 126 || !isspace(v55))
                        break;
                    v36 += 1;
                }
                v56 = v9;
                v57 = atoi;
                if (v56 == 0xc9)
                {
                    v58 = a0;
                    if (*(char *)(v58 + v36) != 40)
                        goto LABEL_484fbb;
                    v9 = 0x14f;
                }
                else if (v56 == 0xca)
                {
                    v58 = a0;
                    if (*(char *)(v58 + v36) != 40)
                        goto LABEL_484fbb;
                    v9 = 0x150;
                }
                else if (v56 == 0x96)
                {
                    v58 = a0;
                    if (*(char *)(v58 + v36) != 40)
                        goto LABEL_484fbb;
                    v9 = 0x155;
                }
                else if (v56 == 0xa0)
                {
                    v58 = a0;
                    if (*(char *)(v58 + v36) != 40)
                        goto LABEL_484fbb;
                    v9 = 0x156;
                }
                else if (v56 >= 0x140 && v56 <= 0x15f)
                {
                    v58 = a0;
                }
                else
                {
                    v59 = a4;
                    v60 = v22;
LABEL_483d83:
                    if (v56 >= 0x136 && v56 <= 0x136)
                    {
                        while (1)
                        {
                            v61 = *(char *)(a0 + v36);
                            if (v61 - 1 > 126 || !isspace(v61))
                                break;
                            v36 += 1;
                        }
                        if (*(char *)(a0 + v36) != 40)
                        {
                            return 0;
                        }
                        else if (sub_489360(a0 + v36 + 1, "nn:)", a1, &v19, &(&v33)[4 * v60], &(&v31)[v60], &v20, v59 - v60) != 2)
                        {
                            return 0;
                        }
                        else if (*(char *)v19 == 41)
                        {
                            v60 += v20;
                            v36 = v19 - a0 + 1;
                            v56 = v9;
                            v22 = v60;
                        }
                        else
                        {
                            return 0;
                        }
                    }
                    if (v56 >= 0x131 && v56 <= 0x131)
                    {
                        while (1)
                        {
                            v62 = *(char *)(a0 + v36);
                            if (v62 - 1 > 126 || !isspace(v62))
                                break;
                            v36 += 1;
                        }
                        if (*(char *)(a0 + v36) != 40)
                        {
                            return 0;
                        }
                        else if (sub_489360(a0 + v36 + 1, "nnn:)", a1, &v19, &(&v33)[4 * v60], &(&v31)[v60], &v20, v59 - v60) != 3)
                        {
                            return 0;
                        }
                        else if (*(char *)v19 == 41)
                        {
                            v36 = v19 - a0 + 1;
                            v56 = v9;
                            v22 = v60 + v20;
                        }
                        else
                        {
                            return 0;
                        }
                    }
                    if (v56 == 0x190)
                    {
                        while (1)
                        {
                            v63 = *(char *)(a0 + v36);
                            if (v63 - 1 > 126 || !isspace(v63))
                                break;
                            v36 += 1;
                        }
                        if (*(char *)(a0 + v36) != 40)
                            return 0;
                        v64 = sub_4891b0(a1, &v19, &v21);
                        if (v64 == 3)
                        {
                            if (*(char *)v19 == 41)
                            {
                                v36 = v19 - a0 + 1;
                                v56 = v9;
                                v13 = 3;
                                ptr1 = v21;
                                v22 = 0;
                                goto LABEL_483f8f;
                            }
                        }
                        else
                        {
                            if (v64 <= 0)
                                return 0;
                        }
                        v65 = v21;
                        v66 = v64;
                        v67 = v65;
                        do
                        {
                            v68 = v66;
                            sub_438c40(v67);
                        } while ((v67 += 12, v66 = v68 - 1, v68 != 1));
                        if (v64 <= 0)
                            return 0;
                        free(v65);
                        return 0;
                    }
LABEL_483f8f:
                    if (v56 == 0x3e)
                    {
                        num2 = atoi(&v26);
                        v56 = v9;
                    }
                    if (v56 == 0x3f || v56 == 0x40)
                    {
                        num2 = atoi(&v27);
                        v56 = v9;
                    }
                    if (v56 == 0x6d)
                    {
                        if (sub_40e400(2))
                        {
                            num2 = 1;
                            goto LABEL_483fea;
                        }
                        if (sub_40e400(2))
                        {
                            num2 = 3;
                            goto LABEL_483fea;
                        }
                        else if (!v24)
                        {
                            num2 = -0x1;
LABEL_483fea:
                            v56 = v9;
                            goto LABEL_483fee;
                        }
                        else
                        {
                            v2 = &v23;
                            v1 = "Invalid win trigger: %s";
                            v0 = g_52b40c + 73820;
                            goto LABEL_4859d8;
                        }
                    }
                    else
                    {
LABEL_483fee:
                        if (v56 == 0x6e)
                        {
                            if (sub_40e400(2))
                            {
                                num2 = 1;
                                goto LABEL_484016;
                            }
                            if (sub_40e400(2))
                            {
                                num2 = 3;
                                goto LABEL_484016;
                            }
                            else if (!v25)
                            {
                                num2 = -0x1;
LABEL_484016:
                                v56 = v9;
                                goto LABEL_48401a;
                            }
                            else
                            {
                                v2 = &v23;
                                v1 = "Invalid lose trigger: %s";
                                goto LABEL_4859cb;
                            }
                            goto LABEL_4859cb;
                        }
LABEL_48401a:
                        if (v56 == 0xb4)
                        {
                            v69 = a0;
                            if (*(char *)(v69 + v36) != 40)
                            {
                                v2 = &v23;
                                v1 = "Need to specify gethitvar(varname): %s";
                                v0 = g_52b40c + 73820;
                                goto LABEL_4859d8;
                            }
                            else
                            {
                                do
                                {
                                    v70 = *(char *)(v69 + v36 + 1);
                                    v36 += 1;
                                } while (v70 - 1 <= 126 && isspace(v70));
                                v71 = a0;
                                v72 = 0;
                                while (1)
                                {
                                    v57 = _INSERT(v57, 0, *(char *)(v71 + v36));
                                    v73 = isalnum(*(char *)(v71 + v36));
                                    if (!v73 && (char)v57 != 95 && *(char *)(v71 + v36) != 46)
                                        break;
                                    *((char *)&v28 + v72) = *(char *)(v71 + v36);
                                    v36 += 1;
                                    v72 += 1;
                                }
                                *((char *)&v28 + v72) = 0;
                                while (1)
                                {
                                    v74 = *(char *)(v71 + v36);
                                    if (v74 - 1 > 126 || !isspace(v74))
                                        break;
                                    v36 += 1;
                                }
                                if (*(char *)(v71 + v36) != 41)
                                    goto LABEL_4841af;
                                _strlwr(&v28);
                                v75 = "xveladd";
                                v76 = &v28;
                                while (1)
                                {
                                    v77 = *v76;
                                    v78 = v77;
                                    v79 = *v75;
                                    if (v77 != *v75)
                                        break;
                                    if (v77)
                                    {
                                        v80 = v76[1];
                                        v78 = v80;
                                        v79 = v75[1];
                                        if (v80 != v75[1])
                                            break;
                                        v76 += 2;
                                        v75 += 2;
                                        if (!v80)
                                            goto LABEL_484202;
                                    }
                                    else
                                    {
LABEL_484202:
                                        v81 = 0;
                                        goto LABEL_48420b;
                                    }
                                }
                                v82 = _ccall(4, v78, v79, 0);
                                v81 = -(v82 & 1) + 1 - (unsigned int)(v82 & 1 && 1);
LABEL_48420b:
                                v83 = g_52b40c;
                                if (!v81)
                                {
                                    v56 = v9;
                                    v13 = 0;
                                    v36 += 1;
                                    goto LABEL_484c95;
                                }
                                else
                                {
                                    v84 = "yveladd";
                                    v85 = &v28;
                                    while (1)
                                    {
                                        v86 = *v85;
                                        v87 = v86;
                                        v88 = *v84;
                                        if (v86 != *v84)
                                            break;
                                        if (v86)
                                        {
                                            v89 = v85[1];
                                            v87 = v89;
                                            v88 = v84[1];
                                            if (v89 != v84[1])
                                                break;
                                            v85 += 2;
                                            v84 += 2;
                                            if (!v89)
                                                goto LABEL_48424f;
                                        }
                                        else
                                        {
LABEL_48424f:
                                            v90 = 0;
                                            goto LABEL_484258;
                                        }
                                    }
                                    v91 = _ccall(4, v87, v88, 0);
                                    v90 = -(v91 & 1) + 1 - (unsigned int)(v91 & 1 && 1);
LABEL_484258:
                                    if (!v90)
                                    {
                                        v56 = v9;
                                        v13 = 1;
                                        v36 += 1;
                                        goto LABEL_484c95;
                                    }
                                    else
                                    {
                                        v92 = "type";
                                        v93 = &v28;
                                        while (1)
                                        {
                                            v94 = *v93;
                                            v95 = v94;
                                            v96 = *v92;
                                            if (v94 != *v92)
                                                break;
                                            if (v94)
                                            {
                                                v97 = v93[1];
                                                v95 = v97;
                                                v96 = v92[1];
                                                if (v97 != v92[1])
                                                    break;
                                                v93 += 2;
                                                v92 += 2;
                                                if (!v97)
                                                    goto LABEL_48429c;
                                            }
                                            else
                                            {
LABEL_48429c:
                                                v98 = 0;
                                                goto LABEL_4842a5;
                                            }
                                        }
                                        v99 = _ccall(4, v95, v96, 0);
                                        v98 = -(v99 & 1) + 1 - (unsigned int)(v99 & 1 && 1);
LABEL_4842a5:
                                        if (!v98)
                                        {
                                            v56 = v9;
                                            v13 = 2;
                                            v36 += 1;
                                            goto LABEL_484c95;
                                        }
                                        else
                                        {
                                            v100 = "animtype";
                                            v101 = &v28;
                                            while (1)
                                            {
                                                v102 = *v101;
                                                v103 = v102;
                                                v104 = *v100;
                                                if (v102 != *v100)
                                                    break;
                                                if (v102)
                                                {
                                                    v105 = v101[1];
                                                    v103 = v105;
                                                    v104 = v100[1];
                                                    if (v105 != v100[1])
                                                        break;
                                                    v101 += 2;
                                                    v100 += 2;
                                                    if (!v105)
                                                        goto LABEL_4842e3;
                                                }
                                                else
                                                {
LABEL_4842e3:
                                                    v106 = 0;
                                                    goto LABEL_4842ec;
                                                }
                                            }
                                            v107 = _ccall(4, v103, v104, 0);
                                            v106 = -(v107 & 1) + 1 - (unsigned int)(v107 & 1 && 1);
LABEL_4842ec:
                                            if (!v106)
                                            {
                                                v56 = v9;
                                                v13 = 3;
                                                v36 += 1;
                                                goto LABEL_484c95;
                                            }
                                            else
                                            {
                                                v108 = "airtype";
                                                v109 = &v28;
                                                v83 = g_52b40c;
                                                while (1)
                                                {
                                                    v110 = *v109;
                                                    v111 = v110;
                                                    v112 = *v108;
                                                    if (v110 != *v108)
                                                        break;
                                                    if (v110)
                                                    {
                                                        v113 = v109[1];
                                                        v111 = v113;
                                                        v112 = v108[1];
                                                        if (v113 != v108[1])
                                                            break;
                                                        v109 += 2;
                                                        v108 += 2;
                                                        if (!v113)
                                                            goto LABEL_48432c;
                                                    }
                                                    else
                                                    {
LABEL_48432c:
                                                        v114 = 0;
                                                        goto LABEL_484335;
                                                    }
                                                }
                                                v115 = _ccall(4, v111, v112, 0);
                                                v114 = -(v115 & 1) + 1 - (unsigned int)(v115 & 1 && 1);
LABEL_484335:
                                                if (!v114)
                                                {
                                                    v56 = v9;
                                                    v13 = 4;
                                                    v36 += 1;
                                                    goto LABEL_484c95;
                                                }
                                                else
                                                {
                                                    v116 = "groundtype";
                                                    v117 = &v28;
                                                    while (1)
                                                    {
                                                        v118 = *v117;
                                                        v119 = v118;
                                                        v120 = *v116;
                                                        if (v118 != *v116)
                                                            break;
                                                        if (v118)
                                                        {
                                                            v121 = v117[1];
                                                            v119 = v121;
                                                            v120 = v116[1];
                                                            if (v121 != v116[1])
                                                                break;
                                                            v117 += 2;
                                                            v116 += 2;
                                                            if (!v121)
                                                                goto LABEL_484373;
                                                        }
                                                        else
                                                        {
LABEL_484373:
                                                            v122 = 0;
                                                            goto LABEL_48437c;
                                                        }
                                                    }
                                                    v123 = _ccall(4, v119, v120, 0);
                                                    v122 = -(v123 & 1) + 1 - (unsigned int)(v123 & 1 && 1);
LABEL_48437c:
                                                    if (!v122)
                                                    {
                                                        v56 = v9;
                                                        v13 = 5;
                                                        v36 += 1;
                                                        goto LABEL_484c95;
                                                    }
                                                    else
                                                    {
                                                        v124 = "damage";
                                                        v125 = &v28;
                                                        v83 = g_52b40c;
                                                        while (1)
                                                        {
                                                            v126 = *v125;
                                                            v127 = v126;
                                                            v128 = *v124;
                                                            if (v126 != *v124)
                                                                break;
                                                            if (v126)
                                                            {
                                                                v129 = v125[1];
                                                                v127 = v129;
                                                                v128 = v124[1];
                                                                if (v129 != v124[1])
                                                                    break;
                                                                v125 += 2;
                                                                v124 += 2;
                                                                if (!v129)
                                                                    goto LABEL_4843bc;
                                                            }
                                                            else
                                                            {
LABEL_4843bc:
                                                                v130 = 0;
                                                                goto LABEL_4843c5;
                                                            }
                                                        }
                                                        v131 = _ccall(4, v127, v128, 0);
                                                        v130 = -(v131 & 1) + 1 - (unsigned int)(v131 & 1 && 1);
LABEL_4843c5:
                                                        if (!v130)
                                                        {
                                                            v56 = v9;
                                                            v13 = 6;
                                                            v36 += 1;
                                                            goto LABEL_484c95;
                                                        }
                                                        else
                                                        {
                                                            v132 = "hitcount";
                                                            v133 = &v28;
                                                            while (1)
                                                            {
                                                                v134 = *v133;
                                                                v135 = v134;
                                                                v136 = *v132;
                                                                if (v134 != *v132)
                                                                    break;
                                                                if (v134)
                                                                {
                                                                    v137 = v133[1];
                                                                    v135 = v137;
                                                                    v136 = v132[1];
                                                                    if (v137 != v132[1])
                                                                        break;
                                                                    v133 += 2;
                                                                    v132 += 2;
                                                                    if (!v137)
                                                                        goto LABEL_484403;
                                                                }
                                                                else
                                                                {
LABEL_484403:
                                                                    v138 = 0;
                                                                    goto LABEL_48440c;
                                                                }
                                                            }
                                                            v139 = _ccall(4, v135, v136, 0);
                                                            v138 = -(v139 & 1) + 1 - (unsigned int)(v139 & 1 && 1);
LABEL_48440c:
                                                            if (!v138)
                                                            {
                                                                v56 = v9;
                                                                v13 = 7;
                                                                v36 += 1;
                                                                goto LABEL_484c95;
                                                            }
                                                            else
                                                            {
                                                                v140 = "fallcount";
                                                                node = &v28;
                                                                v83 = g_52b40c;
                                                                while (1)
                                                                {
                                                                    v142 = *node;
                                                                    v143 = v142;
                                                                    v144 = *v140;
                                                                    if (v142 != *v140)
                                                                        break;
                                                                    if (v142)
                                                                    {
                                                                        v145 = node[1];
                                                                        v143 = v145;
                                                                        v144 = v140[1];
                                                                        if (v145 != v140[1])
                                                                            break;
                                                                        node += 2;
                                                                        v140 += 2;
                                                                        if (!v145)
                                                                            goto LABEL_48444c;
                                                                    }
                                                                    else
                                                                    {
LABEL_48444c:
                                                                        v146 = 0;
                                                                        goto LABEL_484455;
                                                                    }
                                                                }
                                                                v147 = _ccall(4, v143, v144, 0);
                                                                v146 = -(v147 & 1) + 1 - (unsigned int)(v147 & 1 && 1);
LABEL_484455:
                                                                if (!v146)
                                                                {
                                                                    v56 = v9;
                                                                    v13 = 8;
                                                                    v36 += 1;
                                                                    goto LABEL_484c95;
                                                                }
                                                                else
                                                                {
                                                                    v148 = "hitshaketime";
                                                                    iter = &v28;
                                                                    while (1)
                                                                    {
                                                                        v150 = *iter;
                                                                        v151 = v150;
                                                                        v152 = *v148;
                                                                        if (v150 != *v148)
                                                                            break;
                                                                        if (v150)
                                                                        {
                                                                            v153 = iter[1];
                                                                            v151 = v153;
                                                                            v152 = v148[1];
                                                                            if (v153 != v148[1])
                                                                                break;
                                                                            iter += 2;
                                                                            v148 += 2;
                                                                            if (!v153)
                                                                                goto LABEL_484493;
                                                                        }
                                                                        else
                                                                        {
LABEL_484493:
                                                                            v154 = 0;
                                                                            goto LABEL_48449c;
                                                                        }
                                                                    }
                                                                    v155 = _ccall(4, v151, v152, 0);
                                                                    v154 = -(v155 & 1) + 1 - (unsigned int)(v155 & 1 && 1);
LABEL_48449c:
                                                                    if (!v154)
                                                                    {
                                                                        v56 = v9;
                                                                        v13 = 9;
                                                                        v36 += 1;
                                                                        goto LABEL_484c95;
                                                                    }
                                                                    else
                                                                    {
                                                                        v156 = "hittime";
                                                                        iter1 = &v28;
                                                                        v83 = g_52b40c;
                                                                        while (1)
                                                                        {
                                                                            v158 = *iter1;
                                                                            v159 = v158;
                                                                            v160 = *v156;
                                                                            if (v158 != *v156)
                                                                                break;
                                                                            if (v158)
                                                                            {
                                                                                v161 = iter1[1];
                                                                                v159 = v161;
                                                                                v160 = v156[1];
                                                                                if (v161 != v156[1])
                                                                                    break;
                                                                                iter1 += 2;
                                                                                v156 += 2;
                                                                                if (!v161)
                                                                                    goto LABEL_4844dc;
                                                                            }
                                                                            else
                                                                            {
LABEL_4844dc:
                                                                                v162 = 0;
                                                                                goto LABEL_4844e5;
                                                                            }
                                                                        }
                                                                        v163 = _ccall(4, v159, v160, 0);
                                                                        v162 = -(v163 & 1) + 1 - (unsigned int)(v163 & 1 && 1);
LABEL_4844e5:
                                                                        if (!v162)
                                                                        {
                                                                            v56 = v9;
                                                                            v13 = 10;
                                                                            v36 += 1;
                                                                            goto LABEL_484c95;
                                                                        }
                                                                        else
                                                                        {
                                                                            v164 = "slidetime";
                                                                            iter2 = &v28;
                                                                            while (1)
                                                                            {
                                                                                v166 = *iter2;
                                                                                v167 = v166;
                                                                                v168 = *v164;
                                                                                if (v166 != *v164)
                                                                                    break;
                                                                                if (v166)
                                                                                {
                                                                                    v169 = iter2[1];
                                                                                    v167 = v169;
                                                                                    v168 = v164[1];
                                                                                    if (v169 != v164[1])
                                                                                        break;
                                                                                    iter2 += 2;
                                                                                    v164 += 2;
                                                                                    if (!v169)
                                                                                        goto LABEL_484523;
                                                                                }
                                                                                else
                                                                                {
LABEL_484523:
                                                                                    v170 = 0;
                                                                                    goto LABEL_48452c;
                                                                                }
                                                                            }
                                                                            v171 = _ccall(4, v167, v168, 0);
                                                                            v170 = -(v171 & 1) + 1 - (unsigned int)(v171 & 1 && 1);
LABEL_48452c:
                                                                            if (!v170)
                                                                            {
                                                                                v56 = v9;
                                                                                v13 = 11;
                                                                                v36 += 1;
                                                                                goto LABEL_484c95;
                                                                            }
                                                                            else
                                                                            {
                                                                                v172 = "ctrltime";
                                                                                v173 = &v28;
                                                                                v83 = g_52b40c;
                                                                                while (1)
                                                                                {
                                                                                    v174 = *v173;
                                                                                    v175 = v174;
                                                                                    v176 = *v172;
                                                                                    if (v174 != *v172)
                                                                                        break;
                                                                                    if (v174)
                                                                                    {
                                                                                        v177 = v173[1];
                                                                                        v175 = v177;
                                                                                        v176 = v172[1];
                                                                                        if (v177 != v172[1])
                                                                                            break;
                                                                                        v173 += 2;
                                                                                        v172 += 2;
                                                                                        if (!v177)
                                                                                            goto LABEL_48456c;
                                                                                    }
                                                                                    else
                                                                                    {
LABEL_48456c:
                                                                                        v178 = 0;
                                                                                        goto LABEL_484575;
                                                                                    }
                                                                                }
                                                                                v179 = _ccall(4, v175, v176, 0);
                                                                                v178 = -(v179 & 1) + 1 - (unsigned int)(v179 & 1 && 1);
LABEL_484575:
                                                                                if (!v178)
                                                                                {
                                                                                    v56 = v9;
                                                                                    v13 = 12;
                                                                                    v36 += 1;
                                                                                    goto LABEL_484c95;
                                                                                }
                                                                                else
                                                                                {
                                                                                    v180 = "recovertime";
                                                                                    v181 = &v28;
                                                                                    while (1)
                                                                                    {
                                                                                        v182 = *v181;
                                                                                        v183 = v182;
                                                                                        v184 = *v180;
                                                                                        if (v182 != *v180)
                                                                                            break;
                                                                                        if (v182)
                                                                                        {
                                                                                            v185 = v181[1];
                                                                                            v183 = v185;
                                                                                            v184 = v180[1];
                                                                                            if (v185 != v180[1])
                                                                                                break;
                                                                                            v181 += 2;
                                                                                            v180 += 2;
                                                                                            if (!v185)
                                                                                                goto LABEL_4845b3;
                                                                                        }
                                                                                        else
                                                                                        {
LABEL_4845b3:
                                                                                            v186 = 0;
                                                                                            goto LABEL_4845bc;
                                                                                        }
                                                                                    }
                                                                                    v187 = _ccall(4, v183, v184, 0);
                                                                                    v186 = -(v187 & 1) + 1 - (unsigned int)(v187 & 1 && 1);
LABEL_4845bc:
                                                                                    if (!v186)
                                                                                    {
                                                                                        v56 = v9;
                                                                                        v13 = 13;
                                                                                        v36 += 1;
                                                                                        goto LABEL_484c95;
                                                                                    }
                                                                                    else
                                                                                    {
                                                                                        v188 = "xoff";
                                                                                        v189 = &v28;
                                                                                        v83 = g_52b40c;
                                                                                        while (1)
                                                                                        {
                                                                                            v190 = *v189;
                                                                                            v191 = v190;
                                                                                            v192 = *v188;
                                                                                            if (v190 != *v188)
                                                                                                break;
                                                                                            if (v190)
                                                                                            {
                                                                                                v193 = v189[1];
                                                                                                v191 = v193;
                                                                                                v192 = v188[1];
                                                                                                if (v193 != v188[1])
                                                                                                    break;
                                                                                                v189 += 2;
                                                                                                v188 += 2;
                                                                                                if (!v193)
                                                                                                    goto LABEL_4845fc;
                                                                                            }
                                                                                            else
                                                                                            {
LABEL_4845fc:
                                                                                                v194 = 0;
                                                                                                goto LABEL_484605;
                                                                                            }
                                                                                        }
                                                                                        v195 = _ccall(4, v191, v192, 0);
                                                                                        v194 = -(v195 & 1) + 1 - (unsigned int)(v195 & 1 && 1);
LABEL_484605:
                                                                                        if (!v194)
                                                                                        {
                                                                                            v56 = v9;
                                                                                            v13 = 14;
                                                                                            v12 = 1;
                                                                                            v36 += 1;
                                                                                            goto LABEL_484c95;
                                                                                        }
                                                                                        else
                                                                                        {
                                                                                            v196 = "yoff";
                                                                                            v197 = &v28;
                                                                                            while (1)
                                                                                            {
                                                                                                v198 = *v197;
                                                                                                v199 = v198;
                                                                                                v200 = *v196;
                                                                                                if (v198 != *v196)
                                                                                                    break;
                                                                                                if (v198)
                                                                                                {
                                                                                                    v201 = v197[1];
                                                                                                    v199 = v201;
                                                                                                    v200 = v196[1];
                                                                                                    if (v201 != v196[1])
                                                                                                        break;
                                                                                                    v197 += 2;
                                                                                                    v196 += 2;
                                                                                                    if (!v201)
                                                                                                        goto LABEL_48464c;
                                                                                                }
                                                                                                else
                                                                                                {
LABEL_48464c:
                                                                                                    v202 = 0;
                                                                                                    goto LABEL_484655;
                                                                                                }
                                                                                            }
                                                                                            v203 = _ccall(4, v199, v200, 0);
                                                                                            v202 = -(v203 & 1) + 1 - (unsigned int)(v203 & 1 && 1);
LABEL_484655:
                                                                                            if (!v202)
                                                                                            {
                                                                                                v56 = v9;
                                                                                                v13 = 15;
                                                                                                v12 = 1;
                                                                                                v36 += 1;
                                                                                                goto LABEL_484c95;
                                                                                            }
                                                                                            else if ((unsigned int)*(int *)(a1 + 184) < (unsigned int)*(int *)(g_52b40c + 1032))
                                                                                            {
                                                                                                v204 = "zoff";
                                                                                                v205 = &v28;
                                                                                                while (1)
                                                                                                {
                                                                                                    v206 = *v205;
                                                                                                    v207 = v206;
                                                                                                    v208 = *v204;
                                                                                                    if (v206 != *v204)
                                                                                                        break;
                                                                                                    if (v206)
                                                                                                    {
                                                                                                        v209 = v205[1];
                                                                                                        v207 = v209;
                                                                                                        v208 = v204[1];
                                                                                                        if (v209 != v204[1])
                                                                                                            break;
                                                                                                        v205 += 2;
                                                                                                        v204 += 2;
                                                                                                        if (!v209)
                                                                                                            goto LABEL_4846b0;
                                                                                                    }
                                                                                                    else
                                                                                                    {
LABEL_4846b0:
                                                                                                        v210 = 0;
                                                                                                        goto LABEL_4846b9;
                                                                                                    }
                                                                                                }
                                                                                                v211 = _ccall(4, v207, v208, 0);
                                                                                                v210 = -(v211 & 1) + 1 - (unsigned int)(v211 & 1 && 1);
LABEL_4846b9:
                                                                                                if (v210)
                                                                                                    goto LABEL_4846d7;
                                                                                                v56 = v9;
                                                                                                v13 = 16;
                                                                                                v12 = 1;
                                                                                                v36 += 1;
                                                                                                goto LABEL_484c95;
                                                                                            }
                                                                                            else
                                                                                            {
LABEL_4846d7:
                                                                                                v212 = "xvel";
                                                                                                v213 = &v28;
                                                                                                while (1)
                                                                                                {
                                                                                                    v214 = *v213;
                                                                                                    v215 = v214;
                                                                                                    v216 = *v212;
                                                                                                    if (v214 != *v212)
                                                                                                        break;
                                                                                                    if (v214)
                                                                                                    {
                                                                                                        v217 = v213[1];
                                                                                                        v215 = v217;
                                                                                                        v216 = v212[1];
                                                                                                        if (v217 != v212[1])
                                                                                                            break;
                                                                                                        v213 += 2;
                                                                                                        v212 += 2;
                                                                                                        if (!v217)
                                                                                                            goto LABEL_4846ff;
                                                                                                    }
                                                                                                    else
                                                                                                    {
LABEL_4846ff:
                                                                                                        v218 = 0;
                                                                                                        goto LABEL_484708;
                                                                                                    }
                                                                                                }
                                                                                                v219 = _ccall(4, v215, v216, 0);
                                                                                                v218 = -(v219 & 1) + 1 - (unsigned int)(v219 & 1 && 1);
LABEL_484708:
                                                                                                if (!v218)
                                                                                                {
                                                                                                    v56 = v9;
                                                                                                    v13 = 0x11;
                                                                                                    v12 = 1;
                                                                                                    v36 += 1;
                                                                                                    goto LABEL_484c95;
                                                                                                }
                                                                                                else
                                                                                                {
                                                                                                    v220 = "yvel";
                                                                                                    v221 = &v28;
                                                                                                    while (1)
                                                                                                    {
                                                                                                        v222 = *v221;
                                                                                                        v223 = v222;
                                                                                                        v224 = *v220;
                                                                                                        if (v222 != *v220)
                                                                                                            break;
                                                                                                        if (v222)
                                                                                                        {
                                                                                                            v225 = v221[1];
                                                                                                            v223 = v225;
                                                                                                            v224 = v220[1];
                                                                                                            if (v225 != v220[1])
                                                                                                                break;
                                                                                                            v221 += 2;
                                                                                                            v220 += 2;
                                                                                                            if (!v225)
                                                                                                                goto LABEL_48474e;
                                                                                                        }
                                                                                                        else
                                                                                                        {
LABEL_48474e:
                                                                                                            v226 = 0;
                                                                                                            goto LABEL_484757;
                                                                                                        }
                                                                                                    }
                                                                                                    v227 = _ccall(4, v223, v224, 0);
                                                                                                    v226 = -(v227 & 1) + 1 - (unsigned int)(v227 & 1 && 1);
LABEL_484757:
                                                                                                    if (!v226)
                                                                                                    {
                                                                                                        v56 = v9;
                                                                                                        v13 = 18;
                                                                                                        v12 = 1;
                                                                                                        v36 += 1;
                                                                                                        goto LABEL_484c95;
                                                                                                    }
                                                                                                    else
                                                                                                    {
                                                                                                        v228 = "yaccel";
                                                                                                        v229 = &v28;
                                                                                                        while (1)
                                                                                                        {
                                                                                                            v230 = *v229;
                                                                                                            v231 = v230;
                                                                                                            v232 = *v228;
                                                                                                            if (v230 != *v228)
                                                                                                                break;
                                                                                                            if (v230)
                                                                                                            {
                                                                                                                v233 = v229[1];
                                                                                                                v231 = v233;
                                                                                                                v232 = v228[1];
                                                                                                                if (v233 != v228[1])
                                                                                                                    break;
                                                                                                                v229 += 2;
                                                                                                                v228 += 2;
                                                                                                                if (!v233)
                                                                                                                    goto LABEL_48479d;
                                                                                                            }
                                                                                                            else
                                                                                                            {
LABEL_48479d:
                                                                                                                v234 = 0;
                                                                                                                goto LABEL_4847a6;
                                                                                                            }
                                                                                                        }
                                                                                                        v235 = _ccall(4, v231, v232, 0);
                                                                                                        v234 = -(v235 & 1) + 1 - (unsigned int)(v235 & 1 && 1);
LABEL_4847a6:
                                                                                                        if (!v234)
                                                                                                        {
                                                                                                            v56 = v9;
                                                                                                            v13 = 19;
                                                                                                            v12 = 1;
                                                                                                            v36 += 1;
                                                                                                            goto LABEL_484c95;
                                                                                                        }
                                                                                                        else
                                                                                                        {
                                                                                                            v236 = "chainid";
                                                                                                            v237 = &v28;
                                                                                                            while (1)
                                                                                                            {
                                                                                                                v238 = *v237;
                                                                                                                v239 = v238;
                                                                                                                v240 = *v236;
                                                                                                                if (v238 != *v236)
                                                                                                                    break;
                                                                                                                if (v238)
                                                                                                                {
                                                                                                                    v241 = v237[1];
                                                                                                                    v239 = v241;
                                                                                                                    v240 = v236[1];
                                                                                                                    if (v241 != v236[1])
                                                                                                                        break;
                                                                                                                    v237 += 2;
                                                                                                                    v236 += 2;
                                                                                                                    if (!v241)
                                                                                                                        goto LABEL_4847ec;
                                                                                                                }
                                                                                                                else
                                                                                                                {
LABEL_4847ec:
                                                                                                                    v242 = 0;
                                                                                                                    goto LABEL_4847f5;
                                                                                                                }
                                                                                                            }
                                                                                                            v243 = _ccall(4, v239, v240, 0);
                                                                                                            v242 = -(v243 & 1) + 1 - (unsigned int)(v243 & 1 && 1);
LABEL_4847f5:
                                                                                                            if (!v242)
                                                                                                            {
                                                                                                                v56 = v9;
                                                                                                                v13 = 20;
                                                                                                                v36 += 1;
                                                                                                                goto LABEL_484c95;
                                                                                                            }
                                                                                                            else
                                                                                                            {
                                                                                                                v244 = "guarded";
                                                                                                                v245 = &v28;
                                                                                                                while (1)
                                                                                                                {
                                                                                                                    v246 = *v245;
                                                                                                                    v247 = v246;
                                                                                                                    v248 = *v244;
                                                                                                                    if (v246 != *v244)
                                                                                                                        break;
                                                                                                                    if (v246)
                                                                                                                    {
                                                                                                                        v249 = v245[1];
                                                                                                                        v247 = v249;
                                                                                                                        v248 = v244[1];
                                                                                                                        if (v249 != v244[1])
                                                                                                                            break;
                                                                                                                        v245 += 2;
                                                                                                                        v244 += 2;
                                                                                                                        if (!v249)
                                                                                                                            goto LABEL_484833;
                                                                                                                    }
                                                                                                                    else
                                                                                                                    {
LABEL_484833:
                                                                                                                        v250 = 0;
                                                                                                                        goto LABEL_48483c;
                                                                                                                    }
                                                                                                                }
                                                                                                                v251 = _ccall(4, v247, v248, 0);
                                                                                                                v250 = -(v251 & 1) + 1 - (unsigned int)(v251 & 1 && 1);
LABEL_48483c:
                                                                                                                if (!v250)
                                                                                                                {
                                                                                                                    v56 = v9;
                                                                                                                    v13 = 21;
                                                                                                                    v36 += 1;
                                                                                                                    goto LABEL_484c95;
                                                                                                                }
                                                                                                                else
                                                                                                                {
                                                                                                                    v252 = "isbound";
                                                                                                                    v253 = &v28;
                                                                                                                    v83 = g_52b40c;
                                                                                                                    while (1)
                                                                                                                    {
                                                                                                                        v254 = *v253;
                                                                                                                        v255 = v254;
                                                                                                                        v256 = *v252;
                                                                                                                        if (v254 != *v252)
                                                                                                                            break;
                                                                                                                        if (v254)
                                                                                                                        {
                                                                                                                            v257 = v253[1];
                                                                                                                            v255 = v257;
                                                                                                                            v256 = v252[1];
                                                                                                                            if (v257 != v252[1])
                                                                                                                                break;
                                                                                                                            v253 += 2;
                                                                                                                            v252 += 2;
                                                                                                                            if (!v257)
                                                                                                                                goto LABEL_48487c;
                                                                                                                        }
                                                                                                                        else
                                                                                                                        {
LABEL_48487c:
                                                                                                                            v258 = 0;
                                                                                                                            goto LABEL_484885;
                                                                                                                        }
                                                                                                                    }
                                                                                                                    v259 = _ccall(4, v255, v256, 0);
                                                                                                                    v258 = -(v259 & 1) + 1 - (unsigned int)(v259 & 1 && 1);
LABEL_484885:
                                                                                                                    if (!v258)
                                                                                                                    {
                                                                                                                        v56 = v9;
                                                                                                                        v13 = 22;
                                                                                                                        v36 += 1;
                                                                                                                        goto LABEL_484c95;
                                                                                                                    }
                                                                                                                    else
                                                                                                                    {
                                                                                                                        v260 = "fall";
                                                                                                                        v261 = &v28;
                                                                                                                        while (1)
                                                                                                                        {
                                                                                                                            v262 = *v261;
                                                                                                                            v263 = v262;
                                                                                                                            v264 = *v260;
                                                                                                                            if (v262 != *v260)
                                                                                                                                break;
                                                                                                                            if (v262)
                                                                                                                            {
                                                                                                                                v265 = v261[1];
                                                                                                                                v263 = v265;
                                                                                                                                v264 = v260[1];
                                                                                                                                if (v265 != v260[1])
                                                                                                                                    break;
                                                                                                                                v261 += 2;
                                                                                                                                v260 += 2;
                                                                                                                                if (!v265)
                                                                                                                                    goto LABEL_4848c3;
                                                                                                                            }
                                                                                                                            else
                                                                                                                            {
LABEL_4848c3:
                                                                                                                                v266 = 0;
                                                                                                                                goto LABEL_4848cc;
                                                                                                                            }
                                                                                                                        }
                                                                                                                        v267 = _ccall(4, v263, v264, 0);
                                                                                                                        v266 = -(v267 & 1) + 1 - (unsigned int)(v267 & 1 && 1);
LABEL_4848cc:
                                                                                                                        if (!v266)
                                                                                                                        {
                                                                                                                            v56 = v9;
                                                                                                                            v13 = 23;
                                                                                                                            v36 += 1;
                                                                                                                            goto LABEL_484c95;
                                                                                                                        }
                                                                                                                        else
                                                                                                                        {
                                                                                                                            v268 = "fall.damage";
                                                                                                                            v269 = &v28;
                                                                                                                            v83 = g_52b40c;
                                                                                                                            while (1)
                                                                                                                            {
                                                                                                                                v270 = *v269;
                                                                                                                                v271 = v270;
                                                                                                                                v272 = *v268;
                                                                                                                                if (v270 != *v268)
                                                                                                                                    break;
                                                                                                                                if (v270)
                                                                                                                                {
                                                                                                                                    v273 = v269[1];
                                                                                                                                    v271 = v273;
                                                                                                                                    v272 = v268[1];
                                                                                                                                    if (v273 != v268[1])
                                                                                                                                        break;
                                                                                                                                    v269 += 2;
                                                                                                                                    v268 += 2;
                                                                                                                                    if (!v273)
                                                                                                                                        goto LABEL_48490c;
                                                                                                                                }
                                                                                                                                else
                                                                                                                                {
LABEL_48490c:
                                                                                                                                    v274 = 0;
                                                                                                                                    goto LABEL_484915;
                                                                                                                                }
                                                                                                                            }
                                                                                                                            v275 = _ccall(4, v271, v272, 0);
                                                                                                                            v274 = -(v275 & 1) + 1 - (unsigned int)(v275 & 1 && 1);
LABEL_484915:
                                                                                                                            if (!v274)
                                                                                                                            {
                                                                                                                                v56 = v9;
                                                                                                                                v13 = 24;
                                                                                                                                v36 += 1;
                                                                                                                                goto LABEL_484c95;
                                                                                                                            }
                                                                                                                            else
                                                                                                                            {
                                                                                                                                v276 = "fall.xvel";
                                                                                                                                v277 = &v28;
                                                                                                                                while (1)
                                                                                                                                {
                                                                                                                                    v278 = *v277;
                                                                                                                                    v279 = v278;
                                                                                                                                    v280 = *v276;
                                                                                                                                    if (v278 != *v276)
                                                                                                                                        break;
                                                                                                                                    if (v278)
                                                                                                                                    {
                                                                                                                                        v281 = v277[1];
                                                                                                                                        v279 = v281;
                                                                                                                                        v280 = v276[1];
                                                                                                                                        if (v281 != v276[1])
                                                                                                                                            break;
                                                                                                                                        v277 += 2;
                                                                                                                                        v276 += 2;
                                                                                                                                        if (!v281)
                                                                                                                                            goto LABEL_484953;
                                                                                                                                    }
                                                                                                                                    else
                                                                                                                                    {
LABEL_484953:
                                                                                                                                        v282 = 0;
                                                                                                                                        goto LABEL_48495c;
                                                                                                                                    }
                                                                                                                                }
                                                                                                                                v283 = _ccall(4, v279, v280, 0);
                                                                                                                                v282 = -(v283 & 1) + 1 - (unsigned int)(v283 & 1 && 1);
LABEL_48495c:
                                                                                                                                if (!v282)
                                                                                                                                {
                                                                                                                                    v56 = v9;
                                                                                                                                    v13 = 25;
                                                                                                                                    v12 = 1;
                                                                                                                                    v36 += 1;
                                                                                                                                    goto LABEL_484c95;
                                                                                                                                }
                                                                                                                                else
                                                                                                                                {
                                                                                                                                    v284 = "fall.yvel";
                                                                                                                                    v285 = &v28;
                                                                                                                                    while (1)
                                                                                                                                    {
                                                                                                                                        v286 = *v285;
                                                                                                                                        v287 = v286;
                                                                                                                                        v288 = *v284;
                                                                                                                                        if (v286 != *v284)
                                                                                                                                            break;
                                                                                                                                        if (v286)
                                                                                                                                        {
                                                                                                                                            v289 = v285[1];
                                                                                                                                            v287 = v289;
                                                                                                                                            v288 = v284[1];
                                                                                                                                            if (v289 != v284[1])
                                                                                                                                                break;
                                                                                                                                            v285 += 2;
                                                                                                                                            v284 += 2;
                                                                                                                                            if (!v289)
                                                                                                                                                goto LABEL_4849a2;
                                                                                                                                        }
                                                                                                                                        else
                                                                                                                                        {
LABEL_4849a2:
                                                                                                                                            v290 = 0;
                                                                                                                                            goto LABEL_4849ab;
                                                                                                                                        }
                                                                                                                                    }
                                                                                                                                    v291 = _ccall(4, v287, v288, 0);
                                                                                                                                    v290 = -(v291 & 1) + 1 - (unsigned int)(v291 & 1 && 1);
LABEL_4849ab:
                                                                                                                                    if (!v290)
                                                                                                                                    {
                                                                                                                                        v56 = v9;
                                                                                                                                        v13 = 26;
                                                                                                                                        v12 = 1;
                                                                                                                                        v36 += 1;
                                                                                                                                        goto LABEL_484c95;
                                                                                                                                    }
                                                                                                                                    else
                                                                                                                                    {
                                                                                                                                        v292 = "fall.recover";
                                                                                                                                        v293 = &v28;
                                                                                                                                        while (1)
                                                                                                                                        {
                                                                                                                                            v294 = *v293;
                                                                                                                                            v295 = v294;
                                                                                                                                            v296 = *v292;
                                                                                                                                            if (v294 != *v292)
                                                                                                                                                break;
                                                                                                                                            if (v294)
                                                                                                                                            {
                                                                                                                                                v297 = v293[1];
                                                                                                                                                v295 = v297;
                                                                                                                                                v296 = v292[1];
                                                                                                                                                if (v297 != v292[1])
                                                                                                                                                    break;
                                                                                                                                                v293 += 2;
                                                                                                                                                v292 += 2;
                                                                                                                                                if (!v297)
                                                                                                                                                    goto LABEL_4849f1;
                                                                                                                                            }
                                                                                                                                            else
                                                                                                                                            {
LABEL_4849f1:
                                                                                                                                                v298 = 0;
                                                                                                                                                goto LABEL_4849fa;
                                                                                                                                            }
                                                                                                                                        }
                                                                                                                                        v299 = _ccall(4, v295, v296, 0);
                                                                                                                                        v298 = -(v299 & 1) + 1 - (unsigned int)(v299 & 1 && 1);
LABEL_4849fa:
                                                                                                                                        if (!v298)
                                                                                                                                        {
                                                                                                                                            v56 = v9;
                                                                                                                                            v13 = 27;
                                                                                                                                            v36 += 1;
                                                                                                                                            goto LABEL_484c95;
                                                                                                                                        }
                                                                                                                                        else
                                                                                                                                        {
                                                                                                                                            v300 = "fall.time";
                                                                                                                                            v301 = &v28;
                                                                                                                                            while (1)
                                                                                                                                            {
                                                                                                                                                v302 = *v301;
                                                                                                                                                v303 = v302;
                                                                                                                                                v304 = *v300;
                                                                                                                                                if (v302 != *v300)
                                                                                                                                                    break;
                                                                                                                                                if (v302)
                                                                                                                                                {
                                                                                                                                                    v305 = v301[1];
                                                                                                                                                    v303 = v305;
                                                                                                                                                    v304 = v300[1];
                                                                                                                                                    if (v305 != v300[1])
                                                                                                                                                        break;
                                                                                                                                                    v301 += 2;
                                                                                                                                                    v300 += 2;
                                                                                                                                                    if (!v305)
                                                                                                                                                        goto LABEL_484a3c;
                                                                                                                                                }
                                                                                                                                                else
                                                                                                                                                {
LABEL_484a3c:
                                                                                                                                                    v306 = 0;
                                                                                                                                                    goto LABEL_484a45;
                                                                                                                                                }
                                                                                                                                            }
                                                                                                                                            v307 = _ccall(4, v303, v304, 0);
                                                                                                                                            v306 = -(v307 & 1) + 1 - (unsigned int)(v307 & 1 && 1);
LABEL_484a45:
                                                                                                                                            if (!v306)
                                                                                                                                            {
                                                                                                                                                v56 = v9;
                                                                                                                                                v13 = 28;
                                                                                                                                                v36 += 1;
                                                                                                                                                goto LABEL_484c95;
                                                                                                                                            }
                                                                                                                                            else
                                                                                                                                            {
                                                                                                                                                v308 = "fall.recovertime";
                                                                                                                                                v309 = &v28;
                                                                                                                                                while (1)
                                                                                                                                                {
                                                                                                                                                    v310 = *v309;
                                                                                                                                                    v311 = v310;
                                                                                                                                                    v312 = *v308;
                                                                                                                                                    if (v310 != *v308)
                                                                                                                                                        break;
                                                                                                                                                    if (v310)
                                                                                                                                                    {
                                                                                                                                                        v313 = v309[1];
                                                                                                                                                        v311 = v313;
                                                                                                                                                        v312 = v308[1];
                                                                                                                                                        if (v313 != v308[1])
                                                                                                                                                            break;
                                                                                                                                                        v309 += 2;
                                                                                                                                                        v308 += 2;
                                                                                                                                                        if (!v313)
                                                                                                                                                            goto LABEL_484a83;
                                                                                                                                                    }
                                                                                                                                                    else
                                                                                                                                                    {
LABEL_484a83:
                                                                                                                                                        v314 = 0;
                                                                                                                                                        goto LABEL_484a8c;
                                                                                                                                                    }
                                                                                                                                                }
                                                                                                                                                v315 = _ccall(4, v311, v312, 0);
                                                                                                                                                v314 = -(v315 & 1) + 1 - (unsigned int)(v315 & 1 && 1);
LABEL_484a8c:
                                                                                                                                                if (!v314)
                                                                                                                                                {
                                                                                                                                                    v56 = v9;
                                                                                                                                                    v13 = 29;
                                                                                                                                                    v36 += 1;
                                                                                                                                                    goto LABEL_484c95;
                                                                                                                                                }
                                                                                                                                                else
                                                                                                                                                {
                                                                                                                                                    v316 = "fall.kill";
                                                                                                                                                    v317 = &v28;
                                                                                                                                                    v83 = g_52b40c;
                                                                                                                                                    while (1)
                                                                                                                                                    {
                                                                                                                                                        v318 = *v317;
                                                                                                                                                        v319 = v318;
                                                                                                                                                        v320 = *v316;
                                                                                                                                                        if (v318 != *v316)
                                                                                                                                                            break;
                                                                                                                                                        if (v318)
                                                                                                                                                        {
                                                                                                                                                            v321 = v317[1];
                                                                                                                                                            v319 = v321;
                                                                                                                                                            v320 = v316[1];
                                                                                                                                                            if (v321 != v316[1])
                                                                                                                                                                break;
                                                                                                                                                            v317 += 2;
                                                                                                                                                            v316 += 2;
                                                                                                                                                            if (!v321)
                                                                                                                                                                goto LABEL_484acc;
                                                                                                                                                        }
                                                                                                                                                        else
                                                                                                                                                        {
LABEL_484acc:
                                                                                                                                                            v322 = 0;
                                                                                                                                                            goto LABEL_484ad5;
                                                                                                                                                        }
                                                                                                                                                    }
                                                                                                                                                    v323 = _ccall(4, v319, v320, 0);
                                                                                                                                                    v322 = -(v323 & 1) + 1 - (unsigned int)(v323 & 1 && 1);
LABEL_484ad5:
                                                                                                                                                    if (!v322)
                                                                                                                                                    {
                                                                                                                                                        v56 = v9;
                                                                                                                                                        v13 = 30;
                                                                                                                                                        v36 += 1;
                                                                                                                                                        goto LABEL_484c95;
                                                                                                                                                    }
                                                                                                                                                    else
                                                                                                                                                    {
                                                                                                                                                        v324 = "fall.envshake.time";
                                                                                                                                                        v325 = &v28;
                                                                                                                                                        while (1)
                                                                                                                                                        {
                                                                                                                                                            v326 = *v325;
                                                                                                                                                            v327 = v326;
                                                                                                                                                            v328 = *v324;
                                                                                                                                                            if (v326 != *v324)
                                                                                                                                                                break;
                                                                                                                                                            if (v326)
                                                                                                                                                            {
                                                                                                                                                                v329 = v325[1];
                                                                                                                                                                v327 = v329;
                                                                                                                                                                v328 = v324[1];
                                                                                                                                                                if (v329 != v324[1])
                                                                                                                                                                    break;
                                                                                                                                                                v325 += 2;
                                                                                                                                                                v324 += 2;
                                                                                                                                                                if (!v329)
                                                                                                                                                                    goto LABEL_484b13;
                                                                                                                                                            }
                                                                                                                                                            else
                                                                                                                                                            {
LABEL_484b13:
                                                                                                                                                                v330 = 0;
                                                                                                                                                                goto LABEL_484b1c;
                                                                                                                                                            }
                                                                                                                                                        }
                                                                                                                                                        v331 = _ccall(4, v327, v328, 0);
                                                                                                                                                        v330 = -(v331 & 1) + 1 - (unsigned int)(v331 & 1 && 1);
LABEL_484b1c:
                                                                                                                                                        if (!v330)
                                                                                                                                                        {
                                                                                                                                                            v56 = v9;
                                                                                                                                                            v13 = 31;
                                                                                                                                                            v36 += 1;
                                                                                                                                                            goto LABEL_484c95;
                                                                                                                                                        }
                                                                                                                                                        else
                                                                                                                                                        {
                                                                                                                                                            v332 = "fall.envshake.dir";
                                                                                                                                                            v333 = &v28;
                                                                                                                                                            v83 = g_52b40c;
                                                                                                                                                            while (1)
                                                                                                                                                            {
                                                                                                                                                                v334 = *v333;
                                                                                                                                                                v335 = v334;
                                                                                                                                                                v336 = *v332;
                                                                                                                                                                if (v334 != *v332)
                                                                                                                                                                    break;
                                                                                                                                                                if (v334)
                                                                                                                                                                {
                                                                                                                                                                    v337 = v333[1];
                                                                                                                                                                    v335 = v337;
                                                                                                                                                                    v336 = v332[1];
                                                                                                                                                                    if (v337 != v332[1])
                                                                                                                                                                        break;
                                                                                                                                                                    v333 += 2;
                                                                                                                                                                    v332 += 2;
                                                                                                                                                                    if (!v337)
                                                                                                                                                                        goto LABEL_484b5c;
                                                                                                                                                                }
                                                                                                                                                                else
                                                                                                                                                                {
LABEL_484b5c:
                                                                                                                                                                    v338 = 0;
                                                                                                                                                                    goto LABEL_484b65;
                                                                                                                                                                }
                                                                                                                                                            }
                                                                                                                                                            v339 = _ccall(4, v335, v336, 0);
                                                                                                                                                            v338 = -(v339 & 1) + 1 - (unsigned int)(v339 & 1 && 1);
LABEL_484b65:
                                                                                                                                                            if (!v338)
                                                                                                                                                            {
                                                                                                                                                                v56 = v9;
                                                                                                                                                                v13 = 32;
                                                                                                                                                                v36 += 1;
                                                                                                                                                                goto LABEL_484c95;
                                                                                                                                                            }
                                                                                                                                                            else
                                                                                                                                                            {
                                                                                                                                                                v340 = "fall.envshake.ampl";
                                                                                                                                                                v341 = &v28;
                                                                                                                                                                while (1)
                                                                                                                                                                {
                                                                                                                                                                    v342 = *v341;
                                                                                                                                                                    v343 = v342;
                                                                                                                                                                    v344 = *v340;
                                                                                                                                                                    if (v342 != *v340)
                                                                                                                                                                        break;
                                                                                                                                                                    if (v342)
                                                                                                                                                                    {
                                                                                                                                                                        v345 = v341[1];
                                                                                                                                                                        v343 = v345;
                                                                                                                                                                        v344 = v340[1];
                                                                                                                                                                        if (v345 != v340[1])
                                                                                                                                                                            break;
                                                                                                                                                                        v341 += 2;
                                                                                                                                                                        v340 += 2;
                                                                                                                                                                        if (!v345)
                                                                                                                                                                            goto LABEL_484ba3;
                                                                                                                                                                    }
                                                                                                                                                                    else
                                                                                                                                                                    {
LABEL_484ba3:
                                                                                                                                                                        v346 = 0;
                                                                                                                                                                        goto LABEL_484bac;
                                                                                                                                                                    }
                                                                                                                                                                }
                                                                                                                                                                v347 = _ccall(4, v343, v344, 0);
                                                                                                                                                                v346 = -(v347 & 1) + 1 - (unsigned int)(v347 & 1 && 1);
LABEL_484bac:
                                                                                                                                                                if (!v346)
                                                                                                                                                                {
                                                                                                                                                                    v56 = v9;
                                                                                                                                                                    v13 = 33;
                                                                                                                                                                    v36 += 1;
                                                                                                                                                                    goto LABEL_484c95;
                                                                                                                                                                }
                                                                                                                                                                else
                                                                                                                                                                {
                                                                                                                                                                    v348 = "fall.envshake.freq";
                                                                                                                                                                    v349 = &v28;
                                                                                                                                                                    v83 = g_52b40c;
                                                                                                                                                                    while (1)
                                                                                                                                                                    {
                                                                                                                                                                        v350 = *v349;
                                                                                                                                                                        v351 = v350;
                                                                                                                                                                        v352 = *v348;
                                                                                                                                                                        if (v350 != *v348)
                                                                                                                                                                            break;
                                                                                                                                                                        if (v350)
                                                                                                                                                                        {
                                                                                                                                                                            v353 = v349[1];
                                                                                                                                                                            v351 = v353;
                                                                                                                                                                            v352 = v348[1];
                                                                                                                                                                            if (v353 != v348[1])
                                                                                                                                                                                break;
                                                                                                                                                                            v349 += 2;
                                                                                                                                                                            v348 += 2;
                                                                                                                                                                            if (!v353)
                                                                                                                                                                                goto LABEL_484bec;
                                                                                                                                                                        }
                                                                                                                                                                        else
                                                                                                                                                                        {
LABEL_484bec:
                                                                                                                                                                            v354 = 0;
                                                                                                                                                                            goto LABEL_484bf5;
                                                                                                                                                                        }
                                                                                                                                                                    }
                                                                                                                                                                    v355 = _ccall(4, v351, v352, 0);
                                                                                                                                                                    v354 = -(v355 & 1) + 1 - (unsigned int)(v355 & 1 && 1);
LABEL_484bf5:
                                                                                                                                                                    if (!v354)
                                                                                                                                                                    {
                                                                                                                                                                        v56 = v9;
                                                                                                                                                                        v13 = 0x22;
                                                                                                                                                                        v12 = 1;
                                                                                                                                                                        v36 += 1;
                                                                                                                                                                        goto LABEL_484c95;
                                                                                                                                                                    }
                                                                                                                                                                    else
                                                                                                                                                                    {
                                                                                                                                                                        v356 = "fall.envshake.phase";
                                                                                                                                                                        v357 = &v28;
                                                                                                                                                                        while (1)
                                                                                                                                                                        {
                                                                                                                                                                            v358 = *v357;
                                                                                                                                                                            v359 = v358;
                                                                                                                                                                            v360 = *v356;
                                                                                                                                                                            if (v358 != *v356)
                                                                                                                                                                                break;
                                                                                                                                                                            if (v358)
                                                                                                                                                                            {
                                                                                                                                                                                v361 = v357[1];
                                                                                                                                                                                v359 = v361;
                                                                                                                                                                                v360 = v356[1];
                                                                                                                                                                                if (v361 != v356[1])
                                                                                                                                                                                    break;
                                                                                                                                                                                v357 += 2;
                                                                                                                                                                                v356 += 2;
                                                                                                                                                                                if (!v361)
                                                                                                                                                                                    goto LABEL_484c3c;
                                                                                                                                                                            }
                                                                                                                                                                            else
                                                                                                                                                                            {
LABEL_484c3c:
                                                                                                                                                                                v362 = 0;
                                                                                                                                                                                goto LABEL_484c45;
                                                                                                                                                                            }
                                                                                                                                                                        }
                                                                                                                                                                        v363 = _ccall(4, v359, v360, 0);
                                                                                                                                                                        v362 = -(v363 & 1) + 1 - (unsigned int)(v363 & 1 && 1);
LABEL_484c45:
                                                                                                                                                                        if (v362)
                                                                                                                                                                        {
                                                                                                                                                                            v2 = &v23;
                                                                                                                                                                            v1 = &v28;
                                                                                                                                                                            sprintf(g_52b40c + 73820, "Invalid varname %s in: %s");
                                                                                                                                                                            return 0;
                                                                                                                                                                        }
                                                                                                                                                                        v56 = v9;
                                                                                                                                                                        v13 = 35;
                                                                                                                                                                        v12 = 1;
                                                                                                                                                                        v36 += 1;
                                                                                                                                                                        goto LABEL_484c95;
                                                                                                                                                                    }
                                                                                                                                                                }
                                                                                                                                                            }
                                                                                                                                                        }
                                                                                                                                                    }
                                                                                                                                                }
                                                                                                                                            }
                                                                                                                                        }
                                                                                                                                    }
                                                                                                                                }
                                                                                                                            }
                                                                                                                        }
                                                                                                                    }
                                                                                                                }
                                                                                                            }
                                                                                                        }
                                                                                                    }
                                                                                                }
                                                                                            }
                                                                                        }
                                                                                    }
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            v83 = g_52b40c;
LABEL_484c95:
                            if (v56 == 0xb5)
                            {
                                v364 = a0;
                                if (*(char *)(v364 + v36) != 40)
                                {
                                    v2 = &v23;
                                    v1 = "Need to specify const(varname): %s";
                                    v0 = v83 + 73820;
                                    goto LABEL_4859d8;
                                }
                                else
                                {
                                    do
                                    {
                                        v365 = *(char *)(v364 + v36 + 1);
                                        v36 += 1;
                                    } while (v365 - 1 <= 126 && isspace(v365));
                                    v366 = a0;
                                    for (i = 0; isalnum(*(char *)(v366 + v36)) || *(char *)(v366 + v36) == 95 || *(char *)(v366 + v36) == 46; i += 1)
                                    {
                                        *((char *)&v32 + i) = *(char *)(v366 + v36);
                                        v36 += 1;
                                    }
                                    *((char *)&v32 + i) = 0;
                                    while (1)
                                    {
                                        v368 = *(char *)(v366 + v36);
                                        if (v368 - 1 > 126 || !isspace(v368))
                                            break;
                                        v36 += 1;
                                    }
                                    if (*(char *)(v366 + v36))
                                    {
                                        _strlwr(&v32);
                                        v369 = 0;
                                        v370 = &g_49d278;
                                        do
                                        {
                                            v371 = &*v370->field_0;
                                            v372 = &v32;
                                            while (1)
                                            {
                                                v373 = *v372;
                                                v374 = v373;
                                                v375 = *v371;
                                                if (v373 != *v371)
                                                    break;
                                                if (v373)
                                                {
                                                    v376 = v372[1];
                                                    v374 = v376;
                                                    v375 = v371[1];
                                                    if (v376 != v371[1])
                                                        break;
                                                    v372 += 2;
                                                    v371 += 2;
                                                    if (!v376)
                                                        goto LABEL_484d9c;
                                                }
                                                else
                                                {
LABEL_484d9c:
                                                    v377 = 0;
                                                    goto LABEL_484da5;
                                                }
                                            }
                                            v378 = _ccall(4, v374, v375, 0);
                                            v377 = -(v378 & 1) + 1 - (unsigned int)(v378 & 1 && 1);
LABEL_484da5:
                                            if (!v377)
                                            {
                                                v13 = v369;
                                                v12 = g_49d27c[3 * v369] == 0x1;
                                                break;
                                            }
                                        } while ((v370 += 3, v369 += 1, v370 < &g_49d698));
                                        if (v369 == 88)
                                        {
                                            v2 = &v23;
                                            v1 = &v32;
                                            sprintf(g_52b40c + 73820, "Invalid cnstname %s in: %s");
                                            return 0;
                                        }
                                        v56 = v9;
                                        v83 = g_52b40c;
                                        v36 += 1;
                                        goto LABEL_484e12;
                                        goto LABEL_484e12;
                                    }
LABEL_4841af:
                                    v2 = &v23;
                                    v1 = "Missing closing parenthesis: %s";
                                    v0 = g_52b40c + 73820;
                                    goto LABEL_4859d8;
                                }
                            }
                            else
                            {
LABEL_484e12:
                                if (v56 == 0xb6)
                                {
                                    v379 = a0;
                                    if (*(char *)(v379 + v36) != 40)
                                    {
                                        v2 = &v23;
                                        v1 = "Need to specify stagevar(varname): %s";
                                        v0 = v83 + 73820;
                                        goto LABEL_4859d8;
                                    }
                                    else
                                    {
                                        do
                                        {
                                            v380 = *(char *)(v379 + v36 + 1);
                                            v36 += 1;
                                        } while (v380 - 1 <= 126 && isspace(v380));
                                        v381 = a0;
                                        j = 0;
                                        for (v30 = v29; isalnum(*(char *)(v381 + v36)) || *(char *)(v381 + v36) == 95 || *(char *)(v381 + v36) == 46; j += 1)
                                        {
                                            v30[j] = *(char *)(v381 + v36);
                                            v36 += 1;
                                        }
                                        v30[j] = 0;
                                        while (1)
                                        {
                                            v383 = *(char *)(v381 + v36);
                                            if (v383 - 1 > 126 || !isspace(v383))
                                                break;
                                            v36 += 1;
                                        }
                                        if (!*(char *)(v381 + v36))
                                        {
                                            v2 = &v23;
                                            v1 = "Missing closing parenthesis: %s";
                                            goto LABEL_4859cb;
                                        }
                                        else
                                        {
                                            _strlwr(v30);
                                            v384 = &v30[0];
                                            v385 = "info.name";
                                            while (1)
                                            {
                                                v386 = *v384;
                                                v387 = v386;
                                                v388 = *v385;
                                                if (v386 != *v385)
                                                    break;
                                                if (v386)
                                                {
                                                    v389 = v384[1];
                                                    v387 = v389;
                                                    v388 = v385[1];
                                                    if (v389 != v385[1])
                                                        break;
                                                    v384 += 2;
                                                    v385 += 2;
                                                    if (!v389)
                                                        goto LABEL_484f0c;
                                                }
                                                else
                                                {
LABEL_484f0c:
                                                    v390 = 0;
                                                    goto LABEL_484f15;
                                                }
                                            }
                                            v391 = _ccall(4, v387, v388, 0);
                                            v390 = -(v391 & 1) + 1 - (unsigned int)(v391 & 1 && 1);
LABEL_484f15:
                                            if (!v390)
                                            {
                                                num2 = 0;
                                            }
                                            else
                                            {
                                                v392 = "info.displayname";
                                                v393 = v30;
                                                while (1)
                                                {
                                                    v394 = *(char *)v393;
                                                    v395 = v394;
                                                    v396 = *v392;
                                                    if (v394 != *v392)
                                                        break;
                                                    if (v394)
                                                    {
                                                        v397 = (char)v393[1];
                                                        v395 = v397;
                                                        v396 = v392[1];
                                                        if (v397 != v392[1])
                                                            break;
                                                        v393 += 2;
                                                        v392 += 2;
                                                        if (!v397)
                                                            goto LABEL_484f4c;
                                                    }
                                                    else
                                                    {
LABEL_484f4c:
                                                        v398 = 0;
                                                        goto LABEL_484f55;
                                                    }
                                                }
                                                v399 = _ccall(4, v395, v396, 0);
                                                v398 = -(v399 & 1) + 1 - (unsigned int)(v399 & 1 && 1);
LABEL_484f55:
                                                if (!v398)
                                                {
                                                    num2 = 1;
                                                }
                                                else
                                                {
                                                    v400 = "info.author";
                                                    v401 = v30;
                                                    while (1)
                                                    {
                                                        v402 = *(char *)v401;
                                                        v403 = v402;
                                                        v404 = *v400;
                                                        if (v402 != *v400)
                                                            break;
                                                        if (v402)
                                                        {
                                                            v405 = (char)v401[1];
                                                            v403 = v405;
                                                            v404 = v400[1];
                                                            if (v405 != v400[1])
                                                                break;
                                                            v401 += 2;
                                                            v400 += 2;
                                                            if (!v405)
                                                                goto LABEL_484f8c;
                                                        }
                                                        else
                                                        {
LABEL_484f8c:
                                                            v406 = 0;
                                                            goto LABEL_484f95;
                                                        }
                                                    }
                                                    v407 = _ccall(4, v403, v404, 0);
                                                    v406 = -(v407 & 1) + 1 - (unsigned int)(v407 & 1 && 1);
LABEL_484f95:
                                                    if (v406)
                                                    {
                                                        v2 = &v23;
                                                        v1 = &v30[0];
                                                        sprintf(g_52b40c + 73820, "Invalid varname %s in: %s");
                                                        return 0;
                                                    }
                                                    num2 = 4;
                                                }
                                            }
                                            v56 = v9;
                                            v36 += 1;
                                            goto LABEL_484fa6;
                                        }
                                    }
                                }
                                else
                                {
LABEL_484fa6:
                                    if (v56 == 0xb4 || v56 == 0xb5 || v56 == 0xb6)
                                        goto LABEL_484fc3;
LABEL_484fbb:
                                    v12 = 0;
LABEL_484fc3:
                                    if (v56 <= 0x15)
                                    {
                                        if (v56 == 0x15)
                                            goto LABEL_48513a;
                                        switch (v56)
                                        {
                                        case 2: case 7:
                                            v408 = a0;
                                            v409 = *(char *)(v408 + v36);
                                            if (*(char *)(v408 + v36))
                                            {
                                                do
                                                {
                                                    v410 = v36;
                                                } while (v409 != 61 && (v409 = *(char *)(v408 + v410 + 1), v36 = v410 + 1, *(char *)(v408 + v410 + 1)));
                                            }
                                            if (*(char *)(v408 + v36))
                                                v36 += 1;
                                            while (1)
                                            {
                                                v411 = *(char *)(v408 + v36);
                                                if (v411 - 1 > 126 || !isspace(v411))
                                                    break;
                                                v36 += 1;
                                            }
                                            if (*(char *)(v408 + v36) < 48)
                                                return 0;
                                            if (*(char *)(v408 + v36) <= 57)
                                            {
                                                num = atoi(v408 + v36);
                                                v413 = v9;
                                                num2 = num;
                                                if (v413 == 2)
                                                {
                                                    num -= 1;
                                                    num2 = num;
                                                }
                                                if (num < 0)
                                                {
                                                    if (v413 != 2)
                                                        return 0;
                                                    sprintf(g_52b40c + 73820, "Anim must be > 0");
                                                    return 0;
                                                }
                                                if (v413 == 0x7 && num <= 0)
                                                    return 0;
                                                for (; sub_40e360() || sub_40e340(); v36 += 1);
                                            }
                                            else
                                            {
                                                return 0;
                                            }
LABEL_4850fd:
                                            if (*(char *)(v408 + v36) == 44)
                                            {
                                                v36 += 1;
                                                break;
                                            }
                                            break;
                                        case 3:
LABEL_485281:
                                            v408 = a0;
                                            break;
                                        case 4: case 5: case 6: case 8: case 9:
LABEL_48513a:
                                            v408 = a0;
                                            v12 = 1;
                                            while (1)
                                            {
                                                v414 = *(char *)(v408 + v36);
                                                if (v414 - 1 > 126 || !isspace(v414))
                                                    break;
                                                v36 += 1;
                                            }
                                            v415 = *(char *)(v408 + v36);
                                            if (v415 - 1 <= 126)
                                                v415 = tolower(v415);
                                            v416 = v415 - 120;
                                            if (v415 != 120)
                                            {
                                                if (v416 != 1)
                                                {
                                                    if (v416 == 2)
                                                    {
                                                        if ((unsigned int)*(int *)(a1 + 184) < (unsigned int)*(int *)(g_52b40c + 1032))
                                                        {
                                                            v13 = 2;
                                                            v36 += 1;
                                                            break;
                                                        }
                                                        else
                                                        {
                                                            return 0;
                                                        }
                                                    }
                                                    else
                                                    {
                                                        return 0;
                                                    }
                                                }
                                                else
                                                {
                                                    v13 = 1;
                                                    v36 += 1;
                                                    break;
                                                }
                                            }
                                            else
                                            {
                                                v13 = 0;
                                                v36 += 1;
                                                break;
                                            }
                                        default:
                                            goto LABEL_485281;
                                        }
                                        while (1)
                                        {
                                            v420 = *(char *)(v408 + v36);
                                            if (v420 - 1 > 126 || !(v421 = isspace(v420), isspace(v420)))
                                                break;
                                            v36 += 1;
                                        }
                                        v422 = a0;
                                        v423 = v9;
                                        v424 = v422 + v36;
                                        *a5 = v424;
                                        if (v423 == 0x14)
                                        {
                                            v11 = 0;
                                            v425 = *v424;
                                            if (*v424)
                                            {
                                                do
                                                {
                                                    v426 = v36;
                                                } while (v425 != 61 && (v425 = *(char *)(v422 + v426 + 1), v36 = v426 + 1, *(char *)(v422 + v426 + 1)));
                                            }
                                            if (*(char *)(v422 + v36 - 1) == 33)
                                                v11 = 1;
                                            if (*(char *)(v422 + v36))
                                                v36 += 1;
                                            while (1)
                                            {
                                                v427 = *(char *)(v422 + v36);
                                                if (v427 - 1 > 126 || !isspace(v427))
                                                    break;
                                                v36 += 1;
                                            }
                                            if (*(char *)(v422 + v36) != 0x22)
                                                return 0;
                                            if (!sub_485bb0(v422 + v36 + 1, &v7, a1, a5))
                                            {
                                                sprintf(&g_52be60, "Couldn't parse command.");
                                                return 0;
                                            }
                                            if (*(char *)*a5 != 0x22)
                                                return 0;
                                            *a5 = *a5 + 1;
                                            goto LABEL_485928;
                                            goto LABEL_485928;
                                        }
                                        else
                                        {
                                            if (v423 == 0xb6)
                                            {
                                                if (num2 && num2 != 1 && num2 != 4)
                                                    goto LABEL_485740;
                                            }
                                            else
                                            {
                                                if (v423 < 0x5a || v423 > 0x5e)
                                                {
                                                    if (v423 != 0x2b && v423 != 0x2c)
                                                    {
                                                        if (v423 != 0x2d && v423 != 0x2e)
                                                        {
                                                            if (v423 == 0x79)
                                                            {
                                                                v11 = 0;
                                                                v438 = *v424;
                                                                if (*v424)
                                                                {
                                                                    do
                                                                    {
                                                                        v439 = v36;
                                                                    } while (v438 != 61 && (v438 = *(char *)(v422 + v439 + 1), v36 = v439 + 1, *(char *)(v422 + v439 + 1)));
                                                                }
                                                                if (*(char *)(v422 + v36 - 1) == 33)
                                                                    v11 = 1;
                                                                if (*(char *)(v422 + v36))
                                                                    v36 += 1;
                                                                if (sub_40e340())
                                                                {
                                                                    do
                                                                    {
                                                                        v36 += 1;
                                                                    } while (sub_40e340());
                                                                }
                                                                v440 = a0 + v36;
                                                                v441 = v36;
                                                                v442 = v440;
                                                                if (*(char *)(a0 + v36))
                                                                {
                                                                    do
                                                                    {
                                                                    } while (!sub_40e340() && *v442 != 44 && *v442 != 41 && (v441 += 1, v442 = v441 + a0, *(char *)(v441 + a0)));
                                                                }
                                                                n1 = v441 - v36;
                                                                strncpy(&v29[1], v440, n1);
                                                                v29[1 + n1] = 0;
                                                                _strlwr(&v29[1]);
                                                                v444 = "single";
                                                                v445 = &v29[1];
                                                                while (1)
                                                                {
                                                                    v446 = *v445;
                                                                    v447 = v446;
                                                                    v448 = *v444;
                                                                    if (v446 != *v444)
                                                                        break;
                                                                    if (v446)
                                                                    {
                                                                        v449 = v445[1];
                                                                        v447 = v449;
                                                                        v448 = v444[1];
                                                                        if (v449 != v444[1])
                                                                            break;
                                                                        v445 += 2;
                                                                        v444 += 2;
                                                                        if (!v449)
                                                                            goto LABEL_4855bf;
                                                                    }
                                                                    else
                                                                    {
LABEL_4855bf:
                                                                        v450 = 0;
                                                                        goto LABEL_4855c8;
                                                                    }
                                                                }
                                                                v451 = _ccall(4, v447, v448, 0);
                                                                v450 = -(v451 & 1) + 1 - (unsigned int)(v451 & 1 && 1);
LABEL_4855c8:
                                                                if (!v450)
                                                                {
                                                                    v13 = 0;
                                                                    *a5 = v441 + a0;
                                                                }
                                                                else
                                                                {
                                                                    v452 = "simul";
                                                                    v453 = &v29[1];
                                                                    while (1)
                                                                    {
                                                                        v454 = *v453;
                                                                        v455 = v454;
                                                                        v456 = *v452;
                                                                        if (v454 != *v452)
                                                                            break;
                                                                        if (v454)
                                                                        {
                                                                            v457 = v453[1];
                                                                            v455 = v457;
                                                                            v456 = v452[1];
                                                                            if (v457 != v452[1])
                                                                                break;
                                                                            v453 += 2;
                                                                            v452 += 2;
                                                                            if (!v457)
                                                                                goto LABEL_48560f;
                                                                        }
                                                                        else
                                                                        {
LABEL_48560f:
                                                                            v458 = 0;
                                                                            goto LABEL_485618;
                                                                        }
                                                                    }
                                                                    v459 = _ccall(4, v455, v456, 0);
                                                                    v458 = -(v459 & 1) + 1 - (unsigned int)(v459 & 1 && 1);
LABEL_485618:
                                                                    if (!v458)
                                                                    {
                                                                        v13 = 1;
                                                                        *a5 = v441 + a0;
                                                                    }
                                                                    else
                                                                    {
                                                                        v460 = "turns";
                                                                        v461 = &v29[1];
                                                                        while (1)
                                                                        {
                                                                            v462 = *v461;
                                                                            v463 = v462;
                                                                            v464 = *v460;
                                                                            if (v462 != *v460)
                                                                                break;
                                                                            if (v462)
                                                                            {
                                                                                v465 = v461[1];
                                                                                v463 = v465;
                                                                                v464 = v460[1];
                                                                                if (v465 != v460[1])
                                                                                    break;
                                                                                v461 += 2;
                                                                                v460 += 2;
                                                                                if (!v465)
                                                                                    goto LABEL_485663;
                                                                            }
                                                                            else
                                                                            {
LABEL_485663:
                                                                                v466 = 0;
                                                                                goto LABEL_48566c;
                                                                            }
                                                                        }
                                                                        v467 = _ccall(4, v463, v464, 0);
                                                                        v466 = -(v467 & 1) + 1 - (unsigned int)(v467 & 1 && 1);
LABEL_48566c:
                                                                        if (v466)
                                                                        {
                                                                            v2 = &v23;
                                                                            v1 = v440;
                                                                            sprintf(g_52b40c + 73820, "Invalid TeamMode %s in %s");
                                                                            return 0;
                                                                        }
                                                                        v13 = 3;
                                                                        *a5 = v441 + a0;
                                                                    }
                                                                }
                                                            }
                                                            else
                                                            {
                                                                if (v423 == 0x30)
                                                                {
                                                                    v11 = 0;
                                                                    v468 = *v424;
                                                                    if (*v424)
                                                                    {
                                                                        do
                                                                        {
                                                                            v469 = v36;
                                                                        } while (v468 != 61 && (v468 = *(char *)(v422 + v469 + 1), v36 = v469 + 1, *(char *)(v422 + v469 + 1)));
                                                                    }
                                                                    if (*(char *)(v422 + v36 - 1) == 33)
                                                                        v11 = 1;
                                                                    if (*(char *)(v422 + v36))
                                                                        v36 += 1;
                                                                    if (sub_40e340())
                                                                    {
                                                                        do
                                                                        {
                                                                            v36 += 1;
                                                                        } while (sub_40e340());
                                                                    }
                                                                    v13 = sub_489dc0(v36 + v422, a5);
                                                                    if (v13 == 0xffffffff)
                                                                        return 0;
                                                                }
                                                                else
                                                                {
LABEL_485740:
                                                                    switch (v423)
                                                                    {
                                                                    case 2:
                                                                        v2 = a5;
                                                                        v1 = &v7;
                                                                        goto LABEL_485762;
                                                                    case 7:
                                                                        if (!sub_485cb0(&v7, a5))
                                                                            return 0;
                                                                        break;
                                                                    case 62: case 63: case 64:
                                                                        v2 = a5;
                                                                        v1 = &v7;
LABEL_485762:
                                                                        if (!sub_485cb0(v1, v2))
                                                                        {
                                                                            v11 = 10;
                                                                            v14 = 0;
                                                                            v13 = 0;
                                                                            break;
                                                                        }
                                                                        break;
                                                                    default:
                                                                        break;
                                                                    }
                                                                }
                                                            }
                                                        }
                                                        else
                                                        {
                                                            v11 = 0;
                                                            v470 = *v424;
                                                            if (*v424)
                                                            {
                                                                do
                                                                {
                                                                    v471 = v36;
                                                                } while (v470 != 61 && (v470 = *(char *)(v422 + v471 + 1), v36 = v471 + 1, *(char *)(v422 + v471 + 1)));
                                                            }
                                                            if (*(char *)(v422 + v36 - 1) == 33)
                                                                v11 = 1;
                                                            if (*(char *)(v422 + v36))
                                                                v36 += 1;
                                                            while (1)
                                                            {
                                                                v472 = *(char *)(v422 + v36);
                                                                if (v472 - 1 > 126 || !isspace(v472))
                                                                    break;
                                                                v36 += 1;
                                                            }
                                                            v473 = sub_40e380();
                                                            v474 = v473 - 97;
                                                            if (v473 != 97)
                                                            {
                                                                if (v474 != 0x7)
                                                                {
                                                                    if (v474 == 8)
                                                                    {
                                                                        v13 = 0;
                                                                        *a5 = v422 + v36 + 1;
                                                                    }
                                                                    else
                                                                    {
                                                                        return 0;
                                                                    }
                                                                }
                                                                else
                                                                {
                                                                    v13 = 2;
                                                                    *a5 = v422 + v36 + 1;
                                                                }
                                                            }
                                                            else
                                                            {
                                                                v13 = 1;
                                                                *a5 = v422 + v36 + 1;
                                                            }
                                                        }
                                                    }
                                                    else
                                                    {
                                                        v11 = 0;
                                                        v475 = *v424;
                                                        if (*v424)
                                                        {
                                                            do
                                                            {
                                                                v476 = v36;
                                                            } while (v475 != 61 && (v475 = *(char *)(v422 + v476 + 1), v36 = v476 + 1, *(char *)(v422 + v476 + 1)));
                                                        }
                                                        if (*(char *)(v422 + v36 - 1) == 33)
                                                            v11 = 1;
                                                        if (*(char *)(v422 + v36))
                                                            v36 += 1;
                                                        while (1)
                                                        {
                                                            v477 = *(char *)(v422 + v36);
                                                            if (v477 - 1 > 126 || !isspace(v477))
                                                                break;
                                                            v36 += 1;
                                                        }
                                                        v478 = *(char *)(v422 + v36);
                                                        if (v478 - 1 <= 126)
                                                            v478 = tolower(v478);
                                                        switch (v478)
                                                        {
                                                        case 97:
                                                            v13 = 3;
                                                            break;
                                                        case 98: case 100: case 101: case 102: case 103: case 104: case 105: case 106: case 107: case 109: case 110: case 111: case 112: case 113: case 114:
                                                            return 0;
                                                        case 99:
                                                            v13 = 2;
                                                            break;
                                                        case 108:
                                                            v13 = 4;
                                                            break;
                                                        case 115:
                                                            v13 = 1;
                                                            break;
                                                        default:
                                                            return 0;
                                                        }
                                                        v437 = v422 + v36 + 1;
LABEL_48591f:
                                                        *a5 = v437;
                                                    }
LABEL_485928:
                                                    v21 = v22 + 1;
                                                    if ((int)v21 > (int)a4)
                                                        return 0;
                                                    n2 = v22 * 4;
                                                    memcpy(a2, &v33, n2);
                                                    memcpy(a3, &v31, v22);
                                                    ptr = calloc(1, 56);
                                                    if (!ptr)
                                                        g_52b0d8 = 1;
                                                    v481 = 14;
                                                    k = &v7;
                                                    for (v483 = ptr; v481; k += 1)
                                                    {
                                                        v481 -= 1;
                                                        *v483 = *k;
                                                        v483 += 1;
                                                    }
                                                    *(unsigned int **)((char *)a2 + n2) = ptr;
                                                    *(char *)(v22 + a3) = 3;
                                                    return v21;
                                                }
                                            }
                                            v12 = 0;
                                            v11 = 0;
                                            v428 = *v424;
                                            if (*v424)
                                            {
                                                do
                                                {
                                                    v429 = v36;
                                                } while (v428 != 61 && (v428 = *(char *)(v422 + v429 + 1), v36 = v429 + 1, *(char *)(v422 + v429 + 1)));
                                            }
                                            if (v36 > 0 && *(char *)(v422 + v36 - 1) == 33)
                                                v11 = 1;
                                            if (*(char *)(v422 + v36))
                                                v36 += 1;
                                            while (1)
                                            {
                                                v430 = *(char *)(v422 + v36);
                                                if (v430 - 1 > 126 || !isspace(v430))
                                                    break;
                                                v36 += 1;
                                            }
                                            if (*(char *)(v422 + v36) != 0x22)
                                            {
                                                _snprintf(&g_52be60, 0x100, "String trigger needs to be in the form trig = \"string value\".");
                                                return 0;
                                            }
                                            v431 = a0;
                                            v432 = v36 + 1;
                                            v433 = *(char *)(v431 + v432);
                                            v434 = v432;
                                            if (*(char *)(v431 + v432))
                                            {
                                                do
                                                {
                                                    v435 = v434;
                                                } while (v433 != 0x22 && (v433 = *(char *)(v435 + v431 + 1), v434 = v435 + 1, *(char *)(v435 + v431 + 1)));
                                            }
                                            if (*(char *)(v434 + v431) != 0x22)
                                                return 0;
                                            n = v434 - v432;
                                            ptr1 = sub_40e1b0();
                                            if (!ptr1)
                                                return 0;
                                            strncpy(ptr1, v431 + v432, n);
                                            ptr1[n] = 0;
                                            v13 = 1;
                                            v437 = v434 + a0 + 1;
                                            goto LABEL_48591f;
                                            goto LABEL_48591f;
                                        }
                                    }
                                    else if (v56 <= 0x136)
                                    {
                                        if (v56 == 0x136)
                                            goto LABEL_485279;
                                        switch (v56)
                                        {
                                        case 62: case 63: case 64:
                                            v408 = a0;
                                            v417 = *(char *)(v408 + v36);
                                            if (*(char *)(v408 + v36))
                                            {
                                                do
                                                {
                                                    v418 = v36;
                                                } while (v417 != 61 && (v417 = *(char *)(v408 + v418 + 1), v36 = v418 + 1, *(char *)(v408 + v418 + 1)));
                                            }
                                            if (*(char *)(v408 + v36))
                                                v36 += 1;
                                            while (1)
                                            {
                                                v419 = *(char *)(v408 + v36);
                                                if (v419 - 1 > 126 || !isspace(v419))
                                                    break;
                                                v36 += 1;
                                            }
                                            if (!sub_40e360())
                                                return 0;
                                            for (num1 = atoi(v408 + v36); sub_40e360() || sub_40e340(); v36 += 1);
                                            goto LABEL_4850fd;
                                        case 210: case 211: case 300: case 301:
LABEL_485279:
                                            v12 = 1;
                                            break;
                                        case 65: case 66: case 67: case 68: case 69: case 70: case 71: case 72: case 73: case 74: case 75: case 76: case 77: case 78: case 79: case 80: case 81: case 82: case 83: case 84: case 85: case 86: case 87: case 88: case 89: case 90: case 91: case 92: case 93: case 94: case 95: case 96: case 97: case 98: case 99: case 100: case 101: case 102: case 104: case 105: case 106: case 107: case 108: case 109: case 110: case 111: case 112: case 113: case 114: case 115: case 116: case 117: case 118: case 119: case 120: case 121: case 122: case 123: case 124: case 125: case 126: case 127: case 128: case 129: case 130: case 131: case 132: case 133: case 134: case 135: case 136: case 137: case 138: case 139: case 140: case 141: case 142: case 143: case 144: case 145: case 146: case 147: case 148: case 149: case 150: case 151: case 152: case 153: case 154: case 155: case 156: case 157: case 158: case 159: case 160: case 161: case 162: case 163: case 164: case 165: case 166: case 167: case 168: case 169: case 170: case 171: case 172: case 173: case 174: case 175: case 176: case 177: case 178: case 179: case 180: case 181: case 182: case 183: case 184: case 185: case 186: case 187: case 188: case 189: case 190: case 191: case 192: case 193: case 194: case 195: case 196: case 197: case 198: case 199: case 200: case 201: case 202: case 205: case 206: case 207: case 208: case 209: case 212: case 213: case 214: case 215: case 216: case 217: case 218: case 219: case 220: case 221: case 222: case 223: case 224: case 225: case 226: case 227: case 228: case 229: case 230: case 231: case 232: case 233: case 234: case 235: case 236: case 237: case 238: case 239: case 240: case 241: case 242: case 243: case 244: case 245: case 246: case 247: case 248: case 249: case 250: case 251: case 252: case 253: case 254: case 255: case 256: case 257: case 258: case 259: case 260: case 261: case 262: case 263: case 264: case 265: case 266: case 267: case 268: case 269: case 270: case 271: case 272: case 273: case 274: case 275: case 276: case 277: case 278: case 279: case 280: case 281: case 282: case 283: case 284: case 285: case 286: case 287: case 288: case 289: case 290: case 291: case 292: case 293: case 294: case 295: case 296: case 297: case 298: case 299:
                                            break;
                                        case 103: case 203: case 204:
                                            goto LABEL_48513a;
                                        default:
                                            break;
                                        }
                                    }
                                    else
                                    {
                                        switch (v56)
                                        {
                                        case 320: case 321: case 322: case 323: case 324: case 325: case 326: case 327: case 344: case 346: case 349: case 350: case 351:
                                            goto LABEL_485279;
                                        case 328: case 329: case 330: case 331: case 332: case 333: case 334: case 335: case 336: case 337: case 338: case 339: case 340: case 341: case 342: case 343: case 345: case 347: case 348:
                                            break;
                                        default:
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    goto LABEL_4859d8;
                }
                if (*(char *)(v58 + v36) != 40)
                    return 0;
                v59 = a4;
                if (sub_489360(v58 + v36 + 1, "n:)", a1, &v19, &v33, &v31, &v20, v59) != 1)
                {
                    return 0;
                }
                else if (*(char *)v19 == 41)
                {
                    v60 = v20;
                    v36 = v19 - a0 + 1;
                    v56 = v9;
                    v22 = v60;
                    goto LABEL_483d83;
                }
                else
                {
                    return 0;
                }
            }
            sub_438d20(v2);
LABEL_482d1f:
        }
        v39 = a0;
        if (*(char *)(v39 + v36) == 40)
        {
            if (!sub_438b20(v39 + v36 + 1, &v7, a1, &v19, 1))
            {
                return 0;
            }
            else if (*(char *)v19 == 41)
            {
                v36 = v19 - v39 + 1;
                goto LABEL_482d87;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            if (v10 == 8)
                return 0;
        }
    }
}
