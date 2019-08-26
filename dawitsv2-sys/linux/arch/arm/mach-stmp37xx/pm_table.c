/* ================================================================ */
/*   37XX PM customization table                                    */
/*    - included from pm.c, don't add to Makefile					*/
/* ================================================================ */

/*  */
/* default vddd PVT */
static const unsigned pvt_pclk[][3] = {
	/*          PCLK                Vddd       vddd_bo */
	//{           100000          ,      975     , 900  },  
	{           100000          ,     1200     , 900  }, /*20081229. change to resolve problem that noise is occured in 24M clock mode */
	{           210000          ,     1350     , 1000 },  
	{           270000          ,     1450     , 1100 }, 
	{           320000          ,     1450     , 1250 }, 
	{           360000          ,     1450     , 1250 }, 
};
static const unsigned pvt_hclk[][3] = {
	/*          HCLK                Vddd       vddd_bo */
   //{            100000          ,      975    , 900  }, 
   {            100000          ,     1200    , 900  }, /*20081229, change to resolve problem that noise is occured in 24M clock mode */ 
   {            150000          ,     1350    , 1000 }, 
   {            200000          ,     1450    , 1100 }, 
   {            210000          ,     1450    , 1250 },  
};
static const unsigned pvt_emiclk[][3] = {
	/*          EMICLK                Vddd       vddd_bo */
	//{             48000         ,     975      , 900 }, 
	{             48000         ,    1200      , 900 }, /*20081229, change to resolve problem that noise is occured in 24M clock mode */
	{            100000         ,    1200      , 900 },  //1000 },
	{            133000         ,    1200      , 1000 }, //1100 },
};


/* PM table - OPM_LEVEL */
/* 
	!!!NOTE!!!
	- each clocking setting value will be modified to fit the best one for each avaiable clock source.
	- do not use hclk autoslow for usb operation
	- predefined vddd, vdda, vddio voltage level can be overrided
		e.g.,
		.vddd = 1300,
		.vddd_bo = 1000,
		.vddio = 2900,
		.vddio_bo = 2500,
		.vdda = 1600,
		.vdda_bo = 1300
*/

