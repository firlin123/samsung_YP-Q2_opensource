
#include <linux/fs.h>
#include <linux/proc_fs.h>

#include "fm_global.h"
#include "fd_if.h"
#include "fd_physical.h"

#include "hwecc.h"

#define BCB_NCHIPS PDevCount

typedef struct _OptionConfigBlock {
	int version; 
	int b4GbMLC; 
} __attribute__ ((packed)) OptionConfigBlock_t;

typedef struct _BootConfigBlock {
	u32 m_u32Signature1;
	struct {
		u16 m_u16Major;
		u16 m_u16Minor;
		u16 m_u16Sub;
	} __attribute__ ((packed)) BCBVersion;
	u32 m_u32NANDBitmap;	//bit 0 == NAND 0, bit 1 == NAND 1, bit 2 = NAND 2, bit 3 = NAND3
	u32 m_u32Signature2;
	u32 m_u32Firmware_startingNAND;
	u32 m_u32Firmware_startingSector;
	u32 m_uSectorsInFirmware;
	u32 m_uFirmwareBootTag;
	struct {
		u16 m_u16Major;
		u16 m_u16Minor;
		u16 m_u16Sub;
	} __attribute__ ((packed)) FirmwareVersion;
	u32 Rsvd[10];
	u32 m_u32Signature3;
} __attribute__ ((packed)) BootConfigBlock_t;


#define BCB_SIGNATURE1    0x504d5453
#define BCB_SIGNATURE2    0x32424342
#define BCB_SIGNATURE3    0x41434143

#define BCB_VERSION_MAJOR   0x0001
#define BCB_VERSION_MINOR   0x0000
#define BCB_VERSION_SUB     0x0000

u8 crcvalues[256] = {
	0x00, 0x07, 0x0E, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36,
	    0x31,
	0x24, 0x23, 0x2a, 0x2d, 0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b,
	0x62, 0x65, 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
	0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf,
	0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd, 0x90, 0x97, 0x9E, 0x99,
	0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3,
	0xba, 0xbd, 0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
	0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea, 0xb7, 0xb0,
	0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86,
	0x93, 0x94, 0x9d, 0x9a, 0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c,
	0x35, 0x32, 0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
	0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42, 0x6f, 0x68,
	0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a, 0x89, 0x8e, 0x87, 0x80,
	0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa,
	0xa3, 0xa4, 0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec,
	0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4, 0x69, 0x6e,
	0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c, 0x51, 0x56, 0x5f, 0x58,
	0x4d, 0x4a, 0x43, 0x44, 0x19, 0x1e, 0x17, 0x10, 0x05, 0x02,
	0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
	0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71,
	0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63, 0x3e, 0x39, 0x30, 0x37,
	0x22, 0x25, 0x2c, 0x2b, 0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d,
	0x14, 0x13, 0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb,
	0x96, 0x91, 0x97, 0x9f, 0x8a, 0x8d, 0x84, 0x38, 0xde, 0xd9,
	0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef,
	0xfa, 0xfd, 0xf4, 0xf3
};


#define WRITE_VERIFY 0

void dump_spare(u8 *sbuf)
{
	int i; 
	printk("\nSpare Area\n");
	for ( i = 0; i < 64; i++ ) {
		if ( i % 16 == 0 ) printk("\n%3x: ", i);
		if ( sbuf[i] >= 'A' && sbuf[i] <= 'Z' )
			printk("%3c", sbuf[i]); 
		else 
			printk("%3x", sbuf[i]);
	}
	printk("\n");
}


u8 CRC(u8 * pRA)
{
	int i, wFFcnt = 0;
	u8 temp, crc, newindex;

	for (crc = 0, i = 0; i < 6; i++) {
		temp = pRA[i];
		newindex = crc ^ temp;
		crc = crcvalues[newindex];

		if (temp == 0xFF)
			wFFcnt++;
	}

	/////////////////////////////////////////////////////////////////////////////////
	//  If the RA is all FFs, it's probably erased, so the CRC byte will contain 0xFF.
	//  To match that, we force the computation to 0xFF, here.
	/////////////////////////////////////////////////////////////////////////////////

	if (wFFcnt == 6)
		crc = 0xFF;

	return (crc);
}

