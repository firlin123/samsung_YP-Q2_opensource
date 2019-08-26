#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/miscdevice.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mman.h>
#include <linux/init.h>
#include <linux/rwsem.h>
#include <linux/uaccess.h>
#include <linux/rfs/fm_global.h>
#include <linux/ufd/fd_if.h> 
#include <linux/ufd/fd_physical.h> 

//#define DEBUG

#define NANDSB_MINOR	73
#define NANDSB_SIZE		(256*1024)

//#define USE_FAILSAFE_TIMING

//#define OTP_ROM_READ


#define BOOT_SECTOR_SIZE 2048

//! \name 37xx NCB fingerprint constants
//@{
#define NCB_FINGERPRINT1    0x504d5453    //!< 'STMP'
#define NCB_FINGERPRINT2    0x2042434E    //!< 'NCB<space>' - NAND Control Block
#define NCB_FINGERPRINT3    0x4E494252    //!< 'RBIN' - ROM Boot Image Block - N
//@}

//! \name 37xx LDLB fingerprint constants
//@{
#define LDLB_FINGERPRINT1   0x504d5453   //!< 'STMP'
#define LDLB_FINGERPRINT2   0x424C444C   //!< 'LDLB' - Logical Device Layout Block
#define LDLB_FINGERPRINT3   0x4C494252   //!< 'RBIL' - ROM Boot Image Block - L
//@}

//! \name 37xx LDLB version constants
//@{
#define LDLB_VERSION_MAJOR  0x0001
#define LDLB_VERSION_MINOR  0x0000
#define LDLB_VERSION_SUB    0x0000
//@}

//! \name NAND bitmap constants
//!
//! These bitmap constants are used for the bitmap of present NAND
//! devices that is located in the LDLB boot block.
//@{
#define NAND_1_BITMAP       1
#define NAND_2_BITMAP       2
#define NAND_3_BITMAP       4
#define NAND_4_BITMAP       8
//@}

typedef struct _NAND_Timing
{
    uint8_t m_u8DataSetup;      //!< The data setup time, in nanoseconds.
    uint8_t m_u8DataHold;       //!< The data hold time, in nanoseconds.
    uint8_t m_u8AddressSetup;   //!< The address setup time, in nanoseconds.
    uint8_t m_u8DSAMPLE_TIME;   //!< The data sample time, in nanoseconds.
} NAND_Timing1_struct_t;

//! Enables viewing the timing characteristics structure as a single 32-bit integer.

typedef union {	    		    // All fields in nanoseconds

    //! The 32-bit integer view of the timing characteristics structure.
    // By placing this word before the bitfield it allows structure copies to be done
    //  safely by assignment rather than by memcpy.

    uint32_t initializer;

    //! The timing characteristics structure.
    //! This structure holds the timing for the NAND.  This data is used by
    //! rom_nand_hal_GpmiSetNandTiming to setup the GPMI hardware registers.
    NAND_Timing1_struct_t NAND_Timing;

} NAND_Timing_t;

