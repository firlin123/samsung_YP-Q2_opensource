#include "spiritmp3decoder.h"

typedef struct decoder_fptr
{

void (*SpiritMP3DecoderInit)(
    TSpiritMP3Decoder *pDecoder,                ///< Decoder structure
    fnSpiritMP3ReadCallback* pCallbackFn,       ///< Data reading callback function
//    void * token                                ///< Optional parameter for callback function
        int * token                                ///< Optional parameter for callback function
    );

/**
    Decoding function
    return number of audio samples decoded.
    NOTE: function always produce stereo data (1 sample = 32 bit = 2ch*16 bit).
*/
unsigned int (*SpiritMP3Decode) (
    TSpiritMP3Decoder *pDecoder,                ///< Decoder structure
    short *pPCMSamples,                         ///< [OUT] Output PCM buffer
    unsigned int nSamplesRequired,              ///< Number of samples to decode (1 sample = 32 bit = 2ch*16 bit)
    TSpiritMP3Info * pMP3Info                   ///< [OUT, opt] Optional informational structure
    );
    
}Dec_fptr;

typedef int(*pdecoder)(Dec_fptr *dec_fptr);


