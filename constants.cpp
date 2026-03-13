#include "stdafx.h"
#include "char.h"
#include "../../common/Controls.h"
#include "char_manager.h"
TJobInitialPoints JobInitialPoints[JOB_MAX_NUM] =
/*
   {
   int st, ht, dx, iq;
   int max_hp, max_sp;
   int hp_per_ht, sp_per_iq;
   int hp_per_lv_begin, hp_per_lv_end;
   int sp_per_lv_begin, sp_per_lv_end;
   int max_stamina;
   int stamina_per_con;
   int stamina_per_lv_begin, stamina_per_lv_end;
   }
 */
{
	{   6,  4,  3,  3,  600,   300,     40,    20,    44, 44,     22, 22,     800,      5,      3, 3  }, // JOB_WARRIOR  16
	{   4,  3,  6,  3,  650,   300,     40,    20,    44, 44,     22, 22,     800,      5,      3, 3  }, // JOB_ASSASSIN 16
	{   5,  3,  3,  5,  650,   300,     40,    20,    44, 44,     22, 22,     800,      5,      3, 3  }, // JOB_SURA	 16
	{   3,  4,  3,  6,  700,   300,     40,    20,    44, 44,     22, 22,     800,      5,      3, 3  }, // JOB_SHAMANa  16
#ifdef ENABLE_WOLFMAN_CHARACTER
	{   2,  6,  6,  2,  600,   300,     40,    20,    44, 44,     22, 22,     800,      5,      3, 3  },
#endif
};

const TMobRankStat MobRankStats[MOB_RANK_MAX_NUM] =
/*
   {
   int         iGoldPercent;
   }
 */
{
	{  20,  }, // MOB_RANK_PAWN,
	{  20,  }, // MOB_RANK_S_PAWN,
	{  25,  }, // MOB_RANK_KNIGHT,
	{  30,  }, // MOB_RANK_S_KNIGHT,
	{  50,  }, // MOB_RANK_BOSS,
	{ 100,  }  // MOB_RANK_KING,
};

TBattleTypeStat BattleTypeStats[BATTLE_TYPE_MAX_NUM] =
/*
   {
   int         AttGradeBias;
   int         DefGradeBias;
   int         MagicAttGradeBias;
   int         MagicDefGradeBias;
   }
 */
{
	{	  0,	  0,	  0,	-10	}, // BATTLE_TYPE_MELEE,
	{	 10,	-20,	-10,	-15	}, // BATTLE_TYPE_RANGE,
	{	 -5,	 -5,	 10,	 10	}, // BATTLE_TYPE_MAGIC,
	{	  0,	  0,	  0,	  0	}, // BATTLE_TYPE_SPECIAL,
	{	 10,	-10,	  0,	-15	}, // BATTLE_TYPE_POWER,
	{	-10,	 10,	-10,	  0	}, // BATTLE_TYPE_TANKER,
	{	 20,	-20,	  0,	-10	}, // BATTLE_TYPE_SUPER_POWER,
	{	-20,	 20,	-10,	  0	}, // BATTLE_TYPE_SUPER_TANKER,
};

const DWORD* exp_table = NULL;

const DWORD exp_table_common[PLAYER_MAX_LEVEL_CONST + 1] =
{
	0,			// 0
	300,        //500
	800,        //700
	1500,        //1000
	2500,       
	4300,       
	7200,       
	11000,       
	17000,       
	24000,       
	33000,    //    10
	43000,       
	58000,       
	76000,       
	100000,       
	130000,       
	169000,       
	219000,       
	283000,       
	365000,       
	472000,    //    20
	610000,       
	705000,       
	813000,       
	937000,       
	1077000,       
	1237000,       
	1418000,       
	1624000,       
	1857000,       
	2122000,    //    30
	2421000,       
	2761000,       
	3145000,       
	3580000,       
	4073000,       
	4632000,       
	5194000,       
	5717000,       
	6264000,       
	6837000,    //    40
	7600000,       
	8274000,       
	8990000,       
	9753000,       
	10560000,       
	11410000,       
	12320000,       
	13270000,       
	14280000,       
	15340000,    //    50
	16870000,       
	18960000,       
	19980000,       
	21420000,       
	22930000,       
	24530000,       
	26200000,       
	27960000,       
	29800000,       
	32780000,    //    60
	36060000,       
	39670000,       
	43640000,       
	48000000,       
	52800000,       
	58080000,       
	63890000,       
	70280000,       
	77310000,       
	85040000,    //    70
	93540000,       
	102900000,       
	113200000,       
	124500000,       
	137000000,       
	150700000,       
	165700000,       
	236990000,       
	260650000,       
	286780000,    //    80
	315380000,       
	346970000,       
	381680000,       
	419770000,       
	461760000,       
	508040000,       
	558740000,       
	614640000,       
	676130000,       
	743730000,    //    90
	1041222000,       
	1145344200,       
	1259878620,       
	1385866482,       
	1524453130,       
	1676898443,       
	1844588288,       
	1929047116,       
	2000000000,    //    99
	2020000000,    //    100
	2040000000,       
	2060000000,       
	2080000000,       
	2090000000,       
	2100000000,    //    105
	2100000000,       
	2100000000,       
	2100000000,       
	2100000000,       
	2100000000,    //    110
	2100000000,       
	2100000000,       
	2100000000,       
	2100000000,       
	2100000000,    //    115
	2100000000,       
	2100000000,       
	2100000000,       
	2100000000,       
	2100000000,    //    120
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 130
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 140
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 150
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 160
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 170
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 180
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 190
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 200
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 210
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 220
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 230
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 240
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 250
};

const DWORD exp_table_newcibn[PLAYER_EXP_TABLE_MAX + 1 ] =
{
	300,    // 0
	800,    // 1
	1500,
	2500,
	4300,
	7200,
	11400,
	17000,
	24000,
	33000,
	46000,    // 10
	62000,
	83000,
	111000,
	149000,
	200000,
	268000,
	360000,
	482000,
	647000,
	868000,    // 20
	996000,
	1143000,
	1312000,
	1506000,
	1729000,
	1984000,
	2277000,
	2614000,
	3000000,
	3443000,    // 30
	3952000,
	4536000,
	5206000,
	5975000,
	6858000,
	7730000,
	8504000,
	9307000,
	10140000,
	11330000,    // 40
	12320000,
	13370000,
	14490000,
	15670000,
	16920000,
	18240000,
	19630000,
	21090000,
	22630000,
	24670000,    // 50
	26890000,
	29310000,
	31950000,
	34820000,
	37960000,
	41370000,
	45100000,
	49160000,
	53580000,
	58400000,    // 60
	63660000,
	69390000,
	75630000,
	82440000,
	89860000,
	97950000,
	106760000,
	136370000,
	151800000,
	168300000,    // 70
	250000000,
	340000000,
	450000000,
	570000000,
	690000000,
	810000000,
	912600000,
	1004000000,
	1094000000,
	1182000000,    // 80
	1269000000,
	1354000000,
	1438000000,
	1521000000,
	1603000000,
	1684000000,
	1764000000,
	1844000000,
	1922000000,
	2000000000,    // 90
	2000000000,
	2000000000,
	2000000000,
	2000000000,
	2000000000,
	2000000000,
	2000000000,
	2000000000,
	2000000000,    //    99
	2000000000,    //    100
	2000000000,       
	2000000000,       
	2000000000,       
	2000000000,       
	2000000000,    //    105
	2000000000,       
	2000000000,       
	2000000000,       
	2000000000,       
	2000000000,    //    110
	2000000000,       
	2000000000,       
	2000000000,       
	2000000000,       
	2000000000,    //    115
	2000000000,       
	2000000000,       
	2000000000,       
	2000000000,       
	2000000000,    //    120
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 130
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 140
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 150
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 160
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 170
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 180
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 190
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 200
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 210
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 220
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 230
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 240
	2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, 2100000000, // 250
};

const int* aiPercentByDeltaLev = NULL;
const int* aiPercentByDeltaLevForBoss = NULL;

const int aiPercentByDeltaLevForBoss_euckr[MAX_EXP_DELTA_OF_LEV] =
{
	1,      // -15  0
	3,          // -14  1
	5,          // -13  2
	7,          // -12  3
	15,         // -11  4
	30,         // -10  5
	60,         // -9   6
	90,         // -8   7
	91,         // -7   8
	92,         // -6   9
	93,         // -5   10
	94,         // -4   11
	95,         // -3   12
	97,         // -2   13
	99,         // -1   14
	100,        // 0    15
	105,        // 1    16
	110,        // 2    17
	115,        // 3    18
	120,        // 4    19
	125,        // 5    20
	130,        // 6    21
	135,        // 7    22
	140,        // 8    23
	145,        // 9    24
	150,        // 10   25
	155,        // 11   26
	160,        // 12   27
	165,        // 13   28
	170,        // 14   29
	180         // 15   30
};

const int aiPercentByDeltaLev_euckr[MAX_EXP_DELTA_OF_LEV] =
{
	1,  //  -15 0
	5,  //  -14 1
	10, //  -13 2
	20, //  -12 3
	30, //  -11 4
	50, //  -10 5
	70, //  -9  6
	80, //  -8  7
	85, //  -7  8
	90, //  -6  9
	92, //  -5  10
	94, //  -4  11
	96, //  -3  12
	98, //  -2  13
	100,    //  -1  14
	100,    //  0   15
	105,    //  1   16
	110,    //  2   17
	115,    //  3   18
	120,    //  4   19
	125,    //  5   20
	130,    //  6   21
	135,    //  7   22
	140,    //  8   23
	145,    //  9   24
	150,    //  10  25
	155,    //  11  26
	160,    //  12  27
	165,    //  13  28
	170,    //  14  29
	180,    //  15  30
};

const DWORD party_exp_distribute_table[PLAYER_EXP_TABLE_MAX + 1] =
{
	0,
	10,		10,		10,		10,		15,		15,		20,		25,		30,		40,		// 1 - 10
	50,		60,		80,		100,	120,	140,	160,	184,	210,	240,	// 11 - 20
	270,	300,	330,	360,	390,	420,	450,	480,	510,	550,	// 21 - 30
	600,	640,	700,	760,	820,	880,	940,	1000,	1100,	1180,	// 31 - 40
	1260,	1320,	1380,	1440,	1500,	1560,	1620,	1680,	1740,	1800,	// 41 - 50
	1860,	1920,	2000,	2100,	2200,	2300,	2450,	2600,	2750,	2900,	// 51 - 60
	3050,	3200,	3350,	3500,	3650,	3800,	3950,	4100,	4250,	4400,	// 61 - 70
	4600,	4800,	5000,	5200,	5400,	5600,	5800,	6000,	6200,	6400,	// 71 - 80
	6600,	6900,	7100,	7300,	7600,	7800,	8000,	8300,	8500,	8800,	// 81 - 90
	9000,	9000,	9000,	9000,	9000,	9000,	9000,	9000,	9000,	9000,	// 91 - 100
	10000,	10000,	10000,	10000,	10000,	10000,	10000,	10000,	10000,	10000,	// 101 - 110
	12000,	12000,	12000,	12000,	12000,	12000,	12000,	12000,	12000,	12000,	// 111 - 120
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 130
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 140
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 150
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 160
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 170
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 180
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 190
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 200
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 210
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 220
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 230
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 240
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 250
};

