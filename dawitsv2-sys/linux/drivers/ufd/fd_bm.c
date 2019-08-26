/************************************************************************/
/*                                                                      */
/*  Copyright (c) 2004 Flash Planning Group, Samsung Electronics, Inc.  */
/*  Copyright (c) 2004 Zeen Information Technologies, Inc.              */
/*  All right reserved.                                                 */
/*                                                                      */
/*  This software is the confidential and proprietary information of    */
/*  Samsung Electronics, Inc. and Zeen Information Technologies, Inc.   */
/*  ("Confidential Information"). You shall not disclose such           */
/*  confidential information and shall use it only in accordance with   */
/*  the terms of the license agreement you entered into with one of     */
/*  the above copyright holders.                                        */
/*                                                                      */
/************************************************************************/
/*  This file implements the Flash Device Bad Block Management Layer.   */
/*                                                                      */
/*  @author  Hak-Yong Lee                                               */
/*  @author  Sung-Kwan Kim                                              */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : Linux RFS (Robust FAT File System) for NAND Flash Memory  */
/*  FILE    : fd_bm.c                                                   */
/*  PURPOSE : Code for Flash Device Bad Block Management Layer (BM)     */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  NOTES                                                               */
/*                                                                      */
/*  - This module's functionality is officially called DLBM             */
/*    (Driver-Level Bad Block Management).                              */
/*  - To enable/disable DLBM, define the 'USE_DLBM' macro in 'fd_bm.h'  */
/*    appropriately.                                                    */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  REVISION HISTORY (Ver 1.0)                                          */
/*                                                                      */
/*  - 01/12/2003 [Sung-Kwan Kim] : First writing                        */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  TECHNICAL SUPPORT                                                   */
/*                                                                      */
/*  - Samsung Flash Planning Group (flashsw@samsung.com)                */
/*                                                                      */
/************************************************************************/

#include <linux/proc_fs.h>

#include "fm_global.h"
#include "fd_if.h"
#include "fd_bm.h"
#include "fd_physical.h"

#if CFG_HWECC_PIPELINE == HWECC_PIPELINE_UFD
#include "lld/large/hwecc.h" /* HWECC_PIPELINEING */
#endif

#if USE_DLBM

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions (Configurable)                         */
/*----------------------------------------------------------------------*/

#define USE_BAD_BLOCK_REVIVAL   0               /* revive bad blocks when
                                                   there's no more remaining
                                                   free block in DLBM area?
                                                   1: yes
                                                   0: no (default) */

#define COPY_WRITE_BAD_PAGE     0               /* copy the bad page itself
                                                   after block replacement
                                                   for a write bad block?
                                                   1: yes
                                                   0: no (default) */

#define NUMBER_OF_MAP_BLOCKS    2               /* must be >= 2 */
#define MAX_DLBM_BLKS_RESERVED  240             /* should be equal to or
                                                   greater than maximum of
                                                   pdev->BmAreaNumBlocks */

#define DELAY_FOR_ERASE         4000            /* in usec; this delay is 
                                                   inserted before bad block 
                                                   handling when an erase bad
                                                   block is detected */

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions (Non-Configurable)                     */
/*----------------------------------------------------------------------*/

#define MAP_PAGE_SIGNATURE      0x5DB3C70E
#define BADBLK_ENTRIES_PER_PAGE 120             /* in 512 byte page */
#define MAX_NUMBER_OF_MAP_PAGES \
        ((MAX_DLBM_BLKS_RESERVED - 1) / BADBLK_ENTRIES_PER_PAGE + 1)

#define SWAP_TABLE_HASH_SIZE    16              /* must be a kind of (2^n) */
#define END_OF_LIST             0xFFFF

/* BM (Bad block Management) module's state; BM module's facility can be
   used only when it is in the 'BM_FORMATTED' state */

enum BAD_BLOCK_MANAGEMENT_MODULE_STATE {
    BM_CLOSED                   = 0x00,
    BM_OPEN                     = 0x01,
    BM_FORMATTED                = 0x02
};

/* macros for calculating block numbers & offsets in the DLBM area */

#define GET_BLK_OFFSET(block)   ((block) - pdev->BmAreaStartBlock)
#define GET_BLK_NUMBER(offset)  (pdev->BmAreaStartBlock + (offset))

#define MAP_OFFSET(blk_offset)  \
        ((UINT16)((blk_offset) / BADBLK_ENTRIES_PER_PAGE))

/* each swap table entry corresponds to a block in the DLBM area;
   by looking up the entries, we can know how each DLBM area block
   is used currently -- followings are the possible usages;
   
   free block        : a DLBM area block is not used yet
   map block         : a DLBM area block is used to store the swap table
   bad block itself  : a DLBM area block is bad itself
   replacement block : used as a replacement block for a bad block
   
   to save the above information in the 32-bit long swap table entries,
   together with some aditional information for a registered bad block,
   the following bit field struture is used;

   31           24            16             8             0
   +-------------+-------------+-------------+-------------+
   |    Byte3    |    Byte2    |    Byte1    |    Byte0    |
   +-------------+-------------+-------------+-------------+
    `-----+-----'  | `---- replaced bad block number -----'
          |        |
          |        +------ special entry indicator
          |
          +--------------- additional bad block information

   bit 31..30 : flash memory operation that caused the bad block
   bit 29..24 : reserved (may be used to hold page number)
   bit 23     : special entry indicator 
                0 - entry for bad block replacement (swap entry)
                1 - entry for free block, map block, bad block itself
   bit 22...0 : replaced (original) bad block number 
*/

/* special swap table entries */
   
#define ENTRY_FREE_BLOCK        0x00FFFFFF      /* not used; free to use */
#define ENTRY_MAP_BLOCK         0x00FFFFFC      /* block is used for map */
#define ENTRY_BAD_BLOCK         0x00FFFFF9      /* block is bad itself */

/* macros to extract a specific bit field from the swap table entries */

#define OP_INFO_MASK            0xC0000000      /* mask for operation info */
#define OP_INFO_SHIFT           30
#define BLK_NUMBER_MASK         0x00FFFFFF      /* mask for block number &
                                                   special entry indicator */

#define IS_SPECIAL_ENTRY(ent)   ((UINT32)ent & 0x00800000)
#define IS_SWAP_ENTRY(ent)      (~((UINT32)ent) & 0x00800000)

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

/* data structure for instance of bad block management module */

