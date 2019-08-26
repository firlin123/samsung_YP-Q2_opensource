
typedef unsigned long tWMA_U32;
typedef int * tHWMAFileState;
typedef int bool;

typedef enum _asi_StreamStatus
{
   STREAM_OFF=0,
   STREAM_ON=1
} asi_StreamStatus_t;

typedef enum _asi_AudioFileType
{
   AUDIO_FILE_TYPE_UNKNOWN=0,
   AUDIO_FILE_TYPE_MP3,
   AUDIO_FILE_TYPE_WMA,
   AUDIO_FILE_TYPE_PCM,
   AUDIO_FILE_TYPE_IMAADPCM,
   AUDIO_FILE_TYPE_MSADPCM,
   AUDIO_FILE_TYPE_OGG,
   /* COUNT must be last */
   /* Used for the Decoder File Format Interface indexing */
   AUDIO_FILE_TYPE_COUNT
} asi_AudioFileType_t;

typedef enum _asi_ChannelMix
{
   STEREO_MIX=0,    // Mix the stream to stereo
   LEFT_ONLY,       // Mix the stream to the LEFT channel
   RIGHT_ONLY       // Mix the stream to the RIGHT channel
} asi_ChannelMix_t;

typedef void *TX_QUEUE;
typedef void *TX_SEMAPHORE;
typedef void *TX_THREAD;
typedef void *TX_MUTEX;

/* Defines mainly buffer properties */
typedef struct _asi_StreamParameters
{
   asi_StreamStatus_t  eStreamStatus;   /*0:STREAM_OFF(disabled),1:STREAM_ON(enabled) */
   asi_AudioFileType_t eStreamType;     /* MP3, WMA, IMA ADPCM, PCM, UNKNOWN, etc. */
   bool           bVbr;                 /* Is the encoding VBR (1) or CBR (0) ? */
   bool           bValidVbrHeader;      /* Is the VBR header valid ? */
   void           *pBufferBaseAddr;     /* start address of stream buffer */
   void           *pBufferEndAddr;      /* ending address of stream buffer (Base + size) */
   int32_t        i32BufferSize;        /* buffer size in bytes, must be a multiple of 4 (alignment reason) */
   /* pDataHead is written by the module FILLING the buffer */
   void           *pDataHead;           /* source routine begins writing at this point */
   /* the block (decoder or effect) processing this buffer changes these each time */
   int32_t        i32DataRequired;      /* TOTAL bytes needed for next call */
   void           *pDataTail;           /* sink routine begins reading from this point */
   int32_t        i32DataOffset;        /* for seeking (DECODER writes it, SOURCE clears it) */
   int32_t        i32SeekPosition;      /* Sets Reference Point for File Seek (SEEK_SET, SEEK_CUR, or SEEK_END) */
      /* stream properties - filled by decoder usually one time per file (exception, vbr) */
   uint32_t       u32SampleRate;        /* frequency of audio stream in samples/second (Hz) */
   uint32_t       u32BitRate;           /* bits per second */
   uint32_t       u32AvgBitRate;        /* bitrate averaged over all song frames (vbr). 0 if unknown */
   int32_t        i32NumChannels;       /* 1 = Mono, 2 = Stereo */
   asi_ChannelMix_t eChannelMix;        /* 0 = Mix this stream to Stereo, 1 = Mix to LEFT channel, 2 = Mix to RIGHT channel */
   int32_t        i32NumBitsPerSample;  /* Use for recording? */
   int32_t        i32Interleaved;       /* 0:Channels interleaved */
   int32_t        i32ChannelOffsets;    /* If interleaved, how many samples in between. */
   int32_t        i32HeaderOffset;      /* Offset to get to beginning of data in file. */
   struct _asi_StreamModule *pstInputModule;    /* ptr to the input stream module */
   struct _asi_StreamModule *pstOutputModule;   /* ptr to the output stream module */
   uint32_t       u32NewSamplingRate;
} asi_StreamParameters_t;

typedef struct _asi_Streams
{
   uint32_t               u32NumStreams;    /* Number of streams at this node */
   asi_StreamParameters_t *pStreams;        /* Ptr to an array of ptrs to */
                                            /* the stream parameters */
} asi_Streams_t;

typedef struct _asi_StreamModule
{
   asi_Streams_t    InputStream;        /* Input streams for this module */
   asi_Streams_t    OutputStream;       /* Output streams for this module */
   uint32_t         u32MinInputFrame;   /* Min input frame needed to process */
   uint32_t         u32MinOutputSpace;  /* Min num of bytes to output a frame */
   TX_QUEUE         *pQueCtrlBlk;       /* Msg Queue for module communication */
   TX_SEMAPHORE     *pSemCtrlBlk;       /* Binary semaphore to signal module */
   TX_THREAD        *pThrCtrlBlk;       /* Task associated with this Module */
   TX_MUTEX         *pMutexCtrlBlk;     /* Mutex (if needed) for this Module */
   void             *piDataStorage;     /* Ptr. to user-defined data storage */
//   pfnAsiAsmCallbackFunction_t *pfnCallback;
//   void             (*pGetProperty)();  /* Get Property pointer */
//   void             (*pSetProperty)();  /* Set Property pointer */
   int SamplestoDecode;					/* Samples to decode */
   int ContinuationFlag;				/* Continuation Flag */
} asi_StreamModule_t;


typedef struct decoder_fptr
{   
	int (* Wma_Decode)(asi_StreamParameters_t *pOutputParams,unsigned char **ppSinkHeadPtr,int *pi32BytesWritten);
	int (*initWmaDecoder)(asi_StreamModule_t *pstDecStrmMod, unsigned char *write_base_addr, 
						  unsigned char *write_ptr, int write_buf_size);
	void (*SetPmid)(int length, char * g_pmid_array);
	unsigned long (* FileSeek)(int32_t);
	
	tWMA_U32 (* WMAFileCBGetData)(tHWMAFileState *state,tWMA_U32 offset,tWMA_U32 num_bytes,unsigned char **ppData);
	int32_t (* CopySinkData)(asi_StreamParameters_t *pstParameters,uint8_t *pAlgorithOutputBuffer,const int32_t size, const int32_t bufferOffset);
	int32_t (* asi_util_GetFreeSpace)(asi_StreamParameters_t *StreamParams);

}Dec_fptr_t;

typedef int(*pdecoder)(Dec_fptr_t *Dec_fptr);