Coord aArroundCoords[ARROUND_COORD_MAX_NUM] =
{
	{	     0,	      0	    },
	{        0,      50     },
	{       35,     35      },
	{       50,     -0      },
	{       35,     -35     },
	{       0,      -50     },
	{       -35,    -35     },
	{       -50,    0       },
	{       -35,    35      },
	{       0,      100     },
	{       71,     71      },
	{       100,    -0      },
	{       71,     -71     },
	{       0,      -100    },
	{       -71,    -71     },
	{       -100,   0       },
	{       -71,    71      },
	{       0,      150     },
	{       106,    106     },
	{       150,    -0      },
	{       106,    -106    },
	{       0,      -150    },
	{       -106,   -106    },
	{       -150,   0       },
	{       -106,   106     },
	{       0,      200     },
	{       141,    141     },
	{       200,    -0      },
	{       141,    -141    },
	{       0,      -200    },
	{       -141,   -141    },
	{       -200,   0       },
	{       -141,   141     },
	{       0,      250     },
	{       177,    177     },
	{       250,    -0      },
	{       177,    -177    },
	{       0,      -250    },
	{       -177,   -177    },
	{       -250,   0       },
	{       -177,   177     },
	{       0,      300     },
	{       212,    212     },
	{       300,    -0      },
	{       212,    -212    },
	{       0,      -300    },
	{       -212,   -212    },
	{       -300,   0       },
	{       -212,   212     },
	{       0,      350     },
	{       247,    247     },
	{       350,    -0      },
	{       247,    -247    },
	{       0,      -350    },
	{       -247,   -247    },
	{       -350,   0       },
	{       -247,   247     },
	{       0,      400     },
	{       283,    283     },
	{       400,    -0      },
	{       283,    -283    },
	{       0,      -400    },
	{       -283,   -283    },
	{       -400,   0       },
	{       -283,   283     },
	{       0,      450     },
	{       318,    318     },
	{       450,    -0      },
	{       318,    -318    },
	{       0,      -450    },
	{       -318,   -318    },
	{       -450,   0       },
	{       -318,   318     },
	{       0,      500     },
	{       354,    354     },
	{       500,    -0      },
	{       354,    -354    },
	{       0,      -500    },
	{       -354,   -354    },
	{       -500,   0       },
	{       -354,   354     },
	{       0,      550     },
	{       389,    389     },
	{       550,    -0      },
	{       389,    -389    },
	{       0,      -550    },
	{       -389,   -389    },
	{       -550,   0       },
	{       -389,   389     },
	{       0,      600     },
	{       424,    424     },
	{       600,    -0      },
	{       424,    -424    },
	{       0,      -600    },
	{       -424,   -424    },
	{       -600,   0       },
	{       -424,   424     },
	{       0,      650     },
	{       460,    460     },
	{       650,    -0      },
	{       460,    -460    },
	{       0,      -650    },
	{       -460,   -460    },
	{       -650,   0       },
	{       -460,   460     },
	{       0,      700     },
	{       495,    495     },
	{       700,    -0      },
	{       495,    -495    },
	{       0,      -700    },
	{       -495,   -495    },
	{       -700,   0       },
	{       -495,   495     },
	{       0,      750     },
	{       530,    530     },
	{       750,    -0      },
	{       530,    -530    },
	{       0,      -750    },
	{       -530,   -530    },
	{       -750,   0       },
	{       -530,   530     },
	{       0,      800     },
	{       566,    566     },
	{       800,    -0      },
	{       566,    -566    },
	{       0,      -800    },
	{       -566,   -566    },
	{       -800,   0       },
	{       -566,   566     },
	{       0,      850     },
	{       601,    601     },
	{       850,    -0      },
	{       601,    -601    },
	{       0,      -850    },
	{       -601,   -601    },
	{       -850,   0       },
	{       -601,   601     },
	{       0,      900     },
	{       636,    636     },
	{       900,    -0      },
	{       636,    -636    },
	{       0,      -900    },
	{       -636,   -636    },
	{       -900,   0       },
	{       -636,   636     },
	{       0,      950     },
	{       672,    672     },
	{       950,    -0      },
	{       672,    -672    },
	{       0,      -950    },
	{       -672,   -672    },
	{       -950,   0       },
	{       -672,   672     },
	{       0,      1000    },
	{       707,    707     },
	{       1000,   -0      },
	{       707,    -707    },
	{       0,      -1000   },
	{       -707,   -707    },
	{       -1000,  0       },
	{       -707,   707     },
};

const DWORD guild_exp_table[GUILD_MAX_LEVEL + 1] =
{
	0,
	15000UL,
	45000UL,
	90000UL,
	160000UL,
	235000UL,
	325000UL,
	430000UL,
	550000UL,
	685000UL,
	835000UL,
	1000000UL,
	1500000UL,
	2100000UL,
	2800000UL,
	3600000UL,
	4500000UL,
	6500000UL,
	8000000UL,
	10000000UL,
	42000000UL,	//20级
	52000000UL,
	62000000UL,
	72000000UL,
	82000000UL,
	92000000UL,
	102000000UL,
	122000000UL,
	142000000UL,
	162000000UL,
	182000000UL	//30级
};

const DWORD guild_exp_table2[GUILD_MAX_LEVEL + 1] =
{
	0,
	6000UL,
	18000UL,
	36000UL,
	64000UL,
	94000UL,
	130000UL,
	172000UL,
	220000UL,
	274000UL,
	334000UL,
	400000UL,
	600000UL,
	840000UL,
	1120000UL,
	1440000UL,
	1800000UL,
	2600000UL,
	3200000UL,
	4000000UL,
	16800000UL,	//20级
	22000000UL,
	32000000UL,
	42000000UL,
	52000000UL,
	62000000UL,
	72000000UL,
	82000000UL,
	92000000UL,
	102000000UL,
	122000000UL	//30级
};

const int aiMobEnchantApplyIdx[MOB_ENCHANTS_MAX_NUM] =
{
	APPLY_CURSE_PCT,
	APPLY_SLOW_PCT,
	APPLY_POISON_PCT,
	APPLY_STUN_PCT,
	APPLY_CRITICAL_PCT,
	APPLY_PENETRATE_PCT,
#if defined(ENABLE_WOLFMAN_CHARACTER) && !defined(USE_MOB_BLEEDING_AS_POISON)
	APPLY_BLEEDING_PCT,
#endif
};

const int aiMobResistsApplyIdx[MOB_RESISTS_MAX_NUM] =
{
	APPLY_RESIST_SWORD,
	APPLY_RESIST_TWOHAND,
	APPLY_RESIST_DAGGER,
	APPLY_RESIST_BELL,
	APPLY_RESIST_FAN,
	APPLY_RESIST_BOW,
	APPLY_RESIST_FIRE,
	APPLY_RESIST_ELEC,
	APPLY_RESIST_MAGIC,
	APPLY_RESIST_WIND,
	APPLY_POISON_REDUCE,
#if defined(ENABLE_WOLFMAN_CHARACTER) && !defined(USE_MOB_CLAW_AS_DAGGER)
	APPLY_RESIST_CLAW,
#endif
#if defined(ENABLE_WOLFMAN_CHARACTER) && !defined(USE_MOB_BLEEDING_AS_POISON)
	APPLY_BLEEDING_REDUCE,
#endif
};

const int aiSocketPercentByQty[5][4] =
{
	{  0,  0,  0,  0 },
	{  3,  0,  0,  0 },
	{ 10,  1,  0,  0 },
	{ 15, 10,  1,  0 },
	{ 20, 15, 10,  1 }
};

const int aiWeaponSocketQty[WEAPON_NUM_TYPES] =
{
	3, // WEAPON_SWORD,
	3, // WEAPON_DAGGER,
	3, // WEAPON_BOW,
	3, // WEAPON_TWO_HANDED,
	3, // WEAPON_BELL,
	3, // WEAPON_FAN,
	0, // WEAPON_ARROW,
	0, // WEAPON_MOUNT_SPEAR
#ifdef ENABLE_WOLFMAN_CHARACTER
	3, // WEAPON_CLAW
#endif
};

const int aiArmorSocketQty[ARMOR_NUM_TYPES] =
{
	3, // ARMOR_BODY,
	1, // ARMOR_HEAD,
	1, // ARMOR_SHIELD,
	0, // ARMOR_WRIST,
	0, // ARMOR_FOOTS,
	0  // ARMOR_ACCESSORY
};

TItemAttrMap g_map_itemAttr;
TItemAttrRareMap g_map_itemRare;