typedef struct _BootBlockStruct_t
{
    uint32_t    m_u32FingerPrint1;      //!< First fingerprint in first byte.
    union
    {
        struct
        {            
            NAND_Timing_t   m_NANDTiming;           //!< Optimum timing parameters for Tas, Tds, Tdh in nsec.
            uint32_t        m_u32DataPageSize;      //!< 2048 for 2K pages, 4096 for 4K pages.
            uint32_t        m_u32TotalPageSize;     //!< 2112 for 2K pages, 4314 for 4K pages.
            uint32_t        m_u32SectorsPerBlock;   //!< Number of 2K sections per block.
            uint32_t        m_u32SectorInPageMask;  //!< Mask for handling pages > 2K.
            uint32_t        m_u32SectorToPageShift; //!< Address shift for handling pages > 2K.
            uint32_t        m_u32NumberOfNANDs;     //!< Total Number of NANDs - not used by ROM.
        } NCB_Block1;
        struct
        {
            struct  
            {
                uint16_t    m_u16Major;             
                uint16_t    m_u16Minor;
                uint16_t    m_u16Sub;
                uint16_t    m_u16Reserved;
            } LDLB_Version;                     //!< LDLB version - not used by ROM.
            uint32_t    m_u32NANDBitmap;        //!< bit 0 == NAND 0, bit 1 == NAND 1, bit 2 = NAND 2, bit 3 = NAND3
        } LDLB_Block1;
        // This one just forces the spacing.
        uint32_t    m_Reserved1[10];
    };
    uint32_t    m_u32FingerPrint2;      //!< 2nd fingerprint at word 10.
    union
    {
        struct
        {
            uint32_t        m_u32NumRowBytes;   //!< Number of row bytes in read/write transactions.
            uint32_t        m_u32NumColumnBytes;//!< Number of row bytes in read/write transactions.
            uint32_t        m_u32TotalInternalDie;  //!< Number of separate chips in this NAND.
            uint32_t        m_u32InternalPlanesPerDie;  //!< Number of internal planes - treat like separate chips.
            uint32_t        m_u32CellType;      //!< MLC or SLC.
            uint32_t        m_u32ECCType;       //!< 4 symbol or 8 symbol ECC?
            uint32_t        m_u32Read1stCode;   //!< First value sent to initiate a NAND Read sequence.
            uint32_t        m_u32Read2ndCode;   //!< Second value sent to initiate a NAND Read sequence.
        } NCB_Block2;
        struct
        {
            uint32_t    m_u32Firmware_startingNAND;     //!< Firmware image starts on this NAND.
            uint32_t    m_u32Firmware_startingSector;   //!< Firmware image starts on this sector.
            uint32_t    m_u32Firmware_sectorStride;     //!< Amount to jump between sectors - unused in ROM.
            uint32_t    m_uSectorsInFirmware;           //!< Number of sectors in firmware image.
            uint32_t    m_u32Firmware_startingNAND2;    //!< Secondary FW Image starting NAND.
            uint32_t    m_u32Firmware_startingSector2;  //!< Secondary FW Image starting Sector.
            uint32_t    m_u32Firmware_sectorStride2;    //!< Secondary FW Image stride - unused in ROM.
            uint32_t    m_uSectorsInFirmware2;          //!< Number of sector in secondary FW image.
            struct  
            {
                uint16_t    m_u16Major;
                uint16_t    m_u16Minor;
                uint16_t    m_u16Sub;
                uint16_t    m_u16Reserved;
            } FirmwareVersion;
            uint32_t    m_u32DiscoveredBBTableSector;   //!< Location of Discovered Bad Block Table (DBBT).
            uint32_t    m_u32DiscoveredBBTableSector2;  //!< Location of backup DBBT 
        } LDLB_Block2;
        // This one just forces the spacing.
        uint32_t    m_Reserved2[19];    
    };

    uint16_t    m_u16Major;         //!< Major version of BootBlockStruct_t
    uint16_t    m_u16Minor;         //!< Minor version of BootBlockStruct_t

    uint32_t    m_u32FingerPrint3;    //!< 3rd fingerprint at word 30.

} __attribute__ ((packed)) BootBlockStruct_t;


static char *nandsb_base = 0;
static unsigned nandsb_size = 0;


static int erase_block (int chip, int block)
{
	int ret; 

	pr_debug("%s: chip(%d), block(%d)\n", __FUNCTION__, chip, block); 

	ret = PDev[chip].DevOps.Erase(chip, block); 
	if ( ret ) return ret;

	ret = PDev[chip].DevOps.Sync(chip);
	return ret; 
}

static int write_page (int chip, int block, int page, u8 *data, u8* spare)
{
	int ret; 
	FLASH_SPEC *spec = &PDev[0].DevSpec;

	pr_debug("%s: chip(%d), block(%d), page(%d)\n", __FUNCTION__, chip, block, page); 

	ret = PDev[chip].DevOps.WritePage(chip, block, page, 0, spec->SectorsPerPage, data, spare, TRUE); 

	if ( ret ) return ret; 
	ret = PDev[chip].DevOps.Sync(chip); 

	return ret; 
}

