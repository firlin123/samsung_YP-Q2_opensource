#ifndef SI4703_REG_H
#define SI4703_REG_H

/* DEVICEID */
typedef union {
	US U;
	struct {
		unsigned MFGID : 12;
		unsigned PN    : 4;
	} __attribute__ ((packed)) B;
} __attribute__ ((packed)) si4703_reg_deviceid;

/* CHIPID */
typedef union {
	US U;
	struct {
		unsigned FIRMWARE : 6;
		unsigned DEV      : 4;
		unsigned REV      : 6;
	} __attribute__ ((packed)) B;
} __attribute__ ((packed)) si4703_reg_chipid;

/* POWERCFG */
typedef union {
	US U;
	struct {
		unsigned ENABLE     : 1;
		unsigned R1         : 5;
		unsigned DISABLE    : 1;
		unsigned R2         : 1;
		unsigned SEEK       : 1;
		unsigned SEEKUP     : 1;
		unsigned SKMODE     : 1;
		unsigned RDSM       : 1;
		unsigned R3         : 1;
		unsigned MONO       : 1;
		unsigned DMUTE      : 1; 
		unsigned DSMUTE     : 1;
	} __attribute__ ((packed)) B;
} __attribute__ ((packed)) si4703_reg_powercfg;

/* CHANNEL */
typedef union {
	US U;
	struct {
		unsigned CHAN     : 10;
		unsigned R1       : 5; 
		unsigned TUNE     : 1;
	} __attribute__ ((packed)) B;
} __attribute__ ((packed)) si4703_reg_channel;

/* SYSCONFIG1 */
typedef union {
	US U;
	struct {
		unsigned GPIO1      : 2;
		unsigned GPIO2      : 2;
		unsigned GPIO3      : 2;
		unsigned BLNDADJ    : 2;
		unsigned R1         : 2;
		unsigned AGCD       : 1;
		unsigned DE         : 1;
		unsigned RDS        : 1;
		unsigned R2         : 1;
		unsigned STCIEN     : 1; 
		unsigned RDSIEN     : 1;
	} __attribute__ ((packed)) B;
} __attribute__ ((packed)) si4703_reg_sysconfig1;

/* SYSCONFIG2 */
typedef union {
	US U;
	struct {
		unsigned VOLUME     : 4;
		unsigned SPACE      : 2;
		unsigned BAND       : 2; 
		unsigned SEEKTH     : 8;
	} __attribute__ ((packed)) B;
} __attribute__ ((packed)) si4703_reg_sysconfig2;

/* SYSCONFIG3 */
typedef union {
	US U;
	struct {

		unsigned SKCNT      : 4;
		unsigned SKSNR      : 4;
		unsigned VOLEXT     : 1;
		unsigned RDSPRF     : 1;
		unsigned R1         : 2;
		unsigned SMUTEA     : 2; 
		unsigned SMUTER     : 2;
	} __attribute__ ((packed)) B;
} __attribute__ ((packed)) si4703_reg_sysconfig3;

/* TEST1 */
typedef union {
	US U;
	struct {
		unsigned R1         : 14;
		unsigned AHIZEN     : 1;
		unsigned XOSCEN     : 1;
	} __attribute__ ((packed)) B;
} __attribute__ ((packed)) si4703_reg_test1;

/* TEST2 */
typedef struct {
	US U;
} __attribute__ ((packed)) si4703_reg_test2;

/* BOOTCONFIG */
typedef struct {
	US U;
} __attribute__ ((packed)) si4703_reg_bootconfig;

/* STATUSRSSI */
typedef union {
	US U;
	struct {
		unsigned RSSI   : 8;
		unsigned ST     : 1;
		unsigned BLERA  : 2;
		unsigned RDSS   : 1;
		unsigned AFCRL  : 1;
		unsigned SF_BL  : 1;
		unsigned STC    : 1;
		unsigned RDSR   : 1;
	} __attribute__ ((packed)) B;
} __attribute__ ((packed)) si4703_reg_statusrssi;

/* READCHAN */
typedef union {
	US U;
	struct {
		unsigned READCHAN  : 10;
		unsigned BLERD     : 2;
		unsigned BLERC     : 2;
		unsigned BLERB     : 2;
	} __attribute__ ((packed)) B;
} __attribute__ ((packed)) si4703_reg_readchan;

/* RDSA */
typedef union {
	US U;
	struct {
		US RDSA;
	} __attribute__ ((packed)) B;
} __attribute__ ((packed)) si4703_reg_rdsa;