const TApplyInfo aApplyInfo[MAX_APPLY_NUM] =
{
	{POINT_NONE				,},	//APPLY_NONE					= 0,
	{POINT_MAX_HP			,},	//APPLY_MAX_HP					= 1,
	{POINT_MAX_SP			,},	//APPLY_MAX_SP					= 2,
	{POINT_HT				,},	//APPLY_CON						= 3,
	{POINT_IQ				,},	//APPLY_INT						= 4,
	{POINT_ST				,},	//APPLY_STR						= 5,
	{POINT_DX				,},	//APPLY_DEX						= 6,
	{POINT_ATT_SPEED		,},	//APPLY_ATT_SPEED				= 7,
	{POINT_MOV_SPEED		,},	//APPLY_MOV_SPEED				= 8,
	{POINT_CASTING_SPEED	,},	//APPLY_CAST_SPEED				= 9,
	{POINT_HP_REGEN			,},	//APPLY_HP_REGEN				= 10,
	{POINT_SP_REGEN			,},	//APPLY_SP_REGEN				= 11,
	{POINT_POISON_PCT		,},	//APPLY_POISON_PCT				= 12,
	{POINT_STUN_PCT			,},	//APPLY_STUN_PCT				= 13,
	{POINT_SLOW_PCT			,},	//APPLY_SLOW_PCT				= 14,
	{POINT_CRITICAL_PCT		,},	//APPLY_CRITICAL_PCT			= 15,
	{POINT_PENETRATE_PCT	,},	//APPLY_PENETRATE_PCT			= 16,
	{POINT_ATTBONUS_HUMAN	,},	//APPLY_ATTBONUS_HUMAN			= 17,
	{POINT_ATTBONUS_ANIMAL	,},	//APPLY_ATTBONUS_ANIMAL			= 18,
	{POINT_ATTBONUS_ORC		,},	//APPLY_ATTBONUS_ORC			= 19,
	{POINT_ATTBONUS_MILGYO	,},	//APPLY_ATTBONUS_MILGYO			= 20,
	{POINT_ATTBONUS_UNDEAD	,},	//APPLY_ATTBONUS_UNDEAD			= 21,
	{POINT_ATTBONUS_DEVIL	,},	//APPLY_ATTBONUS_DEVIL			= 22,
	{POINT_STEAL_HP			,},	//APPLY_STEAL_HP				= 23,
	{POINT_STEAL_SP			,},	//APPLY_STEAL_SP				= 24,
	{POINT_MANA_BURN_PCT	,},	//APPLY_MANA_BURN_PCT			= 25,
	{POINT_DAMAGE_SP_RECOVER,},	//APPLY_DAMAGE_SP_RECOVER		= 26,
	{POINT_BLOCK			,},	//APPLY_BLOCK					= 27,
	{POINT_DODGE			,},	//APPLY_DODGE					= 28,
	{POINT_RESIST_SWORD		,},	//APPLY_RESIST_SWORD			= 29,
	{POINT_RESIST_TWOHAND	,},	//APPLY_RESIST_TWOHAND			= 30,
	{POINT_RESIST_DAGGER	,},	//APPLY_RESIST_DAGGER			= 31,
	{POINT_RESIST_BELL		,},	//APPLY_RESIST_BELL				= 32,
	{POINT_RESIST_FAN		,},	//APPLY_RESIST_FAN				= 33,
	{POINT_RESIST_BOW		,},	//APPLY_RESIST_BOW				= 34,
	{POINT_RESIST_FIRE		,},	//APPLY_RESIST_FIRE				= 35,
	{POINT_RESIST_ELEC		,},	//APPLY_RESIST_ELEC				= 36,
	{POINT_RESIST_MAGIC		,},	//APPLY_RESIST_MAGIC			= 37,
	{POINT_RESIST_WIND		,},	//APPLY_RESIST_WIND				= 38,
	{POINT_REFLECT_MELEE	,},	//APPLY_REFLECT_MELEE			= 39,
	{POINT_REFLECT_CURSE	,},	//APPLY_REFLECT_CURSE			= 40,
	{POINT_POISON_REDUCE	,},	//APPLY_POISON_REDUCE			= 41,
	{POINT_KILL_SP_RECOVER	,},	//APPLY_KILL_SP_RECOVER			= 42,
	{POINT_EXP_DOUBLE_BONUS	,},	//APPLY_EXP_DOUBLE_BONUS		= 43,
	{POINT_GOLD_DOUBLE_BONUS,},	//APPLY_GOLD_DOUBLE_BONUS		= 44,
	{POINT_ITEM_DROP_BONUS	,},	//APPLY_ITEM_DROP_BONUS			= 45,
	{POINT_POTION_BONUS		,},	//APPLY_POTION_BONUS			= 46,
	{POINT_KILL_HP_RECOVERY	,},	//APPLY_KILL_HP_RECOVER			= 47,
	{POINT_IMMUNE_STUN		,},	//APPLY_IMMUNE_STUN				= 48,
	{POINT_IMMUNE_SLOW		,},	//APPLY_IMMUNE_SLOW				= 49,
	{POINT_IMMUNE_FALL		,},	//APPLY_IMMUNE_FALL				= 50,
	{POINT_NONE				,},	//APPLY_SKILL					= 51,
	{POINT_BOW_DISTANCE		,},	//APPLY_BOW_DISTANCE			= 52,
	{POINT_ATT_GRADE_BONUS	,},	//APPLY_ATT_GRADE_BONUS			= 53,
	{POINT_DEF_GRADE_BONUS	,},	//APPLY_DEF_GRADE_BONUS			= 54,
	{POINT_MAGIC_ATT_GRADE_BONUS,},	//APPLY_MAGIC_ATT_GRADE		= 55,
	{POINT_MAGIC_DEF_GRADE_BONUS,},	//APPLY_MAGIC_DEF_GRADE		= 56,
	{POINT_CURSE_PCT		,},	//APPLY_CURSE_PCT				= 57,
	{POINT_MAX_STAMINA		,},	//APPLY_MAX_STAMINA				= 58,
	{POINT_ATTBONUS_WARRIOR	,},	//APPLY_ATTBONUS_WARRIOR		= 59,	对猛将强悍
	{POINT_ATTBONUS_ASSASSIN,},	//APPLY_ATTBONUS_ASSASSIN		= 60,	对刺客强悍
	{POINT_ATTBONUS_SURA	,},	//APPLY_ATTBONUS_SURA			= 61,	对修罗强悍
	{POINT_ATTBONUS_SHAMAN	,},	//APPLY_ATTBONUS_SHAMAN			= 62,	对法师强悍
	{POINT_ATTBONUS_MONSTER	,},	//APPLY_ATTBONUS_MONSTER		= 63,	对怪物强悍
	{POINT_MALL_ATTBONUS	,},	//APPLY_MALL_ATTBONUS			= 64,
	{POINT_MALL_DEFBONUS	,},	//APPLY_MALL_DEFBONUS			= 65,
	{POINT_MALL_EXPBONUS	,},	//APPLY_MALL_EXPBONUS			= 66,
	{POINT_MALL_ITEMBONUS	,},	//APPLY_MALL_ITEMBONUS			= 67,
	{POINT_MALL_GOLDBONUS	,},	//APPLY_MALL_GOLDBONUS			= 68,
	{POINT_MAX_HP_PCT		,},	//APPLY_MAX_HP_PCT				= 69,
	{POINT_MAX_SP_PCT		,},	//APPLY_MAX_SP_PCT				= 70,
	{POINT_SKILL_DAMAGE_BONUS,},//APPLY_SKILL_DAMAGE_BONUS		= 71,
	{POINT_NORMAL_HIT_DAMAGE_BONUS,},//APPLY_NORMAL_HIT_DAMAGE_BONUS	= 72,
	{POINT_SKILL_DEFEND_BONUS,},//APPLY_SKILL_DEFEND_BONUS		= 73,
	{POINT_NORMAL_HIT_DEFEND_BONUS,},//APPLY_NORMAL_HIT_DEFEND_BONUS	= 74,
	{POINT_NONE				,},	//APPLY_EXTRACT_HP_PCT			= 75,
	{POINT_RESIST_WARRIOR	,},	//APPLY_RESIST_WARRIOR			= 76,	减少猛将攻击
	{POINT_RESIST_ASSASSIN	,},	//APPLY_RESIST_ASSASSIN			= 77,	减少刺客攻击
	{POINT_RESIST_SURA		,},	//APPLY_RESIST_SURA				= 78,	减少修罗攻击
	{POINT_RESIST_SHAMAN	,},	//APPLY_RESIST_SHAMAN			= 79,	减少法师攻击
	{POINT_ENERGY			,},	//APPLY_ENERGY					= 80,	能源系统
	{POINT_DEF_GRADE		,},	//APPLY_DEF_GRADE				= 81,	防御力
	{POINT_COSTUME_ATTR_BONUS,},//APPLY_COSTUME_ATTR_BONUS		= 82,	时装属性
	{POINT_MAGIC_ATT_BONUS_PER,},//APPLY_MAGIC_ATTBONUS_PER		= 83,	魔法攻击力
	{POINT_MELEE_MAGIC_ATT_BONUS_PER,},//	APPLY_MELEE_MAGIC_ATTBONUS_PER	= 84,
	{POINT_RESIST_ICE		,},	//APPLY_RESIST_ICE				= 85,
	{POINT_RESIST_EARTH		,},	//APPLY_RESIST_EARTH			= 86,
	{POINT_RESIST_DARK		,},	//APPLY_RESIST_DARK				= 87,
	{POINT_RESIST_CRITICAL	,},	//APPLY_ANTI_CRITICAL_PCT		= 88,
	{POINT_RESIST_PENETRATE	,},	//APPLY_ANTI_PENETRATE_PCT		= 89,
#ifdef ENABLE_WOLFMAN_CHARACTER
	{POINT_BLEEDING_REDUCE	,},	//APPLY_BLEEDING_REDUCE			= 90,
	{POINT_BLEEDING_PCT		,},	//APPLY_BLEEDING_PCT			= 91,
	{POINT_ATTBONUS_WOLFMAN	,},	//APPLY_ATTBONUS_WOLFMAN		= 92,
	{POINT_RESIST_WOLFMAN	,},	//APPLY_RESIST_WOLFMAN			= 93,
	{POINT_RESIST_CLAW		,},	//APPLY_RESIST_CLAW				= 94,
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	{POINT_ACCEDRAIN_RATE	,},	//APPLY_ACCEDRAIN_RATE			= 95,
#endif
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
	{POINT_RESIST_MAGIC_REDUCTION,},//APPLY_RESIST_MAGIC_REDUCTION	= 96,	//降低对手魔法抵抗
#endif
#ifdef ENABLE_EXTRA_APPLY_BONUS
	{POINT_ATTBONUS_STONE,	},	//APPLY_ATTBONUS_STONE			= 97,
	{POINT_ATTBONUS_BOSS,	},	//APPLY_ATTBONUS_BOSS			= 98,
	{POINT_ATTBONUS_PVM_STR,},	//APPLY_ATTBONUS_PVM_STR		= 99,
	{POINT_ATTBONUS_PVM_AVG,},	//APPLY_ATTBONUS_PVM_AVG		= 100,
	{POINT_ATTBONUS_PVM_BERSERKER, },//APPLY_ATTBONUS_PVM_BERSERKER	= 101,
	{POINT_ATTBONUS_ELEMENT,},	//APPLY_ATTBONUS_ELEMENT			= 102,
	{POINT_DEFBONUS_PVM,	},	//APPLY_DEFBONUS_PVM				= 103,
	{POINT_DEFBONUS_ELEMENT,},	//APPLY_DEFBONUS_ELEMENT			= 104,
	{POINT_ATTBONUS_PVP,	},	//APPLY_ATTBONUS_PVP				= 105,
	{POINT_DEFBONUS_PVP,	},	//APPLY_DEFBONUS_PVP				= 106,

	{POINT_ATT_FIRE,		},	//APPLY_ATTBONUS_FIRE				= 107,
	{POINT_ATT_ICE,			},	//APPLY_ATTBONUS_ICE				= 108,
	{POINT_ATT_WIND,		},	//APPLY_ATTBONUS_WIND				= 109,
	{POINT_ATT_EARTH,		},	//APPLY_ATTBONUS_EARTH				= 110,
	{POINT_ATT_DARK,		},	//APPLY_ATTBONUS_DARK				= 111,
	{POINT_ATT_ELEC,		},	//APPLY_ATTBONUS_ELEC				= 112,
#endif
};

const int aiItemMagicAttributePercentHigh[ITEM_ATTRIBUTE_MAX_LEVEL] =
{
	//25, 25, 40, 8, 2,
	30, 40, 20, 8, 2
};

const int aiItemMagicAttributePercentLow[ITEM_ATTRIBUTE_MAX_LEVEL] =
{
	//45, 25, 20, 10, 0,
	50, 40, 10, 0, 0
};

// ADD_ITEM_ATTRIBUTE 追加前五属性成功率
const int aiItemAttributeAddPercent[ITEM_ATTRIBUTE_MAX_NUM] =
{
	100, 80, 60, 50, 30, 0, 0,
};
// END_OF_ADD_ITEM_ATTRIBUTE
// 添加皮肤坐骑等属性失败率检查
const int aiItemAttributeAddRareAttrItem[ITEM_ATTRIBUTE_MAX_NUM] =
{
	80, 70, 50, 30, 15, 0, 0,
};

//时装属性初始化3属性难度
#ifdef ENABLE_USE_COSTUME_ATTR
int aiCostumeAttributeAddPercent[COSTUME_ATTRIBUTE_MAX_NUM] =
{
	100, 30, 5,
};
//时装属性转换属性难度
int aiCostumeAttributeLevelPercent[ITEM_ATTRIBUTE_MAX_LEVEL] =
{
	5, 10, 60, 20, 10,
};
#endif

const int aiExpLossPercents[PLAYER_EXP_TABLE_MAX + 1] =
{
	0,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 4, // 1 - 10
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, // 11 - 20
	4, 4, 4, 4, 4, 4, 4, 3, 3, 3, // 21 - 30
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, // 31 - 40
	3, 3, 3, 3, 2, 2, 2, 2, 2, 2, // 41 - 50
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 51 - 60
	2, 2, 1, 1, 1, 1, 1, 1, 1, 1, // 61 - 70
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 71 - 80
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 81 - 90
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 91 - 100
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 101 - 110
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 111 - 120
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 130
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 140
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 150
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 160
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 170
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 180
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 190
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 200
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 210
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 220
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 230
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 240
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 250
};

