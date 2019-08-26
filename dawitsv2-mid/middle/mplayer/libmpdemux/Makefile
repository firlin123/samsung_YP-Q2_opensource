include ../../../Rules.mk

include ../config.mak

LIBNAME_COMMON   = libmpdemux.a
LIBNAME_MENCODER = libmpmux.a

ifeq ($(STMP_BUILD),no)
SRCS_COMMON = \
              demux_aac.c \
              demux_film.c \
              demux_fli.c \
              demux_lmlm4.c \
              demux_mf.c \
              demux_mkv.c ebml.c \
              demux_mov.c \
              demux_mpg.c \
              demux_nsv.c \
              demux_nuv.c \
              demux_pva.c \
               demux_rawvideo.c \
              demux_realaud.c \
              demux_real.c \
              demux_roq.c \
              demux_smjpeg.c \
              demux_ts.c \
              demux_ty.c \
              demux_ty_osd.c \
              demux_viv.c \
              demux_vqf.c \
              demux_y4m.c \
              extension.c \
              mf.c \
              mpeg_packetizer.c \
              parse_mp4.c \
              yuv4mpeg.c \
              yuv4mpeg_ratio.c \
              demux_audio.c \
              mp3_hdr.c \

endif

SRCS_COMMON += \
              aviheader.c \
              aviprint.c \
              demuxer.c \
              demux_avi.c \
              demux_demuxers.c \
              extension.c \
              mpeg_hdr.c \
              video.c \
              parse_es.c \
			  asfheader.c \
              demux_asf.c \
			  
SRCS_COMMON-$(GIF)                     += demux_gif.c
SRCS_COMMON-$(LIBAVFORMAT)             += demux_lavf.c
SRCS_COMMON-$(LIBDV)                   += demux_rawdv.c
SRCS_COMMON-$(LIBNEMESI)               += demux_nemesi.c
SRCS_COMMON-$(LIBNUT)                  += demux_nut.c
SRCS_COMMON-$(LIBVORBIS)               += demux_ogg.c
SRCS_COMMON-$(MUSEPACK)                += demux_mpc.c
SRCS_COMMON-$(STREAMING_LIVE555)       += demux_rtp.cpp demux_rtp_codec.cpp
SRCS_COMMON-$(WIN32DLL)                += demux_avs.c
SRCS_COMMON-$(XMMS_PLUGINS)            += demux_xmms.c

SRCS_MENCODER = muxer.c \
                muxer_avi.c \
                muxer_mpeg.c \
                muxer_rawaudio.c \
                muxer_rawvideo.c \

SRCS_MENCODER-$(LIBAVFORMAT)           += muxer_lavf.c

include ../mpcommon.mak

dep depend: CFLAGS += $(EXTRAXX_INC)
