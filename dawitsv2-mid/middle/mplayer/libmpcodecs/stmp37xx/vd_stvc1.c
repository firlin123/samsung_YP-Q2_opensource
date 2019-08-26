#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "config.h"

#include "mp_msg.h"
#include "help_mp.h"

#include "vd_internal.h"
#include "vf.h"
#include "vf_csc.h"
#include "vd_stcommon.h"

#include "mcw_sigmatel_wmv9dec.h"

#define MAX_MEM_REQ_VC1		3

static vd_info_t info = {
	"STMP37xx video codecs",
	"stvc1",
	NULL,
	NULL,
	NULL
};

LIBVD_EXTERN(stvc1)

struct sequence_layer
{
   struct
   {
       uint32_t u24NumberOfCompressedFrames : 24;
       uint32_t u8Reserved : 8;
   };
   uint32_t u32Reserved1;
   char 	ShcValBE[4];

   /* STRUCT_SEQUENCE_HEADER_A */
   uint32_t VERT_SIZE;
   uint32_t HORIZ_SIZE;

   uint32_t u32Reserved2;

   /* STRUCT_SEQUENCE_HEADER_B shb */
   struct
   {
       uint32_t HRD_BUFFER : 24;
       uint32_t RES1 : 4;
       uint32_t CBR : 1;
       uint32_t LEVEL : 3;
   };
   uint32_t HRD_RATE;
   uint32_t FRAMERATE;
} ;

static void *pvHandleCodec;
static VIDDEC_MCW_MEM_REQ	MemReq[MAX_MEM_REQ_VC1];

// to set/get/query special features/parameters
static int control (sh_video_t *sh, int cmd, void* arg,...)
{
/*
	switch (cmd)
     {
         case VDCTRL_QUERY_FORMAT:
         {
             int format =(*((int*)arg));
             switch (format)
             {
				 case IMGFMT_YV12:   return CONTROL_TRUE;
				 default:            return CONTROL_FALSE;
             }
		 }
     }
*/
    return CONTROL_UNKNOWN;
}

// init driver
static int init (sh_video_t *sh)
{
	VIDDEC_MCW_INIT_IN	InitIn;
	VIDDEC_MCW_INIT_OUT	InitOut;
	WMV9DEC_MCW_INIT_IN         stInitInSpecWMV9;
	WMV9DEC_MCW_INIT_OUT        stInitOutSpecWMV9;

	vf_instance_t* vf;
	unsigned char *aapbyFrameBuff[4/*MAX_MEM_REQ_VC1*/][3];
	struct sequence_layer seq_layer;
	int i, ret;


	// output format check
	switch (sh->codec->outfmt[sh->outfmtidx]) {
		case IMGFMT_YV12:
			break;
		default:
			mp_msg(MSGT_DECVIDEO,MSGL_V,"Not supported output format\n");
			return 0;
	}

	// CSC filter should be opened first to get YUV buffers
	// YUV buffer pointers are store in vf->priv structrues
	// vf->priv->y[3] vf->priv->u[3] vf->priv->v[3]
	sh->vfilter = vf = vf_open_filter(sh->vfilter,"csc",NULL);
	if (vf == NULL) {
		return 0;
	}

	// Printout version info
	{
		VIDDEC_MCW_VERSION  Version;
		MCW_SIGMATEL_WMV9DEC_GetVersion(&Version);
		mp_msg(MSGT_DECVIDEO,MSGL_INFO, "SigmaTel WMV9 Decoders Version %d.%d.%d\n",
			 Version.iMajor, Version.iMinor, Version.iPatch);
	}

	// MCW lib. init
	// frame buffer
	for (i = 0; i < 4/*MAX_MEM_REQ_VC1*/; i++) {
		aapbyFrameBuff[i][0] = vf->priv->y[i];
		aapbyFrameBuff[i][1] = vf->priv->u[i];
		aapbyFrameBuff[i][2] = vf->priv->v[i];
	}

	// setup sequence layer
	memset(&seq_layer, 0, sizeof(seq_layer));
	seq_layer.u8Reserved = 0xc5;
	seq_layer.u32Reserved1 = 0x04;
	seq_layer.u32Reserved2 = 0x0c;
	// Bitmap Info Header extra data
	memcpy(seq_layer.ShcValBE, (void*)(sh->bih + 1), sizeof(seq_layer.ShcValBE));
	seq_layer.HORIZ_SIZE = sh->disp_w;
	seq_layer.VERT_SIZE = sh->disp_h;

	stInitInSpecWMV9.pbySeqInfo   = (unsigned char*)&seq_layer;
	stInitInSpecWMV9.dwSeqHdrSize = sizeof(seq_layer);

	#if 0
		fprintf(stderr, "WMV9 specific info size - %ld\n[ ", stInitInSpecWMV9.dwSeqHdrSize);
		for (i = 0; i < stInitInSpecWMV9.dwSeqHdrSize; i++) {
			fprintf(stderr, "%x ", stInitInSpecWMV9.pbySeqInfo[i]);
		}
		fprintf(stderr, "]\n");
	#endif

	InitIn.pvInitInSpecific = (void *)(&stInitInSpecWMV9);

	ret = MCW_SIGMATEL_WMV9DEC_ParseVidConfig(&InitIn, &InitOut);
	if (ret) {
		mp_msg(MSGT_DECVIDEO, MSGL_ERR, 
			   "SigmaTel VC1 decoder error(%d) - MCW_SIGMATEL_WMV9DEC_ParseVidConfig\n", ret);
		return 0;
	}
	InitIn.uDecodableMaxWidth  = MAX_FRAME_WIDTH; //InitOut.uDispWidth;
	InitIn.uDecodableMaxHeight = MAX_FRAME_HEIGHT; //InitOut.uDispHeight;
	InitIn.papbyFrmBuffer = aapbyFrameBuff;
	InitIn.uNumFrmBuffer = 4; //MAX_MEM_REQ_VC1;

	ret = MCW_SIGMATEL_WMV9DEC_GetMemRequirement(&InitIn, MemReq);
	if (ret != MAX_MEM_REQ_VC1) {
		mp_msg(MSGT_DECVIDEO, MSGL_ERR, 
			   "SigmaTel VC1 decoder error(%d) - MCW_SIGMATEL_WMV9DEC_GetMemRequirement\n", ret);
		return 0;
	}

	//=============================================================================
	// allocates memory for memory tablets
	for (i = 0; i < MAX_MEM_REQ_VC1; i++) {
		MemReq[i].pvBase = malloc(MemReq[i].uSize);
	}

	//=============================================================================
	InitOut.pvInitOutSpecific = (void *)(&stInitOutSpecWMV9);
	stInitOutSpecWMV9.uRcvNumFrames = 0;


	//=API FUNCTION================================================================
	// @ This function
	// 1) initializes the decoder with separate configuration if provided by caller
	// 2) initializes all the internal memory pointers using pMemReq->pvBase
	// 3) returns the main decoder handle
	pvHandleCodec = MCW_SIGMATEL_WMV9DEC_Init(MemReq, &InitIn, &InitOut);

	//=============================================================================
	if (pvHandleCodec == NULL)
	{
		for (i = 0; i < MAX_MEM_REQ_VC1; i++)
			free(MemReq[i].pvBase);

		mp_msg(MSGT_DECVIDEO, MSGL_ERR, "SigmaTel VC1 decoder init error (%d)\n", stInitOutSpecWMV9.iErrorCode);
		return 0;
	}

	mp_msg(MSGT_DECVIDEO,MSGL_V, "SigmaTel VC1 decoder init OK\n");

	return mpcodecs_config_vo(sh, sh->disp_w, sh->disp_h, IMGFMT_YV12);
}