const int aiSkillBookCountForLevelUp[10] =
{
	3, 3, 3, 3, 3, 4, 4, 5, 5, 6
};

// ADD_GRANDMASTER_SKILL
const int aiGrandMasterSkillBookCountForLevelUp[10] =
{
	3, 3, 5, 5, 7, 7, 10, 10, 10, 20,
};

const int aiGrandMasterSkillBookMinCount[10] =
{
	//	1, 1, 3, 5, 10, 15, 20, 30, 40, 50,
	//	3, 3, 5, 5, 10, 10, 15, 15, 20, 30
		1, 1, 1, 2,  2,  3,  3,  4,  5,  6
};

const int aiGrandMasterSkillBookMaxCount[10] =
{
	//	6, 15, 30, 45, 60, 80, 100, 120, 160, 200,
	//	6, 10, 15, 20, 30, 40, 50, 60, 70, 80
		5,  7,  9, 11, 13, 15, 20, 25, 30, 35
};
// END_OF_ADD_GRANDMASTER_SKILL

const int CHN_aiPartyBonusExpPercentByMemberCount[9] =
{
	0, 0, 12, 18, 26, 40, 53, 70, 100
};

// UPGRADE_PARTY_BONUS
const int KOR_aiPartyBonusExpPercentByMemberCount[9] =
{
	0,
	0,
	30, // 66% * 2 - 100
	60, // 53% * 3 - 100
	75, // 44% * 4 - 100
	90, // 38% * 5 - 100
	105, // 34% * 6 - 100
	110, // 30% * 7 - 100
	140, // 30% * 8 - 100
};

const int KOR_aiUniqueItemPartyBonusExpPercentByMemberCount[9] =
{
	0,
	0,
	15 * 2,
	14 * 3,
	13 * 4,
	12 * 5,
	11 * 6,
	10 * 7,
	10 * 8,
};
// END_OF_UPGRADE_PARTY_BONUS

const int* aiChainLightningCountBySkillLevel = NULL;

const int aiChainLightningCountBySkillLevel_euckr[SKILL_MAX_LEVEL + 1] =
{
	0,	// 0
	2,	// 1
	2,	// 2
	2,	// 3
	2,	// 4
	2,	// 5
	2,	// 6
	2,	// 7
	2,	// 8
	3,	// 9
	3,	// 10
	3,	// 11
	3,	// 12
	3,	// 13
	3,	// 14
	3,	// 15
	3,	// 16
	3,	// 17
	3,	// 18
	4,	// 19
	4,	// 20
	4,	// 21
	4,	// 22
	4,	// 23
	5,	// 24
	5,	// 25
	5,	// 26
	5,	// 27
	5,	// 28
	5,	// 29
	5,	// 30
	5,	// 31
	5,	// 32
	5,	// 33
	5,	// 34
	5,	// 35
	5,	// 36
	5,	// 37
	5,	// 38
	5,	// 39
	5,	// 40
};

const SStoneDropInfo aStoneDrop[STONE_INFO_MAX_NUM] =
{
#ifdef ENABLE_METIN_DROP_LV3_SYSTEM
	//mob	概率 {+0	+1	+2	+3	+4	}
	{8005,	60,	{0,40,	40,	20,	0}	},	//lv25
	{8006,	60,	{0,40,	40,	20,	0}	},	//lv30
	{8007,	60,	{0,40,	40,	20,	0}	},	//lv35
	{8008,	60,	{0,40,	40,	20,	0}	},	//lv40
	{8009,	60,	{0,40,	40,	20,	0}	},	//lv45
	{8010,	60,	{0,40,	40,	20,	0}	},	//lv50
	{8011,	60,	{0,40,	40,	20,	0}	},	//lv55
	{8012,	60,	{0,40,	40,	20,	0}	},	//lv60
	{8013,	60,	{0,40,	40,	20,	0}	},	//lv65
	{8014,	60,	{0,40,	40,	20,	0}	},	//lv70
	{8024,	60,	{0,40,	40,	20,	0}	},	//lv75
	{8025,	60,	{0,40,	40,	20,	0}	},	//lv80
	{8026,	60,	{0,40,	40,	20,	0}	},	//lv85
	{8027,	60,	{0,40,	40,	20,	0}	},	//lv90
	{8051,	100,{0,30,	46,	24,	0}	},	//魂魔石
	{8052,	100,{0,30,	46,	24,	0}	},	//鬼魔石
	{8053,	100,{0,30,	46,	24,	0}	},	//暴魔石
	{8054,	100,{0,30,	46,	24,	0}	},	//龙魔石
	{8055,	100,{0,30,	46,	24,	0}	},	//地魔石
	{8056,	100,{0,30,	46,	24,	0}	},	//芒魔石
	{8057,	100,{0,30,	46,	24,	0}	},	//火魔石
	{8058,	100,{0,30,	46,	24,	0}	},	//冰魔石
	{8059,	100,{0,30,	46,	24,	0}	},	//木魔石
	{8060,	100,{0,30,	46,	24,	0}	},	//万圣节南瓜
	{8061,	100,{0,30,	46,	24,	0}	},	//木魔石
	{8041,	100,{0,30,	46,	24,	0}	},	//复活节石
	{41005,	100,{0,30,	46,	24,	0}	},	//远古石
	{40132,	100,{0,30,	46,	24,	0}	},	//冰龙石
#else
	//mob	概率 {+0	+1	+2	+3	+4	}
	{8005,	60,	{0,40,	40,	18,	2}	},	//lv25
	{8006,	60,	{0,40,	40,	18,	2}	},	//lv30
	{8007,	60,	{0,40,	40,	18,	2}	},	//lv35
	{8008,	60,	{0,40,	40,	18,	2}	},	//lv40
	{8009,	60,	{0,40,	40,	18,	2}	},	//lv45
	{8010,	60,	{0,40,	40,	18,	2}	},	//lv50
	{8011,	60,	{0,40,	40,	18,	2}	},	//lv55
	{8012,	60,	{0,40,	40,	18,	2}	},	//lv60
	{8013,	60,	{0,40,	40,	17,	3}	},	//lv65
	{8014,	60,	{0,40,	40,	17,	3}	},	//lv70
	{8024,	60,	{0,40,	40,	17,	3}	},	//lv75
	{8025,	60,	{0,40,	40,	17,	3}	},	//lv80
	{8026,	60,	{0,40,	40,	17,	3}	},	//lv85
	{8027,	60,	{0,40,	40,	17,	3}	},	//lv90
	{8051,	100,{0,30,	46,	20,	4}	},	//魂魔石
	{8052,	100,{0,30,	46,	20,	4}	},	//鬼魔石
	{8053,	100,{0,30,	46,	20,	4}	},	//暴魔石
	{8054,	100,{0,30,	46,	20,	4}	},	//龙魔石
	{8055,	100,{0,30,	46,	20,	4}	},	//地魔石
	{8056,	100,{0,30,	46,	20,	4}	},	//芒魔石
	{8057,	100,{0,30,	46,	20,	4}	},	//火魔石
	{8058,	100,{0,30,	46,	20,	4}	},	//冰魔石
	{8059,	100,{0,30,	46,	20,	4}	},	//木魔石
	{8060,	100,{0,30,	46,	20,	4}	},	//万圣节南瓜
	{8061,	100,{0,30,	46,	20,	4}	},	//木魔石
	{8041,	100,{0,30,	46,	20,	4}	},	//复活节石
	{41005,	100,{0,30,	46,	20,	4}	},	//远古石
	{40132,	100,{0, 0,	45,	50,	5}	},	//冰龙石
#endif
};

const char* c_apszEmpireNames[EMPIRE_MAX_NUM] =
{
	"傈力惫",
	"脚荐惫",
	"玫炼惫",
	"柳畴惫"
};

const char* c_apszPrivNames[MAX_PRIV_NUM] =
{
	"",
	"酒捞袍捞 唱棵 犬伏",
	"捣捞 唱棵 犬伏",
	"捣 措冠捞 唱棵 犬伏",
	"版氰摹 硅啦",
};

//幻化能力等级
const int aiPolymorphPowerByLevel[SKILL_MAX_LEVEL + 1] =
{
	10,   // 1
	11,   // 2
	11,   // 3
	12,   // 4
	13,   // 5
	13,   // 6
	14,   // 7
	15,   // 8
	16,   // 9
	17,   // 10
	18,   // 11
	19,   // 12
	20,   // 13
	22,   // 14
	23,   // 15
	24,   // 16
	26,   // 17
	27,   // 18
	29,   // 19
	31,   // 20
	33,   // 21
	35,   // 22
	37,   // 23
	39,   // 24
	41,   // 25
	44,   // 26
	46,   // 27
	49,   // 28
	52,   // 29
	55,   // 30
	59,   // 31
	62,   // 32
	66,   // 33
	70,   // 34
	74,   // 35
	79,   // 36
	84,   // 37
	89,   // 38
	94,   // 39
	100,  // 40
};

TGuildWarInfo KOR_aGuildWarInfo[GUILD_WAR_TYPE_MAX_NUM] =
/*
   {
   long lMapIndex;
   int iWarPrice;
   int iWinnerPotionRewardPctToWinner;
   int iLoserPotionRewardPctToWinner;
   int iInitialScore;
   int iEndScore;
   };
 */
{
	{ 0,        0,      0,      0,      0,      0       },
	{ 110,      0,      100,    50,     0,      100     },
	{ 111,      0,      100,    50,     0,      10      },
};

// 钻石打孔几率
const int aiAccessorySocketAddPct[ITEM_ACCESSORY_SOCKET_MAX_NUM] =
{
#ifdef ENABLE_JEWELS_RENEWAL
	50, 50, 50
#else
	50, 50, 50
#endif
};
//添加该数值为矿石的%属性性能
const int aiAccessorySocketEffectivePct[ITEM_ACCESSORY_SOCKET_MAX_NUM + 1] =
{
#ifdef ENABLE_JEWELS_RENEWAL
	0, 10, 20, 40
#else
	0, 10, 20, 40
#endif	
};
//启用首饰宝石续订
#ifdef ENABLE_JEWELS_RENEWAL
const int aiAccessorySocketPermaEffectivePct[ITEM_ACCESSORY_SOCKET_MAX_NUM + 1] =
{
	0, 15, 25, 55
};
#endif
// 首饰矿石消失每档时间 24, 12, 6
const int aiAccessorySocketDegradeTime[ITEM_ACCESSORY_SOCKET_MAX_NUM + 1] =
{
#ifdef ENABLE_JEWELS_RENEWAL
	0, 3600 * 24, 3600 * 12, 3600 * 6
#else
	0, 3600 * 24, 3600 * 12, 3600 * 6
#endif
};
// 镶嵌矿石成功率
const int aiAccessorySocketPutPct[ITEM_ACCESSORY_SOCKET_MAX_NUM + 1] =
{
#ifdef ENABLE_JEWELS_RENEWAL
	70, 50, 50, 0 
#else
	90, 50, 50, 0
#endif
};
// END_OF_ACCESSORY_REFINE

#include "../../common/length.h"
// from import_item_proto.c
typedef struct SValueName
{
	const char* c_pszName;
	long		lValue;
} TValueName;