int erase_block(int chip, int block)
{
	int ret; 
	PFDEV* pdev; 
	pdev = PFD_GetPhysicalDevice(chip);

	printk("%s: chip(%d), block(%d)\n", 
		    __FUNCTION__, chip, block); 

	if ( !pdev ) return -1; 

	ret = pdev->DevOps.Erase(chip, block); 
	if ( ret ) return ret; 

	ret = pdev->DevOps.Sync(chip);

	return ret; 
}

int write_page(int chip, int block, int page, u8 *data, u8* spare )
{
	int ret; 
	PFDEV* pdev; 
	pdev = PFD_GetPhysicalDevice(chip);

	printk("%s: chip(%d), block(%d), page(%d)\n", 
		    __FUNCTION__, chip, block, page); 

	if ( !pdev ) return -1; 

#if (ECC_METHOD != NO_ECC)
	if ( block < 4 ) set_ecc_layout(ECC_STMP_LAYOUT); 
#endif
	ret = pdev->DevOps.WritePage(chip, block, page, 0, 4, data, spare, TRUE); 

#if (ECC_METHOD != NO_ECC)
	if ( block < 4 ) set_ecc_layout(ECC_RFS_LAYOUT); 
#endif
	if ( ret ) return ret; 
	ret = pdev->DevOps.Sync(chip); 


#if WRITE_VERIFY 
 {
	 u8 tmp_data[2048];
	 u8 tmp_spare[64]; 
	 ret = read_page(chip, block, page, tmp_data, tmp_spare); 
	 if ( ret ) return ret; 

	 if (memcmp(data, tmp_data, 2048) != 0) {
		 int i, cnt = 0; 
		 printk("ERROR: main area different\n"); 
		 printk("\npos:\twr\trd\n"); 
		 for ( i = 0; i < 2048; i++ ) {
			 if ( data[i] != tmp_data[i] ) {
				 printk("%3x:\t%3x\t%3x\n", i, data[i], tmp_data[i]); 
				 cnt++; 
			 }
		 }
		 printk("Total Error : %d\n", cnt); 
		 return (FM_WRITE_ERROR);
	 }

	 if (memcmp(spare, tmp_spare, 64) != 0) {
		 int i, cnt = 0; 
		 printk("ERROR: spare area different\n"); 
		 printk("\npos:\twr\trd\n"); 
		 for ( i = 0; i < 64; i++ ) {
			 if ( spare[i] != tmp_spare[i] ) {
				 printk("%3x:\t%3x\t%3x\n", i, spare[i], tmp_spare[i]); 
				 cnt ++;
			 }
		 }
		 printk("Total Error : %d\n", cnt); 

		 dump_spare(tmp_spare); // display readed spare buf contents...

		 return (FM_WRITE_ERROR);
	 }
 }
#endif 

	return ret; 
}

int read_page(int chip, int block, int page, u8 *data, u8* spare )
{
	int ret; 
	PFDEV* pdev; 
	pdev = PFD_GetPhysicalDevice(chip);
	if ( !pdev ) return -1; 

#if (ECC_METHOD != NO_ECC)
	if ( block < 4 ) set_ecc_layout(ECC_STMP_LAYOUT); 
#endif
	ret = pdev->DevOps.ReadPage(chip, block, page, 0, 4, data, spare); 

#if (ECC_METHOD != NO_ECC)
	if ( block < 4 ) set_ecc_layout(ECC_RFS_LAYOUT); 
#endif
	if ( ret ) return ret; 
	ret = pdev->DevOps.Sync(chip); 
	return ret; 
}