typedef struct _BAD_BLOCK_MANAGEMENT_MODULE {
    UINT32      Sequence;
    UINT32      MapBlock[NUMBER_OF_MAP_BLOCKS];
    UINT16      CurMapBlockIdx;
    UINT16      CurMapPageNum;
    UINT16      NumMapPages;
    UINT16      State;
    
    UINT32      SwapTable[MAX_DLBM_BLKS_RESERVED];
    UINT16      NextLink[MAX_DLBM_BLKS_RESERVED];
    UINT16      SwapTableHashList[SWAP_TABLE_HASH_SIZE];
    UINT16      FreeBlockList;
} BM_MOD;

/* structure for bad block map page (512 bytes) */

typedef struct _BAD_BLOCK_MAP_PAGE {
    UINT32      Signature;
    UINT32      Sequence;
    UINT32      MapOffset;
    UINT32      SwapTable[BADBLK_ENTRIES_PER_PAGE];
    UINT8       Version[16];
    UINT32      CheckSum;
} BM_MAPPAGE;

/*----------------------------------------------------------------------*/
/*  Local Variable Definitions                                          */
/*----------------------------------------------------------------------*/

static BM_MOD       BMMod[MAX_FLASH_CHIPS];
static UINT8        pbuf[MAX_PAGE_SIZE];
static BM_MAPPAGE  *map_page = (BM_MAPPAGE *)pbuf;

/*----------------------------------------------------------------------*/
/*  Local Function Declarations                                         */
/*----------------------------------------------------------------------*/

static void     init_bm_module(BM_MOD *mod);
static void     construct_fast_lookup_table(PFDEV *pdev, BM_MOD *mod);
static UINT16   get_swap_table_free_block_entry(BM_MOD *mod);
static UINT16   get_swap_table_last_free_block_entry(BM_MOD *mod);
static INT32    save_swap_table(PFDEV *pdev, BM_MOD *mod, UINT16 map_offset);
static INT32    read_map_page(PFDEV *pdev, UINT32 block, UINT16 page);
static INT32    write_map_page(PFDEV *pdev, BM_MOD *mod, UINT16 map_offset,
                               UINT32 block, UINT16 page);
static INT32    bm_copy_page(PFDEV *pdev, UINT32 src_block, UINT32 dest_block, 
                          UINT16 page);
static void     mark_bad_block(PFDEV *pdev, UINT32 block);
static INT32    flash_write(PFDEV *pdev, UINT32 block, UINT16 page, 
                            UINT16 sector_offset, UINT16 num_sectors,
                            UINT8 *dbuf, UINT8 *sbuf);
static INT32    flash_erase(PFDEV *pdev, UINT32 block);
static UINT32   calc_checksum(UINT32 *data, UINT16 size);
static BOOL     is_all_FF(UINT8 *buf, UINT32 size);
static INT32    no_free_block_panic(PFDEV *pdev, BM_MOD *mod);

#if USE_BAD_BLOCK_REVIVAL
static INT32    bad_block_revival(PFDEV *pdev, BM_MOD *mod);
#endif


/*======================================================================*/
/*  External Function Definitions                                       */
/*======================================================================*/

int bm_read_proc(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int chip, i; 
	int len = 0; 
	PFDEV  *pdev;
	BM_MOD *mod;
	BOOL is_initial, first_bad_entry = TRUE;

	for (chip = 0; chip < PDevCount; chip++) {
		int nbads; 
		pdev = PFD_GetPhysicalDevice(chip);
		mod = &BMMod[chip];	
		/* if not open yet, open the device first */
		if (mod->State == BM_CLOSED) {
			printk("%s: should be ok for any case", __FUNCTION__); 
			goto out;
		}

		len += sprintf(buf + len, "-- Chip %d\n", chip); 

		len += sprintf(buf + len, "Map Blocks : "); 		
		for ( i = 0; i < NUMBER_OF_MAP_BLOCKS; i++ ) {
			if ( i > 0 ) len += sprintf (buf + len, ", ");
			len += sprintf(buf + len, "%d", mod->MapBlock[i]);
		}

		len += sprintf(buf + len, "\nBad Blocks : \n");
		nbads = 0; 
		first_bad_entry = TRUE; 
		for ( i = 0; i < pdev->BmAreaNumBlocks; i++ ) {
			int entry = mod->SwapTable[i];
			int blknum = (entry & BLK_NUMBER_MASK); 

			if ( IS_SWAP_ENTRY(entry) || blknum == ENTRY_BAD_BLOCK ) {
				if ( !first_bad_entry ) len += sprintf (buf + len, ", ");
				first_bad_entry = FALSE;

				if ( entry == blknum ) is_initial = TRUE;	// initial
				else is_initial = FALSE;			// run-time

				if ( blknum == ENTRY_BAD_BLOCK ) {
					blknum = GET_BLK_NUMBER(i);
				}

				if ( is_initial )
					len += sprintf(buf + len, "%d", blknum);
				else
					len += sprintf(buf + len, "%d (r)", blknum);

				if ( ++nbads % 10 == 0 ) {
					len += sprintf(buf + len, "\n"); 
					first_bad_entry = TRUE; 
				}
			}
		}
		len += sprintf(buf + len, "\n"); 
	}
 out:
	*eof = 1; 
	return len; 
}

extern INT32    
BM_Init(void)
{
    int i;
    struct proc_dir_entry *part_root; 
    
    /* initialize the bad block management modules */
    for (i = 0; i < MAX_FLASH_CHIPS; i++) {
        init_bm_module(&BMMod[i]);
    }

    // Register proc entry 
    part_root = create_proc_entry("dlbm", S_IWUSR | S_IRUGO, NULL); 
    part_root->read_proc = bm_read_proc;
    part_root->write_proc = NULL; 

    return(FM_SUCCESS);
}


extern UINT32   
BM_CalcBmAreaNumBlocks(UINT16 chip_id)
{
    PFDEV  *pdev;
    
    /* existing chip id? */
    if (chip_id >= PFD_GetNumberOfChips()) return(0);
    pdev = PFD_GetPhysicalDevice(chip_id);
    
    /* return the number of blocks in the DLBM area */
    return(pdev->DevSpec.MaxNumBadBlocks + NUMBER_OF_MAP_BLOCKS);
    
}