TValueName c_aApplyTypeNames[] =
{
	{ "STR",		APPLY_STR		},
	{ "DEX",		APPLY_DEX		},
	{ "CON",		APPLY_CON		},
	{ "INT",		APPLY_INT		},
	{ "MAX_HP",		APPLY_MAX_HP		},
	{ "MAX_SP",		APPLY_MAX_SP		},
	{ "MAX_STAMINA",	APPLY_MAX_STAMINA	},
	{ "POISON_REDUCE",	APPLY_POISON_REDUCE	},
	{ "EXP_DOUBLE_BONUS", APPLY_EXP_DOUBLE_BONUS },
	{ "GOLD_DOUBLE_BONUS", APPLY_GOLD_DOUBLE_BONUS },
	{ "ITEM_DROP_BONUS", APPLY_ITEM_DROP_BONUS	},
	{ "HP_REGEN",	APPLY_HP_REGEN		},
	{ "SP_REGEN",	APPLY_SP_REGEN		},
	{ "ATTACK_SPEED",	APPLY_ATT_SPEED		},
	{ "MOVE_SPEED",	APPLY_MOV_SPEED		},
	{ "CAST_SPEED",	APPLY_CAST_SPEED	},
	{ "ATT_BONUS",	APPLY_ATT_GRADE_BONUS	},
	{ "DEF_BONUS",	APPLY_DEF_GRADE_BONUS	},
	{ "MAGIC_ATT_GRADE",APPLY_MAGIC_ATT_GRADE	},
	{ "MAGIC_DEF_GRADE",APPLY_MAGIC_DEF_GRADE	},
	{ "SKILL",		APPLY_SKILL		},
	{ "ATTBONUS_ANIMAL",APPLY_ATTBONUS_ANIMAL	},
	{ "ATTBONUS_UNDEAD",APPLY_ATTBONUS_UNDEAD	},
	{ "ATTBONUS_DEVIL", APPLY_ATTBONUS_DEVIL	},
	{ "ATTBONUS_HUMAN", APPLY_ATTBONUS_HUMAN	},
	{ "ADD_BOW_DISTANCE",APPLY_BOW_DISTANCE	},
	{ "DODGE",		APPLY_DODGE		},
	{ "BLOCK",		APPLY_BLOCK		},
	{ "RESIST_SWORD",	APPLY_RESIST_SWORD	},
	{ "RESIST_TWOHAND",	APPLY_RESIST_TWOHAND	},
	{ "RESIST_DAGGER",	APPLY_RESIST_DAGGER    },
	{ "RESIST_BELL",	APPLY_RESIST_BELL	},
	{ "RESIST_FAN",	APPLY_RESIST_FAN	},
	{ "RESIST_BOW",	APPLY_RESIST_BOW	},
	{ "RESIST_FIRE",	APPLY_RESIST_FIRE	},
	{ "RESIST_ELEC",	APPLY_RESIST_ELEC	},
	{ "RESIST_MAGIC",	APPLY_RESIST_MAGIC	},
	{ "RESIST_WIND",	APPLY_RESIST_WIND	},
	{ "REFLECT_MELEE",	APPLY_REFLECT_MELEE },
	{ "REFLECT_CURSE",	APPLY_REFLECT_CURSE },
	{ "RESIST_ICE",		APPLY_RESIST_ICE	},
	{ "RESIST_EARTH",	APPLY_RESIST_EARTH	},
	{ "RESIST_DARK",	APPLY_RESIST_DARK	},
	{ "RESIST_CRITICAL",	APPLY_ANTI_CRITICAL_PCT	},
	{ "RESIST_PENETRATE",	APPLY_ANTI_PENETRATE_PCT	},
	{ "POISON",		APPLY_POISON_PCT	},
	{ "SLOW",		APPLY_SLOW_PCT		},
	{ "STUN",		APPLY_STUN_PCT		},
	{ "STEAL_HP",	APPLY_STEAL_HP		},
	{ "STEAL_SP",	APPLY_STEAL_SP		},
	{ "MANA_BURN_PCT",	APPLY_MANA_BURN_PCT	},
	{ "CRITICAL",	APPLY_CRITICAL_PCT	},
	{ "PENETRATE",	APPLY_PENETRATE_PCT	},
	{ "KILL_SP_RECOVER",APPLY_KILL_SP_RECOVER	},
	{ "KILL_HP_RECOVER",APPLY_KILL_HP_RECOVER	},
	{ "PENETRATE_PCT",	APPLY_PENETRATE_PCT	},
	{ "CRITICAL_PCT",	APPLY_CRITICAL_PCT	},
	{ "POISON_PCT",	APPLY_POISON_PCT	},
	{ "STUN_PCT",	APPLY_STUN_PCT		},
	{ "ATT_BONUS_TO_WARRIOR",	APPLY_ATTBONUS_WARRIOR  },
	{ "ATT_BONUS_TO_ASSASSIN",	APPLY_ATTBONUS_ASSASSIN },
	{ "ATT_BONUS_TO_SURA",	APPLY_ATTBONUS_SURA	    },
	{ "ATT_BONUS_TO_SHAMAN",	APPLY_ATTBONUS_SHAMAN   },
	{ "ATT_BONUS_TO_MONSTER",	APPLY_ATTBONUS_MONSTER  },
	{ "ATT_BONUS_TO_MOB",	APPLY_ATTBONUS_MONSTER  },
	{ "MALL_ATTBONUS",	APPLY_MALL_ATTBONUS	},
	{ "MALL_EXPBONUS",	APPLY_MALL_EXPBONUS	},
	{ "MALL_DEFBONUS",	APPLY_MALL_DEFBONUS	},
	{ "MALL_ITEMBONUS",	APPLY_MALL_ITEMBONUS	},
	{ "MALL_GOLDBONUS", APPLY_MALL_GOLDBONUS	},
	{ "MAX_HP_PCT",	APPLY_MAX_HP_PCT	},
	{ "MAX_SP_PCT",	APPLY_MAX_SP_PCT	},
	{ "SKILL_DAMAGE_BONUS",	APPLY_SKILL_DAMAGE_BONUS	},
	{ "NORMAL_HIT_DAMAGE_BONUS",APPLY_NORMAL_HIT_DAMAGE_BONUS	},
	{ "SKILL_DEFEND_BONUS",	APPLY_SKILL_DEFEND_BONUS	},
	{ "NORMAL_HIT_DEFEND_BONUS",APPLY_NORMAL_HIT_DEFEND_BONUS	},

	{ "RESIST_WARRIOR",		APPLY_RESIST_WARRIOR},
	{ "RESIST_ASSASSIN",	APPLY_RESIST_ASSASSIN},
	{ "RESIST_SURA",		APPLY_RESIST_SURA},
	{ "RESIST_SHAMAN",		APPLY_RESIST_SHAMAN},
	{ "INFINITE_AFFECT_DURATION", 0x1FFFFFFF	},
	{ "ENERGY", APPLY_ENERGY },
	{ "COSTUME_ATTR_BONUS", APPLY_COSTUME_ATTR_BONUS },
	{ "MAGIC_ATTBONUS_PER",	APPLY_MAGIC_ATTBONUS_PER	},
	{ "MELEE_MAGIC_ATTBONUS_PER",	APPLY_MELEE_MAGIC_ATTBONUS_PER	},

#ifdef ENABLE_WOLFMAN_CHARACTER
	{ "BLEEDING_REDUCE",APPLY_BLEEDING_REDUCE },
	{ "BLEEDING_PCT",APPLY_BLEEDING_PCT },
	{ "ATT_BONUS_TO_WOLFMAN",APPLY_ATTBONUS_WOLFMAN },
	{ "RESIST_WOLFMAN",APPLY_RESIST_WOLFMAN },
	{ "RESIST_CLAW",APPLY_RESIST_CLAW },
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	{ "ACCEDRAIN_RATE",APPLY_ACCEDRAIN_RATE },
#endif
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
	{ "RESIST_MAGIC_REDUCTION",APPLY_RESIST_MAGIC_REDUCTION },
#endif
#ifdef ENABLE_EXTRA_APPLY_BONUS
	{ "ATTBONUS_STONE",	APPLY_ATTBONUS_STONE },
	{ "ATTBONUS_BOSS",	APPLY_ATTBONUS_BOSS },
	{ "ATTBONUS_PVM_STR",	APPLY_ATTBONUS_PVM_STR },
	{ "ATTBONUS_PVM_AVG",	APPLY_ATTBONUS_PVM_AVG },
	{ "ATTBONUS_PVM_BERSERKER",	APPLY_ATTBONUS_PVM_BERSERKER },
	{ "ATTBONUS_ELEMENT",	APPLY_ATTBONUS_ELEMENT },
	{ "DEFBONUS_PVM",	APPLY_DEFBONUS_PVM },
	{ "DEFBONUS_ELEMENT",	APPLY_DEFBONUS_ELEMENT },
	{ "ATTBONUS_PVP",	APPLY_ATTBONUS_PVP },
	{ "DEFBONUS_PVP",	APPLY_DEFBONUS_PVP },

	{ "ATTBONUS_FIRE",		APPLY_ATTBONUS_FIRE },
	{ "ATTBONUS_ICE",		APPLY_ATTBONUS_ICE },
	{ "ATTBONUS_WIND",		APPLY_ATTBONUS_WIND },
	{ "ATTBONUS_EARTH",		APPLY_ATTBONUS_EARTH },
	{ "ATTBONUS_DARK",		APPLY_ATTBONUS_DARK },
	{ "ATTBONUS_ELEC",		APPLY_ATTBONUS_ELEC },

	{ "RESIST_FIRE",		APPLY_RESIST_FIRE },
	{ "RESIST_ICE",			APPLY_RESIST_ICE },
	{ "RESIST_WIND",		APPLY_RESIST_WIND },
	{ "RESIST_EARTH",		APPLY_RESIST_EARTH },
	{ "RESIST_DARK",		APPLY_RESIST_DARK },
	{ "RESIST_ELEC",		APPLY_RESIST_ELEC },
#endif
	{ "ATTBONUS_ORC",		APPLY_ATTBONUS_ORC },
	{ "ATTBONUS_MILGYO",	APPLY_ATTBONUS_MILGYO },
	{ "IMMUNE_STUN",	APPLY_IMMUNE_STUN },
	{ "IMMUNE_SLOW",	APPLY_IMMUNE_SLOW },
	{ NULL,		0			}
};
// from import_item_proto.c

long FN_get_apply_type(const char* apply_type_string)
{
	TValueName* value_name;
	for (value_name = c_aApplyTypeNames; value_name->c_pszName; ++value_name)
	{
		if (0 == strcasecmp(value_name->c_pszName, apply_type_string))
			return value_name->lValue;
	}
	return 0;
}

#ifdef ENABLE_PASSIVE_SKILLS
const int PassiveReadBook[50] =
{
	//正常
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

	//掌握
	1, 3, 6, 10, 15, 21, 28, 36, 45, 55,

	//完美大师
	1, 3, 6, 10, 15, 21, 28, 36, 45, 55,

	//完美大师
	1, 3, 6, 10, 15, 21, 28, 36, 45, 55
};
#endif
//S级技能修炼需要的修炼石数量
#ifdef ENABLE_SKILLS_LEVEL_OVER_P
const int MaxReadSoulStone[10] =
{
	4, 7, 10, 15, 20, 26, 27, 29, 31, 35
};
const int MinReadSoulStone[10] =
{
	2, 5, 8, 11, 16, 21, 24, 26, 28, 30
};
const int ReadSoulStone[10] =
{
	3, 6, 9, 13, 19, 23, 25, 27, 30, 32
};
#endif