void write_BCB(int nsect)
{
	int i;
	BootConfigBlock_t *bcb;	

	u8 data[2048];
	u8 spare[64];

	OptionConfigBlock_t *opt;	
	u8 option[512]; 

	memset(data, 0x0, 2048); 
	memset(spare, 0x0, 64); 

	memset(option, 0xff, 512); 

	printk("%s: total nsect = %d\n", __FUNCTION__, nsect); 

	printk("%s: backup option page\n", __FUNCTION__ ); 
	PDev[0].DevOps.ReadPage(0, 0, 1, 0, 1, option, NULL);
	PDev[0].DevOps.Sync(0); 
	
	bcb = (BootConfigBlock_t *)data; 

	bcb->m_u32Signature1 = BCB_SIGNATURE1;

	bcb->BCBVersion.m_u16Major = BCB_VERSION_MAJOR;
	bcb->BCBVersion.m_u16Minor = BCB_VERSION_MINOR;
	bcb->BCBVersion.m_u16Sub = BCB_VERSION_SUB;

	for ( i = 0; i < BCB_NCHIPS; i++ ) { 
		bcb->m_u32NANDBitmap |= (1 << i); 
	}
	bcb->m_u32Signature2 = BCB_SIGNATURE2;
	bcb->m_u32Firmware_startingNAND = 1;
	bcb->m_u32Firmware_startingSector = 0; // block 1. (skip 64 page) 
	bcb->m_uSectorsInFirmware = nsect; 
	bcb->m_uFirmwareBootTag = BCB_SIGNATURE1;
	
	bcb->FirmwareVersion.m_u16Major = 0;
	bcb->FirmwareVersion.m_u16Minor = 0;
	bcb->FirmwareVersion.m_u16Sub = 0;

	for (i = 0; i < 10; i++) {
		bcb->Rsvd[i] = 0;
	}

	bcb->m_u32Signature3 = BCB_SIGNATURE3;

	spare[0] = 0xff;
	spare[2] = 'B';
	spare[3] = 'C';
	spare[4] = 'B';
	spare[5] = ' ';
	spare[1] = 0;
	spare[6] = CRC(spare);	

	printk("[BCB] Num chips = %d (bitmap=0x%x)\n", BCB_NCHIPS, 
		    bcb->m_u32NANDBitmap); 
	printk("[BCB] Starting NAND = %d\n", bcb->m_u32Firmware_startingNAND);
	printk("[BCB] Num sectors = %d\n", nsect); 
	printk("[BCB] Starting sectors = %d\n", bcb->m_u32Firmware_startingSector);
	if ( erase_block(0, 0) != FM_SUCCESS ) 
		panic("%s: block erase failed\n", __FUNCTION__); 
	if ( write_page(0, 0, 0, data, spare) ) 
		panic("%s: write bcb failed\n", __FUNCTION__);


	opt = (OptionConfigBlock_t *)option; 
	// todo. more option config 
	printk("re-write saved option page\n"); 
	printk("opt->version = %d\n", opt->version); 
	printk("opt->b4GbMLC = %d\n", opt->b4GbMLC); 

	PDev[0].DevOps.WritePage(0, 0, 1, 0, 1, option, NULL, TRUE);
	PDev[0].DevOps.Sync(0); 
}

int nand_writesb(u8 *image, int nsect)
{
	int offset; 
	int chip, block, page; 
	int blk_num;
	int size = nsect * 512; 

	u8 spare[64];

	write_BCB(nsect); 

	memset(spare, 0, 64); 

	if ( BCB_NCHIPS > 1 ) 
		{ chip = 1; block = 0; }
	else 
		{ chip = 0; block = 1; } 

	blk_num = offset = page = 0; 

	while ( size > 0 ) { 
		memset(spare, 0, 64); 

		spare[0] = 0x00;
		spare[2] = 'S';
		spare[3] = 'T';
		spare[4] = 'M';
		spare[5] = 'P';
		
		spare[1] = blk_num; 
		spare[6] = CRC(spare);

		if ( page == 0 ) 
			if (erase_block(chip, block)) 
				panic("Erase failed chip(%d), block(%d)\n", 
				      chip, block); 
		
		if ( write_page(chip, block, page, image + offset, spare) ) {
			printk("Write failed. chip(%d), block(%d), page(%d)\n",
			      chip, block, page); 
			return -1; 
		}

		size   -= 2048; 
		offset += 2048; 
		page ++;

		if ( page == PDev[0].DevSpec.PagesPerBlock ) { 
			page = 0; 
			chip ++; 
			if ( chip == BCB_NCHIPS ) { 
				chip = 0; 
				block ++; 
			}
			blk_num ++; 
		}
		
	}

	return 0; 
}