#if 1
static const pm_mode_t pm_table[] = {
	[SS_POWER_ON] = {
		/* 320/160/133/120 */
		.name			= "Power On",
		.pclk			= 320000,
		.pclk_intr_wait = true,
		.hclkdiv		= 2,
		.hclkslowenable = false,
		.hclkslowdiv	= SLOW_DIV_BY1,
		.emiclk 		= 133000,
		.gpmiclk		= 120000,
		.xclk			= 24000,
		.pixclk 		= 24000,
#if 0
		.vdda			= 2100,
#endif
	},
	[SS_IDLE] = {
		/* 4/??/6/ */
		.name			= "Idle",
		.pclk			= 4000,
		.pclk_intr_wait = true,
		.hclkdiv		= 31,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 6000,
		.gpmiclk		= 1,
		.xclk			= 375,
		.pixclk 		= 1,
#if 1
		.vddio			= 2800,
		.vddio_bo		= 2600,
		.vdda			= 1700,
		.vdda_bo		= 1400,
		/*20081229, add to reduce current consumption   */
	        .vddd                   = 1050,	
		.vddd_bo		= 900,
#endif
	},
	[SS_CLOCK_LEVEL1] = {
		/* 24/24/24/24 */
		.name			= "SS_CLOCK_24_24_24",
		.pclk			= 24000, 
		.pclk_intr_wait = true,
		.hclkdiv		= 1,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 24000,
		.gpmiclk		= 24000,
		.xclk			= 12000,
		.pixclk 		= 24000,
	},
	[SS_CLOCK_LEVEL2] = {
		/* 40/40/40/60 */
		.name			= "SS_CLOCK_40_40_40",
		.pclk			= 40000,
		.pclk_intr_wait = true,
		.hclkdiv		= 1,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 40000,
		.gpmiclk		= 60000,
		.xclk			= 12000,
		.pixclk 		= 24000,
	},
	[SS_CLOCK_LEVEL3] = {
		/* 60/60/60/60 */
		.name			= "SS_CLOCK_60_60_60",
		.pclk			= 60000,
		.pclk_intr_wait = true,
		.hclkdiv		= 1,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 60000,
		.gpmiclk		= 60000,
		.xclk			= 12000,
		.pixclk 		= 24000,
	},
	[SS_CLOCK_LEVEL4] = {
		/* 80/40/40/60 */
		.name			= "SS_CLOCK_80_40_40",
		.pclk			= 80000,
		.pclk_intr_wait = true,
		.hclkdiv		= 2,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 40000,
		.gpmiclk		= 60000,
		.xclk			= 12000,
		.pixclk 		= 24000,
	},
	[SS_CLOCK_LEVEL5] = {
		/* 80/80/80/60 */
		.name			= "SS_CLOCK_80_80_80",
		.pclk			= 80000,
		.pclk_intr_wait = true,
		.hclkdiv		= 1,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 80000,
		.gpmiclk		= 60000,
		.xclk			= 12000,
		.pixclk 		= 24000,
	},
	[SS_CLOCK_LEVEL6] = {
		/* 100/100/100/75 */
		.name			= "SS_CLOCK_100_100_100",
		.pclk			= 100000,
		.pclk_intr_wait = true,
		.hclkdiv		= 1,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 100000,
		.gpmiclk		= 75000,
		.xclk			= 12000,
		.pixclk 		= 24000,
	},
	[SS_CLOCK_LEVEL7] = {
		/* 120/60/60/60 */
		.name			= "SS_CLOCK_120_60_60",
		.pclk			= 120000,
		.pclk_intr_wait = true,
		.hclkdiv		= 2,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 60000,
		.gpmiclk		= 60000,
		.xclk			= 12000,
		.pixclk 		= 24000,
	},
	[SS_CLOCK_LEVEL8] = {
		/* 140/70/70/70 */
		.name			= "SS_CLOCK_140_70_70",
		.pclk			= 140000,
		.pclk_intr_wait = true,
		.hclkdiv		= 2,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 70000,
		.gpmiclk		= 70000,
		.xclk			= 12000,
		.pixclk 		= 24000,
	},
	[SS_CLOCK_LEVEL9] = {
		/* 160/80/80/80 */
		.name			= "SS_CLOCK_160_80_80",
		.pclk			= 160000,
		.pclk_intr_wait = true,
		.hclkdiv		= 2,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 80000,
		.gpmiclk		= 80000,
		.xclk			= 12000,
		.pixclk 		= 24000,
	},
	[SS_CLOCK_LEVEL10] = {
		/* 180/90/90/90 */
		.name			= "SS_CLOCK_180_90_90",
		.pclk			= 180000,
		.pclk_intr_wait = true,
		.hclkdiv		= 2,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 90000,
		.gpmiclk		= 90000,
		.xclk			= 12000,
		.pixclk 		= 24000,
	},
	[SS_CLOCK_LEVEL11] = {
		/* 200/100/100/100 */
		.name			= "SS_CLOCK_200_100_100",
		.pclk			= 200000,
		.pclk_intr_wait = true,
		.hclkdiv		= 2,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 100000,
		.gpmiclk		= 100000,
		.xclk			= 12000,
		.pixclk 		= 24000,
	},
	[SS_CLOCK_LEVEL12] = {
		/* 220/110/110/110 */
		.name			= "SS_CLOCK_220_110_110",
		.pclk			= 220000,
		.pclk_intr_wait = true,
		.hclkdiv		= 2,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 110000,
		.gpmiclk		= 110000,
		.xclk			= 12000,
		.pixclk 		= 24000,
	},
	[SS_CLOCK_LEVEL13] = {
		/* 240/120/120/120 */
		.name			= "SS_CLOCK_240_120_120",
		.pclk			= 240000,
		.pclk_intr_wait = true,
		.hclkdiv		= 2,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 120000,
		.gpmiclk		= 120000,
		.xclk			= 12000,
		.pixclk 		= 24000,
	},
	[SS_CLOCK_LEVEL14] = {
		/* 260/130/130/120 */
		.name			= "SS_CLOCK_260_130_130",
		.pclk			= 260000,
		.pclk_intr_wait = true,
		.hclkdiv		= 2,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 130000,
		.gpmiclk		= 120000,
		.xclk			= 12000,
		.pixclk 		= 24000,
	},
	[SS_CLOCK_LEVEL15] = {
		/* 280/140/133/120 */
		.name			= "SS_CLOCK_280_140_133",
		.pclk			= 280000,
		.pclk_intr_wait = true,
		.hclkdiv		= 2,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 133000,
		.gpmiclk		= 120000,
		.xclk			= 12000,
		.pixclk 		= 24000,
	},
	[SS_CLOCK_LEVEL16] = {
		/* 300/150/133/120 */
		.name			= "SS_CLOCK_300_150_133",
		.pclk			= 300000,
		.pclk_intr_wait = true,
		.hclkdiv		= 2,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 133000,
		.gpmiclk		= 120000,
		.xclk			= 12000,
		.pixclk 		= 24000,
	},
	[SS_CLOCK_STABLE] = {
#if 0
		/* 48/48/48/60 */
		.name			= "SS_CLOCK_STABLE",
		.pclk			= 40000,
		.pclk_intr_wait = true,
		.hclkdiv		= 1,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 40000,
		.gpmiclk		= 60000,
		.xclk			= 12000,
		.pixclk 		= 24000,
#else
		/* 80/80/80/60 */
		.name			= "SS_CLOCK_STABLE",
		.pclk			= 80000,
		.pclk_intr_wait = true,
		.hclkdiv		= 1,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 80000,
		.gpmiclk		= 60000,
		.xclk			= 12000,
		.pixclk 		= 24000,
#endif
	},
	[SS_CLOCK_LEVEL17] = {
		/* 24/24/133/24 */
		.name			= "SS_CLOCK_24_24_133",
		.pclk			= 24000, 
		.pclk_intr_wait = true,
		.hclkdiv		= 1,
		.hclkslowenable = true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk 		= 133000,
		.gpmiclk		= 24000,
		.xclk			= 12000,
		.pixclk 		= 24000,
	},
	[SS_MAX_CPU] = {
		/* 360/120/133/120 */
		.name			= "Max CPU",
		.pclk			= 360000,
		.pclk_intr_wait = true,
		.hclkdiv		= 3,
		.hclkslowenable = false,
		.hclkslowdiv	= SLOW_DIV_BY1,
		.emiclk 		= 133000,
		.gpmiclk		= 120000,
		.xclk			= 24000,
		.pixclk 		= 24000,
	},
	[SS_MAX_PERF] = {
		/* 360/120/133/120 */
		.name			= "Max Perf",
		.pclk			= 360000,
		.pclk_intr_wait = true,
		.hclkdiv		= 3,
		.hclkslowenable = false,
		.hclkslowdiv	= SLOW_DIV_BY1,
		.emiclk 		= 133000,
		.gpmiclk		= 120000,
		.xclk			= 24000,
		.pixclk 		= 24000,
	},
};
#else /*NOT USED  */
static const pm_mode_t pm_table[] = {
	[SS_POWER_ON] = {
		/* 320/160/133/120 */
		.name			= "Power On",
		.pclk			= 320000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 2,
		.hclkslowenable	= false,
		.hclkslowdiv	= SLOW_DIV_BY1,
		.emiclk			= 133000,
		.gpmiclk		= 120000,
		.xclk			= 24000,
		.pixclk			= 24000,
#if 0
		.vdda			= 2100,
#endif
	},
	[SS_IDLE] = {
		/* 24/??/6/120 */
		.name			= "Idle",
		.pclk			= 4000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 31,
		.hclkslowenable	= true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk			= 6000,
		.gpmiclk		= 1,
		.xclk			= 375,
		.pixclk			= 1,
#if 1
		.vddio			= 2800,
		.vddio_bo		= 2600,
		.vdda			= 1700,
		.vdda_bo		= 1400,
#endif
	},
	[SS_MP3] = {
		/* 24/24/24/24 */
		.name			= "MP3",
		.pclk			= 24000, 
		.pclk_intr_wait	= true,
		.hclkdiv		= 1,
		.hclkslowenable	= true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk			= 24000,
		.gpmiclk		= 24000,
		.xclk			= 12000,
		.pixclk			= 24000,
#if 0 //add dhsong
		.vddio			= 3300,
		.vddio_bo		= 3100,
#endif
		//.vdda			= 1750,
	},
	[SS_MP3_DNSE] = {
		/* 80/80/80/60 */
		.name			= "MP3 DNSE",
		.pclk			= 80000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 1,
		.hclkslowenable	= true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk			= 80000,
		.gpmiclk		= 60000,
		.xclk			= 12000,
		.pixclk			= 24000,
	},
#if 1
	[SS_MP3_DNSE_SPEED] = {
		/* 100/100/100/75 */
		.name			= "MP3 DNSE SPEED",
		.pclk			= 100000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 1,
		.hclkslowenable	= true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk			= 100000,
		.gpmiclk		= 75000,
		.xclk			= 12000,
		.pixclk			= 24000,
	},
#endif
	[SS_WMA] = {
		/* 40/40/40/60 */
		.name			= "WMA",
		.pclk			= 40000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 1,
		.hclkslowenable	= true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk			= 40000,
		.gpmiclk		= 60000,
		.xclk			= 12000,
		.pixclk			= 24000,
	},
	[SS_WMA_DNSE] = {
		/* 120/60/60/60 */
		.name			= "WMA DNSE",
		.pclk			= 120000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 2,
		.hclkslowenable	= true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk			= 60000,
		.gpmiclk		= 60000,
		.xclk			= 12000,
		.pixclk			= 24000,
	},
#if 1
	[SS_WMA_DNSE_SPEED] = {
		/* 140/70/70/70 */
		.name			= "WMA DNSE SPEED",
		.pclk			= 140000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 2,
		.hclkslowenable	= true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk			= 70000,
		.gpmiclk		= 70000,
		.xclk			= 12000,
		.pixclk			= 24000,
	},
#endif
	[SS_OGG] = {
		/* 80/40/40/60 */
		.name			= "OGG",
		.pclk			= 80000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 2,
		.hclkslowenable	= true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk			= 60000,
		.gpmiclk		= 60000,
		.xclk			= 12000,
		.pixclk			= 24000,
	},
	[SS_OGG_DNSE] = {
		/* 150/75/80/80 */
		.name			= "OGG DNSE",
		.pclk			= 150000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 2,
		.hclkslowenable	= true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk			= 80000,
		.gpmiclk		= 80000,
		.xclk			= 12000,
		.pixclk			= 24000,
	},
#if 1
	[SS_OGG_DNSE_SPEED] = {
		/* 160/80/80/80 */
		.name			= "OGG DNSE SPEED",
		.pclk			= 160000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 2,
		.hclkslowenable	= true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk			= 80000,
		.gpmiclk		= 80000,
		.xclk			= 12000,
		.pixclk			= 24000,
	},
#endif
	[SS_FM] = {
		/* 24/24/24/24 */
		.name			= "FM",
		.pclk			= 24000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 1,
		.hclkslowenable	= true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk			= 24000,
		.gpmiclk		= 24000,
		.xclk			= 12000,
		.pixclk			= 24000,
	},
	[SS_FM_DNSE] = {
		/* 48/48/48/48 */
		.name			= "FM DNSE",
		.pclk			= 48000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 1,
		.hclkslowenable	= true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk			= 48000,
		.gpmiclk		= 48000,
		.xclk			= 12000,
		.pixclk			= 24000,
	},
	[SS_RECORDING] = {
		/* 80/40/40/60 */
		.name			= "RECORDING",
		.pclk			= 80000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 2,
		.hclkslowenable	= true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk			= 60000,
		.gpmiclk		= 60000,
		.xclk			= 12000,
		.pixclk			= 24000,
	},
	[SS_AVI] = {
		/* 230/230/133/120 */
		.name			= "AVI",
		.pclk			= 230000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 1,
		.hclkslowenable	= true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk			= 133000,
		.gpmiclk		= 120000,
		.xclk			= 24000,
		.pixclk			= 24000,
	},
	[SS_AVI_DNSE] = {
		/* 250/250/133/120 */
		.name			= "AVI DNSE",
		.pclk			= 250000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 1,
		.hclkslowenable	= true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk			= 133000,
		.gpmiclk		= 120000,
		.xclk			= 24000,
		.pixclk			= 24000,
	},
	[SS_WMV] = {
		/* 230/230/133/120 */
		.name			= "WMV",
		.pclk			= 230000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 1,
		.hclkslowenable	= true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk			= 133000,
		.gpmiclk		= 120000,
		.xclk			= 24000,
		.pixclk			= 24000,
	},
	[SS_WMV_DNSE] = {
		/* 250/250/133/120 */
		.name			= "WMV DNSE",
		.pclk			= 250000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 1,
		.hclkslowenable	= true,
		.hclkslowdiv	= SLOW_DIV_BY32,
		.emiclk			= 133000,
		.gpmiclk		= 120000,
		.xclk			= 24000,
		.pixclk			= 24000,
	},
	[SS_MAX_CPU] = {
		/* 360/120/133/120 */
		.name			= "Max CPU",
		.pclk			= 360000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 3,
		.hclkslowenable	= false,
		.hclkslowdiv	= SLOW_DIV_BY1,
		.emiclk			= 133000,
		.gpmiclk		= 120000,
		.xclk			= 24000,
		.pixclk			= 24000,
	},
	[SS_MAX_PERF] = {
		/* 360/120/133/120 */
		.name			= "Max Perf",
		.pclk			= 360000,
		.pclk_intr_wait	= true,
		.hclkdiv		= 3,
		.hclkslowenable	= false,
		.hclkslowdiv	= SLOW_DIV_BY1,
		.emiclk			= 133000,
		.gpmiclk		= 120000,
		.xclk			= 24000,
		.pixclk			= 24000,
	},
};
#endif