static int write_ncb (FLASH_SPEC *spec, int chip, int block)
{
	BootBlockStruct_t	 *ncb;
	unsigned pageToSector;
	unsigned pageToSectorShift;

	int bDataSize       = spec->DataSize; 
	int bPageSize       = spec->PageSize; 
	int bSectorsPerBlock= spec->BlockSize/BOOT_SECTOR_SIZE;

	u8 data[4096];
	u8 spare[128];

	memset(data, 0, sizeof(data));
	memset(spare, 0, sizeof(spare)); 

	ncb = (BootBlockStruct_t *)data;

	/* adjust page & data size */
	if (spec->SpareSize <= 128) {
		bDataSize = 2048;
		bPageSize = 2112;
	}

	// Compute the multiplier to convert from natural NAND pages to 2K sectors.
	pageToSector = bDataSize / BOOT_SECTOR_SIZE;
	// Convert the multiplier to a shift.
	pageToSectorShift = 0;
	while ((1 << pageToSectorShift) < pageToSector) {
		pageToSectorShift++;
	}

	ncb->m_u32FingerPrint1 = NCB_FINGERPRINT1;
	ncb->m_u32FingerPrint2 = NCB_FINGERPRINT2;
	ncb->m_u32FingerPrint3 = NCB_FINGERPRINT3;

#ifdef USE_FAILSAFE_TIMING
	ncb->NCB_Block1.m_NANDTiming.NAND_Timing.m_u8DataSetup = 0x64;
	ncb->NCB_Block1.m_NANDTiming.NAND_Timing.m_u8DataHold = 0x50;
	ncb->NCB_Block1.m_NANDTiming.NAND_Timing.m_u8AddressSetup = 0x78;
	ncb->NCB_Block1.m_NANDTiming.NAND_Timing.m_u8DSAMPLE_TIME = 0x0a;
#else
	ncb->NCB_Block1.m_NANDTiming.NAND_Timing.m_u8DataSetup = 0x0f;
	ncb->NCB_Block1.m_NANDTiming.NAND_Timing.m_u8DataHold = 0x0f;
	ncb->NCB_Block1.m_NANDTiming.NAND_Timing.m_u8AddressSetup = 0x19;
	ncb->NCB_Block1.m_NANDTiming.NAND_Timing.m_u8DSAMPLE_TIME = 0x06;
#endif

	ncb->NCB_Block1.m_u32DataPageSize = bDataSize;
	ncb->NCB_Block1.m_u32TotalPageSize = bPageSize;
	ncb->NCB_Block1.m_u32SectorsPerBlock = bSectorsPerBlock;
	ncb->NCB_Block1.m_u32SectorInPageMask = pageToSector - 1;
	ncb->NCB_Block1.m_u32SectorToPageShift = pageToSectorShift;

	ncb->NCB_Block2.m_u32NumRowBytes = 3;
	ncb->NCB_Block2.m_u32NumColumnBytes = 2;

	ncb->NCB_Block2.m_u32Read1stCode = 0x00;
	ncb->NCB_Block2.m_u32Read2ndCode = 0x30;

#if 0
	// NOT used in Boot
	ncb->NCB_Block1.m_u32NumberOfNANDs
	ncb->NCB_Block2.m_u32TotalInternalDie
	ncb->NCB_Block2.m_u32InternalPlanesPerDie
	ncb->NCB_Block2.m_u32CellType
	ncb->NCB_Block2.m_u32ECCType
#endif

	if (erase_block(chip, block) != FM_SUCCESS) {
		pr_err("%s: erase failed\n", __FUNCTION__); 
		return 1;
	}

	if (write_page(chip, block, 0, data, spare) != FM_SUCCESS) {
		pr_err("Write failed. chip(%d), block(%d), page(%d)\n", chip, block, 0); 
		return 1;
	}
	return 0;
}