extern INT32    
BM_Open(UINT16 chip_id)
{
    PFDEV  *pdev;
    BM_MOD *mod;
    INT32  err;
    UINT32 block, j;
    UINT32 seq[MAX_NUMBER_OF_MAP_PAGES], map_block_seq[NUMBER_OF_MAP_BLOCKS];
    UINT16 i, page, map_blocks_found;
    
    /* existing chip id? */
    if (chip_id >= PFD_GetNumberOfChips()) return(FM_BAD_DEVICE_ID);
    pdev = PFD_GetPhysicalDevice(chip_id);
    mod = &BMMod[chip_id];
    
    /* already open? */
    if (mod->State > BM_CLOSED) return(FM_SUCCESS);
    
    /* initialize local variables */
    mod->NumMapPages = MAP_OFFSET(pdev->BmAreaNumBlocks - 1) + 1;
    for (i = 0; i < NUMBER_OF_MAP_BLOCKS; i++) map_block_seq[i] = 0;
    map_blocks_found = 0;
    
    /* starting from the first block in the DLBM area, 
       find #(NUMBER_OF_MAP_BLOCKS) of map blocks */
    for (block = pdev->BmAreaStartBlock;
         block < pdev->DevSpec.NumBlocks; block++) {
        
        /* skip bad blocks */
        if (pdev->DevOps.IsBadBlock(pdev->LocalChipID, block)) continue;
        
        /* read the first page in this block;
           if it is not valid, proceed to check the next block */
        err = read_map_page(pdev, block, 0);
        if (err != FM_SUCCESS) continue;
        
        /* a valid map page (thus a valid map block) found;
           keep the block number and the map block sequence number 
           (i.e. the sequence number of the first map page) */
        mod->MapBlock[map_blocks_found] = block;
        map_block_seq[map_blocks_found] = map_page->Sequence;
        map_blocks_found++;

        /* has found all map blocks? */
        if (map_blocks_found >= NUMBER_OF_MAP_BLOCKS) break;
    }
    
    if (map_blocks_found == 0) {
        /* no map blocks found; format required */
        return(FM_SUCCESS);
    }
    
    mod->State = BM_OPEN;

get_latest_map_block:
    /* find the most up-to-date map block, i.e. a map block 
       that has the largest sequence number */
    mod->Sequence = 0;
    for (i = 0; i < NUMBER_OF_MAP_BLOCKS; i++) {
        if (mod->Sequence < map_block_seq[i]) {
            mod->Sequence = map_block_seq[i];
            mod->CurMapBlockIdx = i;
        }
    }
    if (mod->Sequence == 0) {
        /* no valid map blocks found; re-format required */
        return(FM_SUCCESS);
    }
    
    /* the latest map block is the current map block */
    block = mod->MapBlock[mod->CurMapBlockIdx];
    
    for (i = 0; i < mod->NumMapPages; i++) seq[i] = 0;
    
    /* read map pages from the current map block */
    for (page = 0; page < pdev->DevSpec.PagesPerBlock; page++) {

        /* read one page */
        err = read_map_page(pdev, block, page);
        if (err != FM_SUCCESS) break;
        
        /* a valid map page found; check its sequence number */
        if (seq[map_page->MapOffset] < map_page->Sequence) {
            
            /* up-to-date map found; keep this in memory */
            for (i = 0, j = map_page->MapOffset * BADBLK_ENTRIES_PER_PAGE;
                 i < BADBLK_ENTRIES_PER_PAGE && j < pdev->BmAreaNumBlocks; 
                 i++, j++) {
                mod->SwapTable[j] = map_page->SwapTable[i];
            }
            seq[map_page->MapOffset] = map_page->Sequence;
        }
    }
    
    /* check if all swap table segments are found */
    for (i = 0; i < mod->NumMapPages; i++) {
        if (seq[i] == 0) {
            /* some part of the map pages are missing;
               this means that the current map block is corrupt;
               try to find the map pages in another map block */
            map_block_seq[mod->CurMapBlockIdx] = 0;
            goto get_latest_map_block;
        }
        if (mod->Sequence < seq[i]) {
            mod->Sequence = seq[i];
        }
    }
    
    /* if we need to find more map blocks, do it here */
    if (map_blocks_found < NUMBER_OF_MAP_BLOCKS) {

        /* start the map block scan all over again using the swap table */
        for (i = 0, j = 0; i < pdev->BmAreaNumBlocks; i++) {
            
            /* is this swap table entry indicates a map block? */
            if (mod->SwapTable[i] == ENTRY_MAP_BLOCK) {
                mod->MapBlock[j] = GET_BLK_NUMBER(i);
                if (mod->MapBlock[j] == block) mod->CurMapBlockIdx = j;
                j++;
                if (j >= NUMBER_OF_MAP_BLOCKS) break;
            }
        }
        
        if (j < NUMBER_OF_MAP_BLOCKS) {
            /* some of map blocks are missing; re-format required */
            return(FM_SUCCESS);
        }
    }

    /* set the current map page number to be the last page number,
       so that updated map pages could be written to a new map block */
    mod->CurMapPageNum = pdev->DevSpec.PagesPerBlock - 1;

    /* construct the hash table indexing the swap table */
    construct_fast_lookup_table(pdev, mod);

    /* low-level BM format identified */
    mod->State = BM_FORMATTED;
    return(FM_SUCCESS);
}

#define DETECT_FACTORY_BB 

