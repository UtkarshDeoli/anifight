// controller_dispatch entry=0x473ed0
typedef struct st_473ed0_3 {
    char padding_0[184];
    int field_b8;
} st_473ed0_3;

typedef struct st_473ed0_1 {
    unsigned int field_0;
    char padding_4[44];
    unsigned short field_30;
    char padding_32[358];
    unsigned int field_198;
} st_473ed0_1;

typedef struct st_473ed0_4 {
    char padding_0[3];
    char field_3;
} st_473ed0_4;

typedef struct st_473ed0_0 {
    char padding_0[73819];
    char field_1205b;
    unsigned short field_1205c;
    char field_1205e;
    char padding_1205f[1];
    char field_12060;
} st_473ed0_0;

extern int g_49da0a;
extern st_473ed0_0 *g_52b40c;

char * sub_473ed0(int a0, void* a1, st_473ed0_3 *a2)
{
    void* v14;  // ebx
    st_473ed0_3 *v15;  // edi
    void* v24;  // esi
    unsigned int v114;  // cc_dep2
    char v115;  // 4103
    unsigned int v116;  // eax
    unsigned int v117;  // 4111
    int v118;  // eax
    char *v119;  // ecx
    char *iter;  // eax
    char v121;  // 4101
    unsigned int v122;  // cc_dep1
    unsigned int v123;  // cc_dep2
    char *v25;  // ecx
    char v124;  // 4103
    unsigned int v125;  // eax
    unsigned int v126;  // 4111
    int v127;  // eax
    int v128;  // edi
    char *v129;  // ecx
    char *iter1;  // eax
    char v131;  // 4101
    unsigned int v132;  // cc_dep1
    unsigned int v133;  // cc_dep2
    char *iter2;  // eax
    char v134;  // 4103
    unsigned int v135;  // eax
    unsigned int v136;  // 4111
    char *v137;  // ecx
    char *v138;  // eax
    char v139;  // 4101
    unsigned int v140;  // cc_dep1
    unsigned int v141;  // cc_dep2
    char v142;  // 4103
    unsigned int v143;  // eax
    char v27;  // 4101
    unsigned int v144;  // 4111
    int v145;  // eax
    char *v146;  // ecx
    char *v147;  // eax
    char v148;  // 4101
    unsigned int v149;  // cc_dep1
    unsigned int v150;  // cc_dep2
    char v151;  // 4103
    unsigned int v152;  // eax
    unsigned int v153;  // 4111
    unsigned int v28;  // cc_dep1
    char *v154;  // ecx
    char *v155;  // eax
    char v156;  // 4101
    unsigned int v157;  // cc_dep1
    unsigned int v158;  // cc_dep2
    char v159;  // 4103
    unsigned int v160;  // eax
    unsigned int v161;  // 4111
    char *v162;  // ecx
    char *v163;  // eax
    unsigned int v29;  // cc_dep2
    char v164;  // 4101
    unsigned int v165;  // cc_dep1
    unsigned int v166;  // cc_dep2
    char v167;  // 4103
    unsigned int v168;  // eax
    unsigned int v169;  // 4111
    char *v170;  // ecx
    char *v171;  // eax
    char v172;  // 4101
    unsigned int v173;  // cc_dep1
    char v30;  // 4103
    unsigned int v174;  // cc_dep2
    char v175;  // 4103
    unsigned int v176;  // eax
    unsigned int v177;  // 4111
    char *v178;  // ecx
    char *v179;  // eax
    char v180;  // 4101
    unsigned int v181;  // cc_dep1
    unsigned int v182;  // cc_dep2
    char v183;  // 4103
    unsigned int v31;  // eax
    unsigned int v184;  // eax
    unsigned int v185;  // 4111
    char *v186;  // eax
    char *v187;  // ecx
    char v188;  // 4101
    unsigned int v189;  // cc_dep1
    unsigned int v190;  // cc_dep2
    char v191;  // 4103
    unsigned int v192;  // eax
    unsigned int v193;  // 4111
    unsigned int v32;  // 4111
    char *v194;  // ecx
    char *v195;  // eax
    char v196;  // 4101
    unsigned int v197;  // cc_dep1
    unsigned int v198;  // cc_dep2
    char v199;  // 4103
    unsigned int v200;  // eax
    unsigned int v201;  // 4111
    char *v202;  // ecx
    char *v203;  // eax
    int v33;  // eax
    char v204;  // 4101
    unsigned int v205;  // cc_dep1
    unsigned int v206;  // cc_dep2
    char v207;  // 4103
    unsigned int v208;  // eax
    unsigned int v209;  // 4111
    int v210;  // edi
    int v211;  // edi
    int v212;  // edi
    unsigned int v213;  // eax
    char *v16;  // eax
    void* v34;  // esi
    char *v214;  // ecx
    char *v215;  // eax
    char v216;  // 4101
    unsigned int v217;  // cc_dep1
    unsigned int v218;  // cc_dep2
    char v219;  // 4103
    unsigned int v220;  // eax
    unsigned int v221;  // 4111
    char *v222;  // ecx
    char *v223;  // eax
    char *v35;  // ecx
    st_473ed0_3 *v224;  // edi
    char v225;  // 4101
    unsigned int v226;  // cc_dep1
    unsigned int v227;  // cc_dep2
    char v228;  // 4103
    unsigned int v229;  // eax
    unsigned int v230;  // 4111
    st_473ed0_1 *ptr;  // esi
    void* v232;  // edi
    unsigned int v233;  // eax
    char *v36;  // eax
    unsigned int v234;  // eax
    unsigned int idx;  // eax
    char *v236;  // ecx
    char *v237;  // eax
    char v238;  // 4101
    unsigned int v239;  // cc_dep1
    unsigned int v240;  // cc_dep2
    char v241;  // 4103
    unsigned int v242;  // eax
    unsigned int v243;  // 4111
    char v37;  // 4101
    st_473ed0_1 *ptr1;  // esi
    int v245;  // edi
    unsigned int v246;  // eax
    unsigned int v247;  // edi
    unsigned int v248;  // edi
    unsigned int v249;  // edi
    unsigned int v250;  // ecx
    void* k;  // esi
    char *v252;  // ecx
    char *v253;  // eax
    unsigned int v38;  // cc_dep1
    char v254;  // 4101
    unsigned int v255;  // cc_dep1
    unsigned int v256;  // cc_dep2
    char v257;  // 4103
    unsigned int v258;  // eax
    unsigned int v259;  // 4111
    int v260;  // edi
    int v261;  // eax
    char *v262;  // ecx
    char *v263;  // eax
    unsigned int v39;  // cc_dep2
    char v264;  // 4101
    unsigned int v265;  // cc_dep1
    unsigned int v266;  // cc_dep2
    char v267;  // 4103
    unsigned int v268;  // eax
    unsigned int v269;  // 4111
    int v270;  // edi
    unsigned int v271;  // eax
    char *v272;  // ecx
    char *v273;  // eax
    char v40;  // 4103
    char v274;  // 4101
    unsigned int v275;  // cc_dep1
    unsigned int v276;  // cc_dep2
    char v277;  // 4103
    unsigned int v278;  // eax
    unsigned int v279;  // 4111
    char *v280;  // ecx
    char *v281;  // eax
    char v282;  // 4101
    unsigned int v283;  // cc_dep1
    unsigned int v41;  // eax
    unsigned int v284;  // cc_dep2
    char v285;  // 4103
    unsigned int v286;  // eax
    unsigned int v287;  // 4111
    int v288;  // esi
    char *v289;  // ecx
    char *v290;  // eax
    char v291;  // 4101
    unsigned int v292;  // cc_dep1
    unsigned int v293;  // cc_dep2
    unsigned int v42;  // 4111
    char v294;  // 4103
    unsigned int v295;  // eax
    unsigned int v296;  // 4111
    char *v297;  // ecx
    char *v298;  // eax
    char v299;  // 4101
    unsigned int v300;  // cc_dep1
    unsigned int v301;  // cc_dep2
    char v302;  // 4103
    unsigned int v303;  // eax
    int v43;  // eax
    unsigned int v304;  // 4111
    char *v305;  // ecx
    char *v306;  // eax
    char v307;  // 4101
    unsigned int v308;  // cc_dep1
    unsigned int v309;  // cc_dep2
    char v310;  // 4103
    unsigned int v311;  // eax
    unsigned int v312;  // 4111
    char *v313;  // ecx
    char *v17;  // eax
    char *v44;  // ecx
    char *v314;  // eax
    char v315;  // 4101
    unsigned int v316;  // cc_dep1
    unsigned int v317;  // cc_dep2
    char v318;  // 4103
    unsigned int v319;  // eax
    unsigned int v320;  // 4111
    char *v321;  // ecx
    char *v322;  // eax
    char v323;  // 4101
    char *v45;  // eax
    unsigned int v324;  // cc_dep1
    unsigned int v325;  // cc_dep2
    char v326;  // 4103
    unsigned int v327;  // eax
    unsigned int v328;  // 4111
    int v329;  // eax
    char *v330;  // ecx
    char *v331;  // eax
    char v332;  // 4101
    unsigned int v333;  // cc_dep1
    char v46;  // 4101
    unsigned int v334;  // cc_dep2
    char v335;  // 4103
    unsigned int v336;  // eax
    unsigned int v337;  // 4111
    unsigned int v338;  // esi
    unsigned int v339;  // esi
    unsigned int v340;  // eax
    unsigned int v341;  // eax
    unsigned int v342;  // esi
    char *v343;  // ecx
    unsigned int v47;  // cc_dep1
    char *v344;  // eax
    char v345;  // 4101
    unsigned int v346;  // cc_dep1
    unsigned int v347;  // cc_dep2
    char v348;  // 4103
    unsigned int v349;  // eax
    unsigned int v350;  // 4111
    unsigned int v351;  // edi
    unsigned int v352;  // edi
    unsigned int node;  // edi
    unsigned int v48;  // cc_dep2
    unsigned int v354;  // ecx
    void* l;  // esi
    char *v356;  // ecx
    char *v357;  // eax
    char v358;  // 4101
    unsigned int v359;  // cc_dep1
    unsigned int v360;  // cc_dep2
    char v361;  // 4103
    unsigned int v362;  // eax
    unsigned int v363;  // 4111
    char v49;  // 4103
    char *v364;  // ecx
    char *v365;  // eax
    char v366;  // 4101
    unsigned int v367;  // cc_dep1
    unsigned int v368;  // cc_dep2
    char v369;  // 4103
    unsigned int v370;  // eax
    unsigned int v371;  // 4111
    char *v372;  // ecx
    char *v373;  // eax
    unsigned int v50;  // eax
    char v374;  // 4101
    unsigned int v375;  // cc_dep1
    unsigned int v376;  // cc_dep2
    char v377;  // 4103
    unsigned int v378;  // eax
    unsigned int v379;  // 4111
    unsigned int v380;  // ebx
    unsigned int v381;  // ebx
    unsigned int v382;  // ebx
    char *v383;  // ecx
    unsigned int v51;  // 4111
    char *v384;  // eax
    char v385;  // 4101
    unsigned int v386;  // cc_dep1
    unsigned int v387;  // cc_dep2
    char v388;  // 4103
    unsigned int v389;  // eax
    unsigned int v390;  // 4111
    unsigned int v391;  // edi
    unsigned int v392;  // edi
    unsigned int v393;  // edi
    char *v52;  // ecx
    unsigned int v394;  // ecx
    void* m;  // esi
    char *v396;  // ecx
    char *v397;  // eax
    char v398;  // 4101
    unsigned int v399;  // cc_dep1
    unsigned int v400;  // cc_dep2
    char v401;  // 4103
    unsigned int v402;  // eax
    unsigned int v403;  // 4111
    char *v53;  // eax
    int v404;  // esi
    int v405;  // esi
    void* v406;  // esi
    void* v407;  // edi
    char *v408;  // ecx
    char *v409;  // eax
    char v410;  // 4101
    unsigned int v411;  // cc_dep1
    unsigned int v412;  // cc_dep2
    char v413;  // 4103
    char i;  // 4102
    char v54;  // 4101
    unsigned int v414;  // eax
    unsigned int v415;  // 4111
    char *v416;  // ecx
    char *v417;  // eax
    char v418;  // 4101
    unsigned int v419;  // cc_dep1
    unsigned int v420;  // cc_dep2
    char v421;  // 4103
    unsigned int v422;  // eax
    unsigned int v423;  // 4111
    unsigned int v55;  // cc_dep1
    int v424;  // esi
    char *v425;  // ecx
    char *v426;  // eax
    char v427;  // 4101
    unsigned int v428;  // cc_dep1
    unsigned int v429;  // cc_dep2
    char v430;  // 4103
    unsigned int v431;  // eax
    unsigned int v432;  // 4111
    char *v433;  // ecx
    unsigned int v56;  // cc_dep2
    char *v434;  // eax
    char v435;  // 4101
    unsigned int v436;  // cc_dep1
    unsigned int v437;  // cc_dep2
    char v438;  // 4103
    unsigned int v439;  // eax
    unsigned int v440;  // 4111
    char *v441;  // ecx
    char *v442;  // eax
    char v443;  // 4101
    char v57;  // 4103
    unsigned int v444;  // cc_dep1
    unsigned int v445;  // cc_dep2
    char v446;  // 4103
    unsigned int v447;  // eax
    unsigned int v448;  // 4111
    char *v449;  // ecx
    char *v450;  // eax
    char v451;  // 4101
    unsigned int v452;  // cc_dep1
    unsigned int v453;  // cc_dep2
    unsigned int v58;  // eax
    char v454;  // 4103
    unsigned int v455;  // eax
    unsigned int v456;  // 4111
    char *v457;  // esi
    int v458;  // eax
    char *v459;  // ecx
    char *v460;  // eax
    char v461;  // 4101
    unsigned int v462;  // cc_dep1
    unsigned int v463;  // cc_dep2
    unsigned int v59;  // 4111
    char v464;  // 4103
    unsigned int v465;  // eax
    unsigned int v466;  // 4111
    int v467;  // esi
    char *v468;  // ecx
    char *v469;  // eax
    char v470;  // 4101
    unsigned int v471;  // cc_dep1
    unsigned int v472;  // cc_dep2
    char v473;  // 4103
    int v60;  // eax
    unsigned int v474;  // eax
    unsigned int v475;  // 4111
    st_473ed0_3 *v476;  // edi
    int v477;  // esi
    char *v478;  // ecx
    char *v479;  // eax
    char v480;  // 4101
    unsigned int v481;  // cc_dep1
    unsigned int v482;  // cc_dep2
    char v483;  // 4103
    char *v61;  // eax
    unsigned int v484;  // eax
    unsigned int v485;  // 4111
    int v486;  // eax
    int v487;  // eax
    char *v488;  // edx
    char *v489;  // ecx
    char *v490;  // eax
    char v491;  // 4101
    unsigned int v492;  // cc_dep1
    unsigned int v493;  // cc_dep2
    char *v62;  // ecx
    char v494;  // 4103
    unsigned int v495;  // eax
    unsigned int v496;  // 4111
    char *v497;  // ecx
    char *v498;  // eax
    char v499;  // 4101
    unsigned int v500;  // cc_dep1
    unsigned int v501;  // cc_dep2
    char v502;  // 4103
    unsigned int v503;  // eax
    char v63;  // 4101
    unsigned int v504;  // 4111
    int v505;  // eax
    char *v506;  // ecx
    char *v507;  // eax
    char v508;  // 4101
    unsigned int v509;  // cc_dep1
    unsigned int v510;  // cc_dep2
    char v511;  // 4103
    unsigned int v512;  // eax
    unsigned int v513;  // 4111
    void* v19;  // esi
    unsigned int v64;  // cc_dep1
    int v514;  // eax
    char *v515;  // ecx
    char *v516;  // eax
    char v517;  // 4101
    unsigned int v518;  // cc_dep1
    unsigned int v519;  // cc_dep2
    char v520;  // 4103
    unsigned int v521;  // eax
    unsigned int v522;  // 4111
    char *v523;  // eax
    unsigned int v65;  // cc_dep2
    char v524;  // 4101
    unsigned int v525;  // cc_dep1
    unsigned int v526;  // cc_dep2
    char v527;  // 4103
    unsigned int v528;  // eax
    unsigned int v529;  // 4111
    unsigned int v530;  // eax
    unsigned int v531;  // ecx
    unsigned int v532;  // eax
    char *v533;  // ecx
    char v66;  // 4103
    char *v534;  // eax
    char v535;  // 4101
    unsigned int v536;  // cc_dep1
    unsigned int v537;  // cc_dep2
    char v538;  // 4103
    unsigned int v539;  // eax
    unsigned int v540;  // 4111
    char *v541;  // eax
    char *v542;  // ecx
    char v543;  // 4101
    unsigned int v67;  // eax
    unsigned int v544;  // cc_dep1
    unsigned int v545;  // cc_dep2
    char v546;  // 4103
    unsigned int v547;  // eax
    unsigned int v548;  // 4111
    char *v549;  // ecx
    char *v550;  // eax
    char v551;  // 4101
    unsigned int v552;  // cc_dep1
    unsigned int v553;  // cc_dep2
    unsigned int v68;  // 4111
    char v554;  // 4103
    unsigned int v555;  // eax
    unsigned int v556;  // 4111
    char *v557;  // ecx
    char *v558;  // eax
    char v559;  // 4101
    unsigned int v560;  // cc_dep1
    unsigned int v561;  // cc_dep2
    char v562;  // 4103
    unsigned int v563;  // eax
    char *v69;  // ecx
    unsigned int v564;  // 4111
    char *v565;  // ecx
    char *v566;  // eax
    char v567;  // 4101
    unsigned int v568;  // cc_dep1
    unsigned int v569;  // cc_dep2
    char v570;  // 4103
    unsigned int v571;  // eax
    unsigned int v572;  // 4111
    char *v573;  // ecx
    char *v70;  // eax
    char *v574;  // eax
    char v575;  // 4101
    unsigned int v576;  // cc_dep1
    unsigned int v577;  // cc_dep2
    char v578;  // 4103
    unsigned int v579;  // eax
    unsigned int v580;  // 4111
    char *v581;  // ecx
    char *v582;  // eax
    char v583;  // 4101
    char v71;  // 4101
    unsigned int v584;  // cc_dep1
    unsigned int v585;  // cc_dep2
    char v586;  // 4103
    unsigned int v587;  // eax
    unsigned int v588;  // 4111
    int v589;  // ebx
    int v590;  // edi
    char *v591;  // ecx
    char *v592;  // eax
    char v593;  // 4101
    unsigned int v72;  // cc_dep1
    unsigned int v594;  // cc_dep1
    unsigned int v595;  // cc_dep2
    char v596;  // 4103
    unsigned int v597;  // eax
    unsigned int v598;  // 4111
    int v599;  // eax
    void* v600;  // edi
    void* v601;  // esi
    int v602;  // eax
    unsigned int v603;  // eax
    unsigned int v73;  // cc_dep2
    unsigned int v604;  // eax
    unsigned int v605;  // ecx
    char *v606;  // eax
    char *v607;  // ecx
    char v608;  // 4101
    unsigned int v609;  // cc_dep1
    unsigned int v610;  // cc_dep2
    char v611;  // 4103
    unsigned int v612;  // eax
    unsigned int v613;  // 4111
    void* v20;  // esi
    char v74;  // 4103
    int v614;  // edi
    int v615;  // edi
    char *v616;  // ecx
    char *v617;  // eax
    char v618;  // 4101
    unsigned int v619;  // cc_dep1
    unsigned int v620;  // cc_dep2
    char v621;  // 4103
    unsigned int v622;  // eax
    unsigned int v623;  // 4111
    unsigned int v75;  // eax
    st_473ed0_1 *v624;  // edi
    unsigned int v625;  // eax
    unsigned int v626;  // eax
    unsigned int index;  // eax
    char *v628;  // eax
    char *v629;  // ecx
    char v630;  // 4101
    unsigned int v631;  // cc_dep1
    unsigned int v632;  // cc_dep2
    char v633;  // 4103
    unsigned int v76;  // 4111
    unsigned int v634;  // eax
    unsigned int v635;  // 4111
    char *v636;  // ecx
    char *v637;  // eax
    char v638;  // 4101
    unsigned int v639;  // cc_dep1
    unsigned int v640;  // cc_dep2
    char v641;  // 4103
    unsigned int v642;  // eax
    unsigned int v643;  // 4111
    char *v77;  // ecx
    int v644;  // eax
    unsigned int v645;  // eax
    char *v646;  // ecx
    char *v647;  // eax
    char v648;  // 4101
    unsigned int v649;  // cc_dep1
    unsigned int v650;  // cc_dep2
    char v651;  // 4103
    unsigned int v652;  // eax
    unsigned int v653;  // 4111
    char *v78;  // eax
    st_473ed0_3 *v654;  // edi
    int v655;  // eax
    unsigned int v656;  // eax
    char *v657;  // ecx
    char *v658;  // eax
    char v659;  // 4101
    unsigned int v660;  // cc_dep1
    unsigned int v661;  // cc_dep2
    char v662;  // 4103
    unsigned int v663;  // eax
    st_473ed0_3 *v79;  // edi
    unsigned int v664;  // 4111
    unsigned int v665;  // edi
    unsigned int v666;  // edi
    unsigned int v667;  // edi
    unsigned int v668;  // ecx
    void* n;  // esi
    char *v670;  // ecx
    char *v671;  // eax
    char v672;  // 4101
    unsigned int v673;  // cc_dep1
    char v80;  // 4101
    unsigned int v674;  // cc_dep2
    char v675;  // 4103
    unsigned int v676;  // eax
    unsigned int v677;  // 4111
    char *v678;  // ecx
    char *v679;  // eax
    char v680;  // 4101
    unsigned int v681;  // cc_dep1
    unsigned int v682;  // cc_dep2
    char v683;  // 4103
    unsigned int v81;  // cc_dep1
    unsigned int v684;  // eax
    unsigned int v685;  // 4111
    unsigned int v686;  // eax
    unsigned int v687;  // eax
    unsigned int idx1;  // eax
    char *v689;  // ecx
    char *v690;  // eax
    char v691;  // 4101
    unsigned int v692;  // cc_dep1
    unsigned int v693;  // cc_dep2
    unsigned int v82;  // cc_dep2
    char v694;  // 4103
    unsigned int v695;  // eax
    unsigned int v696;  // 4111
    unsigned int v697;  // edi
    unsigned int v698;  // edi
    unsigned int v699;  // edi
    unsigned int v700;  // ecx
    void* i0;  // esi
    char *v702;  // ecx
    char *v703;  // eax
    char v83;  // 4103
    char v704;  // 4101
    unsigned int v705;  // cc_dep1
    unsigned int v706;  // cc_dep2
    char v707;  // 4103
    unsigned int v708;  // eax
    unsigned int v709;  // 4111
    int v710;  // eax
    unsigned int v711;  // eax
    int v712;  // eax
    unsigned int v713;  // eax
    void* v21;  // esi
    unsigned int v84;  // eax
    char *v714;  // ecx
    char *v715;  // eax
    st_473ed0_3 *v716;  // edi
    char v717;  // 4101
    unsigned int v718;  // cc_dep1
    unsigned int v719;  // cc_dep2
    char v720;  // 4103
    unsigned int v721;  // eax
    unsigned int v722;  // 4111
    unsigned int v85;  // 4111
    st_473ed0_1 *v723;  // esi
    char *v724;  // ecx
    char *v725;  // eax
    char v726;  // 4101
    unsigned int v727;  // cc_dep1
    unsigned int v728;  // cc_dep2
    char v729;  // 4103
    unsigned int v730;  // eax
    unsigned int v731;  // 4111
    st_473ed0_1 *v732;  // esi
    char *v86;  // ecx
    char *v733;  // ecx
    char *v734;  // eax
    char v735;  // 4101
    unsigned int v736;  // cc_dep1
    unsigned int v737;  // cc_dep2
    char v738;  // 4103
    unsigned int v739;  // eax
    unsigned int v740;  // 4111
    char *v741;  // ecx
    char *v742;  // eax
    char *v87;  // eax
    char v743;  // 4101
    unsigned int v744;  // cc_dep1
    unsigned int v745;  // cc_dep2
    char v746;  // 4103
    unsigned int v747;  // eax
    unsigned int v748;  // 4111
    int v749;  // eax
    int v750;  // eax
    char *v751;  // ecx
    char *v752;  // eax
    char v88;  // 4101
    char v753;  // 4101
    unsigned int v754;  // cc_dep1
    unsigned int v755;  // cc_dep2
    char v756;  // 4103
    unsigned int v757;  // eax
    unsigned int v758;  // 4111
    char *v759;  // ecx
    char *v760;  // eax
    char v761;  // 4101
    unsigned int v762;  // cc_dep1
    unsigned int v89;  // cc_dep1
    unsigned int v763;  // cc_dep2
    char v764;  // 4103
    unsigned int v765;  // eax
    unsigned int v766;  // 4111
    char *v767;  // ecx
    char *v768;  // eax
    st_473ed0_3 *v769;  // edi
    char v770;  // 4101
    unsigned int v771;  // cc_dep1
    unsigned int v772;  // cc_dep2
    unsigned int v90;  // cc_dep2
    char v773;  // 4103
    unsigned int v774;  // eax
    unsigned int v775;  // 4111
    int v776;  // eax
    char *v777;  // eax
    char *v778;  // ecx
    char v779;  // 4101
    unsigned int v780;  // cc_dep1
    unsigned int v781;  // cc_dep2
    char v782;  // 4103
    char v91;  // 4103
    unsigned int v783;  // eax
    unsigned int v784;  // 4111
    char *v785;  // ecx
    char *v786;  // eax
    void* v787;  // ebx
    char v788;  // 4101
    unsigned int v789;  // cc_dep1
    unsigned int v790;  // cc_dep2
    char v791;  // 4103
    unsigned int v792;  // eax
    unsigned int v92;  // eax
    unsigned int v793;  // 4111
    char *v794;  // ecx
    char *v795;  // eax
    char v796;  // 4101
    unsigned int v797;  // cc_dep1
    unsigned int v798;  // cc_dep2
    char v799;  // 4103
    unsigned int v800;  // eax
    unsigned int v801;  // 4111
    unsigned int v802;  // eax
    unsigned int v93;  // 4111
    unsigned int v803;  // eax
    unsigned int idx2;  // eax
    char *v805;  // ecx
    char *v806;  // eax
    char v807;  // 4101
    unsigned int v808;  // cc_dep1
    unsigned int v809;  // cc_dep2
    char v810;  // 4103
    unsigned int v811;  // eax
    unsigned int v812;  // 4111
    void* v22;  // esi
    int v94;  // eax
    char *v813;  // ecx
    char *v814;  // eax
    char v815;  // 4101
    unsigned int v816;  // cc_dep1
    unsigned int v817;  // cc_dep2
    char v818;  // 4103
    unsigned int v819;  // eax
    unsigned int v820;  // 4111
    int v821;  // esi
    char *v822;  // esi
    int v95;  // eax
    void* v823;  // ebp
    int v824;  // ebx
    st_473ed0_4 *v825;  // edi
    st_473ed0_3 *v826;  // esi
    char *v827;  // eax
    char v828;  // cl
    char *v829;  // eax
    char *v830;  // eax
    char *v831;  // ecx
    char *v832;  // eax
    char *v96;  // ecx
    st_473ed0_3 *v833;  // edi
    char v834;  // 4101
    unsigned int v835;  // cc_dep1
    unsigned int v836;  // cc_dep2
    char v837;  // 4103
    unsigned int v838;  // eax
    unsigned int v839;  // 4111
    char *v840;  // esi
    st_473ed0_4 *v841;  // edi
    char *v842;  // eax
    char *v97;  // eax
    char v843;  // cl
    int v844;  // ecx
    char *v845;  // eax
    char *v846;  // ecx
    char *v847;  // eax
    char v848;  // 4101
    unsigned int v849;  // cc_dep1
    unsigned int v850;  // cc_dep2
    char v851;  // 4103
    unsigned int v852;  // eax
    char v98;  // 4101
    unsigned int v853;  // 4111
    int v854;  // eax
    char *v855;  // eax
    char *v856;  // ecx
    char v857;  // 4101
    unsigned int v858;  // cc_dep1
    unsigned int v859;  // cc_dep2
    char v860;  // 4103
    unsigned int v861;  // eax
    unsigned int v862;  // 4111
    unsigned int v99;  // cc_dep1
    int v863;  // eax
    char *v864;  // ecx
    char *v865;  // eax
    char v866;  // 4101
    unsigned int v867;  // cc_dep1
    unsigned int v868;  // cc_dep2
    char v869;  // 4103
    unsigned int v870;  // eax
    unsigned int v871;  // 4111
    char *v872;  // eax
    unsigned int v100;  // cc_dep2
    char *v873;  // ecx
    char v874;  // 4101
    unsigned int v875;  // cc_dep1
    unsigned int v876;  // cc_dep2
    char v877;  // 4103
    unsigned int v878;  // eax
    unsigned int v879;  // 4111
    char *v880;  // ecx
    char *v881;  // eax
    char v882;  // 4101
    char v101;  // 4103
    unsigned int v883;  // cc_dep1
    unsigned int v884;  // cc_dep2
    char v885;  // 4103
    unsigned int v886;  // eax
    unsigned int v887;  // 4111
    char *v888;  // ecx
    char *v889;  // eax
    char v890;  // 4101
    unsigned int v891;  // cc_dep1
    unsigned int v892;  // cc_dep2
    unsigned int v102;  // eax
    char v893;  // 4103
    unsigned int v894;  // eax
    unsigned int v895;  // 4111
    int v896;  // eax
    st_473ed0_1 *v897;  // esi
    int v898;  // eax
    unsigned int v899;  // eax
    char *v900;  // ecx
    char *v901;  // eax
    char v902;  // 4101
    unsigned int v103;  // 4111
    unsigned int v903;  // cc_dep1
    unsigned int v904;  // cc_dep2
    char v905;  // 4103
    unsigned int v906;  // eax
    unsigned int v907;  // 4111
    char *v908;  // esi
    int v909;  // ebx
    st_473ed0_4 *v910;  // edi
    char *v911;  // eax
    char v912;  // cl
    void* v23;  // esi
    st_473ed0_1 *ptr2;  // esi
    char *v913;  // eax
    char *v914;  // eax
    int v915;  // edi
    st_473ed0_3 *v916;  // ebp
    int v917;  // eax
    st_473ed0_3 *v918;  // edi
    unsigned int v105;  // edi
    unsigned int v106;  // edi
    unsigned int v107;  // edi
    unsigned int v108;  // ecx
    void* j;  // esi
    char *v110;  // eax
    char *v111;  // ecx
    char v112;  // 4101
    unsigned int v113;  // cc_dep1
    void* v0;  // [bp-0x138]
    st_473ed0_3 *v1;  // [bp-0x134]
    unsigned int v2;  // [bp-0x130]
    unsigned int v3;  // [bp-0x12c]
    char *v4;  // [bp-0x128]
    char *v5;  // [bp-0x124], Other Possible Types: int, unsigned int
    int v6;  // [bp-0x120]
    int v7;  // [bp-0x11c]
    int v8;  // [bp-0x118]
    int v9;  // [bp-0x114]
    char *v10;  // [bp-0x10c], Other Possible Types: char
    int v11;  // [bp-0x108]
    int v12;  // [bp-0x104]
    char v13;  // [bp-0x100]

    v14 = a1;
    v15 = a2;
    v16 = sub_433410(a0, "type", v6, v7, v8, v9);
    if (v16)
    {
        v17 = v16;
        do
        {
            i = *v17;
            *(&v13 - v16 + v17) = *v17;
            v17 += 1;
        } while (i);
        _strlwr(&v13);
        v19 = v14 + 24;
        if ((int)v14[32] != 0xffffffff && (int)v14[32])
            sub_438c40(v19);
        *(unsigned int *)&v19[8] = 0xffffffff;
        *(unsigned int *)v19 = 0;
        *(char *)&v19[4] = 0;
        v20 = v14 + 36;
        if ((int)v14[44] != 0xffffffff && (int)v14[44])
            sub_438c40(v20);
        *(unsigned int *)&v20[8] = 0xffffffff;
        *(unsigned int *)v20 = 0;
        *(char *)&v20[4] = 0;
        v21 = v14 + 48;
        if ((int)v14[56] != 0xffffffff && (int)v14[56])
            sub_438c40(v21);
        *(unsigned int *)&v21[8] = 0xffffffff;
        *(unsigned int *)v21 = 0;
        *(char *)&v21[4] = 0;
        v22 = v14 + 60;
        if ((int)v14[0x44] != 0xffffffff && (int)v14[0x44])
            sub_438c40(v22);
        *(unsigned int *)&v22[8] = 0xffffffff;
        *(unsigned int *)v22 = 0;
        *(char *)&v22[4] = 0;
        v23 = v14 + 72;
        if ((int)v14[80] != 0xffffffff && (int)v14[80])
            sub_438c40(v23);
        *(unsigned int *)&v23[8] = 0xffffffff;
        *(unsigned int *)v23 = 0;
        *(char *)&v23[4] = 0;
        v24 = v14 + 84;
        if ((int)v14[92] != 0xffffffff && (int)v14[92])
            sub_438c40(v24);
        *(unsigned int *)&v24[8] = 0xffffffff;
        *(unsigned int *)v24 = 0;
        *(char *)&v24[4] = 0;
        v25 = "changestate";
        iter2 = &v13;
        while (1)
        {
            v27 = *iter2;
            v28 = v27;
            v29 = *v25;
            if (v27 != *v25)
                break;
            if (v27)
            {
                v30 = iter2[1];
                v28 = v30;
                v29 = v25[1];
                if (v30 != v25[1])
                    break;
                iter2 += 2;
                v25 += 2;
                if (!v30)
                    goto LABEL_474043;
            }
            else
            {
LABEL_474043:
                v31 = 0;
                goto LABEL_47404c;
            }
        }
        v32 = _ccall(4, v28, v29, 0);
        v31 = -(v32 & 1) + 1 - (unsigned int)(v32 & 1 && 1);
LABEL_47404c:
        if (!v31)
        {
            *(unsigned int *)&v14[16] = 1;
            v33 = sub_433410(a0, "value");
            if (v33 && sub_438b20(v33, v14 + 24, v15, 0, 1))
            {
                sub_438d20(-0x1);
                v34 = v14 + 48;
                sub_438d20(-0x1);
                if (sub_438b20(sub_433410(a0, "ctrl"), v14 + 36, a2, 0, 1))
                    goto LABEL_4740c5;
            }
        }
        else
        {
            v35 = "tagin";
            v36 = &v13;
            while (1)
            {
                v37 = *v36;
                v38 = v37;
                v39 = *v35;
                if (v37 != *v35)
                    break;
                if (v37)
                {
                    v40 = v36[1];
                    v38 = v40;
                    v39 = v35[1];
                    if (v40 != v35[1])
                        break;
                    v36 += 2;
                    v35 += 2;
                    if (!v40)
                        goto LABEL_474120;
                }
                else
                {
LABEL_474120:
                    v41 = 0;
                    goto LABEL_474129;
                }
            }
            v42 = _ccall(4, v38, v39, 0);
            v41 = -(v42 & 1) + 1 - (unsigned int)(v42 & 1 && 1);
LABEL_474129:
            if (!v41)
            {
                *(unsigned int *)&v14[16] = 551;
                v43 = sub_433410(a0, "stateno");
                if (v43 && sub_438b20(v43, v14 + 24, v15, 0, 1))
                {
                    sub_438d20(-0x1);
                    v34 = v14 + 48;
                    sub_438d20(-0x1);
                    if (sub_438b20(sub_433410(a0, "partnerstateno"), v14 + 36, a2, 0, 1))
                    {
                        v5 = "ctrl";
                        goto LABEL_4740ca;
                    }
                }
            }
            else
            {
                v44 = "tagout";
                v45 = &v13;
                while (1)
                {
                    v46 = *v45;
                    v47 = v46;
                    v48 = *v44;
                    if (v46 != *v44)
                        break;
                    if (v46)
                    {
                        v49 = v45[1];
                        v47 = v49;
                        v48 = v44[1];
                        if (v49 != v44[1])
                            break;
                        v45 += 2;
                        v44 += 2;
                        if (!v49)
                            goto LABEL_4741cf;
                    }
                    else
                    {
LABEL_4741cf:
                        v50 = 0;
                        goto LABEL_4741d8;
                    }
                }
                v51 = _ccall(4, v47, v48, 0);
                v50 = -(v51 & 1) + 1 - (unsigned int)(v51 & 1 && 1);
LABEL_4741d8:
                if (!v50)
                {
                    *(unsigned int *)&v14[16] = 552;
                    goto LABEL_476430;
                }
                v52 = "selfstate";
                v53 = &v13;
                while (1)
                {
                    v54 = *v53;
                    v55 = v54;
                    v56 = *v52;
                    if (v54 != *v52)
                        break;
                    if (v54)
                    {
                        v57 = v53[1];
                        v55 = v57;
                        v56 = v52[1];
                        if (v57 != v52[1])
                            break;
                        v53 += 2;
                        v52 += 2;
                        if (!v57)
                            goto LABEL_47420d;
                    }
                    else
                    {
LABEL_47420d:
                        v58 = 0;
                        goto LABEL_474216;
                    }
                }
                v59 = _ccall(4, v55, v56, 0);
                v58 = -(v59 & 1) + 1 - (unsigned int)(v59 & 1 && 1);
LABEL_474216:
                if (!v58)
                {
                    *(unsigned int *)&v14[16] = 2;
                    v60 = sub_433410(a0, "value");
                    if (v60 && sub_438b20(v60, v14 + 24, v15, 0, 1))
                    {
                        sub_438d20(-0x1);
                        v34 = v14 + 48;
                        sub_438d20(-0x1);
                        if (!sub_438b20(sub_433410(a0, "ctrl"), v14 + 36, a2, 0, 1))
                            return NULL;
LABEL_4740c5:
                        v5 = "anim";
                        goto LABEL_4740ca;
                    }
                }
                else
                {
                    v61 = &v13;
                    v62 = "ctrlset";
                    while (1)
                    {
                        v63 = *v61;
                        v64 = v63;
                        v65 = *v62;
                        if (v63 != *v62)
                            break;
                        if (v63)
                        {
                            v66 = v61[1];
                            v64 = v66;
                            v65 = v62[1];
                            if (v66 != v62[1])
                                break;
                            v61 += 2;
                            v62 += 2;
                            if (!v66)
                                goto LABEL_4742cc;
                        }
                        else
                        {
LABEL_4742cc:
                            v67 = 0;
                            goto LABEL_4742d5;
                        }
                    }
                    v68 = _ccall(4, v64, v65, 0);
                    v67 = -(v68 & 1) + 1 - (unsigned int)(v68 & 1 && 1);
LABEL_4742d5:
                    if (!v67)
                    {
                        *(unsigned int *)&v14[16] = 20;
                        goto LABEL_4742e0;
                    }
                    v69 = "turn";
                    v70 = &v13;
                    while (1)
                    {
                        v71 = *v70;
                        v72 = v71;
                        v73 = *v69;
                        if (v71 != *v69)
                            break;
                        if (v71)
                        {
                            v74 = v70[1];
                            v72 = v74;
                            v73 = v69[1];
                            if (v74 != v69[1])
                                break;
                            v70 += 2;
                            v69 += 2;
                            if (!v74)
                                goto LABEL_47432c;
                        }
                        else
                        {
LABEL_47432c:
                            v75 = 0;
                            goto LABEL_474335;
                        }
                    }
                    v76 = _ccall(4, v72, v73, 0);
                    v75 = -(v76 & 1) + 1 - (unsigned int)(v76 & 1 && 1);
LABEL_474335:
                    if (!v75)
                    {
                        *(unsigned int *)&v14[16] = 3;
                        goto LABEL_476430;
                    }
                    v77 = "changeanim";
                    v78 = &v13;
                    v79 = v15;
                    while (1)
                    {
                        v80 = *v78;
                        v81 = v80;
                        v82 = *v77;
                        if (v80 != *v77)
                            break;
                        if (v80)
                        {
                            v83 = v78[1];
                            v81 = v83;
                            v82 = v77[1];
                            if (v83 != v77[1])
                                break;
                            v78 += 2;
                            v77 += 2;
                            if (!v83)
                                goto LABEL_47436c;
                        }
                        else
                        {
LABEL_47436c:
                            v84 = 0;
                            goto LABEL_474375;
                        }
                    }
                    v85 = _ccall(4, v81, v82, 0);
                    v84 = -(v85 & 1) + 1 - (unsigned int)(v85 & 1 && 1);
LABEL_474375:
                    if (!v84)
                    {
                        *(unsigned int *)&v14[16] = 32;
                        goto LABEL_474380;
                    }
                    v86 = "changeanim2";
                    v87 = &v13;
                    while (1)
                    {
                        v88 = *v87;
                        v89 = v88;
                        v90 = *v86;
                        if (v88 != *v86)
                            break;
                        if (v88)
                        {
                            v91 = v87[1];
                            v89 = v91;
                            v90 = v86[1];
                            if (v91 != v86[1])
                                break;
                            v87 += 2;
                            v86 += 2;
                            if (!v91)
                                goto LABEL_4743ec;
                        }
                        else
                        {
LABEL_4743ec:
                            v92 = 0;
                            goto LABEL_4743f5;
                        }
                    }
                    v93 = _ccall(4, v89, v90, 0);
                    v92 = -(v93 & 1) + 1 - (unsigned int)(v93 & 1 && 1);
LABEL_4743f5:
                    if (!v92)
                    {
                        *(unsigned int *)&v14[16] = 33;
LABEL_474380:
                        v94 = sub_433410(a0, "value");
                        if (v94 && sub_438b20(v94, v14 + 24, v79, 0, 1))
                        {
                            v95 = sub_433410(a0, "elem");
                            v3 = 1;
                            v2 = 0;
                            v1 = v79;
                            v0 = v14 + 36;
                            goto LABEL_4740dd;
                        }
                    }
                    else
                    {
                        v96 = "playsnd";
                        v97 = &v13;
                        v15 = v79;
                        while (1)
                        {
                            v98 = *v97;
                            v99 = v98;
                            v100 = *v96;
                            if (v98 != *v96)
                                break;
                            if (v98)
                            {
                                v101 = v97[1];
                                v99 = v101;
                                v100 = v96[1];
                                if (v101 != v96[1])
                                    break;
                                v97 += 2;
                                v96 += 2;
                                if (!v101)
                                    goto LABEL_47442c;
                            }
                            else
                            {
LABEL_47442c:
                                v102 = 0;
                                goto LABEL_474435;
                            }
                        }
                        v103 = _ccall(4, v99, v100, 0);
                        v102 = -(v103 & 1) + 1 - (unsigned int)(v103 & 1 && 1);
LABEL_474435:
                        if (!v102)
                        {
                            ptr2 = sub_40e1b0();
                            if (ptr2)
                            {
                                *(unsigned int *)&v14[16] = 0x22;
                                if (!sub_47d2f0(a0, v15))
                                {
                                    free(ptr2);
                                    v105 = &g_52b40c->field_1205b;
                                    do
                                    {
                                        v106 = v105;
                                        v107 = v106 + 1;
                                        v105 = v107;
                                    } while (*(char *)(v106 + 1));
                                    v108 = 5;
                                    for (j = "\nError parsing PlaySnd"; v108; j += 4)
                                    {
                                        v108 -= 1;
                                        *(int *)v107 = *(int *)j;
                                        v107 += 4;
                                    }
                                    *(short *)v107 = *(short *)j;
                                    *(char *)(v107 + 2) = (char)j[2];
                                    return NULL;
                                }
                                goto LABEL_476e7b;
                            }
                        }
                        else
                        {
                            v110 = &v13;
                            v111 = "stopsnd";
                            while (1)
                            {
                                v112 = *v110;
                                v113 = v112;
                                v114 = *v111;
                                if (v112 != *v111)
                                    break;
                                if (v112)
                                {
                                    v115 = v110[1];
                                    v113 = v115;
                                    v114 = v111[1];
                                    if (v115 != v111[1])
                                        break;
                                    v110 += 2;
                                    v111 += 2;
                                    if (!v115)
                                        goto LABEL_4744cc;
                                }
                                else
                                {
LABEL_4744cc:
                                    v116 = 0;
                                    goto LABEL_4744d5;
                                }
                            }
                            v117 = _ccall(4, v113, v114, 0);
                            v116 = -(v117 & 1) + 1 - (unsigned int)(v117 & 1 && 1);
LABEL_4744d5:
                            if (!v116)
                            {
                                *(unsigned int *)&v14[16] = 35;
                                v118 = sub_433410(a0, "channel");
                                if (!v118)
                                {
                                    sprintf(&g_52b40c->field_1205c, "Missing channel parameter");
                                    return NULL;
                                }
                                goto LABEL_476a86;
                            }
                            v119 = "sndpan";
                            iter = &v13;
                            while (1)
                            {
                                v121 = *iter;
                                v122 = v121;
                                v123 = *v119;
                                if (v121 != *v119)
                                    break;
                                if (v121)
                                {
                                    v124 = iter[1];
                                    v122 = v124;
                                    v123 = v119[1];
                                    if (v124 != v119[1])
                                        break;
                                    iter += 2;
                                    v119 += 2;
                                    if (!v124)
                                        goto LABEL_474543;
                                }
                                else
                                {
LABEL_474543:
                                    v125 = 0;
                                    goto LABEL_47454c;
                                }
                            }
                            v126 = _ccall(4, v122, v123, 0);
                            v125 = -(v126 & 1) + 1 - (unsigned int)(v126 & 1 && 1);
LABEL_47454c:
                            if (!v125)
                            {
                                *(unsigned int *)&v14[16] = 36;
                                v127 = sub_433410(a0, "channel");
                                if (!v127)
                                {
                                    sprintf(&g_52b40c->field_1205c, "Missing channel parameter");
                                    return NULL;
                                }
                                if (sub_438b20(v127, v14 + 24, v15, 0, 1) && (v128 = sub_433410(a0, "abspan"), !v128 || (sub_438d20(1), sub_438b20(v128, v14 + 36, a2, 0, 1))))
                                {
                                    v95 = sub_433410(a0, "pan");
                                    v3 = 1;
                                    v2 = 0;
                                    v1 = a2;
                                    v0 = v14 + 36;
                                    goto LABEL_4740dd;
                                }
                            }
                            else
                            {
                                v129 = "lifeset";
                                iter1 = &v13;
                                while (1)
                                {
                                    v131 = *iter1;
                                    v132 = v131;
                                    v133 = *v129;
                                    if (v131 != *v129)
                                        break;
                                    if (v131)
                                    {
                                        v134 = iter1[1];
                                        v132 = v134;
                                        v133 = v129[1];
                                        if (v134 != v129[1])
                                            break;
                                        iter1 += 2;
                                        v129 += 2;
                                        if (!v134)
                                            goto LABEL_47463c;
                                    }
                                    else
                                    {
LABEL_47463c:
                                        v135 = 0;
                                        goto LABEL_474645;
                                    }
                                }
                                v136 = _ccall(4, v132, v133, 0);
                                v135 = -(v136 & 1) + 1 - (unsigned int)(v136 & 1 && 1);
LABEL_474645:
                                if (!v135)
                                {
                                    *(unsigned int *)&v14[16] = 10;
                                    goto LABEL_4742e0;
                                }
                                v137 = "lifeadd";
                                v138 = &v13;
                                while (1)
                                {
                                    v139 = *v138;
                                    v140 = v139;
                                    v141 = *v137;
                                    if (v139 != *v137)
                                        break;
                                    if (v139)
                                    {
                                        v142 = v138[1];
                                        v140 = v142;
                                        v141 = v137[1];
                                        if (v142 != v137[1])
                                            break;
                                        v138 += 2;
                                        v137 += 2;
                                        if (!v142)
                                            goto LABEL_47467c;
                                    }
                                    else
                                    {
LABEL_47467c:
                                        v143 = 0;
                                        goto LABEL_474685;
                                    }
                                }
                                v144 = _ccall(4, v140, v141, 0);
                                v143 = -(v144 & 1) + 1 - (unsigned int)(v144 & 1 && 1);
LABEL_474685:
                                if (!v143)
                                {
                                    *(unsigned int *)&v14[16] = 11;
                                    v145 = sub_433410(a0, "value");
                                    if (v145 && sub_438b20(v145, v14 + 24, v15, 0, 1) && (sub_438d20(1), sub_438b20(sub_433410(a0, "kill"), v14 + 36, v15, 0, 1)))
                                    {
                                        v95 = sub_433410(a0, "absolute");
                                        v3 = 1;
                                        v2 = 0;
                                        v1 = v15;
                                        v0 = v14 + 48;
                                        goto LABEL_4740dd;
                                    }
                                }
                                else
                                {
                                    v146 = "powerset";
                                    v147 = &v13;
                                    while (1)
                                    {
                                        v148 = *v147;
                                        v149 = v148;
                                        v150 = *v146;
                                        if (v148 != *v146)
                                            break;
                                        if (v148)
                                        {
                                            v151 = v147[1];
                                            v149 = v151;
                                            v150 = v146[1];
                                            if (v151 != v146[1])
                                                break;
                                            v147 += 2;
                                            v146 += 2;
                                            if (!v151)
                                                goto LABEL_474730;
                                        }
                                        else
                                        {
LABEL_474730:
                                            v152 = 0;
                                            goto LABEL_474739;
                                        }
                                    }
                                    v153 = _ccall(4, v149, v150, 0);
                                    v152 = -(v153 & 1) + 1 - (unsigned int)(v153 & 1 && 1);
LABEL_474739:
                                    if (!v152)
                                    {
                                        *(unsigned int *)&v14[16] = 12;
                                        goto LABEL_4742e0;
                                    }
                                    v154 = "poweradd";
                                    v155 = &v13;
                                    while (1)
                                    {
                                        v156 = *v155;
                                        v157 = v156;
                                        v158 = *v154;
                                        if (v156 != *v154)
                                            break;
                                        if (v156)
                                        {
                                            v159 = v155[1];
                                            v157 = v159;
                                            v158 = v154[1];
                                            if (v159 != v154[1])
                                                break;
                                            v155 += 2;
                                            v154 += 2;
                                            if (!v159)
                                                goto LABEL_47476e;
                                        }
                                        else
                                        {
LABEL_47476e:
                                            v160 = 0;
                                            goto LABEL_474777;
                                        }
                                    }
                                    v161 = _ccall(4, v157, v158, 0);
                                    v160 = -(v161 & 1) + 1 - (unsigned int)(v161 & 1 && 1);
LABEL_474777:
                                    if (!v160)
                                    {
                                        *(unsigned int *)&v14[16] = 13;
                                        goto LABEL_4742e0;
                                    }
                                    v162 = "posset";
                                    v163 = &v13;
                                    while (1)
                                    {
                                        v164 = *v163;
                                        v165 = v164;
                                        v166 = *v162;
                                        if (v164 != *v162)
                                            break;
                                        if (v164)
                                        {
                                            v167 = v163[1];
                                            v165 = v167;
                                            v166 = v162[1];
                                            if (v167 != v162[1])
                                                break;
                                            v163 += 2;
                                            v162 += 2;
                                            if (!v167)
                                                goto LABEL_4747ac;
                                        }
                                        else
                                        {
LABEL_4747ac:
                                            v168 = 0;
                                            goto LABEL_4747b5;
                                        }
                                    }
                                    v169 = _ccall(4, v165, v166, 0);
                                    v168 = -(v169 & 1) + 1 - (unsigned int)(v169 & 1 && 1);
LABEL_4747b5:
                                    if (!v168)
                                    {
                                        *(unsigned int *)&v14[16] = 21;
                                        goto LABEL_4747c4;
                                    }
                                    v170 = "posadd";
                                    v171 = &v13;
                                    while (1)
                                    {
                                        v172 = *v171;
                                        v173 = v172;
                                        v174 = *v170;
                                        if (v172 != *v170)
                                            break;
                                        if (v172)
                                        {
                                            v175 = v171[1];
                                            v173 = v175;
                                            v174 = v170[1];
                                            if (v175 != v170[1])
                                                break;
                                            v171 += 2;
                                            v170 += 2;
                                            if (!v175)
                                                goto LABEL_4748b0;
                                        }
                                        else
                                        {
LABEL_4748b0:
                                            v176 = 0;
                                            goto LABEL_4748b9;
                                        }
                                    }
                                    v177 = _ccall(4, v173, v174, 0);
                                    v176 = -(v177 & 1) + 1 - (unsigned int)(v177 & 1 && 1);
LABEL_4748b9:
                                    if (!v176)
                                    {
                                        *(unsigned int *)&v14[16] = 22;
                                        goto LABEL_4747c4;
                                    }
                                    v178 = "posfreeze";
                                    v179 = &v13;
                                    while (1)
                                    {
                                        v180 = *v179;
                                        v181 = v180;
                                        v182 = *v178;
                                        if (v180 != *v178)
                                            break;
                                        if (v180)
                                        {
                                            v183 = v179[1];
                                            v181 = v183;
                                            v182 = v178[1];
                                            if (v183 != v178[1])
                                                break;
                                            v179 += 2;
                                            v178 += 2;
                                            if (!v183)
                                                goto LABEL_4748ee;
                                        }
                                        else
                                        {
LABEL_4748ee:
                                            v184 = 0;
                                            goto LABEL_4748f7;
                                        }
                                    }
                                    v185 = _ccall(4, v181, v182, 0);
                                    v184 = -(v185 & 1) + 1 - (unsigned int)(v185 & 1 && 1);
LABEL_4748f7:
                                    if (!v184)
                                    {
                                        *(unsigned int *)&v14[16] = 23;
                                        v5 = 1;
                                        goto LABEL_474904;
                                    }
                                    v186 = &v13;
                                    v187 = "velset";
                                    while (1)
                                    {
                                        v188 = *v186;
                                        v189 = v188;
                                        v190 = *v187;
                                        if (v188 != *v187)
                                            break;
                                        if (v188)
                                        {
                                            v191 = v186[1];
                                            v189 = v191;
                                            v190 = v187[1];
                                            if (v191 != v187[1])
                                                break;
                                            v186 += 2;
                                            v187 += 2;
                                            if (!v191)
                                                goto LABEL_47494c;
                                        }
                                        else
                                        {
LABEL_47494c:
                                            v192 = 0;
                                            goto LABEL_474955;
                                        }
                                    }
                                    v193 = _ccall(4, v189, v190, 0);
                                    v192 = -(v193 & 1) + 1 - (unsigned int)(v193 & 1 && 1);
LABEL_474955:
                                    if (!v192)
                                    {
                                        *(unsigned int *)&v14[16] = 24;
                                        goto LABEL_4747c4;
                                    }
                                    v194 = "veladd";
                                    v195 = &v13;
                                    while (1)
                                    {
                                        v196 = *v195;
                                        v197 = v196;
                                        v198 = *v194;
                                        if (v196 != *v194)
                                            break;
                                        if (v196)
                                        {
                                            v199 = v195[1];
                                            v197 = v199;
                                            v198 = v194[1];
                                            if (v199 != v194[1])
                                                break;
                                            v195 += 2;
                                            v194 += 2;
                                            if (!v199)
                                                goto LABEL_47498c;
                                        }
                                        else
                                        {
LABEL_47498c:
                                            v200 = 0;
                                            goto LABEL_474995;
                                        }
                                    }
                                    v201 = _ccall(4, v197, v198, 0);
                                    v200 = -(v201 & 1) + 1 - (unsigned int)(v201 & 1 && 1);
LABEL_474995:
                                    if (!v200)
                                    {
                                        *(unsigned int *)&v14[16] = 25;
                                        goto LABEL_4747c4;
                                    }
                                    v202 = "velmul";
                                    v203 = &v13;
                                    while (1)
                                    {
                                        v204 = *v203;
                                        v205 = v204;
                                        v206 = *v202;
                                        if (v204 != *v202)
                                            break;
                                        if (v204)
                                        {
                                            v207 = v203[1];
                                            v205 = v207;
                                            v206 = v202[1];
                                            if (v207 != v202[1])
                                                break;
                                            v203 += 2;
                                            v202 += 2;
                                            if (!v207)
                                                goto LABEL_4749cc;
                                        }
                                        else
                                        {
LABEL_4749cc:
                                            v208 = 0;
                                            goto LABEL_4749d5;
                                        }
                                    }
                                    v209 = _ccall(4, v205, v206, 0);
                                    v208 = -(v209 & 1) + 1 - (unsigned int)(v209 & 1 && 1);
LABEL_4749d5:
                                    if (!v208)
                                    {
                                        *(unsigned int *)&v14[16] = 26;
LABEL_4747c4:
                                        v210 = sub_433410(a0, "x");
                                        if ((!v210 || (sub_438d20(1), sub_438b20(v210, v14 + 60, a2, 0, 2))) && (v211 = sub_433410(a0, "y"), !v211 || !(sub_438d20(1), !sub_438b20(v211, v14 + 72, a2, 0, 2))))
                                        {
                                            v212 = sub_433410(a0, "z");
                                            if (!v212)
                                                goto LABEL_476430;
                                            sub_438d20(1);
                                            v213 = sub_438b20(v212, v14 + 84, a2, 0, 2);
                                            goto LABEL_4740e6;
                                        }
                                    }
                                    else
                                    {
                                        v214 = "gravity";
                                        v215 = &v13;
                                        while (1)
                                        {
                                            v216 = *v215;
                                            v217 = v216;
                                            v218 = *v214;
                                            if (v216 != *v214)
                                                break;
                                            if (v216)
                                            {
                                                v219 = v215[1];
                                                v217 = v219;
                                                v218 = v214[1];
                                                if (v219 != v214[1])
                                                    break;
                                                v215 += 2;
                                                v214 += 2;
                                                if (!v219)
                                                    goto LABEL_474a0c;
                                            }
                                            else
                                            {
LABEL_474a0c:
                                                v220 = 0;
                                                goto LABEL_474a15;
                                            }
                                        }
                                        v221 = _ccall(4, v217, v218, 0);
                                        v220 = -(v221 & 1) + 1 - (unsigned int)(v221 & 1 && 1);
LABEL_474a15:
                                        if (!v220)
                                        {
                                            *(unsigned int *)&v14[16] = 27;
                                            goto LABEL_476430;
                                        }
                                        v222 = "hitdef";
                                        v223 = &v13;
                                        v224 = v15;
                                        while (1)
                                        {
                                            v225 = *v223;
                                            v226 = v225;
                                            v227 = *v222;
                                            if (v225 != *v222)
                                                break;
                                            if (v225)
                                            {
                                                v228 = v223[1];
                                                v226 = v228;
                                                v227 = v222[1];
                                                if (v228 != v222[1])
                                                    break;
                                                v223 += 2;
                                                v222 += 2;
                                                if (!v228)
                                                    goto LABEL_474a4c;
                                            }
                                            else
                                            {
LABEL_474a4c:
                                                v229 = 0;
                                                goto LABEL_474a55;
                                            }
                                        }
                                        v230 = _ccall(4, v226, v227, 0);
                                        v229 = -(v230 & 1) + 1 - (unsigned int)(v230 & 1 && 1);
LABEL_474a55:
                                        if (!v229)
                                        {
                                            ptr = sub_40e1b0();
                                            if (ptr)
                                            {
                                                v232 = v14;
                                                *(unsigned int *)&v232[16] = 37;
                                                if (!sub_47da40(a0, ptr))
                                                {
                                                    free(ptr);
                                                    v233 = &g_52b40c->field_1205b;
                                                    do
                                                    {
                                                        v234 = v233;
                                                        idx = v234 + 1;
                                                        v233 = idx;
                                                    } while (*(char *)(v234 + 1));
                                                    *(unsigned int *)idx = 1920091402;
                                                    *(unsigned int *)(idx + 4) = 1881174639;
                                                    *(unsigned int *)(idx + 8) = 1769173601;
                                                    *(unsigned int *)(idx + 12) = 1210083182;
                                                    *(unsigned int *)(idx + 16) = 1701082217;
                                                    *(unsigned short *)(idx + 20) = 0x66;
                                                    return NULL;
                                                }
                                                goto LABEL_477177;
                                            }
                                        }
                                        else
                                        {
                                            v236 = "reversaldef";
                                            v237 = &v13;
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
                                                        goto LABEL_474b21;
                                                }
                                                else
                                                {
LABEL_474b21:
                                                    v242 = 0;
                                                    goto LABEL_474b2a;
                                                }
                                            }
                                            v243 = _ccall(4, v239, v240, 0);
                                            v242 = -(v243 & 1) + 1 - (unsigned int)(v243 & 1 && 1);
LABEL_474b2a:
                                            if (!v242)
                                            {
                                                ptr1 = sub_40e1b0();
                                                if (ptr1)
                                                {
                                                    *(unsigned int *)&v14[16] = 38;
                                                    v245 = sub_433410(a0, "reversal.attr");
                                                    if (!v245)
                                                    {
                                                        sprintf(&g_52b40c->field_1205c, "Missing reversal.attr parameter");
                                                        return NULL;
                                                    }
                                                    v246 = sub_489dc0(v245, 0);
                                                    ptr1->field_198 = v246;
                                                    if (v246 == 0xffffffff)
                                                    {
                                                        v5 = v245;
                                                        v4 = "Invalid reversal.attr: %s";
                                                        goto LABEL_474ba8;
                                                    }
                                                    else
                                                    {
                                                        if (!sub_47da40(a0, ptr1))
                                                        {
                                                            free(ptr1);
                                                            v247 = &g_52b40c->field_1205b;
                                                            do
                                                            {
                                                                v248 = v247;
                                                                v249 = v248 + 1;
                                                                v247 = v249;
                                                            } while (*(char *)(v248 + 1));
                                                            v250 = 6;
                                                            for (k = "\nError parsing Reversaldef"; v250; k += 4)
                                                            {
                                                                v250 -= 1;
                                                                *(int *)v249 = *(int *)k;
                                                                v249 += 4;
                                                            }
                                                            *(short *)v249 = *(short *)k;
                                                            *(char *)(v249 + 2) = (char)k[2];
                                                            return NULL;
                                                        }
                                                        *(st_473ed0_1 **)&a1[96] = ptr1;
                                                        goto LABEL_476430;
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                v252 = "hitby";
                                                v253 = &v13;
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
                                                            goto LABEL_474c51;
                                                    }
                                                    else
                                                    {
LABEL_474c51:
                                                        v258 = 0;
                                                        goto LABEL_474c5a;
                                                    }
                                                }
                                                v259 = _ccall(4, v255, v256, 0);
                                                v258 = -(v259 & 1) + 1 - (unsigned int)(v259 & 1 && 1);
LABEL_474c5a:
                                                if (!v258)
                                                {
                                                    *(unsigned int *)&v14[16] = 30;
                                                    v260 = sub_433410(a0, "value");
                                                    if (v260)
                                                    {
                                                        v5 = 0;
                                                        goto LABEL_474c9b;
                                                    }
                                                    v260 = sub_433410(a0, "value2");
                                                    if (v260)
                                                    {
                                                        v5 = 1;
LABEL_474c9b:
                                                        sub_438d20(v5);
                                                        v261 = sub_489dc0(v260, 0);
                                                        if (v261 != 0xffffffff)
                                                        {
                                                            sub_438d20(v261);
                                                            v34 = v14 + 36;
                                                            sub_438d20(1);
                                                            v95 = sub_433410(a0, "time");
                                                            v3 = 1;
                                                            v2 = 0;
                                                            v1 = a2;
                                                            goto LABEL_4740dc;
                                                        }
                                                    }
                                                }
                                                else
                                                {
                                                    v262 = "nothitby";
                                                    v263 = &v13;
                                                    while (1)
                                                    {
                                                        v264 = *v263;
                                                        v265 = v264;
                                                        v266 = *v262;
                                                        if (v264 != *v262)
                                                            break;
                                                        if (v264)
                                                        {
                                                            v267 = v263[1];
                                                            v265 = v267;
                                                            v266 = v262[1];
                                                            if (v267 != v262[1])
                                                                break;
                                                            v263 += 2;
                                                            v262 += 2;
                                                            if (!v267)
                                                                goto LABEL_474d14;
                                                        }
                                                        else
                                                        {
LABEL_474d14:
                                                            v268 = 0;
                                                            goto LABEL_474d1d;
                                                        }
                                                    }
                                                    v269 = _ccall(4, v265, v266, 0);
                                                    v268 = -(v269 & 1) + 1 - (unsigned int)(v269 & 1 && 1);
LABEL_474d1d:
                                                    if (!v268)
                                                    {
                                                        *(unsigned int *)&v14[16] = 30;
                                                        v270 = sub_433410(a0, "value");
                                                        if (v270)
                                                        {
                                                            v5 = 0;
                                                            goto LABEL_474d5e;
                                                        }
                                                        v270 = sub_433410(a0, "value2");
                                                        if (v270)
                                                        {
                                                            v5 = 1;
LABEL_474d5e:
                                                            sub_438d20(v5);
                                                            v271 = sub_489dc0(v270, 0);
                                                            if (v271 != 0xffffffff)
                                                            {
                                                                sub_438d20(~v271 & 0xfff);
                                                                v34 = v14 + 36;
                                                                sub_438d20(1);
                                                                v5 = "time";
                                                                goto LABEL_4740ca;
                                                            }
                                                        }
                                                    }
                                                    else
                                                    {
                                                        v272 = "hitvelset";
                                                        v273 = &v13;
                                                        while (1)
                                                        {
                                                            v274 = *v273;
                                                            v275 = v274;
                                                            v276 = *v272;
                                                            if (v274 != *v272)
                                                                break;
                                                            if (v274)
                                                            {
                                                                v277 = v273[1];
                                                                v275 = v277;
                                                                v276 = v272[1];
                                                                if (v277 != v272[1])
                                                                    break;
                                                                v273 += 2;
                                                                v272 += 2;
                                                                if (!v277)
                                                                    goto LABEL_474dcc;
                                                            }
                                                            else
                                                            {
LABEL_474dcc:
                                                                v278 = 0;
                                                                goto LABEL_474dd5;
                                                            }
                                                        }
                                                        v279 = _ccall(4, v275, v276, 0);
                                                        v278 = -(v279 & 1) + 1 - (unsigned int)(v279 & 1 && 1);
LABEL_474dd5:
                                                        if (!v278)
                                                        {
                                                            *(unsigned int *)&v14[16] = 101;
                                                            if (sub_438b20(sub_433410(a0, "x"), v14 + 24, v224, 0, 1))
                                                            {
                                                                v95 = sub_433410(a0, "y");
                                                                v3 = 1;
                                                                v2 = 0;
                                                                v1 = v224;
                                                                v0 = v14 + 36;
                                                                goto LABEL_4740dd;
                                                            }
                                                        }
                                                        else
                                                        {
                                                            v280 = "hitfallset";
                                                            v281 = &v13;
                                                            while (1)
                                                            {
                                                                v282 = *v281;
                                                                v283 = v282;
                                                                v284 = *v280;
                                                                if (v282 != *v280)
                                                                    break;
                                                                if (v282)
                                                                {
                                                                    v285 = v281[1];
                                                                    v283 = v285;
                                                                    v284 = v280[1];
                                                                    if (v285 != v280[1])
                                                                        break;
                                                                    v281 += 2;
                                                                    v280 += 2;
                                                                    if (!v285)
                                                                        goto LABEL_474e41;
                                                                }
                                                                else
                                                                {
LABEL_474e41:
                                                                    v286 = 0;
                                                                    goto LABEL_474e4a;
                                                                }
                                                            }
                                                            v287 = _ccall(4, v283, v284, 0);
                                                            v286 = -(v287 & 1) + 1 - (unsigned int)(v287 & 1 && 1);
LABEL_474e4a:
                                                            if (!v286)
                                                            {
                                                                *(unsigned int *)&v14[16] = 100;
                                                                sub_438d20(-0x1);
                                                                sub_438d50(-3.4028234663852886e+38f);
                                                                sub_438d50(-3.4028234663852886e+38f);
                                                                if (sub_438b20(sub_433410(a0, "value"), v14 + 24, a2, 0, 1))
                                                                {
                                                                    v288 = sub_433410(a0, "velocity");
                                                                    sub_488ff0(v288, "ff", a2, &v10, v14 + 60, v14 + 72);
                                                                    if (v288 && *v10)
                                                                        return NULL;
                                                                }
                                                            }
                                                            else
                                                            {
                                                                v289 = "hitfalldamage";
                                                                v290 = &v13;
                                                                while (1)
                                                                {
                                                                    v291 = *v290;
                                                                    v292 = v291;
                                                                    v293 = *v289;
                                                                    if (v291 != *v289)
                                                                        break;
                                                                    if (v291)
                                                                    {
                                                                        v294 = v290[1];
                                                                        v292 = v294;
                                                                        v293 = v289[1];
                                                                        if (v294 != v289[1])
                                                                            break;
                                                                        v290 += 2;
                                                                        v289 += 2;
                                                                        if (!v294)
                                                                            goto LABEL_474f2c;
                                                                    }
                                                                    else
                                                                    {
LABEL_474f2c:
                                                                        v295 = 0;
                                                                        goto LABEL_474f35;
                                                                    }
                                                                }
                                                                v296 = _ccall(4, v292, v293, 0);
                                                                v295 = -(v296 & 1) + 1 - (unsigned int)(v296 & 1 && 1);
LABEL_474f35:
                                                                if (!v295)
                                                                {
                                                                    *(unsigned int *)&v14[16] = 0x66;
                                                                    goto LABEL_476430;
                                                                }
                                                                v297 = "hitfallvel";
                                                                v298 = &v13;
                                                                while (1)
                                                                {
                                                                    v299 = *v298;
                                                                    v300 = v299;
                                                                    v301 = *v297;
                                                                    if (v299 != *v297)
                                                                        break;
                                                                    if (v299)
                                                                    {
                                                                        v302 = v298[1];
                                                                        v300 = v302;
                                                                        v301 = v297[1];
                                                                        if (v302 != v297[1])
                                                                            break;
                                                                        v298 += 2;
                                                                        v297 += 2;
                                                                        if (!v302)
                                                                            goto LABEL_474f6c;
                                                                    }
                                                                    else
                                                                    {
LABEL_474f6c:
                                                                        v303 = 0;
                                                                        goto LABEL_474f75;
                                                                    }
                                                                }
                                                                v304 = _ccall(4, v300, v301, 0);
                                                                v303 = -(v304 & 1) + 1 - (unsigned int)(v304 & 1 && 1);
LABEL_474f75:
                                                                if (!v303)
                                                                {
                                                                    *(unsigned int *)&v14[16] = 103;
                                                                    goto LABEL_476430;
                                                                }
                                                                v305 = "hitadd";
                                                                v306 = &v13;
                                                                v15 = v224;
                                                                while (1)
                                                                {
                                                                    v307 = *v306;
                                                                    v308 = v307;
                                                                    v309 = *v305;
                                                                    if (v307 != *v305)
                                                                        break;
                                                                    if (v307)
                                                                    {
                                                                        v310 = v306[1];
                                                                        v308 = v310;
                                                                        v309 = v305[1];
                                                                        if (v310 != v305[1])
                                                                            break;
                                                                        v306 += 2;
                                                                        v305 += 2;
                                                                        if (!v310)
                                                                            goto LABEL_474fac;
                                                                    }
                                                                    else
                                                                    {
LABEL_474fac:
                                                                        v311 = 0;
                                                                        goto LABEL_474fb5;
                                                                    }
                                                                }
                                                                v312 = _ccall(4, v308, v309, 0);
                                                                v311 = -(v312 & 1) + 1 - (unsigned int)(v312 & 1 && 1);
LABEL_474fb5:
                                                                if (!v311)
                                                                {
                                                                    *(unsigned int *)&v14[16] = 104;
                                                                    goto LABEL_4742e0;
                                                                }
                                                                v313 = "attackmulset";
                                                                v314 = &v13;
                                                                while (1)
                                                                {
                                                                    v315 = *v314;
                                                                    v316 = v315;
                                                                    v317 = *v313;
                                                                    if (v315 != *v313)
                                                                        break;
                                                                    if (v315)
                                                                    {
                                                                        v318 = v314[1];
                                                                        v316 = v318;
                                                                        v317 = v313[1];
                                                                        if (v318 != v313[1])
                                                                            break;
                                                                        v314 += 2;
                                                                        v313 += 2;
                                                                        if (!v318)
                                                                            goto LABEL_474fec;
                                                                    }
                                                                    else
                                                                    {
LABEL_474fec:
                                                                        v319 = 0;
                                                                        goto LABEL_474ff5;
                                                                    }
                                                                }
                                                                v320 = _ccall(4, v316, v317, 0);
                                                                v319 = -(v320 & 1) + 1 - (unsigned int)(v320 & 1 && 1);
LABEL_474ff5:
                                                                if (!v319)
                                                                {
                                                                    *(unsigned int *)&v14[16] = 105;
                                                                    goto LABEL_475000;
                                                                }
                                                                v321 = "defencemulset";
                                                                v322 = &v13;
                                                                while (1)
                                                                {
                                                                    v323 = *v322;
                                                                    v324 = v323;
                                                                    v325 = *v321;
                                                                    if (v323 != *v321)
                                                                        break;
                                                                    if (v323)
                                                                    {
                                                                        v326 = v322[1];
                                                                        v324 = v326;
                                                                        v325 = v321[1];
                                                                        if (v326 != v321[1])
                                                                            break;
                                                                        v322 += 2;
                                                                        v321 += 2;
                                                                        if (!v326)
                                                                            goto LABEL_475051;
                                                                    }
                                                                    else
                                                                    {
LABEL_475051:
                                                                        v327 = 0;
                                                                        goto LABEL_47505a;
                                                                    }
                                                                }
                                                                v328 = _ccall(4, v324, v325, 0);
                                                                v327 = -(v328 & 1) + 1 - (unsigned int)(v328 & 1 && 1);
LABEL_47505a:
                                                                if (!v327)
                                                                {
                                                                    *(unsigned int *)&v14[16] = 106;
LABEL_475000:
                                                                    v329 = sub_433410(a0, "value");
                                                                    if (v329)
                                                                    {
                                                                        v213 = sub_438b20(v329, v14 + 60, v15, 0, 2);
                                                                        goto LABEL_4740e6;
                                                                    }
                                                                }
                                                                else
                                                                {
                                                                    v330 = "statetypeset";
                                                                    v331 = &v13;
                                                                    while (1)
                                                                    {
                                                                        v332 = *v331;
                                                                        v333 = v332;
                                                                        v334 = *v330;
                                                                        if (v332 != *v330)
                                                                            break;
                                                                        if (v332)
                                                                        {
                                                                            v335 = v331[1];
                                                                            v333 = v335;
                                                                            v334 = v330[1];
                                                                            if (v335 != v330[1])
                                                                                break;
                                                                            v331 += 2;
                                                                            v330 += 2;
                                                                            if (!v335)
                                                                                goto LABEL_47508c;
                                                                        }
                                                                        else
                                                                        {
LABEL_47508c:
                                                                            v336 = 0;
                                                                            goto LABEL_475095;
                                                                        }
                                                                    }
                                                                    v337 = _ccall(4, v333, v334, 0);
                                                                    v336 = -(v337 & 1) + 1 - (unsigned int)(v337 & 1 && 1);
LABEL_475095:
                                                                    if (!v336)
                                                                    {
                                                                        *(unsigned int *)&v14[16] = 110;
                                                                        sub_438d20(-0x1);
                                                                        sub_438d20(-0x1);
                                                                        sub_438d20(-0x1);
                                                                        v338 = sub_433410(a0, "statetype");
                                                                        if (v338 || (v338 = sub_433410(a0, "value"), v338))
                                                                        {
                                                                            sub_40e380();
                                                                            switch (sub_40e380())
                                                                            {
                                                                            case 97:
                                                                                v5 = 3;
                                                                                break;
                                                                            case 99:
                                                                                v5 = 2;
                                                                                break;
                                                                            case 108:
                                                                                v5 = 4;
                                                                                break;
                                                                            case 115:
                                                                                v5 = 1;
                                                                                break;
                                                                            default:
                                                                                v5 = v338;
                                                                                sprintf(&g_52b40c->field_1205c, "State type \"%s\" not valid");
                                                                                return NULL;
                                                                            }
                                                                            sub_438d20(v5);
                                                                        }
                                                                        v339 = sub_433410(a0, "movetype");
                                                                        if (v339)
                                                                        {
                                                                            v340 = sub_40e380();
                                                                            v341 = v340 - 97;
                                                                            if (v340 == 97)
                                                                            {
                                                                                v5 = 1;
LABEL_475192:
                                                                                sub_438d20(v5);
                                                                                goto LABEL_47519d;
                                                                            }
                                                                            else if (v341 == 7)
                                                                            {
                                                                                v5 = 2;
                                                                                goto LABEL_475192;
                                                                            }
                                                                            else if (v341 != 8)
                                                                            {
                                                                                v5 = v339;
                                                                                v4 = "State movetype \"%s\" not valid";
                                                                                goto LABEL_474ba8;
                                                                            }
                                                                            else
                                                                            {
                                                                                v5 = 0;
                                                                                goto LABEL_475192;
                                                                            }
                                                                        }
                                                                        else
                                                                        {
LABEL_47519d:
                                                                            v342 = sub_433410(a0, "physics");
                                                                            if (v342)
                                                                            {
                                                                                sub_40e380();
                                                                                switch (sub_40e380())
                                                                                {
                                                                                case 97:
                                                                                    sub_438d20(3);
                                                                                    break;
                                                                                case 99:
                                                                                    sub_438d20(2);
                                                                                    break;
                                                                                case 110:
                                                                                    sub_438d20(0);
                                                                                    break;
                                                                                case 115:
LABEL_47757b:
                                                                                    sub_438d20(1);
                                                                                    break;
                                                                                default:
                                                                                    v5 = v342;
                                                                                    v4 = "State physics \"%s\" not valid";
                                                                                    v3 = &g_52b40c->field_1205c;
                                                                                    goto LABEL_474bb5;
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                    v343 = "hitoverride";
                                                                    v344 = &v13;
                                                                    while (1)
                                                                    {
                                                                        v345 = *v344;
                                                                        v346 = v345;
                                                                        v347 = *v343;
                                                                        if (v345 != *v343)
                                                                            break;
                                                                        if (v345)
                                                                        {
                                                                            v348 = v344[1];
                                                                            v346 = v348;
                                                                            v347 = v343[1];
                                                                            if (v348 != v343[1])
                                                                                break;
                                                                            v344 += 2;
                                                                            v343 += 2;
                                                                            if (!v348)
                                                                                goto LABEL_475244;
                                                                        }
                                                                        else
                                                                        {
LABEL_475244:
                                                                            v349 = 0;
                                                                            goto LABEL_47524d;
                                                                        }
                                                                    }
                                                                    v350 = _ccall(4, v346, v347, 0);
                                                                    v349 = -(v350 & 1) + 1 - (unsigned int)(v350 & 1 && 1);
LABEL_47524d:
                                                                    if (!v349)
                                                                    {
                                                                        ptr = sub_40e1b0();
                                                                        if (ptr)
                                                                        {
                                                                            v232 = v14;
                                                                            *(unsigned int *)&v232[16] = 120;
                                                                            if (!sub_47d820(a2, ptr))
                                                                            {
                                                                                free(ptr);
                                                                                v351 = &g_52b40c->field_1205b;
                                                                                do
                                                                                {
                                                                                    v352 = v351;
                                                                                    node = v352 + 1;
                                                                                    v351 = node;
                                                                                } while (*(char *)(v352 + 1));
                                                                                v354 = 6;
                                                                                for (l = "\nError parsing HitOverride"; v354; l += 4)
                                                                                {
                                                                                    v354 -= 1;
                                                                                    *(int *)node = *(int *)l;
                                                                                    node += 4;
                                                                                }
                                                                                *(short *)node = *(short *)l;
                                                                                *(char *)(node + 2) = (char)l[2];
                                                                                return NULL;
                                                                            }
                                                                            else if (!sub_438b20(sub_433410(a0, "slot"), v232 + 24, a2, 0, 1))
                                                                            {
                                                                                return NULL;
                                                                            }
                                                                            goto LABEL_477177;
                                                                        }
                                                                    }
                                                                    else
                                                                    {
                                                                        v356 = "movehitreset";
                                                                        v357 = &v13;
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
                                                                                    goto LABEL_475323;
                                                                            }
                                                                            else
                                                                            {
LABEL_475323:
                                                                                v362 = 0;
                                                                                goto LABEL_47532c;
                                                                            }
                                                                        }
                                                                        v363 = _ccall(4, v359, v360, 0);
                                                                        v362 = -(v363 & 1) + 1 - (unsigned int)(v363 & 1 && 1);
LABEL_47532c:
                                                                        if (!v362)
                                                                        {
                                                                            *(unsigned int *)&v14[16] = 130;
                                                                            goto LABEL_476430;
                                                                        }
                                                                        v364 = "victoryquote";
                                                                        v365 = &v13;
                                                                        while (1)
                                                                        {
                                                                            v366 = *v365;
                                                                            v367 = v366;
                                                                            v368 = *v364;
                                                                            if (v366 != *v364)
                                                                                break;
                                                                            if (v366)
                                                                            {
                                                                                v369 = v365[1];
                                                                                v367 = v369;
                                                                                v368 = v364[1];
                                                                                if (v369 != v364[1])
                                                                                    break;
                                                                                v365 += 2;
                                                                                v364 += 2;
                                                                                if (!v369)
                                                                                    goto LABEL_475361;
                                                                            }
                                                                            else
                                                                            {
LABEL_475361:
                                                                                v370 = 0;
                                                                                goto LABEL_47536a;
                                                                            }
                                                                        }
                                                                        v371 = _ccall(4, v367, v368, 0);
                                                                        v370 = -(v371 & 1) + 1 - (unsigned int)(v371 & 1 && 1);
LABEL_47536a:
                                                                        if (!v370)
                                                                        {
                                                                            *(unsigned int *)&v14[16] = 140;
                                                                            v5 = -0x1;
LABEL_474904:
                                                                            v34 = v14 + 24;
                                                                            sub_438d20(v5);
                                                                            v95 = sub_433410(a0, "value");
                                                                            v3 = 1;
                                                                            v2 = 0;
                                                                            v1 = v15;
                                                                            goto LABEL_4740dc;
                                                                        }
                                                                        v372 = "assertspecial";
                                                                        v373 = &v13;
                                                                        while (1)
                                                                        {
                                                                            v374 = *v373;
                                                                            v375 = v374;
                                                                            v376 = *v372;
                                                                            if (v374 != *v372)
                                                                                break;
                                                                            if (v374)
                                                                            {
                                                                                v377 = v373[1];
                                                                                v375 = v377;
                                                                                v376 = v372[1];
                                                                                if (v377 != v372[1])
                                                                                    break;
                                                                                v373 += 2;
                                                                                v372 += 2;
                                                                                if (!v377)
                                                                                    goto LABEL_4753a1;
                                                                            }
                                                                            else
                                                                            {
LABEL_4753a1:
                                                                                v378 = 0;
                                                                                goto LABEL_4753aa;
                                                                            }
                                                                        }
                                                                        v379 = _ccall(4, v375, v376, 0);
                                                                        v378 = -(v379 & 1) + 1 - (unsigned int)(v379 & 1 && 1);
LABEL_4753aa:
                                                                        if (!v378)
                                                                        {
                                                                            *(unsigned int *)&v14[16] = 150;
                                                                            v380 = sub_433410(a0, "flag");
                                                                            if (!v380)
                                                                            {
                                                                                sprintf(&g_52b40c->field_1205c, "AssertSpecial flag parameter missing");
                                                                                return NULL;
                                                                            }
                                                                            if (!sub_4895b0())
                                                                            {
                                                                                v5 = v380;
                                                                                v4 = "AssertSpecial flag \"%s\" not valid";
                                                                                goto LABEL_474ba8;
                                                                            }
                                                                            else
                                                                            {
                                                                                sub_438d20(v11);
                                                                                sub_438d20(v12);
                                                                                sub_438d20(-0x1);
                                                                                sub_438d20(-0x1);
                                                                                v381 = sub_433410(a0, "flag2");
                                                                                if (!v381)
                                                                                {
LABEL_4754ab:
                                                                                    v382 = sub_433410(a0, "flag3");
                                                                                    if (!v382)
                                                                                        goto LABEL_476429;
                                                                                    if (!sub_4895b0())
                                                                                    {
                                                                                        v5 = v382;
                                                                                        v4 = "AssertSpecial flag3 \"%s\" not valid";
                                                                                        goto LABEL_474ba8;
                                                                                    }
                                                                                    else
                                                                                    {
                                                                                        sub_438d20(v11);
                                                                                        sub_438d20(v12);
                                                                                        goto LABEL_476430;
                                                                                    }
                                                                                }
                                                                                else if (!sub_4895b0())
                                                                                {
                                                                                    v5 = v381;
                                                                                    v4 = "AssertSpecial flag2 \"%s\" not valid";
                                                                                    goto LABEL_474ba8;
                                                                                }
                                                                                else
                                                                                {
                                                                                    sub_438d20(v11);
                                                                                    sub_438d20(v12);
                                                                                    goto LABEL_4754ab;
                                                                                }
                                                                            }
                                                                        }
                                                                        v383 = "projectile";
                                                                        v384 = &v13;
                                                                        while (1)
                                                                        {
                                                                            v385 = *v384;
                                                                            v386 = v385;
                                                                            v387 = *v383;
                                                                            if (v385 != *v383)
                                                                                break;
                                                                            if (v385)
                                                                            {
                                                                                v388 = v384[1];
                                                                                v386 = v388;
                                                                                v387 = v383[1];
                                                                                if (v388 != v383[1])
                                                                                    break;
                                                                                v384 += 2;
                                                                                v383 += 2;
                                                                                if (!v388)
                                                                                    goto LABEL_475532;
                                                                            }
                                                                            else
                                                                            {
LABEL_475532:
                                                                                v389 = 0;
                                                                                goto LABEL_47553b;
                                                                            }
                                                                        }
                                                                        v390 = _ccall(4, v386, v387, 0);
                                                                        v389 = -(v390 & 1) + 1 - (unsigned int)(v390 & 1 && 1);
LABEL_47553b:
                                                                        if (!v389)
                                                                        {
                                                                            ptr2 = sub_40e1b0();
                                                                            if (ptr2)
                                                                            {
                                                                                *(unsigned int *)&v14[16] = 39;
                                                                                if (!sub_480750(a0))
                                                                                {
                                                                                    free(ptr2);
                                                                                    v391 = &g_52b40c->field_1205b;
                                                                                    do
                                                                                    {
                                                                                        v392 = v391;
                                                                                        v393 = v392 + 1;
                                                                                        v391 = v393;
                                                                                    } while (*(char *)(v392 + 1));
                                                                                    v394 = 6;
                                                                                    for (m = "\nError parsing Projectile"; v394; m += 4)
                                                                                    {
                                                                                        v394 -= 1;
                                                                                        *(int *)v393 = *(int *)m;
                                                                                        v393 += 4;
                                                                                    }
                                                                                    *(short *)v393 = *(short *)m;
                                                                                    return NULL;
                                                                                }
                                                                                goto LABEL_476e7b;
                                                                            }
                                                                        }
                                                                        else
                                                                        {
                                                                            v396 = "width";
                                                                            v397 = &v13;
                                                                            while (1)
                                                                            {
                                                                                v398 = *v397;
                                                                                v399 = v398;
                                                                                v400 = *v396;
                                                                                if (v398 != *v396)
                                                                                    break;
                                                                                if (v398)
                                                                                {
                                                                                    v401 = v397[1];
                                                                                    v399 = v401;
                                                                                    v400 = v396[1];
                                                                                    if (v401 != v396[1])
                                                                                        break;
                                                                                    v397 += 2;
                                                                                    v396 += 2;
                                                                                    if (!v401)
                                                                                        goto LABEL_4755dc;
                                                                                }
                                                                                else
                                                                                {
LABEL_4755dc:
                                                                                    v402 = 0;
                                                                                    goto LABEL_4755e5;
                                                                                }
                                                                            }
                                                                            v403 = _ccall(4, v399, v400, 0);
                                                                            v402 = -(v403 & 1) + 1 - (unsigned int)(v403 & 1 && 1);
LABEL_4755e5:
                                                                            if (!v402)
                                                                            {
                                                                                *(unsigned int *)&v14[16] = 40;
                                                                                sub_438d20(0);
                                                                                sub_438d20(0);
                                                                                sub_438d20(0);
                                                                                sub_438d20(0);
                                                                                v404 = sub_433410(a0, "edge");
                                                                                sub_488ff0(v404, "ii", a2, &v10, v14 + 24, v14 + 36);
                                                                                if (!v404 || !*v10)
                                                                                {
                                                                                    v405 = sub_433410(a0, "player");
                                                                                    sub_488ff0(v405, "ii", a2, &v10, v14 + 48, v14 + 60);
                                                                                    if (!v405 || !*v10)
                                                                                    {
                                                                                        if (v404 || v405)
                                                                                            goto LABEL_476430;
                                                                                        v406 = v14 + 36;
                                                                                        v407 = v14 + 24;
                                                                                        if (sub_488ff0(sub_433410(a0, "value"), "ii", a2, &v10, v407, v406) && !*v10)
                                                                                        {
                                                                                            sub_438e30(v14 + 48, v407);
                                                                                            sub_438e30(v14 + 60, v406);
                                                                                            goto LABEL_476430;
                                                                                        }
                                                                                    }
                                                                                }
                                                                            }
                                                                            else
                                                                            {
                                                                                v408 = "sprpriority";
                                                                                v409 = &v13;
                                                                                while (1)
                                                                                {
                                                                                    v410 = *v409;
                                                                                    v411 = v410;
                                                                                    v412 = *v408;
                                                                                    if (v410 != *v408)
                                                                                        break;
                                                                                    if (v410)
                                                                                    {
                                                                                        v413 = v409[1];
                                                                                        v411 = v413;
                                                                                        v412 = v408[1];
                                                                                        if (v413 != v408[1])
                                                                                            break;
                                                                                        v409 += 2;
                                                                                        v408 += 2;
                                                                                        if (!v413)
                                                                                            goto LABEL_47574c;
                                                                                    }
                                                                                    else
                                                                                    {
LABEL_47574c:
                                                                                        v414 = 0;
                                                                                        goto LABEL_475755;
                                                                                    }
                                                                                }
                                                                                v415 = _ccall(4, v411, v412, 0);
                                                                                v414 = -(v415 & 1) + 1 - (unsigned int)(v415 & 1 && 1);
LABEL_475755:
                                                                                if (!v414)
                                                                                {
                                                                                    *(unsigned int *)&v14[16] = 0x33;
                                                                                    goto LABEL_475760;
                                                                                }
                                                                                v416 = "angledraw";
                                                                                v417 = &v13;
                                                                                while (1)
                                                                                {
                                                                                    v418 = *v417;
                                                                                    v419 = v418;
                                                                                    v420 = *v416;
                                                                                    if (v418 != *v416)
                                                                                        break;
                                                                                    if (v418)
                                                                                    {
                                                                                        v421 = v417[1];
                                                                                        v419 = v421;
                                                                                        v420 = v416[1];
                                                                                        if (v421 != v416[1])
                                                                                            break;
                                                                                        v417 += 2;
                                                                                        v416 += 2;
                                                                                        if (!v421)
                                                                                            goto LABEL_47579d;
                                                                                    }
                                                                                    else
                                                                                    {
LABEL_47579d:
                                                                                        v422 = 0;
                                                                                        goto LABEL_4757a6;
                                                                                    }
                                                                                }
                                                                                v423 = _ccall(4, v419, v420, 0);
                                                                                v422 = -(v423 & 1) + 1 - (unsigned int)(v423 & 1 && 1);
LABEL_4757a6:
                                                                                if (!v422)
                                                                                {
                                                                                    v5 = v416;
                                                                                    *(unsigned int *)&v14[16] = 60;
                                                                                    sub_438d50(1.0f);
                                                                                    sub_438d50(1.0f);
                                                                                    if (sub_488fa0(a2, -3.4028234663852886e+38f))
                                                                                    {
                                                                                        v424 = sub_433410(a0, "scale");
                                                                                        sub_488ff0(v424, "ff", a2, &v10, v14 + 72, v14 + 84);
                                                                                        if (v424 && *v10)
                                                                                            return NULL;
                                                                                    }
                                                                                }
                                                                                else
                                                                                {
                                                                                    v425 = "angleset";
                                                                                    v426 = &v13;
                                                                                    while (1)
                                                                                    {
                                                                                        v427 = *v426;
                                                                                        v428 = v427;
                                                                                        v429 = *v425;
                                                                                        if (v427 != *v425)
                                                                                            break;
                                                                                        if (v427)
                                                                                        {
                                                                                            v430 = v426[1];
                                                                                            v428 = v430;
                                                                                            v429 = v425[1];
                                                                                            if (v430 != v425[1])
                                                                                                break;
                                                                                            v426 += 2;
                                                                                            v425 += 2;
                                                                                            if (!v430)
                                                                                                goto LABEL_475871;
                                                                                        }
                                                                                        else
                                                                                        {
LABEL_475871:
                                                                                            v431 = 0;
                                                                                            goto LABEL_47587a;
                                                                                        }
                                                                                    }
                                                                                    v432 = _ccall(4, v428, v429, 0);
                                                                                    v431 = -(v432 & 1) + 1 - (unsigned int)(v432 & 1 && 1);
LABEL_47587a:
                                                                                    if (!v431)
                                                                                    {
                                                                                        *(unsigned int *)&v14[16] = 61;
                                                                                        goto LABEL_475885;
                                                                                    }
                                                                                    v433 = "angleadd";
                                                                                    v434 = &v13;
                                                                                    while (1)
                                                                                    {
                                                                                        v435 = *v434;
                                                                                        v436 = v435;
                                                                                        v437 = *v433;
                                                                                        if (v435 != *v433)
                                                                                            break;
                                                                                        if (v435)
                                                                                        {
                                                                                            v438 = v434[1];
                                                                                            v436 = v438;
                                                                                            v437 = v433[1];
                                                                                            if (v438 != v433[1])
                                                                                                break;
                                                                                            v434 += 2;
                                                                                            v433 += 2;
                                                                                            if (!v438)
                                                                                                goto LABEL_4758c2;
                                                                                        }
                                                                                        else
                                                                                        {
LABEL_4758c2:
                                                                                            v439 = 0;
                                                                                            goto LABEL_4758cb;
                                                                                        }
                                                                                    }
                                                                                    v440 = _ccall(4, v436, v437, 0);
                                                                                    v439 = -(v440 & 1) + 1 - (unsigned int)(v440 & 1 && 1);
LABEL_4758cb:
                                                                                    if (!v439)
                                                                                    {
                                                                                        *(unsigned int *)&v14[16] = 62;
                                                                                        goto LABEL_475885;
                                                                                    }
                                                                                    v441 = "anglemul";
                                                                                    v442 = &v13;
                                                                                    while (1)
                                                                                    {
                                                                                        v443 = *v442;
                                                                                        v444 = v443;
                                                                                        v445 = *v441;
                                                                                        if (v443 != *v441)
                                                                                            break;
                                                                                        if (v443)
                                                                                        {
                                                                                            v446 = v442[1];
                                                                                            v444 = v446;
                                                                                            v445 = v441[1];
                                                                                            if (v446 != v441[1])
                                                                                                break;
                                                                                            v442 += 2;
                                                                                            v441 += 2;
                                                                                            if (!v446)
                                                                                                goto LABEL_4758fd;
                                                                                        }
                                                                                        else
                                                                                        {
LABEL_4758fd:
                                                                                            v447 = 0;
                                                                                            goto LABEL_475906;
                                                                                        }
                                                                                    }
                                                                                    v448 = _ccall(4, v444, v445, 0);
                                                                                    v447 = -(v448 & 1) + 1 - (unsigned int)(v448 & 1 && 1);
LABEL_475906:
                                                                                    if (!v447)
                                                                                    {
                                                                                        *(unsigned int *)&v14[16] = 63;
LABEL_475885:
                                                                                        v95 = sub_433410(a0, "value");
                                                                                        v3 = 2;
                                                                                        v2 = 0;
                                                                                        v1 = v15;
                                                                                        v0 = v14 + 60;
                                                                                        goto LABEL_4740dd;
                                                                                    }
                                                                                    v449 = "trans";
                                                                                    v450 = &v13;
                                                                                    while (1)
                                                                                    {
                                                                                        v451 = *v450;
                                                                                        v452 = v451;
                                                                                        v453 = *v449;
                                                                                        if (v451 != *v449)
                                                                                            break;
                                                                                        if (v451)
                                                                                        {
                                                                                            v454 = v450[1];
                                                                                            v452 = v454;
                                                                                            v453 = v449[1];
                                                                                            if (v454 != v449[1])
                                                                                                break;
                                                                                            v450 += 2;
                                                                                            v449 += 2;
                                                                                            if (!v454)
                                                                                                goto LABEL_47593c;
                                                                                        }
                                                                                        else
                                                                                        {
LABEL_47593c:
                                                                                            v455 = 0;
                                                                                            goto LABEL_475945;
                                                                                        }
                                                                                    }
                                                                                    v456 = _ccall(4, v452, v453, 0);
                                                                                    v455 = -(v456 & 1) + 1 - (unsigned int)(v456 & 1 && 1);
LABEL_475945:
                                                                                    if (!v455)
                                                                                    {
                                                                                        *(unsigned int *)&v14[16] = 65;
                                                                                        sub_438d20(-0x1);
                                                                                        sub_438d20(0x100);
                                                                                        sub_438d20(0);
                                                                                        v457 = sub_433410(a0, "trans");
                                                                                        if (v457)
                                                                                        {
                                                                                            if (!_stricmp(v457, "none"))
                                                                                            {
                                                                                                v5 = 0;
                                                                                                goto LABEL_475a59;
                                                                                            }
                                                                                            if (*v457 && _stricmp(v457, "default"))
                                                                                            {
                                                                                                if (!_stricmp(v457, "add"))
                                                                                                {
                                                                                                    v5 = 1;
                                                                                                    goto LABEL_475a59;
                                                                                                }
                                                                                                if (!_stricmp(v457, "add1"))
                                                                                                {
                                                                                                    sub_438d20(3);
                                                                                                    sub_438d20(0x100);
                                                                                                    v5 = 128;
                                                                                                    goto LABEL_475a5c;
                                                                                                }
                                                                                                if (!_stricmp(v457, "addalpha"))
                                                                                                {
                                                                                                    v5 = 3;
                                                                                                    goto LABEL_475a59;
                                                                                                }
                                                                                                if (!_stricmp(v457, "sub"))
                                                                                                {
                                                                                                    v5 = 2;
                                                                                                    goto LABEL_475a59;
                                                                                                }
                                                                                                else
                                                                                                {
                                                                                                    v5 = v457;
                                                                                                    v4 = "Invalid trans type: %s";
                                                                                                    v3 = &g_52b40c->field_1205c;
                                                                                                    goto LABEL_474bb5;
                                                                                                }
                                                                                            }
                                                                                            else
                                                                                            {
                                                                                                v5 = 0xffffffff;
LABEL_475a59:
LABEL_475a5c:
                                                                                                sub_438d20();
                                                                                                goto LABEL_475a64;
                                                                                            }
                                                                                            goto LABEL_474bb5;
                                                                                        }
                                                                                        else
                                                                                        {
LABEL_475a64:
                                                                                            v458 = sub_433410(a0, "alpha");
                                                                                            if (v458)
                                                                                            {
                                                                                                sub_488ff0(v458, "ii", a2, &v10, v14 + 36, v14 + 48);
                                                                                                if (*v10)
                                                                                                    return NULL;
                                                                                            }
                                                                                        }
                                                                                    }
                                                                                    v459 = "screenbound";
                                                                                    v460 = &v13;
                                                                                    while (1)
                                                                                    {
                                                                                        v461 = *v460;
                                                                                        v462 = v461;
                                                                                        v463 = *v459;
                                                                                        if (v461 != *v459)
                                                                                            break;
                                                                                        if (v461)
                                                                                        {
                                                                                            v464 = v460[1];
                                                                                            v462 = v464;
                                                                                            v463 = v459[1];
                                                                                            if (v464 != v459[1])
                                                                                                break;
                                                                                            v460 += 2;
                                                                                            v459 += 2;
                                                                                            if (!v464)
                                                                                                goto LABEL_475adc;
                                                                                        }
                                                                                        else
                                                                                        {
LABEL_475adc:
                                                                                            v465 = 0;
                                                                                            goto LABEL_475ae5;
                                                                                        }
                                                                                    }
                                                                                    v466 = _ccall(4, v462, v463, 0);
                                                                                    v465 = -(v466 & 1) + 1 - (unsigned int)(v466 & 1 && 1);
LABEL_475ae5:
                                                                                    if (!v465)
                                                                                    {
                                                                                        *(unsigned int *)&v14[16] = 50;
                                                                                        sub_438d20(0);
                                                                                        if (sub_438b20(sub_433410(a0, "value"), v14 + 24, v15, 0, 1))
                                                                                        {
                                                                                            sub_438d20(0);
                                                                                            sub_438d20(0);
                                                                                            v467 = sub_433410(a0, "movecamera");
                                                                                            sub_488ff0(v467, "ii", a2, &v10, v14 + 36, v14 + 48);
                                                                                            if (v467 && *v10)
                                                                                                return NULL;
                                                                                        }
                                                                                    }
                                                                                    else
                                                                                    {
                                                                                        v468 = "targetbind";
                                                                                        v469 = &v13;
                                                                                        while (1)
                                                                                        {
                                                                                            v470 = *v469;
                                                                                            v471 = v470;
                                                                                            v472 = *v468;
                                                                                            if (v470 != *v468)
                                                                                                break;
                                                                                            if (v470)
                                                                                            {
                                                                                                v473 = v469[1];
                                                                                                v471 = v473;
                                                                                                v472 = v468[1];
                                                                                                if (v473 != v468[1])
                                                                                                    break;
                                                                                                v469 += 2;
                                                                                                v468 += 2;
                                                                                                if (!v473)
                                                                                                    goto LABEL_475bb3;
                                                                                            }
                                                                                            else
                                                                                            {
LABEL_475bb3:
                                                                                                v474 = 0;
                                                                                                goto LABEL_475bbc;
                                                                                            }
                                                                                        }
                                                                                        v475 = _ccall(4, v471, v472, 0);
                                                                                        v474 = -(v475 & 1) + 1 - (unsigned int)(v475 & 1 && 1);
LABEL_475bbc:
                                                                                        if (!v474)
                                                                                        {
                                                                                            *(unsigned int *)&v14[16] = 70;
                                                                                            sub_438d20(1);
                                                                                            sub_438d20(-0x1);
                                                                                            sub_438d50(0.0f);
                                                                                            sub_438d50(0.0f);
                                                                                            sub_438d50(0.0f);
                                                                                            if (sub_438b20(sub_433410(a0, "time"), v14 + 24, a2, 0, 1) && (v476 = a2, sub_438b20(sub_433410(a0, "id"), v14 + 48, v476, 0, 1)))
                                                                                            {
                                                                                                v477 = sub_433410(a0, "pos");
                                                                                                sub_488ff0(v477, "ff", v476, &v10, v14 + 60, v14 + 72);
                                                                                                if (v477 && *v10)
                                                                                                    return NULL;
                                                                                            }
                                                                                        }
                                                                                        else
                                                                                        {
                                                                                            v478 = "bindtotarget";
                                                                                            v479 = &v13;
                                                                                            while (1)
                                                                                            {
                                                                                                v480 = *v479;
                                                                                                v481 = v480;
                                                                                                v482 = *v478;
                                                                                                if (v480 != *v478)
                                                                                                    break;
                                                                                                if (v480)
                                                                                                {
                                                                                                    v483 = v479[1];
                                                                                                    v481 = v483;
                                                                                                    v482 = v478[1];
                                                                                                    if (v483 != v478[1])
                                                                                                        break;
                                                                                                    v479 += 2;
                                                                                                    v478 += 2;
                                                                                                    if (!v483)
                                                                                                        goto LABEL_475cd3;
                                                                                                }
                                                                                                else
                                                                                                {
LABEL_475cd3:
                                                                                                    v484 = 0;
                                                                                                    goto LABEL_475cdc;
                                                                                                }
                                                                                            }
                                                                                            v485 = _ccall(4, v481, v482, 0);
                                                                                            v484 = -(v485 & 1) + 1 - (unsigned int)(v485 & 1 && 1);
LABEL_475cdc:
                                                                                            if (!v484)
                                                                                            {
                                                                                                *(unsigned int *)&v14[16] = 71;
                                                                                                sub_438d20(1);
                                                                                                sub_438d20(-0x1);
                                                                                                sub_438d50(-3.4028234663852886e+38f);
                                                                                                sub_438d50(-3.4028234663852886e+38f);
                                                                                                sub_438d50(0.0f);
                                                                                                sub_438d20(0);
                                                                                                if (sub_438b20(sub_433410(a0, "time"), v14 + 24, a2, 0, 1) && sub_438b20(sub_433410(a0, "id"), v14 + 48, a2, 0, 1))
                                                                                                {
                                                                                                    v486 = sub_433410(a0, "pos");
                                                                                                    if (!v486)
                                                                                                        goto LABEL_476430;
                                                                                                    v487 = sub_488ff0(v486, "ff:,", a2, &v10, v14 + 60, v14 + 72);
                                                                                                    v488 = v10;
                                                                                                    if (!*v488)
                                                                                                    {
LABEL_475df6:
                                                                                                        if (sub_40e340())
                                                                                                        {
                                                                                                            do
                                                                                                            {
                                                                                                                v10 += 1;
                                                                                                            } while (sub_40e340());
                                                                                                        }
                                                                                                        sub_40e380();
                                                                                                        switch (sub_40e380())
                                                                                                        {
                                                                                                        case 0:
LABEL_476430:
                                                                                                            if (sub_473e90(a2->field_b8))
                                                                                                                return 0x1;
                                                                                                            v5 = &v13;
                                                                                                            v4 = "Controller not valid for mugenversion of character: %s";
                                                                                                            break;
                                                                                                        case 102:
                                                                                                            sub_438d20(0);
                                                                                                        case 104:
                                                                                                            sub_438d20(2);
                                                                                                        case 109:
                                                                                                            sub_438d20(1);
                                                                                                        default:
                                                                                                            v5 = v10;
                                                                                                            v4 = "Invalid bind position: %s";
                                                                                                            v3 = &g_52b40c->field_1205c;
                                                                                                            goto LABEL_474bb5;
                                                                                                        }
LABEL_474ba8:
                                                                                                        v3 = &g_52b40c->field_1205c;
                                                                                                        goto LABEL_474bb5;
                                                                                                    }
                                                                                                    else if (*v488 == 44 && v487 >= 2)
                                                                                                    {
                                                                                                        v10 = v488 + 1;
                                                                                                        goto LABEL_475df6;
                                                                                                    }
                                                                                                }
                                                                                            }
                                                                                            else
                                                                                            {
                                                                                                v489 = "targetstate";
                                                                                                v490 = &v13;
                                                                                                while (1)
                                                                                                {
                                                                                                    v491 = *v490;
                                                                                                    v492 = v491;
                                                                                                    v493 = *v489;
                                                                                                    if (v491 != *v489)
                                                                                                        break;
                                                                                                    if (v491)
                                                                                                    {
                                                                                                        v494 = v490[1];
                                                                                                        v492 = v494;
                                                                                                        v493 = v489[1];
                                                                                                        if (v494 != v489[1])
                                                                                                            break;
                                                                                                        v490 += 2;
                                                                                                        v489 += 2;
                                                                                                        if (!v494)
                                                                                                            goto LABEL_475eac;
                                                                                                    }
                                                                                                    else
                                                                                                    {
LABEL_475eac:
                                                                                                        v495 = 0;
                                                                                                        goto LABEL_475eb5;
                                                                                                    }
                                                                                                }
                                                                                                v496 = _ccall(4, v492, v493, 0);
                                                                                                v495 = -(v496 & 1) + 1 - (unsigned int)(v496 & 1 && 1);
LABEL_475eb5:
                                                                                                if (!v495)
                                                                                                {
                                                                                                    *(unsigned int *)&v14[16] = 72;
                                                                                                    goto LABEL_475ec0;
                                                                                                }
                                                                                                v497 = "targetpoweradd";
                                                                                                v498 = &v13;
                                                                                                while (1)
                                                                                                {
                                                                                                    v499 = *v498;
                                                                                                    v500 = v499;
                                                                                                    v501 = *v497;
                                                                                                    if (v499 != *v497)
                                                                                                        break;
                                                                                                    if (v499)
                                                                                                    {
                                                                                                        v502 = v498[1];
                                                                                                        v500 = v502;
                                                                                                        v501 = v497[1];
                                                                                                        if (v502 != v497[1])
                                                                                                            break;
                                                                                                        v498 += 2;
                                                                                                        v497 += 2;
                                                                                                        if (!v502)
                                                                                                            goto LABEL_475f2e;
                                                                                                    }
                                                                                                    else
                                                                                                    {
LABEL_475f2e:
                                                                                                        v503 = 0;
                                                                                                        goto LABEL_475f37;
                                                                                                    }
                                                                                                }
                                                                                                v504 = _ccall(4, v500, v501, 0);
                                                                                                v503 = -(v504 & 1) + 1 - (unsigned int)(v504 & 1 && 1);
LABEL_475f37:
                                                                                                if (!v503)
                                                                                                {
                                                                                                    *(unsigned int *)&v14[16] = 73;
LABEL_475ec0:
                                                                                                    v505 = sub_433410(a0, "value");
                                                                                                    if (v505 && sub_438b20(v505, v14 + 24, v15, 0, 1))
                                                                                                        goto LABEL_475eef;
                                                                                                }
                                                                                                else
                                                                                                {
                                                                                                    v506 = "targetlifeadd";
                                                                                                    v507 = &v13;
                                                                                                    while (1)
                                                                                                    {
                                                                                                        v508 = *v507;
                                                                                                        v509 = v508;
                                                                                                        v510 = *v506;
                                                                                                        if (v508 != *v506)
                                                                                                            break;
                                                                                                        if (v508)
                                                                                                        {
                                                                                                            v511 = v507[1];
                                                                                                            v509 = v511;
                                                                                                            v510 = v506[1];
                                                                                                            if (v511 != v506[1])
                                                                                                                break;
                                                                                                            v507 += 2;
                                                                                                            v506 += 2;
                                                                                                            if (!v511)
                                                                                                                goto LABEL_475f6c;
                                                                                                        }
                                                                                                        else
                                                                                                        {
LABEL_475f6c:
                                                                                                            v512 = 0;
                                                                                                            goto LABEL_475f75;
                                                                                                        }
                                                                                                    }
                                                                                                    v513 = _ccall(4, v509, v510, 0);
                                                                                                    v512 = -(v513 & 1) + 1 - (unsigned int)(v513 & 1 && 1);
LABEL_475f75:
                                                                                                    if (!v512)
                                                                                                    {
                                                                                                        *(unsigned int *)&v14[16] = 74;
                                                                                                        v514 = sub_433410(a0, "value");
                                                                                                        if (v514 && sub_438b20(v514, v14 + 24, v15, 0, 1) && (sub_438d20(1), sub_438b20(sub_433410(a0, "kill"), v14 + 36, v15, 0, 1) && sub_438b20(sub_433410(a0, "absolute"), v14 + 48, v15, 0, 1)))
                                                                                                        {
                                                                                                            v213 = sub_488f40(v15, -0x1);
                                                                                                            goto LABEL_4740e6;
                                                                                                        }
                                                                                                    }
                                                                                                    else
                                                                                                    {
                                                                                                        v515 = "targetvelset";
                                                                                                        v516 = &v13;
                                                                                                        while (1)
                                                                                                        {
                                                                                                            v517 = *v516;
                                                                                                            v518 = v517;
                                                                                                            v519 = *v515;
                                                                                                            if (v517 != *v515)
                                                                                                                break;
                                                                                                            if (v517)
                                                                                                            {
                                                                                                                v520 = v516[1];
                                                                                                                v518 = v520;
                                                                                                                v519 = v515[1];
                                                                                                                if (v520 != v515[1])
                                                                                                                    break;
                                                                                                                v516 += 2;
                                                                                                                v515 += 2;
                                                                                                                if (!v520)
                                                                                                                    goto LABEL_47604c;
                                                                                                            }
                                                                                                            else
                                                                                                            {
LABEL_47604c:
                                                                                                                v521 = 0;
                                                                                                                goto LABEL_476055;
                                                                                                            }
                                                                                                        }
                                                                                                        v522 = _ccall(4, v518, v519, 0);
                                                                                                        v521 = -(v522 & 1) + 1 - (unsigned int)(v522 & 1 && 1);
LABEL_476055:
                                                                                                        if (!v521)
                                                                                                        {
                                                                                                            *(unsigned int *)&v14[16] = 75;
                                                                                                            goto LABEL_476064;
                                                                                                        }
                                                                                                        v515 = "targetveladd";
                                                                                                        v523 = &v13;
                                                                                                        while (1)
                                                                                                        {
                                                                                                            v524 = *v523;
                                                                                                            v525 = v524;
                                                                                                            v526 = *v515;
                                                                                                            if (v524 != *v515)
                                                                                                                break;
                                                                                                            if (v524)
                                                                                                            {
                                                                                                                v527 = v523[1];
                                                                                                                v525 = v527;
                                                                                                                v526 = v515[1];
                                                                                                                if (v527 != v515[1])
                                                                                                                    break;
                                                                                                                v523 += 2;
                                                                                                                v515 += 2;
                                                                                                                if (!v527)
                                                                                                                    goto LABEL_476103;
                                                                                                            }
                                                                                                            else
                                                                                                            {
LABEL_476103:
                                                                                                                v528 = 0;
                                                                                                                goto LABEL_47610c;
                                                                                                            }
                                                                                                        }
                                                                                                        v529 = _ccall(4, v525, v526, 0);
                                                                                                        v528 = -(v529 & 1) + 1 - (unsigned int)(v529 & 1 && 1);
LABEL_47610c:
                                                                                                        if (!v528)
                                                                                                        {
                                                                                                            *(unsigned int *)&v14[16] = 76;
LABEL_476064:
                                                                                                            v5 = v515;
                                                                                                            v530 = sub_488fa0(v15, -3.4028234663852886e+38f);
                                                                                                            if (v530 && (v5 = v531, v532 = sub_488fa0(v15, -3.4028234663852886e+38f), sub_488fa0(v15, -3.4028234663852886e+38f)))
                                                                                                            {
                                                                                                                v5 = v531;
                                                                                                                if (!sub_488fa0(v15, -3.4028234663852886e+38f))
                                                                                                                    return NULL;
                                                                                                                goto LABEL_475eef;
                                                                                                            }
                                                                                                        }
                                                                                                        else
                                                                                                        {
                                                                                                            v533 = "targetfacing";
                                                                                                            v534 = &v13;
                                                                                                            while (1)
                                                                                                            {
                                                                                                                v535 = *v534;
                                                                                                                v536 = v535;
                                                                                                                v537 = *v533;
                                                                                                                if (v535 != *v533)
                                                                                                                    break;
                                                                                                                if (v535)
                                                                                                                {
                                                                                                                    v538 = v534[1];
                                                                                                                    v536 = v538;
                                                                                                                    v537 = v533[1];
                                                                                                                    if (v538 != v533[1])
                                                                                                                        break;
                                                                                                                    v534 += 2;
                                                                                                                    v533 += 2;
                                                                                                                    if (!v538)
                                                                                                                        goto LABEL_476141;
                                                                                                                }
                                                                                                                else
                                                                                                                {
LABEL_476141:
                                                                                                                    v539 = 0;
                                                                                                                    goto LABEL_47614a;
                                                                                                                }
                                                                                                            }
                                                                                                            v540 = _ccall(4, v536, v537, 0);
                                                                                                            v539 = -(v540 & 1) + 1 - (unsigned int)(v540 & 1 && 1);
LABEL_47614a:
                                                                                                            if (!v539)
                                                                                                            {
                                                                                                                *(unsigned int *)&v14[16] = 77;
                                                                                                                if (!sub_438b20(sub_433410(a0, "value"), v14 + 24, v15, 0, 1))
                                                                                                                    return NULL;
LABEL_475eef:
                                                                                                                v213 = sub_488f40(v15, -0x1);
                                                                                                                goto LABEL_4740e6;
                                                                                                            }
                                                                                                            v541 = &v13;
                                                                                                            v542 = "targetdrop";
                                                                                                            while (1)
                                                                                                            {
                                                                                                                v543 = *v541;
                                                                                                                v544 = v543;
                                                                                                                v545 = *v542;
                                                                                                                if (v543 != *v542)
                                                                                                                    break;
                                                                                                                if (v543)
                                                                                                                {
                                                                                                                    v546 = v541[1];
                                                                                                                    v544 = v546;
                                                                                                                    v545 = v542[1];
                                                                                                                    if (v546 != v542[1])
                                                                                                                        break;
                                                                                                                    v541 += 2;
                                                                                                                    v542 += 2;
                                                                                                                    if (!v546)
                                                                                                                        goto LABEL_4761ac;
                                                                                                                }
                                                                                                                else
                                                                                                                {
LABEL_4761ac:
                                                                                                                    v547 = 0;
                                                                                                                    goto LABEL_4761b5;
                                                                                                                }
                                                                                                            }
                                                                                                            v548 = _ccall(4, v544, v545, 0);
                                                                                                            v547 = -(v548 & 1) + 1 - (unsigned int)(v548 & 1 && 1);
LABEL_4761b5:
                                                                                                            if (!v547)
                                                                                                            {
                                                                                                                *(unsigned int *)&v14[16] = 78;
                                                                                                                sub_438d20(-0x1);
                                                                                                                v34 = v14 + 36;
                                                                                                                sub_438d20(1);
                                                                                                                if (sub_438b20(sub_433410(a0, "excludeID"), v14 + 24, a2, 0, 1))
                                                                                                                {
                                                                                                                    v5 = "keepone";
LABEL_4740ca:
                                                                                                                    v95 = sub_433410(a0);
                                                                                                                    v3 = 1;
                                                                                                                    v2 = 0;
                                                                                                                    v1 = a2;
                                                                                                                    goto LABEL_4740dc;
                                                                                                                }
                                                                                                            }
                                                                                                            else
                                                                                                            {
                                                                                                                v549 = "attackdist";
                                                                                                                v550 = &v13;
                                                                                                                while (1)
                                                                                                                {
                                                                                                                    v551 = *v550;
                                                                                                                    v552 = v551;
                                                                                                                    v553 = *v549;
                                                                                                                    if (v551 != *v549)
                                                                                                                        break;
                                                                                                                    if (v551)
                                                                                                                    {
                                                                                                                        v554 = v550[1];
                                                                                                                        v552 = v554;
                                                                                                                        v553 = v549[1];
                                                                                                                        if (v554 != v549[1])
                                                                                                                            break;
                                                                                                                        v550 += 2;
                                                                                                                        v549 += 2;
                                                                                                                        if (!v554)
                                                                                                                            goto LABEL_476234;
                                                                                                                    }
                                                                                                                    else
                                                                                                                    {
LABEL_476234:
                                                                                                                        v555 = 0;
                                                                                                                        goto LABEL_47623d;
                                                                                                                    }
                                                                                                                }
                                                                                                                v556 = _ccall(4, v552, v553, 0);
                                                                                                                v555 = -(v556 & 1) + 1 - (unsigned int)(v556 & 1 && 1);
LABEL_47623d:
                                                                                                                if (!v555)
                                                                                                                {
                                                                                                                    *(unsigned int *)&v14[16] = 90;
                                                                                                                    goto LABEL_4742e0;
                                                                                                                }
                                                                                                                v557 = "playerpush";
                                                                                                                v558 = &v13;
                                                                                                                while (1)
                                                                                                                {
                                                                                                                    v559 = *v558;
                                                                                                                    v560 = v559;
                                                                                                                    v561 = *v557;
                                                                                                                    if (v559 != *v557)
                                                                                                                        break;
                                                                                                                    if (v559)
                                                                                                                    {
                                                                                                                        v562 = v558[1];
                                                                                                                        v560 = v562;
                                                                                                                        v561 = v557[1];
                                                                                                                        if (v562 != v557[1])
                                                                                                                            break;
                                                                                                                        v558 += 2;
                                                                                                                        v557 += 2;
                                                                                                                        if (!v562)
                                                                                                                            goto LABEL_476272;
                                                                                                                    }
                                                                                                                    else
                                                                                                                    {
LABEL_476272:
                                                                                                                        v563 = 0;
                                                                                                                        goto LABEL_47627b;
                                                                                                                    }
                                                                                                                }
                                                                                                                v564 = _ccall(4, v560, v561, 0);
                                                                                                                v563 = -(v564 & 1) + 1 - (unsigned int)(v564 & 1 && 1);
LABEL_47627b:
                                                                                                                if (!v563)
                                                                                                                {
                                                                                                                    *(unsigned int *)&v14[16] = 91;
LABEL_4742e0:
                                                                                                                    v118 = sub_433410(a0, "value");
                                                                                                                    if (!v118)
                                                                                                                        return NULL;
                                                                                                                    goto LABEL_476a86;
                                                                                                                }
                                                                                                                v565 = "varset";
                                                                                                                v566 = &v13;
                                                                                                                while (1)
                                                                                                                {
                                                                                                                    v567 = *v566;
                                                                                                                    v568 = v567;
                                                                                                                    v569 = *v565;
                                                                                                                    if (v567 != *v565)
                                                                                                                        break;
                                                                                                                    if (v567)
                                                                                                                    {
                                                                                                                        v570 = v566[1];
                                                                                                                        v568 = v570;
                                                                                                                        v569 = v565[1];
                                                                                                                        if (v570 != v565[1])
                                                                                                                            break;
                                                                                                                        v566 += 2;
                                                                                                                        v565 += 2;
                                                                                                                        if (!v570)
                                                                                                                            goto LABEL_4762b0;
                                                                                                                    }
                                                                                                                    else
                                                                                                                    {
LABEL_4762b0:
                                                                                                                        v571 = 0;
                                                                                                                        goto LABEL_4762b9;
                                                                                                                    }
                                                                                                                }
                                                                                                                v572 = _ccall(4, v568, v569, 0);
                                                                                                                v571 = -(v572 & 1) + 1 - (unsigned int)(v572 & 1 && 1);
LABEL_4762b9:
                                                                                                                if (v571)
                                                                                                                {
                                                                                                                    v573 = "varadd";
                                                                                                                    v574 = &v13;
                                                                                                                    while (1)
                                                                                                                    {
                                                                                                                        v575 = *v574;
                                                                                                                        v576 = v575;
                                                                                                                        v577 = *v573;
                                                                                                                        if (v575 != *v573)
                                                                                                                            break;
                                                                                                                        if (v575)
                                                                                                                        {
                                                                                                                            v578 = v574[1];
                                                                                                                            v576 = v578;
                                                                                                                            v577 = v573[1];
                                                                                                                            if (v578 != v573[1])
                                                                                                                                break;
                                                                                                                            v574 += 2;
                                                                                                                            v573 += 2;
                                                                                                                            if (!v578)
                                                                                                                                goto LABEL_4762ec;
                                                                                                                        }
                                                                                                                        else
                                                                                                                        {
LABEL_4762ec:
                                                                                                                            v579 = 0;
                                                                                                                            goto LABEL_4762f5;
                                                                                                                        }
                                                                                                                    }
                                                                                                                    v580 = _ccall(4, v576, v577, 0);
                                                                                                                    v579 = -(v580 & 1) + 1 - (unsigned int)(v580 & 1 && 1);
LABEL_4762f5:
                                                                                                                    if (!v579)
                                                                                                                        goto LABEL_477929;
                                                                                                                    v581 = "varrangeset";
                                                                                                                    v582 = &v13;
                                                                                                                    while (1)
                                                                                                                    {
                                                                                                                        v583 = *v582;
                                                                                                                        v584 = v583;
                                                                                                                        v585 = *v581;
                                                                                                                        if (v583 != *v581)
                                                                                                                            break;
                                                                                                                        if (v583)
                                                                                                                        {
                                                                                                                            v586 = v582[1];
                                                                                                                            v584 = v586;
                                                                                                                            v585 = v581[1];
                                                                                                                            if (v586 != v581[1])
                                                                                                                                break;
                                                                                                                            v582 += 2;
                                                                                                                            v581 += 2;
                                                                                                                            if (!v586)
                                                                                                                                goto LABEL_476322;
                                                                                                                        }
                                                                                                                        else
                                                                                                                        {
LABEL_476322:
                                                                                                                            v587 = 0;
                                                                                                                            goto LABEL_47632b;
                                                                                                                        }
                                                                                                                    }
                                                                                                                    v588 = _ccall(4, v584, v585, 0);
                                                                                                                    v587 = -(v588 & 1) + 1 - (unsigned int)(v588 & 1 && 1);
LABEL_47632b:
                                                                                                                    if (!v587)
                                                                                                                    {
                                                                                                                        *(unsigned int *)&v14[16] = 7;
                                                                                                                        v589 = sub_433410(a0, "value");
                                                                                                                        if (!v589)
                                                                                                                        {
                                                                                                                            v589 = sub_433410(a0, "fvalue");
                                                                                                                            if (!v589)
                                                                                                                            {
                                                                                                                                sprintf(&g_52b40c->field_1205c, "VarRangeSet value not specified.");
                                                                                                                                sub_443e80(); /* do not return */
                                                                                                                            }
                                                                                                                            v590 = 1;
                                                                                                                        }
                                                                                                                        else
                                                                                                                        {
                                                                                                                            v590 = 0;
                                                                                                                        }
                                                                                                                        if (sub_488f40(a2, 0) && sub_488f40(a2, (0 < (unsigned int)v590 ? 39 : 59)) && sub_438b20(v589, a1 + 48, a2, 0, (v590 != 0) + 1))
                                                                                                                        {
                                                                                                                            sub_438d20(v590);
                                                                                                                            goto LABEL_476429;
                                                                                                                        }
                                                                                                                    }
                                                                                                                    else
                                                                                                                    {
                                                                                                                        v591 = "varrandom";
                                                                                                                        v592 = &v13;
                                                                                                                        while (1)
                                                                                                                        {
                                                                                                                            v593 = *v592;
                                                                                                                            v594 = v593;
                                                                                                                            v595 = *v591;
                                                                                                                            if (v593 != *v591)
                                                                                                                                break;
                                                                                                                            if (v593)
                                                                                                                            {
                                                                                                                                v596 = v592[1];
                                                                                                                                v594 = v596;
                                                                                                                                v595 = v591[1];
                                                                                                                                if (v596 != v591[1])
                                                                                                                                    break;
                                                                                                                                v592 += 2;
                                                                                                                                v591 += 2;
                                                                                                                                if (!v596)
                                                                                                                                    goto LABEL_47648c;
                                                                                                                            }
                                                                                                                            else
                                                                                                                            {
LABEL_47648c:
                                                                                                                                v597 = 0;
                                                                                                                                goto LABEL_476495;
                                                                                                                            }
                                                                                                                        }
                                                                                                                        v598 = _ccall(4, v594, v595, 0);
                                                                                                                        v597 = -(v598 & 1) + 1 - (unsigned int)(v598 & 1 && 1);
LABEL_476495:
                                                                                                                        if (!v597)
                                                                                                                        {
                                                                                                                            *(unsigned int *)&v14[16] = 6;
                                                                                                                            v599 = sub_433410(a0, "v");
                                                                                                                            if (v599 && sub_438b20(v599, v14 + 36, v15, 0, 1))
                                                                                                                            {
                                                                                                                                v600 = v14 + 24;
                                                                                                                                sub_438d20(0);
                                                                                                                                v601 = v14 + 48;
                                                                                                                                sub_438d20(1000);
                                                                                                                                v602 = sub_433410(a0, "range");
                                                                                                                                if (!v602)
                                                                                                                                    goto LABEL_476430;
                                                                                                                                v603 = sub_488ff0(v602, "ii", a2, &v10, v600, v601);
                                                                                                                                if (!*v10 && v603 && v603 == 1)
                                                                                                                                {
                                                                                                                                    v604 = (int)v600[4];
                                                                                                                                    v605 = (int)v600[8];
                                                                                                                                    *(int *)v601 = *(int *)v600;
                                                                                                                                    *(unsigned int *)&v601[4] = v604;
                                                                                                                                    *(unsigned int *)&v601[8] = v605;
                                                                                                                                    sub_438d20(0);
                                                                                                                                    goto LABEL_476430;
                                                                                                                                }
                                                                                                                            }
                                                                                                                        }
                                                                                                                        else
                                                                                                                        {
                                                                                                                            v606 = &v13;
                                                                                                                            v607 = "offset";
                                                                                                                            while (1)
                                                                                                                            {
                                                                                                                                v608 = *v606;
                                                                                                                                v609 = v608;
                                                                                                                                v610 = *v607;
                                                                                                                                if (v608 != *v607)
                                                                                                                                    break;
                                                                                                                                if (v608)
                                                                                                                                {
                                                                                                                                    v611 = v606[1];
                                                                                                                                    v609 = v611;
                                                                                                                                    v610 = v607[1];
                                                                                                                                    if (v611 != v607[1])
                                                                                                                                        break;
                                                                                                                                    v606 += 2;
                                                                                                                                    v607 += 2;
                                                                                                                                    if (!v611)
                                                                                                                                        goto LABEL_47658c;
                                                                                                                                }
                                                                                                                                else
                                                                                                                                {
LABEL_47658c:
                                                                                                                                    v612 = 0;
                                                                                                                                    goto LABEL_476595;
                                                                                                                                }
                                                                                                                            }
                                                                                                                            v613 = _ccall(4, v609, v610, 0);
                                                                                                                            v612 = -(v613 & 1) + 1 - (unsigned int)(v613 & 1 && 1);
LABEL_476595:
                                                                                                                            if (!v612)
                                                                                                                            {
                                                                                                                                *(unsigned int *)&v14[16] = 28;
                                                                                                                                v614 = sub_433410(a0, "x");
                                                                                                                                if (!v614 || !(sub_438d20(1), !sub_438b20(v614, v14 + 60, a2, 0, 2)))
                                                                                                                                {
                                                                                                                                    v615 = sub_433410(a0, "y");
                                                                                                                                    if (!v615)
                                                                                                                                        goto LABEL_476430;
                                                                                                                                    sub_438d20(1);
                                                                                                                                    v213 = sub_438b20(v615, v14 + 72, a2, 0, 2);
                                                                                                                                    goto LABEL_4740e6;
                                                                                                                                }
                                                                                                                            }
                                                                                                                            else
                                                                                                                            {
                                                                                                                                v616 = "envshake";
                                                                                                                                v617 = &v13;
                                                                                                                                while (1)
                                                                                                                                {
                                                                                                                                    v618 = *v617;
                                                                                                                                    v619 = v618;
                                                                                                                                    v620 = *v616;
                                                                                                                                    if (v618 != *v616)
                                                                                                                                        break;
                                                                                                                                    if (v618)
                                                                                                                                    {
                                                                                                                                        v621 = v617[1];
                                                                                                                                        v619 = v621;
                                                                                                                                        v620 = v616[1];
                                                                                                                                        if (v621 != v616[1])
                                                                                                                                            break;
                                                                                                                                        v617 += 2;
                                                                                                                                        v616 += 2;
                                                                                                                                        if (!v621)
                                                                                                                                            goto LABEL_47664e;
                                                                                                                                    }
                                                                                                                                    else
                                                                                                                                    {
LABEL_47664e:
                                                                                                                                        v622 = 0;
                                                                                                                                        goto LABEL_476657;
                                                                                                                                    }
                                                                                                                                }
                                                                                                                                v623 = _ccall(4, v619, v620, 0);
                                                                                                                                v622 = -(v623 & 1) + 1 - (unsigned int)(v623 & 1 && 1);
LABEL_476657:
                                                                                                                                if (!v622)
                                                                                                                                {
                                                                                                                                    *(unsigned int *)&v14[16] = 200;
                                                                                                                                    v624 = sub_40e1b0();
                                                                                                                                    if (v624)
                                                                                                                                    {
                                                                                                                                        if (!sub_47c1e0(a0, a2))
                                                                                                                                        {
                                                                                                                                            free(v624);
                                                                                                                                            v625 = &g_52b40c->field_1205b;
                                                                                                                                            do
                                                                                                                                            {
                                                                                                                                                v626 = v625;
                                                                                                                                                index = v626 + 1;
                                                                                                                                                v625 = index;
                                                                                                                                            } while (*(char *)(v626 + 1));
                                                                                                                                            *(unsigned int *)index = 1920091402;
                                                                                                                                            *(unsigned int *)(index + 4) = 1881174639;
                                                                                                                                            *(unsigned int *)(index + 8) = 1769173601;
                                                                                                                                            *(unsigned int *)(index + 12) = 1159751534;
                                                                                                                                            *(unsigned int *)(index + 16) = 1750300270;
                                                                                                                                            *(unsigned int *)(index + 20) = 6646625;
                                                                                                                                            return NULL;
                                                                                                                                        }
                                                                                                                                        *(st_473ed0_1 **)&v14[96] = v624;
                                                                                                                                        goto LABEL_476430;
                                                                                                                                    }
                                                                                                                                }
                                                                                                                                else
                                                                                                                                {
                                                                                                                                    v628 = &v13;
                                                                                                                                    v629 = "fallenvshake";
                                                                                                                                    while (1)
                                                                                                                                    {
                                                                                                                                        v630 = *v628;
                                                                                                                                        v631 = v630;
                                                                                                                                        v632 = *v629;
                                                                                                                                        if (v630 != *v629)
                                                                                                                                            break;
                                                                                                                                        if (v630)
                                                                                                                                        {
                                                                                                                                            v633 = v628[1];
                                                                                                                                            v631 = v633;
                                                                                                                                            v632 = v629[1];
                                                                                                                                            if (v633 != v629[1])
                                                                                                                                                break;
                                                                                                                                            v628 += 2;
                                                                                                                                            v629 += 2;
                                                                                                                                            if (!v633)
                                                                                                                                                goto LABEL_47672c;
                                                                                                                                        }
                                                                                                                                        else
                                                                                                                                        {
LABEL_47672c:
                                                                                                                                            v634 = 0;
                                                                                                                                            goto LABEL_476735;
                                                                                                                                        }
                                                                                                                                    }
                                                                                                                                    v635 = _ccall(4, v631, v632, 0);
                                                                                                                                    v634 = -(v635 & 1) + 1 - (unsigned int)(v635 & 1 && 1);
LABEL_476735:
                                                                                                                                    if (!v634)
                                                                                                                                    {
                                                                                                                                        *(unsigned int *)&v14[16] = 201;
                                                                                                                                        goto LABEL_476430;
                                                                                                                                    }
                                                                                                                                    v636 = "envcolor";
                                                                                                                                    v637 = &v13;
                                                                                                                                    while (1)
                                                                                                                                    {
                                                                                                                                        v638 = *v637;
                                                                                                                                        v639 = v638;
                                                                                                                                        v640 = *v636;
                                                                                                                                        if (v638 != *v636)
                                                                                                                                            break;
                                                                                                                                        if (v638)
                                                                                                                                        {
                                                                                                                                            v641 = v637[1];
                                                                                                                                            v639 = v641;
                                                                                                                                            v640 = v636[1];
                                                                                                                                            if (v641 != v636[1])
                                                                                                                                                break;
                                                                                                                                            v637 += 2;
                                                                                                                                            v636 += 2;
                                                                                                                                            if (!v641)
                                                                                                                                                goto LABEL_47676c;
                                                                                                                                        }
                                                                                                                                        else
                                                                                                                                        {
LABEL_47676c:
                                                                                                                                            v642 = 0;
                                                                                                                                            goto LABEL_476775;
                                                                                                                                        }
                                                                                                                                    }
                                                                                                                                    v643 = _ccall(4, v639, v640, 0);
                                                                                                                                    v642 = -(v643 & 1) + 1 - (unsigned int)(v643 & 1 && 1);
LABEL_476775:
                                                                                                                                    if (!v642)
                                                                                                                                    {
                                                                                                                                        *(unsigned int *)&v14[16] = 202;
                                                                                                                                        sub_438d20(0xff);
                                                                                                                                        sub_438d20(0xff);
                                                                                                                                        sub_438d20(0xff);
                                                                                                                                        v644 = sub_433410(a0, "value");
                                                                                                                                        if (!v644 || (v645 = sub_488ff0(v644, "iii", a2, &v10, v14 + 60, v14 + 72, v14 + 84), !*v10 && sub_488ff0(v644, "iii", a2, &v10, v14 + 60, v14 + 72, v14 + 84) == 3))
                                                                                                                                        {
                                                                                                                                            sub_438d20(1);
                                                                                                                                            v34 = v14 + 48;
                                                                                                                                            sub_438d20(0);
                                                                                                                                            if (sub_438b20(sub_433410(a0, "time"), v14 + 36, a2, 0, 1))
                                                                                                                                            {
                                                                                                                                                v95 = sub_433410(a0, "under");
                                                                                                                                                v3 = 1;
                                                                                                                                                v2 = 0;
                                                                                                                                                v1 = a2;
                                                                                                                                                goto LABEL_4740dc;
                                                                                                                                            }
                                                                                                                                        }
                                                                                                                                    }
                                                                                                                                    else
                                                                                                                                    {
                                                                                                                                        v646 = "gamemakeanim";
                                                                                                                                        v647 = &v13;
                                                                                                                                        while (1)
                                                                                                                                        {
                                                                                                                                            v648 = *v647;
                                                                                                                                            v649 = v648;
                                                                                                                                            v650 = *v646;
                                                                                                                                            if (v648 != *v646)
                                                                                                                                                break;
                                                                                                                                            if (v648)
                                                                                                                                            {
                                                                                                                                                v651 = v647[1];
                                                                                                                                                v649 = v651;
                                                                                                                                                v650 = v646[1];
                                                                                                                                                if (v651 != v646[1])
                                                                                                                                                    break;
                                                                                                                                                v647 += 2;
                                                                                                                                                v646 += 2;
                                                                                                                                                if (!v651)
                                                                                                                                                    goto LABEL_47688c;
                                                                                                                                            }
                                                                                                                                            else
                                                                                                                                            {
LABEL_47688c:
                                                                                                                                                v652 = 0;
                                                                                                                                                goto LABEL_476895;
                                                                                                                                            }
                                                                                                                                        }
                                                                                                                                        v653 = _ccall(4, v649, v650, 0);
                                                                                                                                        v652 = -(v653 & 1) + 1 - (unsigned int)(v653 & 1 && 1);
LABEL_476895:
                                                                                                                                        if (!v652)
                                                                                                                                        {
                                                                                                                                            *(unsigned int *)&v14[16] = 203;
                                                                                                                                            sub_438d20(0);
                                                                                                                                            sub_438d20(0);
                                                                                                                                            sub_438d50(0.0f);
                                                                                                                                            sub_438d50(0.0f);
                                                                                                                                            if (sub_438b20(sub_433410(a0, "value"), v14 + 24, a2, 0, 1) && (v654 = a2, sub_438b20(sub_433410(a0, "under"), v14 + 36, v654, 0, 1) && (v655 = sub_433410(a0, "pos"), !v655 || (v656 = sub_488ff0(v655, "ff", v654, &v10, v14 + 60, v14 + 72), !*v10 && sub_488ff0(v655, "ff", v654, &v10, v14 + 60, v14 + 72)))))
                                                                                                                                            {
                                                                                                                                                v95 = sub_433410(a0, "random");
                                                                                                                                                v3 = 1;
                                                                                                                                                v2 = 0;
                                                                                                                                                v1 = v654;
                                                                                                                                                v0 = v14 + 48;
                                                                                                                                                goto LABEL_4740dd;
                                                                                                                                            }
                                                                                                                                        }
                                                                                                                                        else
                                                                                                                                        {
                                                                                                                                            v657 = "afterimage";
                                                                                                                                            v658 = &v13;
                                                                                                                                            while (1)
                                                                                                                                            {
                                                                                                                                                v659 = *v658;
                                                                                                                                                v660 = v659;
                                                                                                                                                v661 = *v657;
                                                                                                                                                if (v659 != *v657)
                                                                                                                                                    break;
                                                                                                                                                if (v659)
                                                                                                                                                {
                                                                                                                                                    v662 = v658[1];
                                                                                                                                                    v660 = v662;
                                                                                                                                                    v661 = v657[1];
                                                                                                                                                    if (v662 != v657[1])
                                                                                                                                                        break;
                                                                                                                                                    v658 += 2;
                                                                                                                                                    v657 += 2;
                                                                                                                                                    if (!v662)
                                                                                                                                                        goto LABEL_4769ad;
                                                                                                                                                }
                                                                                                                                                else
                                                                                                                                                {
LABEL_4769ad:
                                                                                                                                                    v663 = 0;
                                                                                                                                                    goto LABEL_4769b6;
                                                                                                                                                }
                                                                                                                                            }
                                                                                                                                            v664 = _ccall(4, v660, v661, 0);
                                                                                                                                            v663 = -(v664 & 1) + 1 - (unsigned int)(v664 & 1 && 1);
LABEL_4769b6:
                                                                                                                                            if (!v663)
                                                                                                                                            {
                                                                                                                                                ptr2 = sub_40e1b0();
                                                                                                                                                if (ptr2)
                                                                                                                                                {
                                                                                                                                                    *(unsigned int *)&v14[16] = 210;
                                                                                                                                                    if (!sub_47b5a0(a0, v15, &g_49da0a))
                                                                                                                                                    {
                                                                                                                                                        free(ptr2);
                                                                                                                                                        v665 = &g_52b40c->field_1205b;
                                                                                                                                                        do
                                                                                                                                                        {
                                                                                                                                                            v666 = v665;
                                                                                                                                                            v667 = v666 + 1;
                                                                                                                                                            v665 = v667;
                                                                                                                                                        } while (*(char *)(v666 + 1));
                                                                                                                                                        v668 = 6;
                                                                                                                                                        for (n = "\nError parsing AfterImage"; v668; n += 4)
                                                                                                                                                        {
                                                                                                                                                            v668 -= 1;
                                                                                                                                                            *(int *)v667 = *(int *)n;
                                                                                                                                                            v667 += 4;
                                                                                                                                                        }
                                                                                                                                                        *(short *)v667 = *(short *)n;
                                                                                                                                                        return NULL;
                                                                                                                                                    }
                                                                                                                                                    goto LABEL_476e7b;
                                                                                                                                                }
                                                                                                                                            }
                                                                                                                                            else
                                                                                                                                            {
                                                                                                                                                v670 = "afterimagetime";
                                                                                                                                                v671 = &v13;
                                                                                                                                                while (1)
                                                                                                                                                {
                                                                                                                                                    v672 = *v671;
                                                                                                                                                    v673 = v672;
                                                                                                                                                    v674 = *v670;
                                                                                                                                                    if (v672 != *v670)
                                                                                                                                                        break;
                                                                                                                                                    if (v672)
                                                                                                                                                    {
                                                                                                                                                        v675 = v671[1];
                                                                                                                                                        v673 = v675;
                                                                                                                                                        v674 = v670[1];
                                                                                                                                                        if (v675 != v670[1])
                                                                                                                                                            break;
                                                                                                                                                        v671 += 2;
                                                                                                                                                        v670 += 2;
                                                                                                                                                        if (!v675)
                                                                                                                                                            goto LABEL_476a5c;
                                                                                                                                                    }
                                                                                                                                                    else
                                                                                                                                                    {
LABEL_476a5c:
                                                                                                                                                        v676 = 0;
                                                                                                                                                        goto LABEL_476a65;
                                                                                                                                                    }
                                                                                                                                                }
                                                                                                                                                v677 = _ccall(4, v673, v674, 0);
                                                                                                                                                v676 = -(v677 & 1) + 1 - (unsigned int)(v677 & 1 && 1);
LABEL_476a65:
                                                                                                                                                if (!v676)
                                                                                                                                                {
                                                                                                                                                    *(unsigned int *)&v14[16] = 211;
                                                                                                                                                    v118 = sub_433410(a0, "time");
                                                                                                                                                    if (!v118)
                                                                                                                                                    {
LABEL_475760:
                                                                                                                                                        v95 = sub_433410(a0, "value");
                                                                                                                                                        v3 = 1;
                                                                                                                                                        v2 = 0;
                                                                                                                                                        v1 = v15;
                                                                                                                                                        v0 = v14 + 24;
                                                                                                                                                        goto LABEL_4740dd;
                                                                                                                                                    }
                                                                                                                                                    else
                                                                                                                                                    {
LABEL_476a86:
                                                                                                                                                        v213 = sub_438b20(v118, v14 + 24, v15, 0, 1);
                                                                                                                                                        goto LABEL_4740e6;
                                                                                                                                                    }
                                                                                                                                                }
                                                                                                                                                v678 = "pause";
                                                                                                                                                v679 = &v13;
                                                                                                                                                while (1)
                                                                                                                                                {
                                                                                                                                                    v680 = *v679;
                                                                                                                                                    v681 = v680;
                                                                                                                                                    v682 = *v678;
                                                                                                                                                    if (v680 != *v678)
                                                                                                                                                        break;
                                                                                                                                                    if (v680)
                                                                                                                                                    {
                                                                                                                                                        v683 = v679[1];
                                                                                                                                                        v681 = v683;
                                                                                                                                                        v682 = v678[1];
                                                                                                                                                        if (v683 != v678[1])
                                                                                                                                                            break;
                                                                                                                                                        v679 += 2;
                                                                                                                                                        v678 += 2;
                                                                                                                                                        if (!v683)
                                                                                                                                                            goto LABEL_476ac1;
                                                                                                                                                    }
                                                                                                                                                    else
                                                                                                                                                    {
LABEL_476ac1:
                                                                                                                                                        v684 = 0;
                                                                                                                                                        goto LABEL_476aca;
                                                                                                                                                    }
                                                                                                                                                }
                                                                                                                                                v685 = _ccall(4, v681, v682, 0);
                                                                                                                                                v684 = -(v685 & 1) + 1 - (unsigned int)(v685 & 1 && 1);
LABEL_476aca:
                                                                                                                                                if (!v684)
                                                                                                                                                {
                                                                                                                                                    ptr = sub_40e1b0();
                                                                                                                                                    if (ptr)
                                                                                                                                                    {
                                                                                                                                                        v232 = v14;
                                                                                                                                                        *(unsigned int *)&v232[16] = 212;
                                                                                                                                                        if (!sub_4824b0(a2))
                                                                                                                                                        {
                                                                                                                                                            free(ptr);
                                                                                                                                                            v686 = &g_52b40c->field_1205b;
                                                                                                                                                            do
                                                                                                                                                            {
                                                                                                                                                                v687 = v686;
                                                                                                                                                                idx1 = v687 + 1;
                                                                                                                                                                v686 = idx1;
                                                                                                                                                            } while (*(char *)(v687 + 1));
                                                                                                                                                            *(unsigned int *)idx1 = 1920091402;
                                                                                                                                                            *(unsigned int *)(idx1 + 4) = 1881174639;
                                                                                                                                                            *(unsigned int *)(idx1 + 8) = 1769173601;
                                                                                                                                                            *(unsigned int *)(idx1 + 12) = 1344300910;
                                                                                                                                                            *(unsigned int *)(idx1 + 16) = 1702065505;
                                                                                                                                                            *(char *)(idx1 + 20) = 0;
                                                                                                                                                            return NULL;
                                                                                                                                                        }
                                                                                                                                                        goto LABEL_477177;
                                                                                                                                                    }
                                                                                                                                                }
                                                                                                                                                else
                                                                                                                                                {
                                                                                                                                                    v689 = "superpause";
                                                                                                                                                    v690 = &v13;
                                                                                                                                                    while (1)
                                                                                                                                                    {
                                                                                                                                                        v691 = *v690;
                                                                                                                                                        v692 = v691;
                                                                                                                                                        v693 = *v689;
                                                                                                                                                        if (v691 != *v689)
                                                                                                                                                            break;
                                                                                                                                                        if (v691)
                                                                                                                                                        {
                                                                                                                                                            v694 = v690[1];
                                                                                                                                                            v692 = v694;
                                                                                                                                                            v693 = v689[1];
                                                                                                                                                            if (v694 != v689[1])
                                                                                                                                                                break;
                                                                                                                                                            v690 += 2;
                                                                                                                                                            v689 += 2;
                                                                                                                                                            if (!v694)
                                                                                                                                                                goto LABEL_476b92;
                                                                                                                                                        }
                                                                                                                                                        else
                                                                                                                                                        {
LABEL_476b92:
                                                                                                                                                            v695 = 0;
                                                                                                                                                            goto LABEL_476b9b;
                                                                                                                                                        }
                                                                                                                                                    }
                                                                                                                                                    v696 = _ccall(4, v692, v693, 0);
                                                                                                                                                    v695 = -(v696 & 1) + 1 - (unsigned int)(v696 & 1 && 1);
LABEL_476b9b:
                                                                                                                                                    if (!v695)
                                                                                                                                                    {
                                                                                                                                                        ptr2 = sub_40e1b0();
                                                                                                                                                        if (ptr2)
                                                                                                                                                        {
                                                                                                                                                            *(unsigned int *)&v14[16] = 214;
                                                                                                                                                            if (!sub_4827d0(v15, ptr2))
                                                                                                                                                            {
                                                                                                                                                                free(ptr2);
                                                                                                                                                                v697 = &g_52b40c->field_1205b;
                                                                                                                                                                do
                                                                                                                                                                {
                                                                                                                                                                    v698 = v697;
                                                                                                                                                                    v699 = v698 + 1;
                                                                                                                                                                    v697 = v699;
                                                                                                                                                                } while (*(char *)(v698 + 1));
                                                                                                                                                                v700 = 6;
                                                                                                                                                                for (i0 = "\nError parsing SuperPause"; v700; i0 += 4)
                                                                                                                                                                {
                                                                                                                                                                    v700 -= 1;
                                                                                                                                                                    *(int *)v699 = *(int *)i0;
                                                                                                                                                                    v699 += 4;
                                                                                                                                                                }
                                                                                                                                                                *(short *)v699 = *(short *)i0;
                                                                                                                                                                return NULL;
                                                                                                                                                            }
                                                                                                                                                            goto LABEL_476e7b;
                                                                                                                                                        }
                                                                                                                                                    }
                                                                                                                                                    else
                                                                                                                                                    {
                                                                                                                                                        v702 = "makedust";
                                                                                                                                                        v703 = &v13;
                                                                                                                                                        while (1)
                                                                                                                                                        {
                                                                                                                                                            v704 = *v703;
                                                                                                                                                            v705 = v704;
                                                                                                                                                            v706 = *v702;
                                                                                                                                                            if (v704 != *v702)
                                                                                                                                                                break;
                                                                                                                                                            if (v704)
                                                                                                                                                            {
                                                                                                                                                                v707 = v703[1];
                                                                                                                                                                v705 = v707;
                                                                                                                                                                v706 = v702[1];
                                                                                                                                                                if (v707 != v702[1])
                                                                                                                                                                    break;
                                                                                                                                                                v703 += 2;
                                                                                                                                                                v702 += 2;
                                                                                                                                                                if (!v707)
                                                                                                                                                                    goto LABEL_476c3c;
                                                                                                                                                            }
                                                                                                                                                            else
                                                                                                                                                            {
LABEL_476c3c:
                                                                                                                                                                v708 = 0;
                                                                                                                                                                goto LABEL_476c45;
                                                                                                                                                            }
                                                                                                                                                        }
                                                                                                                                                        v709 = _ccall(4, v705, v706, 0);
                                                                                                                                                        v708 = -(v709 & 1) + 1 - (unsigned int)(v709 & 1 && 1);
LABEL_476c45:
                                                                                                                                                        if (!v708)
                                                                                                                                                        {
                                                                                                                                                            *(unsigned int *)&v14[16] = 215;
                                                                                                                                                            sub_438d20(3);
                                                                                                                                                            v710 = sub_433410(a0, "pos");
                                                                                                                                                            if (!v710 || !(v711 = sub_488ff0(v710, "ii", a2, &v10, v14 + 24, v14 + 36), *v10 || !sub_488ff0(v710, "ii", a2, &v10, v14 + 24, v14 + 36)))
                                                                                                                                                            {
                                                                                                                                                                v712 = sub_433410(a0, "pos2");
                                                                                                                                                                if (v712)
                                                                                                                                                                {
                                                                                                                                                                    v713 = sub_488ff0(v712, "ff", a2, &v10, v14 + 60, v14 + 72);
                                                                                                                                                                    if (!*v10 && v713)
                                                                                                                                                                    {
                                                                                                                                                                        v5 = v531;
                                                                                                                                                                        sub_438d50(1.0f);
                                                                                                                                                                        goto LABEL_476d08;
                                                                                                                                                                    }
                                                                                                                                                                }
                                                                                                                                                                else
                                                                                                                                                                {
LABEL_476d08:
                                                                                                                                                                    v95 = sub_433410(a0, "spacing");
                                                                                                                                                                    v3 = 1;
                                                                                                                                                                    v2 = 0;
                                                                                                                                                                    v1 = a2;
                                                                                                                                                                    v0 = v14 + 48;
                                                                                                                                                                    goto LABEL_4740dd;
                                                                                                                                                                }
                                                                                                                                                            }
                                                                                                                                                        }
                                                                                                                                                        else
                                                                                                                                                        {
                                                                                                                                                            v714 = "palfx";
                                                                                                                                                            v715 = &v13;
                                                                                                                                                            v716 = v15;
                                                                                                                                                            while (1)
                                                                                                                                                            {
                                                                                                                                                                v717 = *v715;
                                                                                                                                                                v718 = v717;
                                                                                                                                                                v719 = *v714;
                                                                                                                                                                if (v717 != *v714)
                                                                                                                                                                    break;
                                                                                                                                                                if (v717)
                                                                                                                                                                {
                                                                                                                                                                    v720 = v715[1];
                                                                                                                                                                    v718 = v720;
                                                                                                                                                                    v719 = v714[1];
                                                                                                                                                                    if (v720 != v714[1])
                                                                                                                                                                        break;
                                                                                                                                                                    v715 += 2;
                                                                                                                                                                    v714 += 2;
                                                                                                                                                                    if (!v720)
                                                                                                                                                                        goto LABEL_476d4c;
                                                                                                                                                                }
                                                                                                                                                                else
                                                                                                                                                                {
LABEL_476d4c:
                                                                                                                                                                    v721 = 0;
                                                                                                                                                                    goto LABEL_476d55;
                                                                                                                                                                }
                                                                                                                                                            }
                                                                                                                                                            v722 = _ccall(4, v718, v719, 0);
                                                                                                                                                            v721 = -(v722 & 1) + 1 - (unsigned int)(v722 & 1 && 1);
LABEL_476d55:
                                                                                                                                                            if (!v721)
                                                                                                                                                            {
                                                                                                                                                                *(unsigned int *)&v14[16] = 216;
                                                                                                                                                                v723 = sub_40e1b0();
                                                                                                                                                                if (v723 && sub_47cdb0(a0, v716, 0))
                                                                                                                                                                {
                                                                                                                                                                    v723->field_0 = 0;
                                                                                                                                                                    *(st_473ed0_1 **)&v14[96] = v723;
                                                                                                                                                                    goto LABEL_476430;
                                                                                                                                                                }
                                                                                                                                                            }
                                                                                                                                                            else
                                                                                                                                                            {
                                                                                                                                                                v724 = "bgpalfx";
                                                                                                                                                                v725 = &v13;
                                                                                                                                                                while (1)
                                                                                                                                                                {
                                                                                                                                                                    v726 = *v725;
                                                                                                                                                                    v727 = v726;
                                                                                                                                                                    v728 = *v724;
                                                                                                                                                                    if (v726 != *v724)
                                                                                                                                                                        break;
                                                                                                                                                                    if (v726)
                                                                                                                                                                    {
                                                                                                                                                                        v729 = v725[1];
                                                                                                                                                                        v727 = v729;
                                                                                                                                                                        v728 = v724[1];
                                                                                                                                                                        if (v729 != v724[1])
                                                                                                                                                                            break;
                                                                                                                                                                        v725 += 2;
                                                                                                                                                                        v724 += 2;
                                                                                                                                                                        if (!v729)
                                                                                                                                                                            goto LABEL_476dc0;
                                                                                                                                                                    }
                                                                                                                                                                    else
                                                                                                                                                                    {
LABEL_476dc0:
                                                                                                                                                                        v730 = 0;
                                                                                                                                                                        goto LABEL_476dc9;
                                                                                                                                                                    }
                                                                                                                                                                }
                                                                                                                                                                v731 = _ccall(4, v727, v728, 0);
                                                                                                                                                                v730 = -(v731 & 1) + 1 - (unsigned int)(v731 & 1 && 1);
LABEL_476dc9:
                                                                                                                                                                if (!v730)
                                                                                                                                                                {
                                                                                                                                                                    *(unsigned int *)&v14[16] = 216;
                                                                                                                                                                    v732 = sub_40e1b0();
                                                                                                                                                                    if (v732 && sub_47cdb0(a0, v716, 0))
                                                                                                                                                                    {
                                                                                                                                                                        v732->field_0 = 1;
                                                                                                                                                                        *(st_473ed0_1 **)&v14[96] = v732;
                                                                                                                                                                        goto LABEL_476430;
                                                                                                                                                                    }
                                                                                                                                                                }
                                                                                                                                                                else
                                                                                                                                                                {
                                                                                                                                                                    v733 = "allpalfx";
                                                                                                                                                                    v734 = &v13;
                                                                                                                                                                    while (1)
                                                                                                                                                                    {
                                                                                                                                                                        v735 = *v734;
                                                                                                                                                                        v736 = v735;
                                                                                                                                                                        v737 = *v733;
                                                                                                                                                                        if (v735 != *v733)
                                                                                                                                                                            break;
                                                                                                                                                                        if (v735)
                                                                                                                                                                        {
                                                                                                                                                                            v738 = v734[1];
                                                                                                                                                                            v736 = v738;
                                                                                                                                                                            v737 = v733[1];
                                                                                                                                                                            if (v738 != v733[1])
                                                                                                                                                                                break;
                                                                                                                                                                            v734 += 2;
                                                                                                                                                                            v733 += 2;
                                                                                                                                                                            if (!v738)
                                                                                                                                                                                goto LABEL_476e34;
                                                                                                                                                                        }
                                                                                                                                                                        else
                                                                                                                                                                        {
LABEL_476e34:
                                                                                                                                                                            v739 = 0;
                                                                                                                                                                            goto LABEL_476e3d;
                                                                                                                                                                        }
                                                                                                                                                                    }
                                                                                                                                                                    v740 = _ccall(4, v736, v737, 0);
                                                                                                                                                                    v739 = -(v740 & 1) + 1 - (unsigned int)(v740 & 1 && 1);
LABEL_476e3d:
                                                                                                                                                                    if (!v739)
                                                                                                                                                                    {
                                                                                                                                                                        *(unsigned int *)&v14[16] = 216;
                                                                                                                                                                        ptr2 = sub_40e1b0();
                                                                                                                                                                        if (ptr2 && sub_47cdb0(a0, v716, 0))
                                                                                                                                                                        {
                                                                                                                                                                            ptr2->field_0 = 2;
                                                                                                                                                                            goto LABEL_476e7b;
                                                                                                                                                                        }
                                                                                                                                                                    }
                                                                                                                                                                    else
                                                                                                                                                                    {
                                                                                                                                                                        v741 = "remappal";
                                                                                                                                                                        v742 = &v13;
                                                                                                                                                                        while (1)
                                                                                                                                                                        {
                                                                                                                                                                            v743 = *v742;
                                                                                                                                                                            v744 = v743;
                                                                                                                                                                            v745 = *v741;
                                                                                                                                                                            if (v743 != *v741)
                                                                                                                                                                                break;
                                                                                                                                                                            if (v743)
                                                                                                                                                                            {
                                                                                                                                                                                v746 = v742[1];
                                                                                                                                                                                v744 = v746;
                                                                                                                                                                                v745 = v741[1];
                                                                                                                                                                                if (v746 != v741[1])
                                                                                                                                                                                    break;
                                                                                                                                                                                v742 += 2;
                                                                                                                                                                                v741 += 2;
                                                                                                                                                                                if (!v746)
                                                                                                                                                                                    goto LABEL_476eac;
                                                                                                                                                                            }
                                                                                                                                                                            else
                                                                                                                                                                            {
LABEL_476eac:
                                                                                                                                                                                v747 = 0;
                                                                                                                                                                                goto LABEL_476eb5;
                                                                                                                                                                            }
                                                                                                                                                                        }
                                                                                                                                                                        v748 = _ccall(4, v744, v745, 0);
                                                                                                                                                                        v747 = -(v748 & 1) + 1 - (unsigned int)(v748 & 1 && 1);
LABEL_476eb5:
                                                                                                                                                                        if (!v747)
                                                                                                                                                                        {
                                                                                                                                                                            *(unsigned int *)&v14[16] = 217;
                                                                                                                                                                            sub_438d20(0);
                                                                                                                                                                            sub_438d20(0);
                                                                                                                                                                            v749 = sub_433410(a0, "source");
                                                                                                                                                                            if ((!v749 || (sub_488ff0(v749, "ii", a2, &v10, v14 + 24, v14 + 36), !*v10)) && (v750 = sub_433410(a0, "dest"), v750))
                                                                                                                                                                            {
                                                                                                                                                                                sub_488ff0(v750, "ii", a2, &v10, v14 + 60, v14 + 72);
                                                                                                                                                                                if (*v10)
                                                                                                                                                                                    return NULL;
                                                                                                                                                                            }
                                                                                                                                                                        }
                                                                                                                                                                        else
                                                                                                                                                                        {
                                                                                                                                                                            v751 = "explod";
                                                                                                                                                                            v752 = &v13;
                                                                                                                                                                            while (1)
                                                                                                                                                                            {
                                                                                                                                                                                v753 = *v752;
                                                                                                                                                                                v754 = v753;
                                                                                                                                                                                v755 = *v751;
                                                                                                                                                                                if (v753 != *v751)
                                                                                                                                                                                    break;
                                                                                                                                                                                if (v753)
                                                                                                                                                                                {
                                                                                                                                                                                    v756 = v752[1];
                                                                                                                                                                                    v754 = v756;
                                                                                                                                                                                    v755 = v751[1];
                                                                                                                                                                                    if (v756 != v751[1])
                                                                                                                                                                                        break;
                                                                                                                                                                                    v752 += 2;
                                                                                                                                                                                    v751 += 2;
                                                                                                                                                                                    if (!v756)
                                                                                                                                                                                        goto LABEL_476f94;
                                                                                                                                                                                }
                                                                                                                                                                                else
                                                                                                                                                                                {
LABEL_476f94:
                                                                                                                                                                                    v757 = 0;
                                                                                                                                                                                    goto LABEL_476f9d;
                                                                                                                                                                                }
                                                                                                                                                                            }
                                                                                                                                                                            v758 = _ccall(4, v754, v755, 0);
                                                                                                                                                                            v757 = -(v758 & 1) + 1 - (unsigned int)(v758 & 1 && 1);
LABEL_476f9d:
                                                                                                                                                                            if (!v757)
                                                                                                                                                                            {
                                                                                                                                                                                ptr = sub_40e1b0();
                                                                                                                                                                                if (ptr)
                                                                                                                                                                                {
                                                                                                                                                                                    v232 = v14;
                                                                                                                                                                                    *(unsigned int *)&v232[16] = 220;
                                                                                                                                                                                    if (!sub_47c5d0(a2, 0))
                                                                                                                                                                                        return NULL;
                                                                                                                                                                                    goto LABEL_477177;
                                                                                                                                                                                }
                                                                                                                                                                            }
                                                                                                                                                                            else
                                                                                                                                                                            {
                                                                                                                                                                                v759 = "removeexplod";
                                                                                                                                                                                v760 = &v13;
                                                                                                                                                                                while (1)
                                                                                                                                                                                {
                                                                                                                                                                                    v761 = *v760;
                                                                                                                                                                                    v762 = v761;
                                                                                                                                                                                    v763 = *v759;
                                                                                                                                                                                    if (v761 != *v759)
                                                                                                                                                                                        break;
                                                                                                                                                                                    if (v761)
                                                                                                                                                                                    {
                                                                                                                                                                                        v764 = v760[1];
                                                                                                                                                                                        v762 = v764;
                                                                                                                                                                                        v763 = v759[1];
                                                                                                                                                                                        if (v764 != v759[1])
                                                                                                                                                                                            break;
                                                                                                                                                                                        v760 += 2;
                                                                                                                                                                                        v759 += 2;
                                                                                                                                                                                        if (!v764)
                                                                                                                                                                                            goto LABEL_47700f;
                                                                                                                                                                                    }
                                                                                                                                                                                    else
                                                                                                                                                                                    {
LABEL_47700f:
                                                                                                                                                                                        v765 = 0;
                                                                                                                                                                                        goto LABEL_477018;
                                                                                                                                                                                    }
                                                                                                                                                                                }
                                                                                                                                                                                v766 = _ccall(4, v762, v763, 0);
                                                                                                                                                                                v765 = -(v766 & 1) + 1 - (unsigned int)(v766 & 1 && 1);
LABEL_477018:
                                                                                                                                                                                if (!v765)
                                                                                                                                                                                {
                                                                                                                                                                                    v34 = v14 + 24;
                                                                                                                                                                                    *(unsigned int *)&v14[16] = 0xdd;
                                                                                                                                                                                    sub_438d20(-0x1);
                                                                                                                                                                                    v95 = sub_433410(a0, "id");
                                                                                                                                                                                    v3 = 1;
                                                                                                                                                                                    v2 = 0;
                                                                                                                                                                                    v1 = v716;
LABEL_4740dc:
                                                                                                                                                                                    v0 = v34;
LABEL_4740dd:
                                                                                                                                                                                    v213 = sub_438b20(v95);
                                                                                                                                                                                    goto LABEL_4740e6;
                                                                                                                                                                                }
                                                                                                                                                                                else
                                                                                                                                                                                {
                                                                                                                                                                                    v767 = "explodbindtime";
                                                                                                                                                                                    v768 = &v13;
                                                                                                                                                                                    v769 = v716;
                                                                                                                                                                                    while (1)
                                                                                                                                                                                    {
                                                                                                                                                                                        v770 = *v768;
                                                                                                                                                                                        v771 = v770;
                                                                                                                                                                                        v772 = *v767;
                                                                                                                                                                                        if (v770 != *v767)
                                                                                                                                                                                            break;
                                                                                                                                                                                        if (v770)
                                                                                                                                                                                        {
                                                                                                                                                                                            v773 = v768[1];
                                                                                                                                                                                            v771 = v773;
                                                                                                                                                                                            v772 = v767[1];
                                                                                                                                                                                            if (v773 != v767[1])
                                                                                                                                                                                                break;
                                                                                                                                                                                            v768 += 2;
                                                                                                                                                                                            v767 += 2;
                                                                                                                                                                                            if (!v773)
                                                                                                                                                                                                goto LABEL_47706c;
                                                                                                                                                                                        }
                                                                                                                                                                                        else
                                                                                                                                                                                        {
LABEL_47706c:
                                                                                                                                                                                            v774 = 0;
                                                                                                                                                                                            goto LABEL_477075;
                                                                                                                                                                                        }
                                                                                                                                                                                    }
                                                                                                                                                                                    v775 = _ccall(4, v771, v772, 0);
                                                                                                                                                                                    v774 = -(v775 & 1) + 1 - (unsigned int)(v775 & 1 && 1);
LABEL_477075:
                                                                                                                                                                                    if (!v774)
                                                                                                                                                                                    {
                                                                                                                                                                                        *(unsigned int *)&v14[16] = 222;
                                                                                                                                                                                        sub_438d20(-0x1);
                                                                                                                                                                                        sub_438d20(1);
                                                                                                                                                                                        if (sub_438b20(sub_433410(a0, "id"), v14 + 24, a2, 0, 1))
                                                                                                                                                                                        {
                                                                                                                                                                                            v776 = sub_433410(a0, "time");
                                                                                                                                                                                            if (!v776)
                                                                                                                                                                                                v776 = sub_433410(a0, "value");
                                                                                                                                                                                            v213 = sub_438b20(v776, v14 + 36, a2, 0, 1);
                                                                                                                                                                                            goto LABEL_4740e6;
                                                                                                                                                                                        }
                                                                                                                                                                                    }
                                                                                                                                                                                    else
                                                                                                                                                                                    {
                                                                                                                                                                                        v777 = &v13;
                                                                                                                                                                                        v778 = "modifyexplod";
                                                                                                                                                                                        while (1)
                                                                                                                                                                                        {
                                                                                                                                                                                            v779 = *v777;
                                                                                                                                                                                            v780 = v779;
                                                                                                                                                                                            v781 = *v778;
                                                                                                                                                                                            if (v779 != *v778)
                                                                                                                                                                                                break;
                                                                                                                                                                                            if (v779)
                                                                                                                                                                                            {
                                                                                                                                                                                                v782 = v777[1];
                                                                                                                                                                                                v780 = v782;
                                                                                                                                                                                                v781 = v778[1];
                                                                                                                                                                                                if (v782 != v778[1])
                                                                                                                                                                                                    break;
                                                                                                                                                                                                v777 += 2;
                                                                                                                                                                                                v778 += 2;
                                                                                                                                                                                                if (!v782)
                                                                                                                                                                                                    goto LABEL_47712c;
                                                                                                                                                                                            }
                                                                                                                                                                                            else
                                                                                                                                                                                            {
LABEL_47712c:
                                                                                                                                                                                                v783 = 0;
                                                                                                                                                                                                goto LABEL_477135;
                                                                                                                                                                                            }
                                                                                                                                                                                        }
                                                                                                                                                                                        v784 = _ccall(4, v780, v781, 0);
                                                                                                                                                                                        v783 = -(v784 & 1) + 1 - (unsigned int)(v784 & 1 && 1);
LABEL_477135:
                                                                                                                                                                                        if (!v783)
                                                                                                                                                                                        {
                                                                                                                                                                                            ptr = sub_40e1b0();
                                                                                                                                                                                            if (ptr && (v232 = v14, *(unsigned int *)&v232[16] = 223, sub_47c5d0(a2, 1)))
                                                                                                                                                                                            {
LABEL_477177:
                                                                                                                                                                                                *(st_473ed0_1 **)&v232[96] = ptr;
                                                                                                                                                                                                goto LABEL_476430;
                                                                                                                                                                                            }
                                                                                                                                                                                        }
                                                                                                                                                                                        else
                                                                                                                                                                                        {
                                                                                                                                                                                            v785 = "forcefeedback";
                                                                                                                                                                                            v786 = &v13;
                                                                                                                                                                                            v787 = v14;
                                                                                                                                                                                            while (1)
                                                                                                                                                                                            {
                                                                                                                                                                                                v788 = *v786;
                                                                                                                                                                                                v789 = v788;
                                                                                                                                                                                                v790 = *v785;
                                                                                                                                                                                                if (v788 != *v785)
                                                                                                                                                                                                    break;
                                                                                                                                                                                                if (v788)
                                                                                                                                                                                                {
                                                                                                                                                                                                    v791 = v786[1];
                                                                                                                                                                                                    v789 = v791;
                                                                                                                                                                                                    v790 = v785[1];
                                                                                                                                                                                                    if (v791 != v785[1])
                                                                                                                                                                                                        break;
                                                                                                                                                                                                    v786 += 2;
                                                                                                                                                                                                    v785 += 2;
                                                                                                                                                                                                    if (!v791)
                                                                                                                                                                                                        goto LABEL_4771ac;
                                                                                                                                                                                                }
                                                                                                                                                                                                else
                                                                                                                                                                                                {
LABEL_4771ac:
                                                                                                                                                                                                    v792 = 0;
                                                                                                                                                                                                    goto LABEL_4771b5;
                                                                                                                                                                                                }
                                                                                                                                                                                            }
                                                                                                                                                                                            v793 = _ccall(4, v789, v790, 0);
                                                                                                                                                                                            v792 = -(v793 & 1) + 1 - (unsigned int)(v793 & 1 && 1);
LABEL_4771b5:
                                                                                                                                                                                            if (!v792)
                                                                                                                                                                                            {
                                                                                                                                                                                                *(unsigned int *)&v787[16] = 0;
                                                                                                                                                                                                goto LABEL_476430;
                                                                                                                                                                                            }
                                                                                                                                                                                            v794 = "helper";
                                                                                                                                                                                            v795 = &v13;
                                                                                                                                                                                            v14 = v787;
                                                                                                                                                                                            while (1)
                                                                                                                                                                                            {
                                                                                                                                                                                                v796 = *v795;
                                                                                                                                                                                                v797 = v796;
                                                                                                                                                                                                v798 = *v794;
                                                                                                                                                                                                if (v796 != *v794)
                                                                                                                                                                                                    break;
                                                                                                                                                                                                if (v796)
                                                                                                                                                                                                {
                                                                                                                                                                                                    v799 = v795[1];
                                                                                                                                                                                                    v797 = v799;
                                                                                                                                                                                                    v798 = v794[1];
                                                                                                                                                                                                    if (v799 != v794[1])
                                                                                                                                                                                                        break;
                                                                                                                                                                                                    v795 += 2;
                                                                                                                                                                                                    v794 += 2;
                                                                                                                                                                                                    if (!v799)
                                                                                                                                                                                                        goto LABEL_4771ec;
                                                                                                                                                                                                }
                                                                                                                                                                                                else
                                                                                                                                                                                                {
LABEL_4771ec:
                                                                                                                                                                                                    v800 = 0;
                                                                                                                                                                                                    goto LABEL_4771f5;
                                                                                                                                                                                                }
                                                                                                                                                                                            }
                                                                                                                                                                                            v801 = _ccall(4, v797, v798, 0);
                                                                                                                                                                                            v800 = -(v801 & 1) + 1 - (unsigned int)(v801 & 1 && 1);
LABEL_4771f5:
                                                                                                                                                                                            if (!v800)
                                                                                                                                                                                            {
                                                                                                                                                                                                *(unsigned int *)&v14[16] = 300;
                                                                                                                                                                                                ptr2 = sub_40e1b0();
                                                                                                                                                                                                if (ptr2)
                                                                                                                                                                                                {
                                                                                                                                                                                                    if (!sub_481ec0(a0, v769))
                                                                                                                                                                                                    {
                                                                                                                                                                                                        free(ptr2);
                                                                                                                                                                                                        v802 = &g_52b40c->field_1205b;
                                                                                                                                                                                                        do
                                                                                                                                                                                                        {
                                                                                                                                                                                                            v803 = v802;
                                                                                                                                                                                                            idx2 = v803 + 1;
                                                                                                                                                                                                            v802 = idx2;
                                                                                                                                                                                                        } while (*(char *)(v803 + 1));
                                                                                                                                                                                                        *(unsigned int *)idx2 = 1920091402;
                                                                                                                                                                                                        *(unsigned int *)(idx2 + 4) = 1881174639;
                                                                                                                                                                                                        *(unsigned int *)(idx2 + 8) = 1769173601;
                                                                                                                                                                                                        *(unsigned int *)(idx2 + 12) = 1210083182;
                                                                                                                                                                                                        *(unsigned int *)(idx2 + 16) = 1701866597;
                                                                                                                                                                                                        *(unsigned short *)(idx2 + 20) = 114;
                                                                                                                                                                                                        return NULL;
                                                                                                                                                                                                    }
LABEL_476e7b:
                                                                                                                                                                                                    *(st_473ed0_1 **)&v14[96] = ptr2;
                                                                                                                                                                                                    goto LABEL_476430;
                                                                                                                                                                                                }
                                                                                                                                                                                            }
                                                                                                                                                                                            else
                                                                                                                                                                                            {
                                                                                                                                                                                                v805 = "destroyself";
                                                                                                                                                                                                v806 = &v13;
                                                                                                                                                                                                while (1)
                                                                                                                                                                                                {
                                                                                                                                                                                                    v807 = *v806;
                                                                                                                                                                                                    v808 = v807;
                                                                                                                                                                                                    v809 = *v805;
                                                                                                                                                                                                    if (v807 != *v805)
                                                                                                                                                                                                        break;
                                                                                                                                                                                                    if (v807)
                                                                                                                                                                                                    {
                                                                                                                                                                                                        v810 = v806[1];
                                                                                                                                                                                                        v808 = v810;
                                                                                                                                                                                                        v809 = v805[1];
                                                                                                                                                                                                        if (v810 != v805[1])
                                                                                                                                                                                                            break;
                                                                                                                                                                                                        v806 += 2;
                                                                                                                                                                                                        v805 += 2;
                                                                                                                                                                                                        if (!v810)
                                                                                                                                                                                                            goto LABEL_4772b4;
                                                                                                                                                                                                    }
                                                                                                                                                                                                    else
                                                                                                                                                                                                    {
LABEL_4772b4:
                                                                                                                                                                                                        v811 = 0;
                                                                                                                                                                                                        goto LABEL_4772bd;
                                                                                                                                                                                                    }
                                                                                                                                                                                                }
                                                                                                                                                                                                v812 = _ccall(4, v808, v809, 0);
                                                                                                                                                                                                v811 = -(v812 & 1) + 1 - (unsigned int)(v812 & 1 && 1);
LABEL_4772bd:
                                                                                                                                                                                                if (!v811)
                                                                                                                                                                                                {
                                                                                                                                                                                                    *(unsigned int *)&v14[16] = 301;
                                                                                                                                                                                                    goto LABEL_476430;
                                                                                                                                                                                                }
                                                                                                                                                                                                v813 = "parentvarset";
                                                                                                                                                                                                v814 = &v13;
                                                                                                                                                                                                while (1)
                                                                                                                                                                                                {
                                                                                                                                                                                                    v815 = *v814;
                                                                                                                                                                                                    v816 = v815;
                                                                                                                                                                                                    v817 = *v813;
                                                                                                                                                                                                    if (v815 != *v813)
                                                                                                                                                                                                        break;
                                                                                                                                                                                                    if (v815)
                                                                                                                                                                                                    {
                                                                                                                                                                                                        v818 = v814[1];
                                                                                                                                                                                                        v816 = v818;
                                                                                                                                                                                                        v817 = v813[1];
                                                                                                                                                                                                        if (v818 != v813[1])
                                                                                                                                                                                                            break;
                                                                                                                                                                                                        v814 += 2;
                                                                                                                                                                                                        v813 += 2;
                                                                                                                                                                                                        if (!v818)
                                                                                                                                                                                                            goto LABEL_4772f2;
                                                                                                                                                                                                    }
                                                                                                                                                                                                    else
                                                                                                                                                                                                    {
LABEL_4772f2:
                                                                                                                                                                                                        v819 = 0;
                                                                                                                                                                                                        goto LABEL_4772fb;
                                                                                                                                                                                                    }
                                                                                                                                                                                                }
                                                                                                                                                                                                v820 = _ccall(4, v816, v817, 0);
                                                                                                                                                                                                v819 = -(v820 & 1) + 1 - (unsigned int)(v820 & 1 && 1);
LABEL_4772fb:
                                                                                                                                                                                                if (!v819)
                                                                                                                                                                                                {
                                                                                                                                                                                                    *(unsigned int *)&v14[16] = 304;
                                                                                                                                                                                                    v821 = sub_433410(a0, "value");
                                                                                                                                                                                                    if (v821)
                                                                                                                                                                                                        goto LABEL_477af0;
                                                                                                                                                                                                    v822 = sub_4337f0();
                                                                                                                                                                                                    if (!sub_40e400(3))
                                                                                                                                                                                                    {
                                                                                                                                                                                                        do
                                                                                                                                                                                                        {
                                                                                                                                                                                                            if (sub_40e400(4))
                                                                                                                                                                                                                break;
                                                                                                                                                                                                            if (!v822)
                                                                                                                                                                                                                goto LABEL_4740ee;
                                                                                                                                                                                                            v822 = sub_433860();
                                                                                                                                                                                                        } while (!sub_40e400(3));
                                                                                                                                                                                                    }
                                                                                                                                                                                                    if (v822)
                                                                                                                                                                                                    {
                                                                                                                                                                                                        v823 = a1;
                                                                                                                                                                                                        v824 = *v822 != 118;
                                                                                                                                                                                                        v825 = &v822[v824 + 3];
                                                                                                                                                                                                        sub_438d20(v824);
                                                                                                                                                                                                        if (sub_40e340())
                                                                                                                                                                                                        {
                                                                                                                                                                                                            do
                                                                                                                                                                                                            {
                                                                                                                                                                                                                v825 = &v825->padding_0[1];
                                                                                                                                                                                                            } while (sub_40e340());
                                                                                                                                                                                                        }
                                                                                                                                                                                                        if (v825->padding_0[0] == 40 && (v826 = a2, sub_438b20(&v825->padding_0[1], v823 + 36, v826, &v10, 1) && (v827 = v10, *v827 == 41)))
                                                                                                                                                                                                        {
                                                                                                                                                                                                            v828 = v827[1];
                                                                                                                                                                                                            v829 = v827 + 1;
                                                                                                                                                                                                            if (v827[1])
                                                                                                                                                                                                            {
                                                                                                                                                                                                                do
                                                                                                                                                                                                                {
                                                                                                                                                                                                                    v830 = v829;
                                                                                                                                                                                                                    if (v828 == 61)
                                                                                                                                                                                                                        goto LABEL_477417;
                                                                                                                                                                                                                } while ((v828 = v830[1], v829 = v830 + 1, v830[1]));
                                                                                                                                                                                                            }
                                                                                                                                                                                                            goto LABEL_47740e;
                                                                                                                                                                                                        }
                                                                                                                                                                                                    }
                                                                                                                                                                                                }
                                                                                                                                                                                                else
                                                                                                                                                                                                {
                                                                                                                                                                                                    v831 = "parentvaradd";
                                                                                                                                                                                                    v832 = &v13;
                                                                                                                                                                                                    v833 = v769;
                                                                                                                                                                                                    while (1)
                                                                                                                                                                                                    {
                                                                                                                                                                                                        v834 = *v832;
                                                                                                                                                                                                        v835 = v834;
                                                                                                                                                                                                        v836 = *v831;
                                                                                                                                                                                                        if (v834 != *v831)
                                                                                                                                                                                                            break;
                                                                                                                                                                                                        if (v834)
                                                                                                                                                                                                        {
                                                                                                                                                                                                            v837 = v832[1];
                                                                                                                                                                                                            v835 = v837;
                                                                                                                                                                                                            v836 = v831[1];
                                                                                                                                                                                                            if (v837 != v831[1])
                                                                                                                                                                                                                break;
                                                                                                                                                                                                            v832 += 2;
                                                                                                                                                                                                            v831 += 2;
                                                                                                                                                                                                            if (!v837)
                                                                                                                                                                                                                goto LABEL_47744c;
                                                                                                                                                                                                        }
                                                                                                                                                                                                        else
                                                                                                                                                                                                        {
LABEL_47744c:
                                                                                                                                                                                                            v838 = 0;
                                                                                                                                                                                                            goto LABEL_477455;
                                                                                                                                                                                                        }
                                                                                                                                                                                                    }
                                                                                                                                                                                                    v839 = _ccall(4, v835, v836, 0);
                                                                                                                                                                                                    v838 = -(v839 & 1) + 1 - (unsigned int)(v839 & 1 && 1);
LABEL_477455:
                                                                                                                                                                                                    if (!v838)
                                                                                                                                                                                                    {
                                                                                                                                                                                                        *(unsigned int *)&v14[16] = 305;
                                                                                                                                                                                                        v821 = sub_433410(a0, "value");
                                                                                                                                                                                                        if (v821)
                                                                                                                                                                                                            goto LABEL_477af0;
                                                                                                                                                                                                        v840 = sub_4337f0();
                                                                                                                                                                                                        if (!sub_40e400(3))
                                                                                                                                                                                                        {
                                                                                                                                                                                                            do
                                                                                                                                                                                                            {
                                                                                                                                                                                                                if (sub_40e400(4))
                                                                                                                                                                                                                    break;
                                                                                                                                                                                                                if (!v840)
                                                                                                                                                                                                                    goto LABEL_4740ee;
                                                                                                                                                                                                                v840 = sub_433860();
                                                                                                                                                                                                            } while (!sub_40e400(3));
                                                                                                                                                                                                        }
                                                                                                                                                                                                        if (v840)
                                                                                                                                                                                                        {
                                                                                                                                                                                                            v823 = a1;
                                                                                                                                                                                                            v824 = *v840 != 118;
                                                                                                                                                                                                            v841 = &v840[v824 + 3];
                                                                                                                                                                                                            sub_438d20(v824);
                                                                                                                                                                                                            if (sub_40e340())
                                                                                                                                                                                                            {
                                                                                                                                                                                                                do
                                                                                                                                                                                                                {
                                                                                                                                                                                                                    v841 = &v841->padding_0[1];
                                                                                                                                                                                                                } while (sub_40e340());
                                                                                                                                                                                                            }
                                                                                                                                                                                                            if (v841->padding_0[0] == 40 && (v826 = a2, sub_438b20(&v841->padding_0[1], v823 + 36, v826, &v10, 1) && (v842 = v10, *v842 == 41)))
                                                                                                                                                                                                            {
                                                                                                                                                                                                                v843 = v842[1];
                                                                                                                                                                                                                v829 = v842 + 1;
                                                                                                                                                                                                                if (v842[1])
                                                                                                                                                                                                                {
                                                                                                                                                                                                                    do
                                                                                                                                                                                                                    {
                                                                                                                                                                                                                        v830 = v829;
                                                                                                                                                                                                                        if (v843 == 61)
                                                                                                                                                                                                                            goto LABEL_477417;
                                                                                                                                                                                                                    } while ((v843 = v830[1], v829 = v830 + 1, v830[1]));
                                                                                                                                                                                                                }
LABEL_47740e:
                                                                                                                                                                                                                v830 = v829;
                                                                                                                                                                                                                if (*v830 == 61)
                                                                                                                                                                                                                {
LABEL_477417:
                                                                                                                                                                                                                    v844 = (v824 != 0) + 1;
                                                                                                                                                                                                                    v845 = v830 + 1;
                                                                                                                                                                                                                    goto LABEL_477acc;
                                                                                                                                                                                                                }
                                                                                                                                                                                                            }
                                                                                                                                                                                                        }
                                                                                                                                                                                                    }
                                                                                                                                                                                                    else
                                                                                                                                                                                                    {
                                                                                                                                                                                                        v846 = "bindtoparent";
                                                                                                                                                                                                        v847 = &v13;
                                                                                                                                                                                                        while (1)
                                                                                                                                                                                                        {
                                                                                                                                                                                                            v848 = *v847;
                                                                                                                                                                                                            v849 = v848;
                                                                                                                                                                                                            v850 = *v846;
                                                                                                                                                                                                            if (v848 != *v846)
                                                                                                                                                                                                                break;
                                                                                                                                                                                                            if (v848)
                                                                                                                                                                                                            {
                                                                                                                                                                                                                v851 = v847[1];
                                                                                                                                                                                                                v849 = v851;
                                                                                                                                                                                                                v850 = v846[1];
                                                                                                                                                                                                                if (v851 != v846[1])
                                                                                                                                                                                                                    break;
                                                                                                                                                                                                                v847 += 2;
                                                                                                                                                                                                                v846 += 2;
                                                                                                                                                                                                                if (!v851)
                                                                                                                                                                                                                    goto LABEL_4775b2;
                                                                                                                                                                                                            }
                                                                                                                                                                                                            else
                                                                                                                                                                                                            {
LABEL_4775b2:
                                                                                                                                                                                                                v852 = 0;
                                                                                                                                                                                                                goto LABEL_4775bb;
                                                                                                                                                                                                            }
                                                                                                                                                                                                        }
                                                                                                                                                                                                        v853 = _ccall(4, v849, v850, 0);
                                                                                                                                                                                                        v852 = -(v853 & 1) + 1 - (unsigned int)(v853 & 1 && 1);
LABEL_4775bb:
                                                                                                                                                                                                        if (!v852)
                                                                                                                                                                                                        {
                                                                                                                                                                                                            *(unsigned int *)&v14[16] = 320;
                                                                                                                                                                                                            sub_438d20(1);
                                                                                                                                                                                                            if (sub_438b20(sub_433410(a0, "time"), v14 + 24, v833, 0, 1) && sub_438b20(sub_433410(a0, "facing"), v14 + 36, v833, 0, 1))
                                                                                                                                                                                                            {
                                                                                                                                                                                                                v854 = sub_433410(a0, "pos");
                                                                                                                                                                                                                if (!v854)
                                                                                                                                                                                                                    goto LABEL_476430;
                                                                                                                                                                                                                if (sub_488ff0(v854, "ff", v833, &v10, v14 + 60, v14 + 72) && *v10)
                                                                                                                                                                                                                    return NULL;
                                                                                                                                                                                                            }
                                                                                                                                                                                                        }
                                                                                                                                                                                                        else
                                                                                                                                                                                                        {
                                                                                                                                                                                                            v855 = &v13;
                                                                                                                                                                                                            v856 = "bindtoroot";
                                                                                                                                                                                                            while (1)
                                                                                                                                                                                                            {
                                                                                                                                                                                                                v857 = *v855;
                                                                                                                                                                                                                v858 = v857;
                                                                                                                                                                                                                v859 = *v856;
                                                                                                                                                                                                                if (v857 != *v856)
                                                                                                                                                                                                                    break;
                                                                                                                                                                                                                if (v857)
                                                                                                                                                                                                                {
                                                                                                                                                                                                                    v860 = v855[1];
                                                                                                                                                                                                                    v858 = v860;
                                                                                                                                                                                                                    v859 = v856[1];
                                                                                                                                                                                                                    if (v860 != v856[1])
                                                                                                                                                                                                                        break;
                                                                                                                                                                                                                    v855 += 2;
                                                                                                                                                                                                                    v856 += 2;
                                                                                                                                                                                                                    if (!v860)
                                                                                                                                                                                                                        goto LABEL_47769c;
                                                                                                                                                                                                                }
                                                                                                                                                                                                                else
                                                                                                                                                                                                                {
LABEL_47769c:
                                                                                                                                                                                                                    v861 = 0;
                                                                                                                                                                                                                    goto LABEL_4776a5;
                                                                                                                                                                                                                }
                                                                                                                                                                                                            }
                                                                                                                                                                                                            v862 = _ccall(4, v858, v859, 0);
                                                                                                                                                                                                            v861 = -(v862 & 1) + 1 - (unsigned int)(v862 & 1 && 1);
LABEL_4776a5:
                                                                                                                                                                                                            if (!v861)
                                                                                                                                                                                                            {
                                                                                                                                                                                                                *(unsigned int *)&v14[16] = 321;
                                                                                                                                                                                                                sub_438d20(1);
                                                                                                                                                                                                                if (sub_438b20(sub_433410(a0, "time"), v14 + 24, v833, 0, 1) && sub_438b20(sub_433410(a0, "facing"), v14 + 36, v833, 0, 1))
                                                                                                                                                                                                                {
                                                                                                                                                                                                                    v863 = sub_433410(a0, "pos");
                                                                                                                                                                                                                    if (!v863)
                                                                                                                                                                                                                        goto LABEL_476430;
                                                                                                                                                                                                                    if (sub_488ff0(v863, "ff", v833, &v10, v14 + 60, v14 + 72) && *v10)
                                                                                                                                                                                                                        return NULL;
                                                                                                                                                                                                                }
                                                                                                                                                                                                            }
                                                                                                                                                                                                            else
                                                                                                                                                                                                            {
                                                                                                                                                                                                                v864 = "displaytoclipboard";
                                                                                                                                                                                                                v865 = &v13;
                                                                                                                                                                                                                while (1)
                                                                                                                                                                                                                {
                                                                                                                                                                                                                    v866 = *v865;
                                                                                                                                                                                                                    v867 = v866;
                                                                                                                                                                                                                    v868 = *v864;
                                                                                                                                                                                                                    if (v866 != *v864)
                                                                                                                                                                                                                        break;
                                                                                                                                                                                                                    if (v866)
                                                                                                                                                                                                                    {
                                                                                                                                                                                                                        v869 = v865[1];
                                                                                                                                                                                                                        v867 = v869;
                                                                                                                                                                                                                        v868 = v864[1];
                                                                                                                                                                                                                        if (v869 != v864[1])
                                                                                                                                                                                                                            break;
                                                                                                                                                                                                                        v865 += 2;
                                                                                                                                                                                                                        v864 += 2;
                                                                                                                                                                                                                        if (!v869)
                                                                                                                                                                                                                            goto LABEL_477783;
                                                                                                                                                                                                                    }
                                                                                                                                                                                                                    else
                                                                                                                                                                                                                    {
LABEL_477783:
                                                                                                                                                                                                                        v870 = 0;
                                                                                                                                                                                                                        goto LABEL_47778c;
                                                                                                                                                                                                                    }
                                                                                                                                                                                                                }
                                                                                                                                                                                                                v871 = _ccall(4, v867, v868, 0);
                                                                                                                                                                                                                v870 = -(v871 & 1) + 1 - (unsigned int)(v871 & 1 && 1);
LABEL_47778c:
                                                                                                                                                                                                                if (v870)
                                                                                                                                                                                                                {
                                                                                                                                                                                                                    v872 = &v13;
                                                                                                                                                                                                                    v873 = "appendtoclipboard";
                                                                                                                                                                                                                    while (1)
                                                                                                                                                                                                                    {
                                                                                                                                                                                                                        v874 = *v872;
                                                                                                                                                                                                                        v875 = v874;
                                                                                                                                                                                                                        v876 = *v873;
                                                                                                                                                                                                                        if (v874 != *v873)
                                                                                                                                                                                                                            break;
                                                                                                                                                                                                                        if (v874)
                                                                                                                                                                                                                        {
                                                                                                                                                                                                                            v877 = v872[1];
                                                                                                                                                                                                                            v875 = v877;
                                                                                                                                                                                                                            v876 = v873[1];
                                                                                                                                                                                                                            if (v877 != v873[1])
                                                                                                                                                                                                                                break;
                                                                                                                                                                                                                            v872 += 2;
                                                                                                                                                                                                                            v873 += 2;
                                                                                                                                                                                                                            if (!v877)
                                                                                                                                                                                                                                goto LABEL_4777bc;
                                                                                                                                                                                                                        }
                                                                                                                                                                                                                        else
                                                                                                                                                                                                                        {
LABEL_4777bc:
                                                                                                                                                                                                                            v878 = 0;
                                                                                                                                                                                                                            goto LABEL_4777c5;
                                                                                                                                                                                                                        }
                                                                                                                                                                                                                    }
                                                                                                                                                                                                                    v879 = _ccall(4, v875, v876, 0);
                                                                                                                                                                                                                    v878 = -(v879 & 1) + 1 - (unsigned int)(v879 & 1 && 1);
LABEL_4777c5:
                                                                                                                                                                                                                    if (!v878)
                                                                                                                                                                                                                        goto LABEL_477861;
                                                                                                                                                                                                                    v880 = "clearclipboard";
                                                                                                                                                                                                                    v881 = &v13;
                                                                                                                                                                                                                    while (1)
                                                                                                                                                                                                                    {
                                                                                                                                                                                                                        v882 = *v881;
                                                                                                                                                                                                                        v883 = v882;
                                                                                                                                                                                                                        v884 = *v880;
                                                                                                                                                                                                                        if (v882 != *v880)
                                                                                                                                                                                                                            break;
                                                                                                                                                                                                                        if (v882)
                                                                                                                                                                                                                        {
                                                                                                                                                                                                                            v885 = v881[1];
                                                                                                                                                                                                                            v883 = v885;
                                                                                                                                                                                                                            v884 = v880[1];
                                                                                                                                                                                                                            if (v885 != v880[1])
                                                                                                                                                                                                                                break;
                                                                                                                                                                                                                            v881 += 2;
                                                                                                                                                                                                                            v880 += 2;
                                                                                                                                                                                                                            if (!v885)
                                                                                                                                                                                                                                goto LABEL_4777f2;
                                                                                                                                                                                                                        }
                                                                                                                                                                                                                        else
                                                                                                                                                                                                                        {
LABEL_4777f2:
                                                                                                                                                                                                                            v886 = 0;
                                                                                                                                                                                                                            goto LABEL_4777fb;
                                                                                                                                                                                                                        }
                                                                                                                                                                                                                    }
                                                                                                                                                                                                                    v887 = _ccall(4, v883, v884, 0);
                                                                                                                                                                                                                    v886 = -(v887 & 1) + 1 - (unsigned int)(v887 & 1 && 1);
LABEL_4777fb:
                                                                                                                                                                                                                    if (!v886)
                                                                                                                                                                                                                    {
                                                                                                                                                                                                                        *(unsigned int *)&v14[16] = 312;
                                                                                                                                                                                                                        goto LABEL_476430;
                                                                                                                                                                                                                    }
                                                                                                                                                                                                                    else
                                                                                                                                                                                                                    {
                                                                                                                                                                                                                        v888 = "null";
                                                                                                                                                                                                                        v889 = &v13;
                                                                                                                                                                                                                        while (1)
                                                                                                                                                                                                                        {
                                                                                                                                                                                                                            v890 = *v889;
                                                                                                                                                                                                                            v891 = v890;
                                                                                                                                                                                                                            v892 = *v888;
                                                                                                                                                                                                                            if (v890 != *v888)
                                                                                                                                                                                                                                break;
                                                                                                                                                                                                                            if (v890)
                                                                                                                                                                                                                            {
                                                                                                                                                                                                                                v893 = v889[1];
                                                                                                                                                                                                                                v891 = v893;
                                                                                                                                                                                                                                v892 = v888[1];
                                                                                                                                                                                                                                if (v893 != v888[1])
                                                                                                                                                                                                                                    break;
                                                                                                                                                                                                                                v889 += 2;
                                                                                                                                                                                                                                v888 += 2;
                                                                                                                                                                                                                                if (!v893)
                                                                                                                                                                                                                                    goto LABEL_477830;
                                                                                                                                                                                                                            }
                                                                                                                                                                                                                            else
                                                                                                                                                                                                                            {
LABEL_477830:
                                                                                                                                                                                                                                v894 = 0;
                                                                                                                                                                                                                                goto LABEL_477839;
                                                                                                                                                                                                                            }
                                                                                                                                                                                                                        }
                                                                                                                                                                                                                        v895 = _ccall(4, v891, v892, 0);
                                                                                                                                                                                                                        v894 = -(v895 & 1) + 1 - (unsigned int)(v895 & 1 && 1);
LABEL_477839:
                                                                                                                                                                                                                        if (!v894)
                                                                                                                                                                                                                        {
                                                                                                                                                                                                                            *(unsigned int *)&v14[16] = 0;
                                                                                                                                                                                                                            goto LABEL_476430;
                                                                                                                                                                                                                        }
                                                                                                                                                                                                                        else
                                                                                                                                                                                                                        {
                                                                                                                                                                                                                            v5 = &v13;
                                                                                                                                                                                                                            v4 = "Not a valid elem type: %s";
                                                                                                                                                                                                                            v3 = &g_52b40c->field_1205c;
                                                                                                                                                                                                                            goto LABEL_474bb5;
                                                                                                                                                                                                                        }
                                                                                                                                                                                                                    }
                                                                                                                                                                                                                    goto LABEL_474bb5;
                                                                                                                                                                                                                }
                                                                                                                                                                                                                else
                                                                                                                                                                                                                {
LABEL_477861:
                                                                                                                                                                                                                    *(unsigned int *)&v14[16] = (*&v13 != 100) + 310;
                                                                                                                                                                                                                    v896 = sub_433410(a0, "text");
                                                                                                                                                                                                                    if (!v896)
                                                                                                                                                                                                                    {
                                                                                                                                                                                                                        v5 = &v13;
                                                                                                                                                                                                                        v4 = "Text not specified for %s.";
                                                                                                                                                                                                                        v3 = &g_52b40c->field_1205c;
LABEL_474bb5:
                                                                                                                                                                                                                        sprintf(v3, v4);
                                                                                                                                                                                                                        return NULL;
                                                                                                                                                                                                                    }
                                                                                                                                                                                                                    v897 = sub_48a540(v896);
                                                                                                                                                                                                                    if (v897)
                                                                                                                                                                                                                    {
                                                                                                                                                                                                                        *(st_473ed0_1 **)&v14[96] = v897;
                                                                                                                                                                                                                        v898 = sub_433410(a0, "params");
                                                                                                                                                                                                                        if (!v898)
                                                                                                                                                                                                                            goto LABEL_476430;
                                                                                                                                                                                                                        v899 = sub_488ff0(v898, "nnnnnn", v833, &v10, v14 + 24, v14 + 36, v14 + 48, v14 + 60, v14 + 72, v14 + 84);
                                                                                                                                                                                                                        if (v899 && v899 == v897->field_30 && *v10)
                                                                                                                                                                                                                            return NULL;
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
LABEL_477929:
                                                                                                                    v900 = "varset";
                                                                                                                    v901 = &v13;
                                                                                                                    while (1)
                                                                                                                    {
                                                                                                                        v902 = *v901;
                                                                                                                        v903 = v902;
                                                                                                                        v904 = *v900;
                                                                                                                        if (v902 != *v900)
                                                                                                                            break;
                                                                                                                        if (v902)
                                                                                                                        {
                                                                                                                            v905 = v901[1];
                                                                                                                            v903 = v905;
                                                                                                                            v904 = v900[1];
                                                                                                                            if (v905 != v900[1])
                                                                                                                                break;
                                                                                                                            v901 += 2;
                                                                                                                            v900 += 2;
                                                                                                                            if (!v905)
                                                                                                                                goto LABEL_47794e;
                                                                                                                        }
                                                                                                                        else
                                                                                                                        {
LABEL_47794e:
                                                                                                                            v906 = 0;
                                                                                                                            goto LABEL_477957;
                                                                                                                        }
                                                                                                                    }
                                                                                                                    v907 = _ccall(4, v903, v904, 0);
                                                                                                                    v906 = -(v907 & 1) + 1 - (unsigned int)(v907 & 1 && 1);
LABEL_477957:
                                                                                                                    if (!v906)
                                                                                                                        *(unsigned int *)&v14[16] = 4;
                                                                                                                    else
                                                                                                                        *(unsigned int *)&v14[16] = 5;
                                                                                                                    v821 = sub_433410(a0, "value");
                                                                                                                    if (v821)
                                                                                                                    {
LABEL_477af0:
                                                                                                                        v915 = sub_433410(a0, "v");
                                                                                                                        if (v915)
                                                                                                                        {
                                                                                                                            v916 = a2;
                                                                                                                            if (sub_438b20(v821, v14 + 24, v916, 0, 1))
                                                                                                                            {
                                                                                                                                v213 = sub_438b20(v915, v14 + 36, v916, 0, 1);
LABEL_4740e6:
                                                                                                                                if (!v213)
                                                                                                                                    goto LABEL_4740ee;
                                                                                                                                else
                                                                                                                                    goto LABEL_476430;
                                                                                                                            }
                                                                                                                        }
                                                                                                                        else
                                                                                                                        {
                                                                                                                            v917 = sub_433410(a0, "fv");
                                                                                                                            if (!v917)
                                                                                                                                goto LABEL_476430;
                                                                                                                            v918 = a2;
                                                                                                                            if (sub_438b20(v917, v14 + 36, v918, 0, 1))
                                                                                                                            {
                                                                                                                                if (!sub_438b20(v821, v14 + 24, v918, 0, 2))
                                                                                                                                    return NULL;
                                                                                                                                goto LABEL_47757b;
                                                                                                                            }
                                                                                                                        }
                                                                                                                    }
                                                                                                                    else if (sub_4337f0())
                                                                                                                    {
                                                                                                                        while (!sub_40e400(3) && !sub_40e400(4) && !sub_40e400(6) && !sub_40e400(7))
                                                                                                                        {
                                                                                                                            v908 = sub_433860();
                                                                                                                            if (!v908)
                                                                                                                                return v908;
                                                                                                                        }
                                                                                                                        if (v908)
                                                                                                                        {
                                                                                                                            v909 = (*v908 == 118 ? 0 : *v908 == 0x66 ? 1 : (v908[3] != 118) + 3);
                                                                                                                            v823 = a1;
                                                                                                                            v910 = &v908[v909 + 3];
                                                                                                                            sub_438d20(v909);
                                                                                                                            if (sub_40e340())
                                                                                                                            {
                                                                                                                                do
                                                                                                                                {
                                                                                                                                    v910 = &v910->padding_0[1];
                                                                                                                                } while (sub_40e340());
                                                                                                                            }
                                                                                                                            if (v910->padding_0[0] == 40 && (v826 = a2, sub_438b20(&v910->padding_0[1], v823 + 36, v826, &v10, 1) && (v911 = v10, *v911 == 41)))
                                                                                                                            {
                                                                                                                                v912 = v911[1];
                                                                                                                                v913 = v911 + 1;
                                                                                                                                if (v911[1])
                                                                                                                                {
                                                                                                                                    do
                                                                                                                                    {
                                                                                                                                        v914 = v913;
                                                                                                                                        if (v912 == 61)
                                                                                                                                            goto LABEL_477ab7;
                                                                                                                                    } while ((v912 = v914[1], v913 = v914 + 1, v914[1]));
                                                                                                                                }
                                                                                                                                if (*v913 == 61)
                                                                                                                                {
LABEL_477ab7:
                                                                                                                                    v845 = v913 + 1;
                                                                                                                                    if (v909 == 1 || (v844 = 1, v909 == 4))
                                                                                                                                        v844 = 2;
LABEL_477acc:
                                                                                                                                    if (!sub_438b20(v845, v823 + 24, v826, 0, v844))
                                                                                                                                        return NULL;
LABEL_476429:
                                                                                                                                    goto LABEL_476430;
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
LABEL_4740ee:
    return NULL;
}