static int write_ldlb (FLASH_SPEC *spec, int chip, int block, int firmware_blk, int firmware_size)
{
	BootBlockStruct_t *ldlb;
	u8 data[4096];
	u8 spare[128];

	memset(data, 0, sizeof(data));
	memset(spare, 0, sizeof(spare)); 

	ldlb = (BootBlockStruct_t *)data;

    ldlb->m_u32FingerPrint1 = LDLB_FINGERPRINT1;
    ldlb->m_u32FingerPrint2 = LDLB_FINGERPRINT2;
    ldlb->m_u32FingerPrint3 = LDLB_FINGERPRINT3;

    ldlb->LDLB_Block1.LDLB_Version.m_u16Major  = LDLB_VERSION_MAJOR;
    ldlb->LDLB_Block1.LDLB_Version.m_u16Minor  = LDLB_VERSION_MINOR;
    ldlb->LDLB_Block1.LDLB_Version.m_u16Sub    = LDLB_VERSION_SUB;
	ldlb->LDLB_Block1.LDLB_Version.m_u16Reserved = 0;

	ldlb->LDLB_Block2.FirmwareVersion.m_u16Major  = LDLB_VERSION_MAJOR;
	ldlb->LDLB_Block2.FirmwareVersion.m_u16Minor  = LDLB_VERSION_MINOR;
	ldlb->LDLB_Block2.FirmwareVersion.m_u16Sub    = LDLB_VERSION_SUB;
	ldlb->LDLB_Block2.FirmwareVersion.m_u16Reserved= 0;
    
    // The DBBT starting sector offset is in natural NAND pages, not 2K sectors.
	/* !!!NOTE!!! we are not using BBT */
    ldlb->LDLB_Block2.m_u32DiscoveredBBTableSector = 0x100;
	ldlb->LDLB_Block2.m_u32DiscoveredBBTableSector2 = 0x100;

	// Firmware starting sector offset is in 2K sectors, not natural pages.
    // Thus, page 2 of 4K page NAND will have a sector offset of 4 because
    // there are 4 2K pages before it.
    ldlb->LDLB_Block2.m_u32Firmware_sectorStride = 0;
    ldlb->LDLB_Block2.m_u32Firmware_startingNAND = 0;
    ldlb->LDLB_Block2.m_u32Firmware_startingSector = firmware_blk*spec->PagesPerBlock/spec->NumDiesPerCE;

    ldlb->LDLB_Block2.m_u32Firmware_sectorStride2 = 0;
    ldlb->LDLB_Block2.m_u32Firmware_startingNAND2 = 0;
    ldlb->LDLB_Block2.m_u32Firmware_startingSector2 = firmware_blk*spec->PagesPerBlock/spec->NumDiesPerCE;

    // The firmware sector count is also in 2K pages.
    ldlb->LDLB_Block2.m_uSectorsInFirmware = firmware_size / BOOT_SECTOR_SIZE;
    ldlb->LDLB_Block2.m_uSectorsInFirmware2 = firmware_size / BOOT_SECTOR_SIZE;

	if (erase_block(chip, block) != FM_SUCCESS) {
		pr_err("%s: erase failed\n", __FUNCTION__); 
		return 1;
	}

	if (write_page(chip, block, 0, data, spare) != FM_SUCCESS) {
		pr_err("Write failed. chip(%d), block(%d), page(%d)\n", chip, block, 0); 
		return 1;
	}
	return 0;
}

#ifdef OTP_ROM_READ
int hw_otp_read_searcharea (void)
{
	static const int table[] = {
		1, 2, 4, 8, 16, 32, 64, 128, 256
	};
	int rom1, searchcount, searcharea;

	/* 0. program HCLK < 200 MHz */
	/* 1. check if error, busy bits are clear */
	HW_OCOTP_CTRL_CLR(BM_OCOTP_CTRL_ERROR | BM_OCOTP_CTRL_BUSY);
	/* 2. set HW_OCOTP_CTRL_RD_BANK_OPEN */
	HW_OCOTP_CTRL_SET(BM_OCOTP_CTRL_RD_BANK_OPEN);
	/* 3. poll HW_OCOTP_CTRL_BUSY clear */
	while ((HW_OCOTP_CTRL_RD() & BM_OCOTP_CTRL_BUSY));

	rom1 = HW_OCOTP_ROMn_RD(1);
	/* 5. clear HW_OCOTP_CTRL_RD_BANK_OPEN */
	HW_OCOTP_CTRL_CLR(BM_OCOTP_CTRL_RD_BANK_OPEN);

	searchcount = (rom1>>8) & 0x0F;
	searcharea = table[searchcount];

	pr_err("hw_otp_rom1 %x, searchcount=%d searcharea=%d\n", rom1, searchcount, searcharea);
	return searcharea;
}
#endif