/* RDSB */
typedef union {
	US U;
	struct {
		US RDSB;
	} __attribute__ ((packed)) B;
} __attribute__ ((packed)) si4703_reg_rdsb;

/* RDSC */
typedef union {
	US U;
	struct {
		US RDSC;
	} __attribute__ ((packed)) B;
} __attribute__ ((packed)) si4703_reg_rdsc;

/* RDSD */
typedef union {
	US U;
	struct {
		US RDSD;
	} __attribute__ ((packed)) B;
} __attribute__ ((packed)) si4703_reg_rdsd;

struct SI4703_REGISTER {
	si4703_reg_deviceid	 DEVICEID;
	si4703_reg_chipid        CHIPID;
	si4703_reg_powercfg      POWERCFG;
	si4703_reg_channel       CHANNEL;
	si4703_reg_sysconfig1    SYSCONFIG1;
	si4703_reg_sysconfig2    SYSCONFIG2;
	si4703_reg_sysconfig3    SYSCONFIG3;
	si4703_reg_test1         TEST1;
	si4703_reg_test2         TEST2;
	si4703_reg_bootconfig    BOOTCONFIG;
	si4703_reg_statusrssi    STATUSRSSI;
	si4703_reg_readchan      READCHAN;
	si4703_reg_rdsa          RDSA;
	si4703_reg_rdsb          RDSB;
	si4703_reg_rdsc          RDSC;
	si4703_reg_rdsd          RDSD;
}__attribute__ ((packed));

static volatile struct SI4703_REGISTER si4703_reg;

#define HW_DEVICEID (*(volatile si4703_reg_deviceid *) &si4703_reg.DEVICEID)
#define HW_CHIPID (*(volatile si4703_reg_powercfg *) &si4703_reg.CHIPID)
#define HW_POWERCFG (*(volatile si4703_reg_powercfg *) &si4703_reg.POWERCFG)
#define HW_CHANNEL (*(volatile si4703_reg_channel *) &si4703_reg.CHANNEL)
#define HW_SYSCONFIG1 (*(volatile si4703_reg_sysconfig1 *) &si4703_reg.SYSCONFIG1)
#define HW_SYSCONFIG2 (*(volatile si4703_reg_sysconfig2 *) &si4703_reg.SYSCONFIG2)
#define HW_SYSCONFIG3 (*(volatile si4703_reg_sysconfig3 *) &si4703_reg.SYSCONFIG3)
#define HW_TEST1 (*(volatile si4703_reg_test1 *) &si4703_reg.TEST1)
#define HW_TEST2 (*(volatile si4703_reg_test2 *) &si4703_reg.TEST2)
#define HW_BOOTCONFIG (*(volatile si4703_reg_bootconfig *) &si4703_reg.BOOTCONFIG)
#define HW_STATUSRSSI (*(volatile si4703_reg_statusrssi *) &si4703_reg.STATUSRSSI)
#define HW_READCHAN (*(volatile si4703_reg_readchan *) &si4703_reg.READCHAN)
#define HW_RDSA (*(volatile si4703_reg_rdsa *) &si4703_reg.RDSA)
#define HW_RDSB (*(volatile si4703_reg_rdsb *) &si4703_reg.RDSB)
#define HW_RDSC (*(volatile si4703_reg_rdsc *) &si4703_reg.RDSC)
#define HW_RDSD (*(volatile si4703_reg_rdsd *) &si4703_reg.RDSD)

#if 0
static si4703_reg_deviceid     HW_DEVICEID;
static si4703_reg_chipid       HW_CHIPID;
static si4703_reg_powercfg     HW_POWERCFG;
static si4703_reg_channel      HW_CHANNEL;
static si4703_reg_sysconfig1   HW_SYSCONFIG1;
static si4703_reg_sysconfig2   HW_SYSCONFIG2;
static si4703_reg_sysconfig3   HW_SYSCONFIG3;
static si4703_reg_test1        HW_TEST1;
static si4703_reg_test2        HW_TEST2;
static si4703_reg_bootconfig   HW_BOOTCONFIG;
static si4703_reg_statusrssi   HW_STATUSRSSI;
static si4703_reg_readchan     HW_READCHAN;
static si4703_reg_rdsa         HW_RDSA;
static si4703_reg_rdsb         HW_RDSB;
static si4703_reg_rdsc         HW_RDSC;
#endif

#endif
