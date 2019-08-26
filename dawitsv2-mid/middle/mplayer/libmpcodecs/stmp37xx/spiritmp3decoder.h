/******************************************************************************

  MPEG-1,2,2.5 Layer 3 (mp3) decoder.
  (C) Spirit corp. Moscow 2000-2002.
  http://www.spiritcorp.com

  File Name:      SpiritMP3Decoder.h

*//** @file

  MP3 Decoder top-level functions.

*//*

  Revision History:

  1.0.0   07/24/2002   ASP     Initial Version

******************************************************************************/
#ifndef SPIRITMP3DECODER_H
#define SPIRITMP3DECODER_H

#ifdef __cplusplus
extern "C" {
#endif  //__cplusplus


/**
    Informational structure to provide information about MP3 stream.
*/
typedef struct
{
    unsigned int nLayer;                        ///< MPEG Audio Layer (1-3). Zero indicates that structure is not valid
    unsigned int nSampleRateHz;                 ///< Sample rate, Hz
    unsigned int nBitrateKbps;                  ///< Current bit rate, kilobit per seconds (0 for free-format stream)
    unsigned int nChannels;                     ///< Number of channels (1 or 2)
    unsigned int IsGoodStream;                  ///< Zero indicates that audio stream is not consistent
    unsigned int anCutOffFrq576[2];             ///< Cut-off frequencies for both channels (range 0-576), where 576 = Nyquist frequency
    unsigned int nBitsReadAfterFrameHeader;     ///< Number of bits, read after last frame header (for AV sync)
    unsigned int nSamplesPerFrame;              ///< Number of samples per audio frame (for AV sync)
    unsigned int nSamplesLeftInFrame;           ///< Number of samples, remaining in the internal buffer (for AV sync)
} TSpiritMP3Info;


/**
    MP3 decoder object (persistent RAM).
*/
typedef struct
{
    //int hidden[3136];                           ///< Structure contents is hidden for demo version
    int hidden[3496];                           ///< Structure contents is hidden for demo version
} TSpiritMP3Decoder;


/**
    Callback function type to supply decoder with input data.
    return number of MAU's read into buffer.
*/
typedef unsigned int (fnSpiritMP3ReadCallback)(
	void * pMP3CompressedData,                  ///< [OUT] Pointer to buffer to fill with coded MP3 data
	unsigned int nMP3DataSizeInChars,           ///< Buffer size in MAU's
	void * token                                ///< Application-supplied token
	);


/**
   Decoder initialization function
*/
void SpiritMP3DecoderInit(
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
unsigned int SpiritMP3Decode (
    TSpiritMP3Decoder *pDecoder,                ///< Decoder structure
    short *pPCMSamples,                         ///< [OUT] Output PCM buffer
    unsigned int nSamplesRequired,              ///< Number of samples to decode (1 sample = 32 bit = 2ch*16 bit)
    TSpiritMP3Info * pMP3Info                   ///< [OUT, opt] Optional informational structure
    );


//extern TSpiritMP3Decoder  mp3_Decoder;  // Decoder object 
//extern TSpiritMP3Info     mp3_Info;     // MP3 audio information 


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //#ifndef SPIRITMP3DECODER_H