//宠物技能属性表
#ifdef ENABLE_NEW_PET_SYSTEM
const DWORD Pet_Skill_Table[15][31] =
{
    { POINT_RESIST_WARRIOR,1,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,8,9,9,10,10,11,12 },		//减少猛将"1"
    { POINT_RESIST_SURA,1,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,8,9,9,10,10,11,12 },			//减少修罗"2"
    { POINT_RESIST_ASSASSIN,1,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,8,9,9,10,10,11,12 },		//减少刺客"3"
    { POINT_RESIST_SHAMAN,1,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,8,9,9,10,10,11,12 },		//减少法师"4"
    { POINT_MELEE_MAGIC_ATT_BONUS_PER,1,1,1,1,2,2,2,3,3,3,3,3,4,4,4,4,4,5,5,5,5,5,6,6,6,6,7,7,7,8 },//物理魔法攻击力"5
    { POINT_ATTBONUS_HUMAN,1,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,8,9,9,10,10,11,12 },		//对人形系追加"6"
    { POINT_PENETRATE_PCT,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,12,13,14,15,16,17,18,19,20 },//无视防御"7"
    { POINT_CRITICAL_PCT,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,12,13,14,15,16,17,18,19,20 },	//双倍破坏"8"
    { POINT_HIT_HP_RECOVERY,1,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,8,9,9,10,10,11,12 },		//将伤害转换生命值"9"
    { POINT_BLOCK,1,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,8,9,9,10,10,11,12 },				//彻底防御物理"10"
	{ POINT_ATTBONUS_MONSTER,1,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,8,9,9,10,10,11,12 },		//对怪物强悍"11"
	{ POINT_ATTBONUS_STONE,1,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,8,9,9,10,10,11,12 },		//对陨石追加伤害"12"
	{ POINT_ATTBONUS_BOSS,1,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,8,9,9,10,10,11,12 },		//对BOSS追加伤害"13"
	{ POINT_DEFBONUS_PVM,1,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,8,9,9,10,10,11,12 },			//怪兽防御"14"
	{ POINT_GOLD_DOUBLE_BONUS,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,12,13,14,15,16,17,18,19,20 },//金钱爆落"15"
};
//宠物类型
const DWORD Pet_Change_Table[8][2] =
{
	{ 1, 3 },
	{ 1, 4 },
	{ 1, 5 },
	{ 1, 6 },
	{ 1, 7 },
	{ 2, 7 },
	{ 3, 7 },
	{ 3, 8 },
};

//宠物属性类型
const BYTE Pet_Bonus_Type[3] =
{
	POINT_MAX_HP, POINT_ATT_GRADE_BONUS, POINT_DEF_GRADE_BONUS
};

//宠物皮肤属性值
const BYTE Pet_Skin_Bonus_Value[6] =
{
	0, 2, 5, 9, 14, 20
};
//宠物皮肤物品编码列表
const DWORD Pet_Skin_Item_Vnum_List[5] =
{
	55130, 55131, 55132, 55133, 55134
};

//宠物皮肤怪物编码
const DWORD Pet_Skin_Mob_Vnum_List[6] =
{
	34045, 34073, 34080, 34064, 34082, 34047
};
//宠物等级进化材料
const DWORD Pet_Evolution_Table[3][4] =
{
	//level - evolution - need item vnum - need item count
	{ 40,1, 55120, 10 },
	{ 60,2, 55121, 20 },
	{ 80,3, 55122, 30 }
};

NEW_PET_EVOLUTION_MAP mapEvolution;

void SetEvolutionMap()
{
	mapEvolution.clear();
	for (auto i : Pet_Evolution_Table)
	{
		mapEvolution.insert(std::make_pair(i[0], i[1]));
	}
}

bool EvolutionLevelCheck(BYTE level, BYTE evolution)
{
	NEW_PET_EVOLUTION_MAP::iterator it = mapEvolution.find(level);

	if (it != mapEvolution.end())
	{
		if (it->second != evolution)
			return false;
	}

	return true;
}
#endif

//超级属性列表
#ifdef ENABLE_6TH_7TH_ATTR
const WORD New67AttrTable[20][6 + ATTRIBUTE_67TH_MAX] =
{
	//Efsun tipi	1	2	3	4	5								武器	盔甲	手镯	鞋子	项链	头盔	盾牌	耳环	护符	手套
	{APPLY_MAX_HP,				500,  500,  500, 500, 500,			true,	true,	true,	true,	true,	true,	true,	true,	false,	true},
	{APPLY_MAX_SP,				50, 50, 50, 50, 50,					false,	false,	false,	false,	false,	false,	false,	false,	false,	false},
	{APPLY_CON,					5, 5, 5, 5, 5,						true,	true,	true,	true,	true,	true,	true,	true,	false,	true},
	{APPLY_INT,					5, 5, 5, 5, 5,						true,	true,	true,	true,	true,	true,	true,	true,	false,	true},
	{APPLY_STR,					5, 5, 5, 5, 5,						true,	true,	true,	true,	true,	true,	true,	true,	false,	true},
	{APPLY_DEX,					5, 5, 5, 5, 5,						true,	true,	true,	true,	true,	true,	true,	true,	false,	true},
	{APPLY_CRITICAL_PCT,		5, 5, 5, 5, 5,						true,	true,	true,	true,	true,	true,	true,	true,	false,	true},
	{APPLY_PENETRATE_PCT,		5, 5, 5, 5, 5,						true,	true,	true,	true,	true,	true,	true,	true,	false,	true},
	{APPLY_ATT_GRADE_BONUS,		50, 50, 50, 50, 50,					true,	true,	true,	true,	true,	true,	true,	true,	false,	true},
	{APPLY_ATTBONUS_MONSTER,	10, 10, 10, 10, 10,					true,	true,	true,	true,	true,	true,	true,	true,	false,	true},
#ifdef ENABLE_67ATTR_ATTBONUS
	{APPLY_ATTBONUS_WARRIOR,	10, 10, 10, 10, 10,					true,	true,	true,	true,	true,	true,	true,	true,	false,	false},
	{APPLY_ATTBONUS_ASSASSIN,	10, 10, 10, 10, 10,					true,	true,	true,	true,	true,	true,	true,	true,	false,	false},
	{APPLY_ATTBONUS_SURA,		10, 10, 10, 10, 10,					true,	true,	true,	true,	true,	true,	true,	true,	false,	false},
	{APPLY_ATTBONUS_SHAMAN,		10, 10, 10, 10, 10,					true,	true,	true,	true,	true,	true,	true,	true,	false,	false},
	{APPLY_RESIST_WARRIOR,		5, 5, 5, 5, 5,						false,	false,	false,	false,	false,	false,	false,	false,	false,	false},
	{APPLY_RESIST_ASSASSIN,		5, 5, 5, 5, 5,						false,	false,	false,	false,	false,	false,	false,	false,	false,	false},
	{APPLY_RESIST_SURA,			5, 5, 5, 5, 5,						false,	false,	false,	false,	false,	false,	false,	false,	false,	false},
	{APPLY_RESIST_SHAMAN,		5, 5, 5, 5, 5,						false,	false,	false,	false,	false,	false,	false,	false,	false,	false},
#else
	{APPLY_ATTBONUS_WARRIOR,	10, 10, 10, 10, 10,					false,	false,	false,	false,	false,	false,	false,	false,	false,	false},
	{APPLY_ATTBONUS_ASSASSIN,	10, 10, 10, 10, 10,					false,	false,	false,	false,	false,	false,	false,	false,	false,	false},
	{APPLY_ATTBONUS_SURA,		10, 10, 10, 10, 10,					false,	false,	false,	false,	false,	false,	false,	false,	false,	false},
	{APPLY_ATTBONUS_SHAMAN,		10, 10, 10, 10, 10,					false,	false,	false,	false,	false,	false,	false,	false,	false,	false},
	{APPLY_RESIST_WARRIOR,		5, 5, 5, 5, 5,						false,	false,	false,	false,	false,	false,	false,	false,	false,	false},
	{APPLY_RESIST_ASSASSIN,		5, 5, 5, 5, 5,						false,	false,	false,	false,	false,	false,	false,	false,	false,	false},
	{APPLY_RESIST_SURA,			5, 5, 5, 5, 5,						false,	false,	false,	false,	false,	false,	false,	false,	false,	false},
	{APPLY_RESIST_SHAMAN,		5, 5, 5, 5, 5,						false,	false,	false,	false,	false,	false,	false,	false,	false,	false},
#endif
	{APPLY_ATT_SPEED,			2, 2, 2, 2, 2,						false,	false,	false,	false,	false,	false,	false,	false,	false,	false},
	{APPLY_MOV_SPEED,			8, 8, 8, 8, 8,						false,	false,	false,	false,	false,	false,	false,	false,	false,	false}
};

map67Attr map_item67ThAttr;
void Set67AttrTable()
{
	sys_log(0, "Attr 6-7 Load");
	for (auto i : New67AttrTable)
	{
		T67AttrTable data;
		data.attrType = i[0];
		for (int y = 0; y < 5; y++)
		{
			data.attrValue[y] = i[1 + y];
		}

		for (int x = 0; x < ATTRIBUTE_67TH_MAX; x++)
		{
			if (i[6 + x])
			{
				map67Attr::iterator it = map_item67ThAttr.find(x);
				if (it == map_item67ThAttr.end())
				{
					std::vector<T67AttrTable> tmpVec;
					tmpVec.push_back(data);
					map_item67ThAttr.insert(std::make_pair(x, tmpVec));
				}
				else
				{
					it->second.push_back(data);
				}

				it = map_item67ThAttr.find(x);
				T67AttrTable logdata = it->second[it->second.size() - 1];
				sys_log(0, "attrType: %d AttrValue %d %d %d %d %d - Attr Type %d", logdata.attrType, logdata.attrValue[0], logdata.attrValue[1], logdata.attrValue[2], logdata.attrValue[3], logdata.attrValue[4], x);
			}
		}
	}
}
#endif

