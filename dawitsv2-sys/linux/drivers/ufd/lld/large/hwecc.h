#ifndef _HWECC_H_
#define _HWECC_H_

#define HWECC_SUCCESS		0
#define HWECC_ALL_ONES		1
#define HWECC_ALL_ZEROS		2
#define HWECC_CORR_ERROR	3
#define HWECC_UNCORR_ERROR	4
#define HWECC_DMA_ERROR		5


#define HWECC_PREV_OP_NONE	0
#define HWECC_PREV_OP_ENCODE	1
#define HWECC_PREV_OP_DECODE	2

typedef struct {
	int chip;
	int block; 
	int page; 
	int offset;
	int nsect;

	u8 op; 
	u8 *dbuf, *sbuf; 
} prev_ecc_t; 

extern prev_ecc_t prev_ecc;

unsigned char *init_hwecc_descriptors(unsigned char *buf, int left); 

int  hwecc_ecc_encode(u16 num_sectors, u8 *dbuf, u8 *sbuf);
int  hwecc_ecc_decode(u16 num_sectors, u8 *dbuf, u8 *sbuf);
#if CFG_HWECC_PIPELINE
int  hwecc_ecc_decode_sync(void); 
#endif 

#define	ECC_RFS_LAYOUT  0
#define	ECC_STMP_LAYOUT 1 
#define ECC_NONE_LAYOUT 2

int    get_ecc_layout(void);
void   set_ecc_layout(int layout);
#endif	// _HWECC_H_