static int nand_writesb (u8 *image, int bootldr_blksize)
{
	FLASH_SPEC *spec = &PDev[0].DevSpec;
	int searcharea;
	int page, size_to_write;
	int firmware_blk, ncb_blk, ldlb_blk;

	#ifdef OTP_ROM_READ
		searcharea = hw_otp_read_searcharea() * spec->NumDiesPerCE;
	#else
		searcharea = 1 * spec->NumDiesPerCE;
	#endif

	/* BCB blocks to write */
	ncb_blk = 0;
	ldlb_blk = searcharea*2/PDevCount;
	firmware_blk = searcharea*4/PDevCount;;
	if (PDevCount == 4) {
		// hack: to set firmware block location
		// todo: find a better way to get firmware block
		firmware_blk = 4;
	}
	
	/* ===================================================== */
	/*  FIRMWARE											 */
	/* ===================================================== */
	/* fw block erase */
	if (erase_block(0, firmware_blk) != FM_SUCCESS) {
		pr_err("%s: erase failed\n", __FUNCTION__); 
		pr_err("%s: failed\n", __FUNCTION__);
	}

	/* cannot write sb bigger than 256K */
	size_to_write = NANDSB_SIZE;
	page = 0;

	/* write firmware to firmware_blk */
	while (size_to_write > 0) {
		if (write_page(0, firmware_blk, page, image, NULL) !=  FM_SUCCESS) {
			pr_err("Write failed. chip(%d), block(%d), page(%d)\n", 0, firmware_blk, page); 
		}
		/* boot sector size is fixed to 2K */
		size_to_write -= BOOT_SECTOR_SIZE;
		image += BOOT_SECTOR_SIZE;
		page++;
	}

	/* ===================================================== */
	/*  NCB & LDLB                                           */
	/* ===================================================== */
	/* 1 CE */
	if (PDevCount == 1) {
		/* NCB1, NCB2 */
		write_ncb(spec, 0, ncb_blk+0);
		write_ncb(spec, 0, ncb_blk+1);

		/* LDLB1, LDLB2 */
		write_ldlb(spec, 0, ldlb_blk+0, firmware_blk, size_to_write);
		write_ldlb(spec, 0, ldlb_blk+1, firmware_blk, size_to_write);
	}
	/* 2 CE */
	else if (PDevCount == 2) {
		ncb_blk /= 2;
		ldlb_blk /= 2;

		/* NCB1, NCB2  */
		write_ncb(spec, 0, ncb_blk);
		write_ncb(spec, 1, ncb_blk);

		/* LDLB1, LDLB2 */
		write_ldlb(spec, 0, ldlb_blk, firmware_blk, size_to_write);
		write_ldlb(spec, 1, ldlb_blk, firmware_blk, size_to_write);
	}
	/* 4 CE */
	else if (PDevCount == 4) {
		/* NCB1, NCB2  */
		write_ncb(spec, 0, ncb_blk);
		write_ncb(spec, 1, ncb_blk);

		/* LDLB1, LDLB2 */
		write_ldlb(spec, 0, ldlb_blk, firmware_blk, size_to_write);
		write_ldlb(spec, 1, ldlb_blk, firmware_blk, size_to_write);
	}

	return 0; 
}


static ssize_t nandsb_write (struct file * file, const char * buf, size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	ssize_t written;

	if (p >= nandsb_size) {
		return 0;
	}

	if (count > nandsb_size - p) {
		count = nandsb_size - p;
	}

	written = 0;

	if (copy_from_user(nandsb_base + p, buf, count))
		return -EFAULT;

	written += count;
	*ppos += written;

	return written;
}

static int nandsb_open (struct inode * inode, struct file * filp)
{
	nandsb_base = (unsigned char *)kmalloc(NANDSB_SIZE, GFP_KERNEL);
	if (!nandsb_base) {
		pr_err("kmalloc failed\n"); 
		return -EAGAIN;
	}
	nandsb_size = NANDSB_SIZE;
	return 0;
}

static int nandsb_release (struct inode * inode, struct file * filp)
{
	printk("[NANDSB] writing to nand\n");
	// bcb size is 10
	// - refer to bootloader code
	nand_writesb(nandsb_base, 10);

	printk("[NANDSB] done...\n");

	if (nandsb_base) {
		kfree(nandsb_base);
		nandsb_base = 0;
	}
	return 0;
}


static struct file_operations nandsb_fops = {
	write:      nandsb_write,
	open:       nandsb_open,
	release:    nandsb_release,
};

static struct miscdevice nandsb_misc = {
	minor : NANDSB_MINOR,
	name  : "misc/nandsb",
	fops  : &nandsb_fops,
};


/*
 * Module housekeeping.
 */
static int __init nandsb_init (void)
{
	pr_info("[NANDSB] device driver init\n"); 

	if (misc_register(&nandsb_misc) != 0) {
		pr_err("Cannot register device /dev/%s\n", nandsb_misc.name);
		return -EFAULT;
	}

	return 0;
}


static void __exit nandsb_cleanup (void)
{
	misc_deregister(&nandsb_misc);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Heechul Yun <heechul.yun@samsung.com>");

module_init(nandsb_init);
module_exit(nandsb_cleanup);