//战力基数 对应的属性获得的值 =最大战力
#ifdef ENABLE_POWER_RANKING
const std::pair<BYTE,BYTE> powerPointList[MAX_APPLY_NUM] =
{
	{POINT_NONE				,5,},	//APPLY_NONE					= 0,
	{POINT_MAX_HP			,1,},	//APPLY_MAX_HP					= 1,
	{POINT_MAX_SP			,1,},	//APPLY_MAX_SP					= 2,
	{POINT_HT				,5,},	//APPLY_CON						= 3,
	{POINT_IQ				,5,},	//APPLY_INT						= 4,
	{POINT_ST				,5,},	//APPLY_STR						= 5,
	{POINT_DX				,5,},	//APPLY_DEX						= 6,
	{POINT_ATT_SPEED		,5,},	//APPLY_ATT_SPEED				= 7,
	{POINT_MOV_SPEED		,5,},	//APPLY_MOV_SPEED				= 8,
	{POINT_CASTING_SPEED	,5,},	//APPLY_CAST_SPEED				= 9,
	{POINT_HP_REGEN			,5,},	//APPLY_HP_REGEN				= 10,
	{POINT_SP_REGEN			,5,},	//APPLY_SP_REGEN				= 11,
	{POINT_POISON_PCT		,5,},	//APPLY_POISON_PCT				= 12,
	{POINT_STUN_PCT			,5,},	//APPLY_STUN_PCT				= 13,
	{POINT_SLOW_PCT			,5,},	//APPLY_SLOW_PCT				= 14,
	{POINT_CRITICAL_PCT		,5,},	//APPLY_CRITICAL_PCT			= 15,
	{POINT_PENETRATE_PCT	,5,},	//APPLY_PENETRATE_PCT			= 16,
	{POINT_ATTBONUS_HUMAN	,5,},	//APPLY_ATTBONUS_HUMAN			= 17,
	{POINT_ATTBONUS_ANIMAL	,5,},	//APPLY_ATTBONUS_ANIMAL			= 18,
	{POINT_ATTBONUS_ORC		,5,},	//APPLY_ATTBONUS_ORC			= 19,
	{POINT_ATTBONUS_MILGYO	,5,},	//APPLY_ATTBONUS_MILGYO			= 20,
	{POINT_ATTBONUS_UNDEAD	,5,},	//APPLY_ATTBONUS_UNDEAD			= 21,
	{POINT_ATTBONUS_DEVIL	,5,},	//APPLY_ATTBONUS_DEVIL			= 22,
	{POINT_STEAL_HP			,5,},	//APPLY_STEAL_HP				= 23,
	{POINT_STEAL_SP			,5,},	//APPLY_STEAL_SP				= 24,
	{POINT_MANA_BURN_PCT	,5,},	//APPLY_MANA_BURN_PCT			= 25,
	{POINT_DAMAGE_SP_RECOVER,5,},	//APPLY_DAMAGE_SP_RECOVER		= 26,
	{POINT_BLOCK			,5,},	//APPLY_BLOCK					= 27,
	{POINT_DODGE			,5,},	//APPLY_DODGE					= 28,
	{POINT_RESIST_SWORD		,5,},	//APPLY_RESIST_SWORD			= 29,
	{POINT_RESIST_TWOHAND	,5,},	//APPLY_RESIST_TWOHAND			= 30,
	{POINT_RESIST_DAGGER	,5,},	//APPLY_RESIST_DAGGER			= 31,
	{POINT_RESIST_BELL		,5,},	//APPLY_RESIST_BELL				= 32,
	{POINT_RESIST_FAN		,5,},	//APPLY_RESIST_FAN				= 33,
	{POINT_RESIST_BOW		,5,},	//APPLY_RESIST_BOW				= 34,
	{POINT_RESIST_FIRE		,5,},	//APPLY_RESIST_FIRE				= 35,
	{POINT_RESIST_ELEC		,5,},	//APPLY_RESIST_ELEC				= 36,
	{POINT_RESIST_MAGIC		,5,},	//APPLY_RESIST_MAGIC			= 37,
	{POINT_RESIST_WIND		,5,},	//APPLY_RESIST_WIND				= 38,
	{POINT_REFLECT_MELEE	,5,},	//APPLY_REFLECT_MELEE			= 39,
	{POINT_REFLECT_CURSE	,5,},	//APPLY_REFLECT_CURSE			= 40,
	{POINT_POISON_REDUCE	,5,},	//APPLY_POISON_REDUCE			= 41,
	{POINT_KILL_SP_RECOVER	,5,},	//APPLY_KILL_SP_RECOVER			= 42,
	{POINT_EXP_DOUBLE_BONUS	,5,},	//APPLY_EXP_DOUBLE_BONUS		= 43,
	{POINT_GOLD_DOUBLE_BONUS,5,},	//APPLY_GOLD_DOUBLE_BONUS		= 44,
	{POINT_ITEM_DROP_BONUS	,5,},	//APPLY_ITEM_DROP_BONUS			= 45,
	{POINT_POTION_BONUS		,5,},	//APPLY_POTION_BONUS			= 46,
	{POINT_KILL_HP_RECOVERY	,5,},	//APPLY_KILL_HP_RECOVER			= 47,
	{POINT_IMMUNE_STUN		,5,},	//APPLY_IMMUNE_STUN				= 48,
	{POINT_IMMUNE_SLOW		,5,},	//APPLY_IMMUNE_SLOW				= 49,
	{POINT_IMMUNE_FALL		,5,},	//APPLY_IMMUNE_FALL				= 50,
	{POINT_NONE				,5,},	//APPLY_SKILL					= 51,
	{POINT_BOW_DISTANCE		,5,},	//APPLY_BOW_DISTANCE			= 52,
	{POINT_ATT_GRADE_BONUS	,5,},	//APPLY_ATT_GRADE_BONUS			= 53,
	{POINT_DEF_GRADE_BONUS	,5,},	//APPLY_DEF_GRADE_BONUS			= 54,
	{POINT_MAGIC_ATT_GRADE_BONUS,5,},	//APPLY_MAGIC_ATT_GRADE		= 55,
	{POINT_MAGIC_DEF_GRADE_BONUS,5,},	//APPLY_MAGIC_DEF_GRADE		= 56,
	{POINT_CURSE_PCT		,5,},	//APPLY_CURSE_PCT				= 57,
	{POINT_MAX_STAMINA		,5,},	//APPLY_MAX_STAMINA				= 58,
	{POINT_ATTBONUS_WARRIOR	,5,},	//APPLY_ATTBONUS_WARRIOR		= 59,
	{POINT_ATTBONUS_ASSASSIN,5,},	//APPLY_ATTBONUS_ASSASSIN		= 60,
	{POINT_ATTBONUS_SURA	,5,},	//APPLY_ATTBONUS_SURA			= 61,
	{POINT_ATTBONUS_SHAMAN	,5,},	//APPLY_ATTBONUS_SHAMAN			= 62,
	{POINT_ATTBONUS_MONSTER	,5,},	//APPLY_ATTBONUS_MONSTER		= 63,
	{POINT_MALL_ATTBONUS	,5,},	//APPLY_MALL_ATTBONUS			= 64,
	{POINT_MALL_DEFBONUS	,5,},	//APPLY_MALL_DEFBONUS			= 65,
	{POINT_MALL_EXPBONUS	,5,},	//APPLY_MALL_EXPBONUS			= 66,
	{POINT_MALL_ITEMBONUS	,5,},	//APPLY_MALL_ITEMBONUS			= 67,
	{POINT_MALL_GOLDBONUS	,5,},	//APPLY_MALL_GOLDBONUS			= 68,
	{POINT_MAX_HP_PCT		,5,},	//APPLY_MAX_HP_PCT				= 69,
	{POINT_MAX_SP_PCT		,5,},	//APPLY_MAX_SP_PCT				= 70,
	{POINT_SKILL_DAMAGE_BONUS,5,},//APPLY_SKILL_DAMAGE_BONUS		= 71,
	{POINT_NORMAL_HIT_DAMAGE_BONUS,5,},//APPLY_NORMAL_HIT_DAMAGE_BONUS	= 72,
	{POINT_SKILL_DEFEND_BONUS,5,},//APPLY_SKILL_DEFEND_BONUS		= 73,
	{POINT_NORMAL_HIT_DEFEND_BONUS,5,},//APPLY_NORMAL_HIT_DEFEND_BONUS	= 74,
	{POINT_NONE				,20,},	//APPLY_EXTRACT_HP_PCT			= 75,
	{POINT_RESIST_WARRIOR	,20,},	//APPLY_RESIST_WARRIOR			= 76,
	{POINT_RESIST_ASSASSIN	,20,},	//APPLY_RESIST_ASSASSIN			= 77,
	{POINT_RESIST_SURA		,20,},	//APPLY_RESIST_SURA				= 78,
	{POINT_RESIST_SHAMAN	,20,},	//APPLY_RESIST_SHAMAN			= 79,
	{POINT_ENERGY			,20,},	//APPLY_ENERGY					= 80,
	{POINT_DEF_GRADE		,20,},	//APPLY_DEF_GRADE				= 81,
	{POINT_COSTUME_ATTR_BONUS,20,},//APPLY_COSTUME_ATTR_BONUS		= 82,
	{POINT_MAGIC_ATT_BONUS_PER,20,},//APPLY_MAGIC_ATTBONUS_PER		= 83,
	{POINT_MELEE_MAGIC_ATT_BONUS_PER,20,},//	APPLY_MELEE_MAGIC_ATTBONUS_PER	= 84,
	{POINT_RESIST_ICE		,20,},	//APPLY_RESIST_ICE				= 85,
	{POINT_RESIST_EARTH		,20,},	//APPLY_RESIST_EARTH			= 86,
	{POINT_RESIST_DARK		,20,},	//APPLY_RESIST_DARK				= 87,
	{POINT_RESIST_CRITICAL	,20,},	//APPLY_ANTI_CRITICAL_PCT		= 88,
	{POINT_RESIST_PENETRATE	,20,},	//APPLY_ANTI_PENETRATE_PCT		= 89,
#ifdef ENABLE_WOLFMAN_CHARACTER
	{POINT_BLEEDING_REDUCE	,20,},	//APPLY_BLEEDING_REDUCE			= 90,
	{POINT_BLEEDING_PCT		,20,},	//APPLY_BLEEDING_PCT			= 91,
	{POINT_ATTBONUS_WOLFMAN	,20,},	//APPLY_ATTBONUS_WOLFMAN		= 92,
	{POINT_RESIST_WOLFMAN	,20,},	//APPLY_RESIST_WOLFMAN			= 93,
	{POINT_RESIST_CLAW		,20,},	//APPLY_RESIST_CLAW				= 94,
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	{POINT_ACCEDRAIN_RATE	,20,},	//APPLY_ACCEDRAIN_RATE			= 95,
#endif
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
	{POINT_RESIST_MAGIC_REDUCTION,15,},//APPLY_RESIST_MAGIC_REDUCTION	= 96,
#endif
#ifdef ENABLE_EXTRA_APPLY_BONUS
	{POINT_ATTBONUS_STONE	,20,},	//APPLY_ATTBONUS_STONE			= 97,
	{POINT_ATTBONUS_BOSS	,20,},	//APPLY_ATTBONUS_BOSS			= 98,
	{POINT_ATTBONUS_PVM_STR	,20,},	//APPLY_ATTBONUS_PVM_STR		= 99,
	{POINT_ATTBONUS_PVM_AVG	,20,},	//APPLY_ATTBONUS_PVM_AVG		= 100,
	{POINT_ATTBONUS_PVM_BERSERKER	,20, },//APPLY_ATTBONUS_PVM_BERSERKER	= 101,
	{POINT_ATTBONUS_ELEMENT,20,},	//APPLY_ATTBONUS_ELEMENT			= 102,
	{POINT_DEFBONUS_PVM	,20,},	//APPLY_DEFBONUS_PVM				= 1APPLY_MELEE_MAGIC_ATTBONUS_PER03,
	{POINT_DEFBONUS_ELEMENT,20,},	//APPLY_DEFBONUS_ELEMENT			= 104,
	{POINT_ATTBONUS_PVP,20,},	//APPLY_ATTBONUS_PVP				= 105,
	{POINT_DEFBONUS_PVP,20,},	//APPLY_DEFBONUS_PVP				= 106,

	{POINT_ATT_FIRE	,30,	},	//APPLY_ATTBONUS_FIRE				= 107,
	{POINT_ATT_ICE	,30,	},	//APPLY_ATTBONUS_ICE				= 108,
	{POINT_ATT_WIND	,30,	},	//APPLY_ATTBONUS_WIND				= 109,
	{POINT_ATT_EARTH,30,		},	//APPLY_ATTBONUS_EARTH				= 110,
	{POINT_ATT_DARK	,25,	},	//APPLY_ATTBONUS_DARK				= 111,
	{POINT_ATT_ELEC	,25,	},	//APPLY_ATTBONUS_ELEC				= 112,
#endif
};
#endif

