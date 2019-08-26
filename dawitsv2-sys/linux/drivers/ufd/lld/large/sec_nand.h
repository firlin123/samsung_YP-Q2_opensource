/*
 * Samsung NAND Flash command codes 
 * Samsung NAND Flash AC Timing constants.. 
 *
 * 2005 (c) Samsung Electronics
 *
 * @author    Heechul Yun <heechul.yun@samsung.com>
 *
 * Taken from fm_driver_omap.c 
 */

#ifndef SEC_NAND_H_
#define SEC_NANE_H_

/* 
 * AC Timings of Samsung NAND Flashes.. 
 * 
 *               tBERS   tPROG   tDBSY(1)  tR
 * K9WAG08U1M    2000    700     1         20  
 * 
 * (1) tDBSY : Dummy busy program 
 */ 


/* Flash Memory Commands (Samsung NAND Flash Memory) */

#define FLASH_READ1                 (u8) 0x00
#define FLASH_READ1_SECOND          (u8) 0x30
#define FLASH_READ2                 (u8) 0x00
#define FLASH_READ2_SECOND          (u8) 0x35 /* read for copy back */ 
#define FLASH_READ_ID               (u8) 0x90
#define FLASH_PAGE_PROGRAM          (u8) 0x80
#define FLASH_PAGE_PROGRAM_SECOND   (u8) 0x10
#define FLASH_CACHE_PROGRAM         (u8) 0x80
#define FLASH_CACHE_PROGRAM_SECOND  (u8) 0x15
#define FLASH_COPY_BACK             (u8) 0x85
#define FLASH_COPY_BACK_SECOND      (u8) 0x10
#define FLASH_BLOCK_ERASE           (u8) 0x60
#define FLASH_BLOCK_ERASE_SECOND    (u8) 0xD0
#define FLASH_RANDOM_INPUT          (u8) 0x85
#define FLASH_RANDOM_OUTPUT         (u8) 0x05
#define FLASH_RANDOM_OUTPUT_SECOND  (u8) 0xE0
#define FLASH_READ_STATUS           (u8) 0x70
#define FLASH_CHIP1_STATUS          (u8) 0xF1
#define FLASH_CHIP2_STATUS          (u8) 0xF2
#define FLASH_RESET                 (u8) 0xFF
 
#define FLASH_READ_GROUP1           (u8) 0x60
#define FLASH_READ_GROUP1_CLE2      (u8) 0x60
#define FLASH_READ_GROUP1_CLE3      (u8) 0x30

#define FLASH_READ_GROUP2           (u8) 0x60
#define FLASH_READ_GROUP2_CLE2      (u8) 0x60
#define FLASH_READ_GROUP2_CLE3      (u8) 0x35 /* read for copy back */ 

#define FLASH_PAGE_PROGRAM_GROUP         (u8) 0x80
#define FLASH_PAGE_PROGRAM_GROUP_CLE2    (u8) 0x11
#define FLASH_PAGE_PROGRAM_GROUP_CLE3    (u8) 0x81
#define FLASH_PAGE_PROGRAM_GROUP_CLE4    (u8) 0x10

#define FLASH_BLOCK_ERASE_CLE1      (u8) 0x60
#define FLASH_BLOCK_ERASE_CLE2      (u8) 0xD0

#define FLASH_COPY_BACK_GROUP       (u8) 0x85
#define FLASH_COPY_BACK_GROUP_CLE2  (u8) 0x11
#define FLASH_COPY_BACK_GROUP_CLE3  (u8) 0x81
#define FLASH_COPY_BACK_GROUP_CLE4  (u8) 0x10

#define FLASH_AREA_A                (u8) 0x00
#define FLASH_AREA_B                (u8) 0x01
#define FLASH_AREA_C                (u8) 0x50



#endif /* SEC_NAND_H_ */ 