static void uninit (sh_video_t *sh)
{
	int i;

	//=API FUNCTION================================================================
	// @ This function delivers the address of memory blocks for free
	//MCW_SIGMATEL_WMV9DEC_GetMemBaseForFree(pvHandleCodec, MemReq);
	//=============================================================================
	for (i = 0; i < MAX_MEM_REQ_VC1; i++)
		free(MemReq[i].pvBase);

	pvHandleCodec = NULL;
}

static mp_image_t* decode (sh_video_t *sh, void* data, int len, int flags)
{
	VIDDEC_MCW_DECFRM_IN	DecodeIn;
	VIDDEC_MCW_DECFRM_OUT	DecodeOut;
	WMV9DEC_MCW_DECFRM_IN	stDecFrmInSpecWMV9;
	int ret;
	mp_image_t* mpi;

    DecodeIn.pvDecFrmInSpecific = (void *)(&stDecFrmInSpecWMV9);
	DecodeIn.pbyFrmBStream      = data;
	DecodeIn.lFrmBStreamLeng    = len;			//??? 0 : flush the last frame
	DecodeIn.iErrorFirstPos		= -1;
	DecodeIn.iFrmDispLevel		= 0;
	DecodeIn.iErrorBrokenPred   = 0;

	//=API FUNCTION================================================================
	// @ This function
	// 1) decodes one frame
	// 2) detects bitstream errors
	// 3) reconstructs one decoded frame
	// 4) informs application of consumed byte
	ret = MCW_SIGMATEL_WMV9DEC_DecodeFrame(pvHandleCodec, &DecodeIn, &DecodeOut);

	//==============================================================================
	if (ret == 0)		//decoding SUCCESS
	{
		mpi = mpcodecs_get_image(sh, MP_IMGTYPE_EXPORT, 0, sh->disp_w, sh->disp_h);
		if (!mpi)
			return NULL;

		mpi->planes[0] = DecodeOut.apbyFrmRecon[0];
		mpi->stride[0] = DecodeOut.uDispWidth;
		mpi->planes[1] = DecodeOut.apbyFrmRecon[1];
		mpi->stride[1] = DecodeOut.uDispWidth/2;
		mpi->planes[2] = DecodeOut.apbyFrmRecon[2];
		mpi->stride[2] = DecodeOut.uDispWidth/2;
	}
	else
		return NULL;  

	return mpi;
}