//加入门派所有技能直接 M1带基本辅助P
#ifdef ENABLE_SKILL_SELECT_GROUP_M1__
const int skillSelectTable[4][2][20][2] = {
	//WARRIOR
	{
		{{1, 20},{2, 20},{3, 20},{4, 20},{5, 20},{6, 20},{130, 21},{131, 10},{122, 2},{137, 20},{138, 20},{139, 20},{121, 20},{124, 20},{125, 20},{129, 20},{208, 20},{180, 1},{181, 1},{182, 1}},
		{{16, 20},{17, 20},{18, 20},{19, 20},{20, 20},{21, 20},{130, 21},{131, 10},{122, 2},{137, 20},{138, 20},{139, 20},{121, 20},{124, 20},{125, 20},{129, 20},{208, 20},{180, 1},{181, 1},{182, 1}}
	},
	//ASSASSIN
	{
		{{31, 20},{32, 20},{33, 20},{34, 20},{35, 20},{36, 20},{130, 21},{131, 10},{122, 2},{137, 20},{138, 20},{139, 20},{121, 20},{124, 20},{125, 20},{129, 20},{208, 20},{180, 1},{181, 1},{182, 1}},
		{{46, 20},{47, 20},{48, 20},{49, 20},{50, 20},{51, 20},{130, 21},{131, 10},{122, 2},{137, 20},{138, 20},{139, 20},{121, 20},{124, 20},{125, 20},{129, 20},{208, 20},{180, 1},{181, 1},{182, 1}}
	},
	//SURA
	{
		{{61, 20},{62, 20},{63, 20},{64, 20},{65, 20},{66, 20},{130,21},{131, 10},{122, 2},{137, 20},{138, 20},{139, 20},{121, 20},{124, 20},{125, 20},{129, 20},{208, 20},{180, 1},{181, 1},{182, 1}},
		{{76, 20},{77, 20},{78, 20},{79, 20},{80, 20},{81, 20},{130, 21},{131, 10},{122, 2},{137, 20},{138, 20},{139, 20},{121, 20},{124, 20},{125, 20},{129, 20},{208, 20},{180, 1},{181, 1},{182, 1}}
	},
	//SHAMAN
	{
		{{91, 20},{92, 20},{93, 20},{94, 20},{95, 20},{96, 20},{130, 21},{131, 10},{122, 2},{137, 20},{138, 20},{139, 20},{121, 20},{124, 20},{125, 20},{129, 20},{208, 20},{180, 1},{181, 1},{182, 1}},
		{{106, 20},{107, 20},{108, 20},{109, 20},{110, 20},{111, 20},{130, 21},{131, 10},{122, 2},{137, 20},{138, 20},{139, 20},{121, 20},{124, 20},{125, 20},{129, 20},{208, 20},{180, 1},{181, 1},{182, 1}}
	}
};
#else
#ifdef ENABLE_SKILL_HORSE_SKILL_LV__
const int skillSelectTable[4][2][11][2] = {
	//WARRIOR
	{
		{{130, 11},{131, 10},{122, 2},{137, 20},{138, 20},{139, 20},{125, 20},{129, 20},{180, 1},{181, 1},{182, 1}},
		{{130, 11},{131, 10},{122, 2},{137, 20},{138, 20},{139, 20},{125, 20},{129, 20},{180, 1},{181, 1},{182, 1}}
	},
	//ASSASSIN
	{
		{{130, 11},{131, 10},{122, 2},{137, 20},{138, 20},{139, 20},{125, 20},{129, 20},{180, 1},{181, 1},{182, 1}},
		{{130, 11},{131, 10},{122, 2},{137, 20},{138, 20},{139, 20},{125, 20},{129, 20},{180, 1},{181, 1},{182, 1}}
	},
	//SURA
	{
		{{130, 11},{131, 10},{122, 2},{137, 20},{138, 20},{139, 20},{125, 20},{129, 20},{180, 1},{181, 1},{182, 1}},
		{{130, 11},{131, 10},{122, 2},{137, 20},{138, 20},{139, 20},{125, 20},{129, 20},{180, 1},{181, 1},{182, 1}}
	},
	//SHAMAN
	{
		{{130, 11},{131, 10},{122, 2},{137, 20},{138, 20},{139, 20},{125, 20},{129, 20},{180, 1},{181, 1},{182, 1}},
		{{130, 11},{131, 10},{122, 2},{137, 20},{138, 20},{139, 20},{125, 20},{129, 20},{180, 1},{181, 1},{182, 1}}
	}
};
#else
const int skillSelectTable[4][2][9][2] = {
	//WARRIOR
	{
		{{122, 2},{137, 20},{138, 20},{139, 20},{125, 20},{129, 20},{180, 1},{181, 1},{182, 1}},
		{{122, 2},{137, 20},{138, 20},{139, 20},{125, 20},{129, 20},{180, 1},{181, 1},{182, 1}}
	},
	//ASSASSIN
	{
		{{122, 2},{137, 20},{138, 20},{139, 20},{125, 20},{129, 20},{180, 1},{181, 1},{182, 1}},
		{{122, 2},{137, 20},{138, 20},{139, 20},{125, 20},{129, 20},{180, 1},{181, 1},{182, 1}}
	},
	//SURA
	{
		{{122, 2},{137, 20},{138, 20},{139, 20},{125, 20},{129, 20},{180, 1},{181, 1},{182, 1}},
		{{122, 2},{137, 20},{138, 20},{139, 20},{125, 20},{129, 20},{180, 1},{181, 1},{182, 1}}
	},
	//SHAMAN
	{
		{{122, 2},{137, 20},{138, 20},{139, 20},{125, 20},{129, 20},{180, 1},{181, 1},{182, 1}},
		{{122, 2},{137, 20},{138, 20},{139, 20},{125, 20},{129, 20},{180, 1},{181, 1},{182, 1}}
	}
};
#endif
#endif

//非宝箱物品
bool NotChestWindowItem(DWORD vnum)
{
	switch (vnum)
	{
	case 200087:
	case 200088:
	case 200089:
	case 200090:
	case 200091:
	case 200092:
		return true;
	default:
		break;
	}
	return false;
}

//活动物品列表 用于活动结束破坏物品时间 
#ifdef ENABLE_EVENT_MANAGER
std::set<DWORD> eventItemList;
void EventItemListTimeLoad()
{
	int itemList[]{ 200301, 200302, 200303, 200304, 200305, 79505, 79603, 79512, 79507, 79506, 79513, 79604 };

	for (auto& vnum : itemList)
	{
		eventItemList.insert(vnum);
	}
}

std::map<DWORD, int> eventItemEndTime;
void EventItemTimeUpdate()
{
	eventItemEndTime.clear();
	const TEventManagerData* eventPtr = nullptr;

	eventPtr = CHARACTER_MANAGER::Instance().CheckEventIsActive(LETTER_EVENT);
	if (eventPtr)
	{
		eventItemEndTime.insert(std::make_pair(200301, eventPtr->endTime));
		eventItemEndTime.insert(std::make_pair(200302, eventPtr->endTime));
		eventItemEndTime.insert(std::make_pair(200303, eventPtr->endTime));
		eventItemEndTime.insert(std::make_pair(200304, eventPtr->endTime));
		eventItemEndTime.insert(std::make_pair(200305, eventPtr->endTime));
	}

	eventPtr = CHARACTER_MANAGER::Instance().CheckEventIsActive(OKEY_CARD_EVENT);
	if (eventPtr)
	{
		eventItemEndTime.insert(std::make_pair(79505, eventPtr->endTime));
		eventItemEndTime.insert(std::make_pair(79506, eventPtr->endTime));
	}

	eventPtr = CHARACTER_MANAGER::Instance().CheckEventIsActive(CATCH_KING_EVENT);
	if (eventPtr)
	{
		eventItemEndTime.insert(std::make_pair(79603, eventPtr->endTime));
		eventItemEndTime.insert(std::make_pair(79604, eventPtr->endTime));
	}

	eventPtr = CHARACTER_MANAGER::Instance().CheckEventIsActive(BLACK_N_WHITE_EVENT);
	if (eventPtr)
	{
		eventItemEndTime.insert(std::make_pair(79512, eventPtr->endTime));
		eventItemEndTime.insert(std::make_pair(79513, eventPtr->endTime));
	}
}

int GetEventItemDestroyTime(DWORD vnum)
{
	const auto& it = eventItemEndTime.find(vnum);
	if (it != eventItemEndTime.end())
	{
		return it->second;
	}
	return time(0);
}

#endif

#ifdef ENABLE_BIOLOG_QUEST_SYSTEM
const DWORD BiyologSistemi[11][14] =
{
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 30006, 10, 0, 100, 30220, 50109, 8, 10, 0, 0, 0, 0, 0, 0 },	//30级
	{ 30047, 15, 0, 100, 30221, 50110, 7, 5, 0, 0, 0, 0, 0, 0 },	//40级
	{ 30015, 15, 0, 100, 30222, 50111, 54, 60, 0, 0, 0, 0, 0, 0 },	//50级
	{ 30050, 20, 0, 100, 30223, 50112, 53, 50, 0, 0, 0, 0, 0, 0 },	//60级
	{ 30165, 25, 0, 100, 30224, 50113, 8, 11, 7, 10, 0, 0, 0, 0 },	//75级
	{ 30166, 30, 0, 100, 30225, 50114, 7, 6, 64, 10, 0, 0, 0, 0 },	//80级
	{ 30167, 40, 0, 100, 30226, 50115, APPLY_RESIST_WARRIOR, 10, APPLY_RESIST_ASSASSIN, 10, APPLY_RESIST_SURA, 10, APPLY_RESIST_SHAMAN, 10 },		//85级
	{ 30168, 50, 30, 50, 30227, 50114, APPLY_ATTBONUS_WARRIOR, 8, APPLY_ATTBONUS_ASSASSIN, 8, APPLY_ATTBONUS_SURA, 8, APPLY_ATTBONUS_SHAMAN, 8 },	//90级
	{ 30251, 20, 60, 40, 0, 0, 1, 2000, 54, 120, 53, 50, 0, 0 },	//92级
	{ 30252, 20, 60, 60, 30228, 0, 1, 2100, 54, 140, 53, 60, 0, 0 }	//94级
};
#endif
#ifdef ENABLE_HIT_LIMITER
const WORD wHitLimiterBow = 550;
const WORD wHitLimiterPoly = 450;
const long lMaxDistanceDiff = 60;
const DWORD dwMinBlockedHitsToLog = 3500;
std::map<BYTE, WORD> m_hit_limiter =
{
	{ MAIN_RACE_WARRIOR_M, 260 },
	{ MAIN_RACE_ASSASSIN_M, 210 },
	{ MAIN_RACE_SURA_M, 260 },
	{ MAIN_RACE_SHAMAN_M, 260 },

	{ MAIN_RACE_WARRIOR_W, 260 },
	{ MAIN_RACE_ASSASSIN_W, 210 },
	{ MAIN_RACE_SURA_W, 260 },
	{ MAIN_RACE_SHAMAN_W, 260 },
	#ifdef ENABLE_WOLFMAN_CHARACTER
	{ MAIN_RACE_WOLFMAN_M, 250 },
	#endif
};
#endif