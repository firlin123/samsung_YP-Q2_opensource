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

#include "mcw_sigmatel_mp4v_decoder.h"

static vd_info_t info = {
	"STMP37xx video codecs",
	"stmpeg4",
	NULL,
	NULL,
	NULL
};

LIBVD_EXTERN(stmpeg4)

static int iNumMemReq;
static void *pvHandleCodec;

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
	MCW_MP4VDEC_MEM_REQ        *pMemReq;
	MCW_MP4VDEC_INIT_IN        InitIn;
	MCW_MP4VDEC_INIT_OUT       InitOut;
	unsigned char *aapbyFrameBuff[MCW_MP4V_DEC_MIN_NUM_FRAMEBUF][3];
	vf_instance_t* vf;
	int i;

	// output format check
	switch (sh->codec->outfmt[sh->outfmtidx])
	{
		case IMGFMT_YV12:
			break;
		default:
			mp_msg(MSGT_DECVIDEO,MSGL_DBG2,"Not supported output format\n");
			return 0;
	}

	// CSC filter should be opened first to get YUV buffers
	// YUV buffer pointers are store in vf->priv structrues
	// vf->priv->y[3] vf->priv->u[3] vf->priv->v[3]
	sh->vfilter = vf = vf_open_filter(sh->vfilter,"csc",NULL);
	if (vf == NULL)
	{
		return 0;
	}

	{
		MCW_MP4VDEC_VERSION  Version;
		MCW_SIGMATEL_MP4VDEC_GetVersion(&Version);
		mp_msg(MSGT_DECVIDEO,MSGL_DBG2, "SigmaTel MPEG4 Decoder Version %d.%d.%d\n",
			 Version.iMajor, Version.iMinor, Version.iPatch);
	}

	// MCW lib. init
	for (i=0; i<MCW_MP4V_DEC_MIN_NUM_FRAMEBUF; i++)	 // frame buffer
	{
		aapbyFrameBuff[i][0] = vf->priv->y[i];
		aapbyFrameBuff[i][1] = vf->priv->u[i];
		aapbyFrameBuff[i][2] = vf->priv->v[i];
	}

	InitIn.iShortHeader = 0;			// 0:MPEG4, 1:H.263;
	InitIn.pbyStreamBase = NULL;
	InitIn.uStreamLength = 0;
	InitIn.uNumFrmBuffer = MCW_MP4V_DEC_MIN_NUM_FRAMEBUF;

	InitIn.papbyFrameBuff = aapbyFrameBuff;
	InitIn.uMaxFrameWidth  = MAX_FRAME_WIDTH;
	InitIn.uMaxFrameHeight = MAX_FRAME_HEIGHT;
	InitOut.pchDbgReport = NULL;		//release mode, no debug report

	//=API FUNCTION================================================================
	// @ This function returns the number of Memory Blocks
	// The caller must allocate memory to Decoder Memory
	// by delivering structure MCW_MP4VDEC_MEM_REQ
	iNumMemReq = MCW_SIGMATEL_MP4VDEC_GetNumMemBlocks();
	//==============================================================================
	pMemReq = (MCW_MP4VDEC_MEM_REQ*)malloc(iNumMemReq * sizeof(MCW_MP4VDEC_MEM_REQ)); // Memory Blocks

	//=API FUNCTION================================================================
	// @ This function returns the size of each Memory Block
	MCW_SIGMATEL_MP4VDEC_GetMemRequirement(&InitIn, pMemReq);
	//==============================================================================

	//=============================================================================
	// allocates memory for memory tablets
	for (i = 0; i < iNumMemReq; i++)
		pMemReq[i].pvBase = malloc(pMemReq[i].uSize);
	//=============================================================================

	//=API FUNCTION================================================================
	// @ This function
	// 1) initializes the decoder with separate configuration if provided by caller
	// 2) initializes all the internal memory pointers using pMemReq->pvBase
	// 3) returns the main decoder handle
	pvHandleCodec = MCW_SIGMATEL_MP4VDEC_Init(pMemReq, &InitIn, &InitOut);

	//=============================================================================
	free(pMemReq);
	if (pvHandleCodec == NULL)
	{
		for (i = 0; i < iNumMemReq; i++)
			free(pMemReq[i].pvBase);

		mp_msg(MSGT_DECVIDEO, MSGL_ERR, "SigmaTel MPEG4 decoder init error\n");
		return 0;
	}

	mp_msg(MSGT_DECVIDEO,MSGL_DBG2, "SigmaTel MPEG4 decoder init OK\n");

	return mpcodecs_config_vo(sh, sh->disp_w, sh->disp_h, IMGFMT_YV12);
}

static void uninit (sh_video_t *sh)
{
	MCW_MP4VDEC_MEM_REQ        *pMemReq;
	int i;

	pMemReq = (MCW_MP4VDEC_MEM_REQ*)malloc(iNumMemReq * sizeof(MCW_MP4VDEC_MEM_REQ)); // Memory Blocks

	//=API FUNCTION================================================================
	// @ This function delivers the address of memory blocks for free
	MCW_SIGMATEL_MP4VDEC_GetMemBaseForFree(pvHandleCodec, pMemReq);
	//=============================================================================
	for (i = 0; i < iNumMemReq; i++)
		free(pMemReq[i].pvBase);
	free(pMemReq);

	pvHandleCodec = NULL;
}

static mp_image_t* decode (sh_video_t *sh, void* data, int len, int flags)
{
	MCW_MP4VDEC_DECODE_FRM_IN  DecodeIn;
	MCW_MP4VDEC_DECODE_FRM_OUT DecodeOut;

	int iConsumedBytes;
	mp_image_t* mpi;

	DecodeIn.pbyStreamBase      = data;
	DecodeIn.uStreamLength      = len;			//??? 0 : flush the last frame
	DecodeIn.iError1stPos       = -1;
	DecodeIn.iErrorBrokenPred   = 0;
	DecodeIn.iUsePostFilter     = 0;
	DecodeOut.pchDbgReport      = NULL;

	//=API FUNCTION================================================================
	// @ This function
	// 1) decodes one frame
	// 2) detects bitstream errors
	// 3) reconstructs one decoded frame
	// 4) informs application of consumed byte
	iConsumedBytes = MCW_SIGMATEL_MP4VDEC_DecodeFrame(pvHandleCodec, &DecodeIn, &DecodeOut);

	//==============================================================================
	if (iConsumedBytes && DecodeOut.iFrmReliable_0_100 >= 80)
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



int SM_GetPostprocInfo (void)
{
    return 0;
}