extern INT32    
BM_Format(UINT16 chip_id, BOOL forced)
{
    PFDEV  *pdev;
    BM_MOD *mod;
    INT32  err;
    UINT32 block;
    UINT16 i, good_blocks_found;

    int nbb = 0; 
    
    /* existing chip id? */
    if (chip_id >= PFD_GetNumberOfChips()) return(FM_BAD_DEVICE_ID);
    pdev = PFD_GetPhysicalDevice(chip_id);
    mod = &BMMod[chip_id];
    
    /* if not open yet, open the device first */
    if (mod->State == BM_CLOSED) {
        err = BM_Open(chip_id);
        if (err ) panic("%s: should be ok for any case", __FUNCTION__); 
    }

    /* already formatted? */
    if (mod->State == BM_FORMATTED && !forced) return(FM_SUCCESS);
    
    // panic("ERROR: UFD is not formatted yet.. it's impossible \n"); 
    
    /* now, formatting starts; initialize bad block management module */
    init_bm_module(mod);
    mod->NumMapPages = MAP_OFFSET(pdev->BmAreaNumBlocks - 1) + 1;
    good_blocks_found = 0;
    
    /* starting from the first block, check all DLBM area blocks;
       for initial bad blocks in the area, mark them in the swap table;
       during the check, locate map blocks too */

    for (block = pdev->BmAreaStartBlock;
         block < pdev->DevSpec.NumBlocks; block++) {


        /* is this a bad block? */
#ifdef DETECT_FACTORY_BB             
        if (pdev->DevOps.IsBadBlock(pdev->LocalChipID, block)) {
#else 
	pdev->DevOps.Erase(pdev->LocalChipID, block); 
	if ( pdev->DevOps.Sync(pdev->LocalChipID) ) {
            if (pdev->PrevOp != NULL) pdev->PrevOp->Command = OP_NONE;
#endif             
            /* mark the corresponding swap table entry as a bad block */
            mod->SwapTable[GET_BLK_OFFSET(block)] = ENTRY_BAD_BLOCK;
	    printk("%s:%d is bad\n", __FUNCTION__, block); nbb++; // hcyun  
            continue;
        }

        /* a good block found; register it as a map block if necessary */
        if (good_blocks_found < NUMBER_OF_MAP_BLOCKS) {
            mod->MapBlock[good_blocks_found] = block;
            good_blocks_found++;
		printk("%s:%d is good block \n", __FUNCTION__, block); // hcyun
            /* mark the corresponding swap table entry as a map block */
            mod->SwapTable[GET_BLK_OFFSET(block)] = ENTRY_MAP_BLOCK;
        }
    }
    
    /* check if a sufficient number of map blocks are found */
    if (good_blocks_found < NUMBER_OF_MAP_BLOCKS) return(FM_INIT_FAIL);

    /* starting from the first block of the chip, check all blocks;
       if a initial bad block found, register it in the swap table */
    for (block = 1, i = pdev->BmAreaNumBlocks - 1; 
         block < pdev->BmAreaStartBlock; block++) {

        /* is this a bad block? */
#ifdef DETECT_FACTORY_BB             
        if (pdev->DevOps.IsBadBlock(pdev->LocalChipID, block)) {
#else 
	pdev->DevOps.Erase(pdev->LocalChipID, block); 
	if ( pdev->DevOps.Sync(pdev->LocalChipID) ) {
            if (pdev->PrevOp != NULL) pdev->PrevOp->Command = OP_NONE;
#endif 
            printk("%s:%d is bad block \n", __FUNCTION__, block); nbb++; // hcyun            

            /* find a free block entry in the bad block swap table */
            while (mod->SwapTable[i] != ENTRY_FREE_BLOCK) {
                if (i == 0) {
                    /* too many initial bad blocks */
                    return(FM_INIT_FAIL);
                }
                i--;
            }
            
            /* register this block in the swap table */
            mod->SwapTable[i] = block;
        }
    }

    /* construct the hash table indexing the swap table */
    construct_fast_lookup_table(pdev, mod);

    /* store the bad block swap table into each of the map blocks */
    for (i = 0; i < NUMBER_OF_MAP_BLOCKS; i++) {
        mod->CurMapBlockIdx = i;
        mod->CurMapPageNum = pdev->DevSpec.PagesPerBlock - 1;
        err = save_swap_table(pdev, mod, 0);
        if (err) return(FM_INIT_FAIL);
    }

    printk("%s:total number of bad blocks = %d \n", __FUNCTION__, nbb); // hcyun

    mod->State = BM_FORMATTED;
    return(FM_SUCCESS);
}


extern BOOL   
BM_isFormatted(UINT16 chip_id)
{
    if (BMMod[chip_id].State == BM_FORMATTED) {
        return(TRUE);
    }

    return(FALSE);
}


extern INT32  
BM_GetNumBadBlocks(UINT16 chip_id)
{
    PFDEV  *pdev;
    BM_MOD *mod;
    UINT32 i;
    INT32  n = 0;
    
    /* existing chip id? */
    if (chip_id >= PFD_GetNumberOfChips()) return(-1);

    pdev = PFD_GetPhysicalDevice(chip_id);
    mod = &BMMod[chip_id];
    
    /* formatted? */
    if (mod->State != BM_FORMATTED) return(-1);
    
    for (i = 0; i < pdev->BmAreaNumBlocks; i++) {
        
        /* check if a bad block is registered in this swap table entry, or
           if the corresponding block for this entry is a bad block itself */
        if (IS_SWAP_ENTRY(mod->SwapTable[i]) ||
            (mod->SwapTable[i] & BLK_NUMBER_MASK) == ENTRY_BAD_BLOCK) {
            n++;
        }
    }
    
    return(n);
}


extern INT32    
BM_SwapWriteBadBlock(UINT16 chip_id, UINT32 block, UINT16 page)
{
    PFDEV  *pdev;
    BM_MOD *mod;
    UINT16 i, j;
    UINT32 new_block, org_block;
    INT32  err;
    
    pdev = PFD_GetPhysicalDevice(chip_id);
    mod = &BMMod[chip_id];
printk("%s : registering a bad block : block = %d, page = %d\n", __FUNCTION__, block, page);
    
    /* 1) find a free block to replace this block with;
       2) erase the free block found;
       3) copy valid pages in the old block to the new block */
       
get_free_block:
    /* find a free block entry from the swap table */
    i = get_swap_table_free_block_entry(mod);
    if (i == END_OF_LIST) {
        /* there's no free block to replace this block with */
        err = no_free_block_panic(pdev, mod);
        if (err == FM_TRY_AGAIN) goto get_free_block;
        else return(err);
    }
    
    /* a new free block found */
    new_block = GET_BLK_NUMBER(i);

    /* erase the free block first */
    err = flash_erase(pdev, new_block);
    if (err) {
        /* erase failed; 
           for an erase bad block, some delay should be inserted before
           bad block handling; at power off time, an erase operation may 
           fail (thus a false bad block detected) but a write operation 
           for saving the updated swap table in flash memory may succeed;
           delay is inserted to prevent such incorrect processing */
        DELAY_IN_USEC(DELAY_FOR_ERASE);

printk("%s : erase failed : block = %d\n", __FUNCTION__, new_block);
    
        /* mark the corresponding swap table entry as a bad block */
        mod->SwapTable[i] = ((UINT32)OP_ERASE << OP_INFO_SHIFT) 
                            | ENTRY_BAD_BLOCK;
        
        /* save the swap table */
        err = save_swap_table(pdev, mod, MAP_OFFSET(i));
        if (err) return(FM_ERROR);
        
        /* find another free block */
        goto get_free_block;
    }

    /* erase done; copy valid pages in the old block to the new block */
    for (j = 0; j < pdev->DevSpec.PagesPerBlock; j++) {
        
#if (COPY_WRITE_BAD_PAGE == 0)
        /* skip the page for which write failed */
        if (j == page) continue;
#endif
        /* copy one page */
        err = bm_copy_page(pdev, block, new_block, j);
        if (err) {
printk("%s : copy failed : block = %d, page = %d\n", __FUNCTION__, new_block, j);

            /* write failed;
               mark the corresponding swap table entry as a bad block */
            mod->SwapTable[i] = ((UINT32)OP_WRITE << OP_INFO_SHIFT) 
                                | ENTRY_BAD_BLOCK;
            
            /* save the swap table */
            err = save_swap_table(pdev, mod, MAP_OFFSET(i));
            if (err) return(FM_ERROR);
            
            /* find another free block */
            goto get_free_block;
        }
    }
    
    /* ok, all valid pages have been successfully copied;
       now, replace this block with the new block;
       this is done by registering this block in the swap table */
    if (block < pdev->BmAreaStartBlock) {
        
        /* annotate the bad block number with information about the 
           operation that caused the block to be bad */
        mod->SwapTable[i] = ((UINT32)OP_WRITE << OP_INFO_SHIFT) | block;
    }
    else {
        /* a replacement block itself has become a bad block;
           find the original block swapped with the replacement block */
        j = (UINT16) GET_BLK_OFFSET(block);
        org_block = mod->SwapTable[j];
        
        /* mark the replacement block itself as bad */
        mod->SwapTable[j] = ((UINT32)OP_WRITE << OP_INFO_SHIFT) 
                            | ENTRY_BAD_BLOCK;
        
        /* save the swap table in flash memory if necessary */
        if (MAP_OFFSET(j) != MAP_OFFSET(i)) {
            if (save_swap_table(pdev, mod, MAP_OFFSET(j)) != FM_SUCCESS)
                return(FM_ERROR);
        }
        
        /* register the original block in the current swap table entry */
        mod->SwapTable[i] = org_block;
    }
    
    /* update the swap table hash lists */
    construct_fast_lookup_table(pdev, mod);
    
    /* save the modified part of the swap table in flash memory */
    return(save_swap_table(pdev, mod, MAP_OFFSET(i)));
}


extern INT32    
BM_SwapEraseBadBlock(UINT16 chip_id, UINT32 block)
{
    PFDEV  *pdev;
    BM_MOD *mod;
    UINT16 i, j;
    UINT32 org_block;
    INT32  err;

    pdev = PFD_GetPhysicalDevice(chip_id);
    mod = &BMMod[chip_id];
printk("%s : registering a bad block : block = %d\n", __FUNCTION__, block);

    /* for an erase bad block, some delay should be inserted before
       bad block handling; at power off time, an erase operation may 
       fail (thus a false bad block detected) but a write operation 
       for saving the updated swap table in flash memory may succeed;
       delay is inserted to prevent such incorrect processing */
    DELAY_IN_USEC(DELAY_FOR_ERASE);

get_free_block:
    /* find a free block entry from the swap table */
    i = get_swap_table_free_block_entry(mod);
    if (i == END_OF_LIST) {
        /* there's no free block to replace this block with */
        err = no_free_block_panic(pdev, mod);
        if (err == FM_TRY_AGAIN) goto get_free_block;
        else return(err);
    }
    
    /* replace this block with the free block found;
       this is done by registering this block in the swap table */
    if (block < pdev->BmAreaStartBlock) {

        /* annotate the bad block number with information about the 
           operation that caused the block to be bad */
        mod->SwapTable[i] = ((UINT32)OP_ERASE << OP_INFO_SHIFT) | block;
    }
    else {
        /* a replacement block itself has become a bad block;
           find the original block swapped with the replacement block */
        j = (UINT16) GET_BLK_OFFSET(block);
        org_block = mod->SwapTable[j];
        
        /* mark the replacement block itself as bad */
        mod->SwapTable[j] = ((UINT32)OP_ERASE << OP_INFO_SHIFT) 
                            | ENTRY_BAD_BLOCK;
        
        /* save the swap table in flash memory if necessary */
        if (MAP_OFFSET(j) != MAP_OFFSET(i)) {
            if (save_swap_table(pdev, mod, MAP_OFFSET(j)) != FM_SUCCESS)
                return(FM_ERROR);
        }
        
        /* register the original block in the current swap table entry */
        mod->SwapTable[i] = org_block;
    }

    /* update the swap table hash lists */
    construct_fast_lookup_table(pdev, mod);
    
    /* save the modified part of the swap table in flash memory */
    return(save_swap_table(pdev, mod, MAP_OFFSET(i)));
}


extern UINT32   
BM_GetSwappingBlock(UINT16 chip_id, UINT32 block)
{
    register PFDEV  *pdev;
    register BM_MOD *mod;
    register UINT16 i;
    
    UINT16 hash;
    
    pdev = PFD_GetPhysicalDevice(chip_id);
    mod = &BMMod[chip_id];
    
    /* look up the swap table */
    hash = (UINT16)(block & (SWAP_TABLE_HASH_SIZE - 1));
    for (i = mod->SwapTableHashList[hash]; i != END_OF_LIST;
         i = mod->NextLink[i]) {
    
        /* check if this block is registered as a bad block */
        if (block == (mod->SwapTable[i] & BLK_NUMBER_MASK)) {
            
            /* found; return the replacement block */
            return(GET_BLK_NUMBER(i));
        }
    }
    
    /* this block has not been registered as a bad block;
       simply return the original block number */
    return(block);
}


extern UINT32 
BM_GetOriginalBlock(UINT16 chip_id, UINT32 block)
{
    PFDEV  *pdev = PFD_GetPhysicalDevice(chip_id);
    BM_MOD *mod = &BMMod[chip_id];
    
    /* if the given block is not in the DLBM area, just return it */
    if (block < pdev->BmAreaStartBlock) return(block);
    
    return(mod->SwapTable[GET_BLK_OFFSET(block)] & BLK_NUMBER_MASK);
}


/*======================================================================*/
/*  Local Function Definitions                                          */
/*======================================================================*/

static void
init_bm_module(BM_MOD *mod)
{
    int i;

    mod->Sequence = 0;
    mod->State = BM_CLOSED;

    for (i = 0; i < MAX_DLBM_BLKS_RESERVED; i++) {
        mod->SwapTable[i] = ENTRY_FREE_BLOCK;
    }
}


static void
construct_fast_lookup_table(PFDEV *pdev, BM_MOD *mod)
{
    UINT16 i, hash;
    
    /* initialize the hash lists */
    for (i = 0; i < SWAP_TABLE_HASH_SIZE; i++) {
        mod->SwapTableHashList[i] = END_OF_LIST;
    }
    
    mod->FreeBlockList = END_OF_LIST;
    
    /* construct the hash lists */
    for (i = 0; i < pdev->BmAreaNumBlocks; i++) {
        
        /* check if a bad block is registered in this swap table entry */
        if (IS_SWAP_ENTRY(mod->SwapTable[i])) {
            
            /* a registered bad block found; calculate hash function */
            hash = (UINT16)(mod->SwapTable[i] & (SWAP_TABLE_HASH_SIZE - 1));
            
            /* insert this entry in the swap table hash list */
            mod->NextLink[i] = mod->SwapTableHashList[hash];
            mod->SwapTableHashList[hash] = i;
        }
        
        /* check if this is a free block entry */
        else if (mod->SwapTable[i] == ENTRY_FREE_BLOCK) {
            
            /* insert this entry in the free block list;
               'FreeBlockList' is maintained in the descending order
               of block offsets within the DLBM area */
            mod->NextLink[i] = mod->FreeBlockList;
            mod->FreeBlockList = i;
        }
    }
}


static UINT16   
get_swap_table_free_block_entry(BM_MOD *mod)
{
    UINT16 i;
    
    i = mod->FreeBlockList;
    if (i != END_OF_LIST) {
        
        /* free block entry found; detach this entry from the list */
        mod->FreeBlockList = mod->NextLink[i];
    }
    
    return(i);
}


static UINT16   
get_swap_table_last_free_block_entry(BM_MOD *mod)
{
    UINT16 i, prev_i;
    
    if (mod->FreeBlockList == END_OF_LIST) return(END_OF_LIST);
    
    /* go to the end of the list */
    for (i = mod->FreeBlockList, prev_i = END_OF_LIST;
         mod->NextLink[i] != END_OF_LIST; 
         prev_i = i, i = mod->NextLink[i]);

    /* free block entry found; detach this entry from the list */
    if (prev_i == END_OF_LIST) {
        mod->FreeBlockList = END_OF_LIST;
    }
    else {
        mod->NextLink[prev_i] = END_OF_LIST;
    }
    
    return(i);
}


static INT32    
save_swap_table(PFDEV *pdev, BM_MOD *mod, UINT16 map_offset)
{
    INT32  err = FM_ERROR;
    UINT32 cur_map_block, bad_block = ~0;
    UINT16 i, page;
    BOOL   bad_block_marking_delayed = FALSE;
    
    /* identify the next map page to write */
    mod->CurMapPageNum++;
    mod->CurMapPageNum %= pdev->DevSpec.PagesPerBlock;
    cur_map_block = mod->MapBlock[mod->CurMapBlockIdx];
    
    /* check if a map page should be written in the next map block */
    if (mod->CurMapPageNum == 0) {
        
        /* get the next map block */
        mod->CurMapBlockIdx++;
        mod->CurMapBlockIdx %= NUMBER_OF_MAP_BLOCKS;
        cur_map_block = mod->MapBlock[mod->CurMapBlockIdx];

erase_block:
        /* current map block changed; erase this new map block first */
        err = flash_erase(pdev, cur_map_block);
        if (err) {
            /* erase failed; 
               for an erase bad block, some delay should be inserted before
               bad block handling; at power off time, an erase operation may
               fail (thus a false bad block detected) but a write operation 
               for saving the updated swap table in flash memory may succeed;
               delay is inserted to prevent such incorrect processing */
            DELAY_IN_USEC(DELAY_FOR_ERASE);
        
            /* explicit bad block marking is required for this map block */
            mark_bad_block(pdev, cur_map_block);
            
            /* mark the corresponding swap table entry as a bad block */
            mod->SwapTable[GET_BLK_OFFSET(cur_map_block)] = 
                ((UINT32)OP_ERASE << OP_INFO_SHIFT) | ENTRY_BAD_BLOCK;

get_free_block:
            /* find a free block to replace this bad map block with */
            i = get_swap_table_last_free_block_entry(mod);
            if (i == END_OF_LIST) {
                /* there's no more free block; critical error!! */
                err = no_free_block_panic(pdev, mod);
                if (err == FM_TRY_AGAIN) goto get_free_block;
                else return(err);
            }
            
            /* found; allocate this free block to save map data */
            mod->SwapTable[i] = ENTRY_MAP_BLOCK;
            
            /* register this new map block */
            cur_map_block = GET_BLK_NUMBER(i);
            mod->MapBlock[mod->CurMapBlockIdx] = cur_map_block;

            /* ok, try to erase the current map block again */
            goto erase_block;
        }

        /* for the ease of maintenance, all map pages should be saved in 
           each map block; so write all map pages in this new map block */
        for (page = 0; page < mod->NumMapPages; page++) {
            err = write_map_page(pdev, mod, page, cur_map_block, page);
            if (err) {
                /* write failed; explicit bad block marking is required 
                   for this map block */
                mark_bad_block(pdev, cur_map_block);

                /* mark the corresponding swap table entry as a bad block */
                mod->SwapTable[GET_BLK_OFFSET(cur_map_block)] = 
                    ((UINT32)OP_WRITE << OP_INFO_SHIFT) | ENTRY_BAD_BLOCK;
                    
                /* try to allocate a new map block again */
                goto get_free_block;
            }
        }
        
        /* keep the current map page number (i.e. last page number saved) */
        mod->CurMapPageNum = page - 1;
    }
    
    else {
        /* the given map page can be saved in the current map block */
        err = write_map_page(pdev, mod, map_offset, cur_map_block, 
                             mod->CurMapPageNum);
        if (err) {
            /* write failed; 
               explicit bad block marking is required for this map block,
               but this bad block marking should be delayed until a new map
               block is allocated and all map pages are completely saved in
               the new map block */
            bad_block_marking_delayed = TRUE;
            bad_block = cur_map_block;
               
            /* mark the corresponding swap table entry as a bad block */
            mod->SwapTable[GET_BLK_OFFSET(cur_map_block)] = 
                ((UINT32)OP_WRITE << OP_INFO_SHIFT) | ENTRY_BAD_BLOCK;
                
            /* try to allocate a new map block again */
            goto get_free_block;
        }
    }

    /* if an explicit bad block marking has been delayed, do it now */
    if (bad_block_marking_delayed) {
        mark_bad_block(pdev, bad_block);
    }
    
    return(FM_SUCCESS);
}


static INT32
read_map_page(PFDEV *pdev, UINT32 block, UINT16 page)
{
    INT32 err;
    
    /* read the designated map page */
    err = pdev->DevOps.ReadPage(pdev->LocalChipID, block, page, 
                                0, pdev->DevSpec.SectorsPerPage, pbuf, NULL);
    if (err != FM_SUCCESS) return(err);

#if (CFG_HWECC_PIPELINE == HWECC_PIPELINE_UFD)
    if ( !err && (err = hwecc_ecc_decode_sync()) ) {
            switch( err ) {
            case HWECC_DMA_ERROR:
                    printk("HWECC DMA Error\n");
            case HWECC_UNCORR_ERROR:
                    printk("%s: Uncorrectable ECC Error\n", __FUNCTION__);
                    printk("chip(%d),block(%d),page(%d),offset(%d),nsect(%d)\n",
                           prev_ecc.chip, prev_ecc.block, prev_ecc.page,
                           prev_ecc.offset, prev_ecc.nsect);
                    lld_stat.uncorr_ecc_err++;
                    err = FM_ECC_ERROR;
                    break;
            case HWECC_CORR_ERROR:
                    lld_stat.corr_ecc_err++;
            default:
                    err = FM_SUCCESS;
                    break;
            }
    }
#endif /* CFG_HWECC_PIPELINE */
    
    /* check if this page is a valid map page */
    if (map_page->Signature != MAP_PAGE_SIGNATURE) return(FM_ERROR);
    if (map_page->CheckSum != calc_checksum((UINT32 *)map_page, 
                                            SECTOR_SIZE - 4)) {
        return(FM_ERROR);
    }
    
    /* OK, a valid map page has been found */
    return(FM_SUCCESS);
}


static INT32
write_map_page(PFDEV *pdev, BM_MOD *mod, UINT16 map_offset, 
               UINT32 block, UINT16 page)
{
    INT32  err;
    UINT32 i, j;
    
    /* first, check if the target page is all 0xFF */
    err = pdev->DevOps.ReadPage(pdev->LocalChipID, block, page,
                                0, pdev->DevSpec.SectorsPerPage,
                                pbuf, pbuf + pdev->DevSpec.DataSize);
    if (err) return(FM_ERROR);

#if (CFG_HWECC_PIPELINE == HWECC_PIPELINE_UFD)
    if ( !err && (err = hwecc_ecc_decode_sync()) ) {
            switch( err ) {
            case HWECC_DMA_ERROR:
                    printk("HWECC DMA Error\n");
            case HWECC_UNCORR_ERROR:
                    printk("%s: Uncorrectable ECC Error\n", __FUNCTION__);
                    printk("chip(%d),block(%d),page(%d),offset(%d),nsect(%d)\n",
                           prev_ecc.chip, prev_ecc.block, prev_ecc.page,
                           prev_ecc.offset, prev_ecc.nsect);
                    lld_stat.uncorr_ecc_err++;
                    err = FM_ECC_ERROR;
                    break;
            case HWECC_CORR_ERROR:
                    lld_stat.corr_ecc_err++;
            default:
                    err = FM_SUCCESS;
                    break;
            }
    }
#endif /* CFG_HWECC_PIPELINE */

    if (!is_all_FF(pbuf, pdev->DevSpec.PageSize)) {
	    printk("%s: not all ff. block(%d), page(%d)\n", 
		   __FUNCTION__, block, page); 
	    return(FM_ERROR);
    }
    
    /* next, clear the map page buffer */
    MEMSET(pbuf, 0xff, pdev->DevSpec.PageSize);
    
    /* now, fill the map page buffer */
    map_page->Signature = MAP_PAGE_SIGNATURE;
    map_page->Sequence  = ++(mod->Sequence);
    map_page->MapOffset = map_offset;
    for (i = 0, j = map_offset * BADBLK_ENTRIES_PER_PAGE;
         i < BADBLK_ENTRIES_PER_PAGE && j < pdev->BmAreaNumBlocks;
         i++, j++) {
        map_page->SwapTable[i] = mod->SwapTable[j];
    }
    
    /* save the current version of the DLBM scheme;
       currently, it is not used and just cleared */
    MEMSET(map_page->Version, 0, 16);
    
    /* calculate the checksum */
    map_page->CheckSum = calc_checksum((UINT32 *)map_page, SECTOR_SIZE - 4);
        
    /* finally, write the map page */
    return(flash_write(pdev, block, page, 0, pdev->DevSpec.SectorsPerPage, 
                       pbuf, pbuf + pdev->DevSpec.DataSize));
}


static INT32    
bm_copy_page(PFDEV *pdev, UINT32 src_block, UINT32 dest_block, UINT16 page)
{
    INT32 err;

    /* read one page from the source block;
       in this case, it's needless to check the result */
    pdev->DevOps.ReadPage(pdev->LocalChipID, src_block, page, 
                          0, pdev->DevSpec.SectorsPerPage,
                          pbuf, pbuf + pdev->DevSpec.DataSize);

#if (CFG_HWECC_PIPELINE == HWECC_PIPELINE_UFD)
    if ( (err = hwecc_ecc_decode_sync()) ) {
            switch( err ) {
            case HWECC_DMA_ERROR:
                    printk("HWECC DMA Error\n");
            case HWECC_UNCORR_ERROR:
                    printk("%s: Uncorrectable ECC Error\n", __FUNCTION__);
                    printk("chip(%d),block(%d),page(%d),offset(%d),nsect(%d)\n",
                           prev_ecc.chip, prev_ecc.block, prev_ecc.page,
                           prev_ecc.offset, prev_ecc.nsect);
                    lld_stat.uncorr_ecc_err++;
                    err = FM_ECC_ERROR;
                    break;
            case HWECC_CORR_ERROR:
                    lld_stat.corr_ecc_err++;
            default:
                    err = FM_SUCCESS;
                    break;
            }
    }
#endif /* CFG_HWECC_PIPELINE */

    /* check if this page contains any valid data */
    if (is_all_FF(pbuf, pdev->DevSpec.PageSize)) return(FM_SUCCESS);

    /* write the page into the destination block */
    return(flash_write(pdev, dest_block, page, 
                       0, pdev->DevSpec.SectorsPerPage,
                       pbuf, pbuf + pdev->DevSpec.DataSize));
}


static void
mark_bad_block(PFDEV *pdev, UINT32 block)
{
    /* mark the block as bad ** in flash memory **;
       the function result is ignored here because it might fail */
    MEMSET(pbuf, 0x00, 16);
    flash_write(pdev, block, 0, 0, pdev->DevSpec.SectorsPerPage, NULL, pbuf);
    flash_write(pdev, block, pdev->DevSpec.PagesPerBlock - 1, 0, pdev->DevSpec.SectorsPerPage, NULL, pbuf);
    flash_write(pdev, block, pdev->DevSpec.PagesPerBlock - 5, 0, pdev->DevSpec.SectorsPerPage, NULL, pbuf);
}


static INT32
flash_write(PFDEV *pdev, UINT32 block, UINT16 page, 
            UINT16 sector_offset, UINT16 num_sectors,
            UINT8 *dbuf, UINT8 *sbuf)
{
    /* write the given data into the given page */
    if (pdev->DevOps.WritePage(pdev->LocalChipID, block, page, 
                               sector_offset, num_sectors,
                               dbuf, sbuf, TRUE) != FM_SUCCESS) {
        return(FM_WRITE_ERROR);
    }
    
    /* for asynchronous devices, sync should also be performed here */
    if (pdev->DevOps.Sync != NULL &&
        pdev->DevOps.Sync(pdev->LocalChipID) != FM_SUCCESS) {
        if (pdev->PrevOp != NULL) pdev->PrevOp->Command = OP_NONE;
        return(FM_WRITE_ERROR);
    }
    
    return(FM_SUCCESS);
}
                            

static INT32
flash_erase(PFDEV *pdev, UINT32 block)
{
    /* erase the given block */
    if (pdev->DevOps.Erase(pdev->LocalChipID, block) != FM_SUCCESS) {
        return(FM_ERASE_ERROR);
    }
    
    /* for asynchronous devices, sync should also be performed here */
    if (pdev->DevOps.Sync != NULL &&
        pdev->DevOps.Sync(pdev->LocalChipID) != FM_SUCCESS) {
        if (pdev->PrevOp != NULL) pdev->PrevOp->Command = OP_NONE;
        return(FM_ERASE_ERROR);
    }
    
    return(FM_SUCCESS);
}


static UINT32
calc_checksum(UINT32 *data, UINT16 size)
{
    UINT32 checksum = 0;

    for ( ; size > 0; size -= 4, data++) {
        checksum += *data;
    }
    
    return(checksum);
}


static BOOL
is_all_FF(UINT8 *buf, UINT32 size)
{
    UINT32 i;
    
    for (i = 0; i < size; i++) {
        if (buf[i] != 0xFF) return(FALSE);
    }
    
    return(TRUE);
}


static INT32
no_free_block_panic(PFDEV *pdev, BM_MOD *mod)
{
#if USE_BAD_BLOCK_REVIVAL
    if (bad_block_revival(pdev, mod) == FM_SUCCESS) return(FM_TRY_AGAIN);
#endif

#if 0
    if (pdev->ChipID == 0) {
        /* if this device corresponds to chip 0, halt the system */
        while (1);
    }
#endif

    return(FM_ERROR);
}


#if USE_BAD_BLOCK_REVIVAL

static INT32    
bad_block_revival(PFDEV *pdev, BM_MOD *mod)
{
    UINT16 i, j;
    UINT32 block;
    INT32  err;
    
    /* starting from the first entry of the swap table,
       find a candidate for bad block revival */
    for (i = 0; i < pdev->BmAreaNumBlocks; i++) {
        
        /* candidates for bad block revival are those that have 
           been detected as bad during the write operation */
        if (IS_SWAP_ENTRY(mod->SwapTable[i])) {
            if (((mod->SwapTable[i] & OP_INFO_MASK) >> OP_INFO_SHIFT) == OP_WRITE) {
            
                /* a candidate is found among the replaced bad blocks;
                   get the block number and try to erase it first */
                block = mod->SwapTable[i] & BLK_NUMBER_MASK;
erase_block:
                if (flash_erase(pdev, block) != FM_SUCCESS) continue;

                /* erase succeeded; this block revives and can re-replace
                   the replacement block now; copy the valid pages before 
                   the re-replacement */
                for (j = 0; j < pdev->DevSpec.PagesPerBlock; j++) {
                    err = bm_copy_page(pdev, GET_BLK_NUMBER(i), block, j);
                    if (err) {
                        /* write failed; erase this block and copy again */
                        goto erase_block;
                    }
                }

                /* the replacement block is free to use now */
                mod->SwapTable[i] = ENTRY_FREE_BLOCK;

                /* re-construct the swap table hash lists */
                construct_fast_lookup_table(pdev, mod);
                
                /* the updated swap table needs not to be saved in flash 
                   memory because it will be done in the caller function */
                return(FM_SUCCESS);
            }
        }
        else {
            if ((mod->SwapTable[i] & BLK_NUMBER_MASK) == ENTRY_BAD_BLOCK &&
                ((mod->SwapTable[i] & OP_INFO_MASK) >> OP_INFO_SHIFT) == OP_WRITE) {
                
                /* a candidate is found in the DLBM area;
                   get the block number and try to erase it first */
                block = GET_BLK_NUMBER(i);
                if (flash_erase(pdev, block) != FM_SUCCESS) continue;
                
                /* erase succeeded; this block revives and 
                   can be put back to the free block list */
                mod->SwapTable[i] = ENTRY_FREE_BLOCK;

                /* re-construct the swap table hash lists */
                construct_fast_lookup_table(pdev, mod);
                
                /* the updated swap table needs not to be saved in flash 
                   memory because it will be done in the caller function */
                return(FM_SUCCESS);
            }
        }
    }

    return(FM_ERROR);
}

#endif /* USE_BAD_BLOCK_REVIVAL */
#endif /* USE_DLBM */
