/* 
 * Virtual codec device
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved 
 *
 * Authors:
 *  Kitae KIM kt920.kim@samsung.com
 *
 * PROPRIETARY/CONFIDENTIAL
 * This software is the confidential and proprietary information of SAMSUNG ELECTRONICS ("Confidential Information").  
 * You shall not disclose such Confidential Information and shall use it only in accordance with the terms of the license agreement 
 * you entered into with SAMSUNG ELECTRONICS.  SAMSUNG make no representations or warranties about the suitability 
 * of the software, either express or implied, including but not limited to the implied warranties of merchantability, fitness for 
 * a particular purpose, or non-infringement. SAMSUNG shall not be liable for any damages suffered by licensee as 
 * a result of using, modifying or distributing this software or its derivatives.
 */

#include "codec_accel.h"

#define QEMU_DEV_NAME			"codec"
#define SVCODEC_MEM_SIZE		(4 * 1024 * 1024)
#define SVCODEC_REG_SIZE		(256)

#define CODEC_THREAD

/* define debug channel */
MULTI_DEBUG_CHANNEL(qemu, svcodec);

enum {
	FUNC_NUM 			= 0x00,
	IN_ARGS 			= 0x04,
	RET_STR 			= 0x08,
	READY_TO_GET_DATA	= 0x0C,
	COPY_RESULT_DATA	= 0x10,
};

static int codec_operate(uint32_t value, SVCodecState *opaque);

SVCodecParam *target_param;

static int thread_cnt = 0;
AVCodecContext *gAVCtx = NULL;
AVFrame *gFrame = NULL;

/* void av_register_all() */
void qemu_av_register_all (void)
{
	TRACE("%s cnt:%d\n", __func__, ++thread_cnt);
	av_register_all();
}

int qemu_avcodec_get_buffer (AVCodecContext *context, AVFrame *picture)
{
	int ret;

	picture->reordered_opaque = context->reordered_opaque;
	picture->opaque = NULL;

	ret = avcodec_default_get_buffer(context, picture);
	TRACE("after avcodec_default_get_buffer, return value:%d,"
		  "internal_buffer_count:%d\n", ret, context->internal_buffer_count);

	return ret;
}

void qemu_avcodec_release_buffer (AVCodecContext *context, AVFrame *picture)
{

	avcodec_default_release_buffer(context, picture);
	TRACE("after avcodec_default_release_buffer," 
		  "internal_buffer_count:%d\n", context->internal_buffer_count);
}

void codec_set_context (AVCodecContext *dstctx,
						AVCodecContext *srcctx) {
	dstctx->av_class = srcctx->av_class;
	dstctx->extradata = srcctx->extradata;
	dstctx->codec = srcctx->codec;
	dstctx->priv_data = srcctx->priv_data;
	dstctx->opaque = srcctx->opaque;
	dstctx->get_buffer = srcctx->get_buffer;
	dstctx->release_buffer = srcctx->release_buffer;
	dstctx->stats_out = srcctx->stats_out;
	dstctx->stats_in = srcctx->stats_in;
	dstctx->rc_override = srcctx->rc_override;
	dstctx->rc_eq = srcctx->rc_eq;
	dstctx->slice_offset = srcctx->slice_offset;
	dstctx->get_format = srcctx->get_format;
	dstctx->internal_buffer = srcctx->internal_buffer;
	dstctx->intra_matrix = srcctx->intra_matrix;
	dstctx->inter_matrix = srcctx->inter_matrix;
	dstctx->reget_buffer = srcctx->reget_buffer;
	dstctx->execute = srcctx->execute;
	dstctx->thread_opaque = srcctx->thread_opaque;
	dstctx->execute2 = srcctx->execute2;
}


/* int avcodec_open (AVCodecContext *avctx, AVCodec *codec)	*/
int qemu_avcodec_open (SVCodecState *s)
{
	AVCodecContext *avctx;
	AVCodecContext tempCtx;
	AVCodec *codec;
	AVCodec tempCodec;
	enum CodecID codec_id;
//	bool bEncode;
	int ret;
	int size;

	TRACE("%s cnt:%d\n", __func__, ++thread_cnt);
	/* guest to host */
	if (!gAVCtx) {
		ERR("AVCodecContext is NULL!!\n");
		return -1;
	}
	avctx = gAVCtx;
	size = sizeof(AVCodecContext); 
	memcpy(&tempCtx, avctx, size);
	memcpy(avctx, s->vaddr, size);
	memcpy(&tempCodec, (uint8_t*)s->vaddr + size, sizeof(AVCodec));

	// restore AVCodecContext's pointer variables
	codec_set_context(avctx, &tempCtx);

	codec_id = tempCodec.id;
	TRACE("[%s][%d] CODEC ID : %d\n", __func__, __LINE__, codec_id);	
	size += sizeof(AVCodec);

	if (avctx->extradata_size > 0) {
		avctx->extradata = (uint8_t*)av_malloc(avctx->extradata_size);
		memcpy(avctx->extradata, (uint8_t*)s->vaddr + size, avctx->extradata_size);		
	} else {
		avctx->extradata = NULL;
	}

	if (tempCodec.encode) {
		codec = avcodec_find_encoder(codec_id);
	} else {
		codec = avcodec_find_decoder(codec_id);
	}

	avctx->get_buffer = qemu_avcodec_get_buffer;
	avctx->release_buffer = qemu_avcodec_release_buffer;
	
	ret = avcodec_open(avctx, codec);
	if (ret != 0) {
		ERR("Failed to open codec, %d\n", ret);
	}

	memcpy(s->vaddr, avctx, sizeof(AVCodecContext));
	memcpy((uint8_t*)s->vaddr + sizeof(AVCodecContext), &ret, sizeof(int));

	return ret;
}

/* int avcodec_close (AVCodecContext *avctx) */
int qemu_avcodec_close (SVCodecState* s)
{
	AVCodecContext *avctx;
	int ret = -1;
	
	TRACE("%s cnt:%d\n", __func__, ++thread_cnt);
	avctx = gAVCtx;
	if (!avctx) {
		ERR("[%s][%d] AVCodecContext is NULL\n", __func__, __LINE__);
		memcpy(s->vaddr, &ret, sizeof(int));
		return ret;
	}

	ret = avcodec_close(avctx);
	TRACE("after avcodec_close. ret:%d\n", ret);

	memcpy(s->vaddr, &ret, sizeof(int));

	return ret;
}

/* AVCodecContext* avcodec_alloc_context (void) */
void qemu_avcodec_alloc_context (void)
{
	TRACE("%s cnt:%d\n", __func__, ++thread_cnt);
	gAVCtx = avcodec_alloc_context();
}

/* AVFrame *avcodec_alloc_frame (void) */
void qemu_avcodec_alloc_frame (void)
{
	TRACE("%s cnt:%d\n", __func__, ++thread_cnt);
	gFrame = avcodec_alloc_frame();
}

/* void av_free (void *ptr) */
void qemu_av_free (SVCodecState *s)
{
	int value;
	TRACE("Enter %s\n", __func__);

	memcpy(&value, s->vaddr, sizeof(int));

	if (value == 1) {
        if (gAVCtx)
            av_free(gAVCtx);
        gAVCtx = NULL;
		TRACE("free AVCodecContext(%d)\n", value);
    } else if (value == 2) {
        if (gFrame)
            av_free(gFrame);
        gFrame = NULL;
		TRACE("free AVFrame(%d)\n", value);

    } else if (value == 3) {
        if (gAVCtx->palctrl)
            av_free(gAVCtx->palctrl);
        gAVCtx->palctrl = NULL;
		TRACE("free AVCodecContext palctrl(%d)\n", value);
    } else {
        if (gAVCtx->extradata && gAVCtx->extradata_size > 0)
            av_free(gAVCtx->extradata);
        gAVCtx->extradata = NULL;
		TRACE("free AVCodecContext extradata(%d)\n", value);
    }

	TRACE("Leave %s\n", __func__);
}

/* void avcodec_get_context_defaults (AVCodecContext *ctx) */
void qemu_avcodec_get_context_defaults (void)
{
	AVCodecContext *avctx;

	TRACE("Enter : %s\n", __func__);
	avctx = gAVCtx;
	if (avctx) {
		avcodec_get_context_defaults(avctx);
	} else {
		ERR("AVCodecContext is NULL\n");
	}
	TRACE("Leave : %s\n", __func__);
}

/* void avcodec_flush_buffers (AVCodecContext *avctx) */
void qemu_avcodec_flush_buffers (void)
{
	AVCodecContext *avctx;

	TRACE("Enter\n");

	avctx = gAVCtx;
	if (avctx) {
		avcodec_flush_buffers(avctx);
	} else {
		ERR("AVCodecContext is NULL\n");
	}
	TRACE("Leave\n");
}

/* int avcodec_default_get_buffer (AVCodecContext *s, AVFrame *pic) */
int qemu_avcodec_default_get_buffer (void)
{
	AVCodecContext *avctx;
	AVFrame *pic;
	int ret;

	TRACE("Enter\n");

	avctx = gAVCtx;
	pic = gFrame;
	if (avctx == NULL || pic == NULL) {
		ERR("AVCodecContext or AVFrame is NULL!!\n");
		return -1;
	} else {
		ret = avcodec_default_get_buffer(avctx, pic);
	}

	TRACE("Leave, return value:%d\n", ret);

	return ret;
}

/* void avcodec_default_release_buffer (AVCodecContext *ctx, AVFrame *frame) */
void qemu_avcodec_default_release_buffer (void)
{
	AVCodecContext *ctx;
	AVFrame *frame;

	TRACE("Enter\n");

	ctx = gAVCtx;
	frame = gFrame;
	if (ctx == NULL || frame == NULL) {
		ERR("AVCodecContext or AVFrame is NULL!!\n");
	} else {
		avcodec_default_release_buffer(ctx, frame);
	}

	TRACE("Leave\n");
}

static int iframe = 0;
/* int avcodec_decode_video (AVCodecContext *avctx, AVFrame *picture,
 * 							int *got_picture_ptr, const uint8_t *buf,
 * 							int buf_size)
 */
int qemu_avcodec_decode_video (SVCodecState *s)
{
	AVCodecContext *avctx;
	AVCodecContext tempCtx;
	AVFrame *picture;
	int got_picture_ptr = 0;
	uint8_t *buf = NULL;
	int buf_size = 0, size;
	int ret;

	avctx = gAVCtx;
	picture = gFrame;
	if (avctx == NULL | picture == NULL) {
		ERR("AVCodecContext or AVFrame is NULL!! avctx:0x%x, picture:0x%x\n", avctx, picture);
		return -1;
	}
	size = sizeof(AVCodecContext);

	memcpy(&tempCtx, avctx, size);
	memcpy(avctx, s->vaddr, size);
	codec_set_context(avctx, &tempCtx);

	memcpy(&buf_size, (uint8_t*)s->vaddr + size, sizeof(int));
	size += sizeof(int);

	if (buf_size > 0) {
		buf = (uint8_t*)av_malloc(buf_size * sizeof(uint8_t));
		memcpy(buf, (uint8_t*)s->vaddr + size, buf_size);
	} else {
		TRACE("There is no input buffer\n");
	}
	
	TRACE("before avcodec_decode_video\n");

	ret = avcodec_decode_video(avctx, picture, &got_picture_ptr, buf, buf_size);

	TRACE("after avcodec_decode_video, ret:%d, iframe:%d\n", ret, ++iframe);
	if (got_picture_ptr == 0) {
		TRACE("There is no frame\n");
	}

#ifdef CODEC_THREAD
	if (buf_size > 0 && buf) {
		size = sizeof(AVCodecContext);
		memcpy(s->vaddr, avctx, size);
		memcpy((uint8_t*)s->vaddr + size, picture, sizeof(AVFrame));
		size += sizeof(AVFrame);
	
		memcpy((uint8_t*)s->vaddr + size, &got_picture_ptr, sizeof(int));
		size += sizeof(int);
		memcpy((uint8_t*)s->vaddr + size, &ret, sizeof(int));
		av_free(buf);
	} else {
		memcpy(s->vaddr, &ret, sizeof(int));
	}
#else
	if (buf_size > 0 && buf) {
		s->codecInfo.num = 4;
		s->codecInfo.tmpParam[0] = got_picture_ptr;
		s->codecInfo.tmpParam[1] = ret; 
		s->codecInfo.param[0] = (uint32_t)avctx;
		s->codecInfo.param[1] = (uint32_t)picture;
		s->codecInfo.param[2] = (uint32_t)&s->codecInfo.tmpParam[0];
		s->codecInfo.param[3] = (uint32_t)&s->codecInfo.tmpParam[1];
		s->codecInfo.param_size[0] = sizeof(AVCodecContext);
		s->codecInfo.param_size[1] = sizeof(AVFrame);
		s->codecInfo.param_size[2] = sizeof(int);
		s->codecInfo.param_size[3] = sizeof(int);
	} else {
		s->codecInfo.num = 1;
		s->codecInfo.tmpParam[0] = ret;
		s->codecInfo.param[0] = (uint32_t)&s->codecInfo.tmpParam[0];
		s->codecInfo.param_size[0] = sizeof(int);
	}
#endif

	return ret;
}

/* int avcodec_decode_audio (AVCodecContext *avctx, AVFrame *picture,
								int *got_picture_ptr, const uint8_t *buf,
								int buf_size) */


/* int avcodec_encode_video (AVCodecContext *avctx, uint8_t *buf,
 * 							int buf_size, const AVFrame *pict)
 */
int qemu_avcodec_encode_video (SVCodecState *s)
{
	AVCodecContext *avctx = NULL;
	AVFrame *pict = NULL;
	uint8_t *outputBuf = NULL;
	uint8_t *inputBuf = NULL;
	int outputBufSize = 0;
//	AVFrame tempFrame;
	int numBytes = 0;
	int bPict = -1;
	int size = 0;
	int ret = -1;

	if (gAVCtx) {
		avctx = gAVCtx;
		pict = gFrame;
	} else {
		ERR("AVCodecContext is NULL\n");
		return -1;
	}

	memcpy(&outputBufSize, s->vaddr, sizeof(int));
	if (outputBufSize > 0) {
		outputBuf = (uint8_t*)av_malloc(outputBufSize * sizeof(uint8_t));
		memcpy(outputBuf, (uint8_t*)s->vaddr + 4, outputBufSize);
	} else {
		ERR("input buffer size is 0\n");
		return -1;
	}

	size = outputBufSize + 4;
	numBytes = avpicture_get_size(avctx->pix_fmt, avctx->width, avctx->height);
    inputBuf = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	if (!inputBuf) {
		ERR("failed to allocate decoded frame inputBuf\n");	
		return -1;
	}
	
	memcpy(&bPict, (uint8_t*)s->vaddr + size, sizeof(int));
	if (bPict == 1) {
		size += 4;
		memcpy(pict, (uint8_t*)s->vaddr + size, sizeof(AVFrame));
		size += sizeof(AVFrame);
		memcpy(inputBuf, (uint8_t*)s->vaddr + size, numBytes);
	} else {
		ERR("Failed to copy AVFrame\n");
	}

    ret = avpicture_fill(pict, inputBuf, avctx->pix_fmt, avctx->width, avctx->height);
	if (ret < 0) {
		ERR("after avpicture_fill, ret:%d\n", ret);
	}

	TRACE("before encode video, ticks_per_frame:%d, pts:%d\n", avctx->ticks_per_frame, pict->pts);
	ret = avcodec_encode_video (avctx, outputBuf, outputBufSize, pict);
	if (ret < 0) {
		ERR("Failed to encode video, ret:%d, pts:%d\n", ret, pict->pts);
	}

	if (ret > 0) {
		memcpy(s->vaddr, outputBuf, outputBufSize);
		memcpy((uint8_t*)s->vaddr + outputBufSize, &ret, sizeof(int));
	} else {
		memcpy(s->vaddr, &ret, sizeof(int));
	}

	av_free(inputBuf);
	if (outputBufSize > 0) {
		av_free(outputBuf);
	}

	return ret;
}


/* int avcodec_encode_audio (AVCodecContext *avctx, uint8_t *buf,
 * 							int buf_size, const AVFrame *pict)
 */


/* void av_picture_copy (AVPicture *dst, const AVPicture *src,
 * 						enum PixelFormat pix_fmt, int width, int height)
 */
void qemu_av_picture_copy (SVCodecState *s)
{
	AVCodecContext* avctx;
	AVPicture dst;
	AVPicture *src;
	int numBytes;
	uint8_t *buffer = NULL;

	TRACE("Enter :%s\n", __func__);
	avctx = gAVCtx;
	src = (AVPicture*)gFrame;
	numBytes = avpicture_get_size(avctx->pix_fmt, avctx->width, avctx->height);
   	buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	avpicture_fill(&dst, buffer, avctx->pix_fmt, avctx->width, avctx->height);
	av_picture_copy(&dst, src, avctx->pix_fmt, avctx->width, avctx->height);

	memcpy(s->vaddr, dst.data[0], numBytes);
	TRACE("After copy image buffer from host to guest\n");
	
	av_free(buffer);
	TRACE("Leave :%s\n", __func__);
}

/* AVCodecParserContext *av_parser_init (int codec_id) */
void qemu_av_parser_init (void)
{
/*	AVCodecParserContext *s;
	int codec_id;

	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[0], &codec_id, sizeof(int), 0);

	s = av_parser_init(codec_id);

	cpu_memory_rw_debug(cpu_single_env, target_param->ret_args, s, sizeof(AVCodecParserContext), 1); */
}

/* int av_parser_parse(AVCodecParserContext *s, AVCodecContext *avctx,
 *						uint8_t **poutbuf, int *poutbuf_size,
 *						const uint8_t *buf, int buf_size,
 * 						int64_t pts, int64_t dts)
 */
int qemu_av_parser_parse (void)
{
/*	AVCodecParserContext *s;
	AVCodecContext *avctx;
	uint8_t *poutbuf;
	int poutbuf_size;
	const uint8_t *buf;
	int buf_size;
	int64_t pts;
	int64_t pos;
	int ret;

	ret = av_parser_parse(s, avctx, &poutbuf, &poutbuf_size, buf, buf_size, pts, pos);

	return ret; */
	return 0;
}

/* void av_parser_close (AVCodecParserContext *s) */
void qemu_av_parser_close (void)
{
//	AVCodecParserContext *s;
//	av_parser_close(s);
}


#if 0

/*
 * THEORA API
 */

th_dec_ctx * _gDec;
th_info* _gInfo;
th_setup_info* _gSetup;
th_comment* _gTC;

/* int th_decode_ctl (th_dec_ctx* _dec, int _req,
 *						void* _buf, size_t _buf_sz)
 */
int qemu_th_decode_ctl (void)

{
	th_dec_ctx* dec;
	int req;
	void* buf;
	size_t buf_size;
	int ret;

	TRACE("Enter\n");

	if (_gDec) {
		dec = _gDec;
	} else {
		ERR("Failed to get th_dec_ctx pointer\n");
	}
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], &req, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], &buf_size, sizeof(size_t), 0);
	if (buf_size > 0) {
		buf = malloc(buf_size);
		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], buf, buf_size, 0);
	}
	ret = th_decode_ctl(dec, req, buf, buf_size);

//	cpu_meory_rw_debug(cpu_single_env, target_param->in_args[0], dec, sizeof(th_dec_ctx), 1);

	_gDec = dec;

	TRACE("Leave : ret(%d)\n", ret);
	return ret;
}

/* th_dec_ctx* th_decode_alloc (const th_info* _info,
 *								const th_setup_info* _setup)
 */
th_dec_ctx* qemu_th_decode_alloc (void)
{
	th_info info;
	th_setup_info* setup;
	th_dec_ctx *ctx;
	
	TRACE("Enter\n");

	if (_gSetup) {
		setup = _gSetup;
	}

	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[0], &info, sizeof(th_info), 0);

	ctx = th_decode_alloc(&info, setup);
	if (ctx) {
		_gDec = ctx;
	} else {
		ERR("Failed to alloc theora decoder\n");
	}

	TRACE("Leave\n");

	return ctx;
}

/* int th_decode_headerin (th_info* _info, th_comment* _tc,
 *						   th_setup_info** _setup, ogg_packet* _op)
 */
int qemu_th_decode_headerin (void)
{
	th_info info;
	th_comment tc;
	th_setup_info *setup;
	ogg_packet op;
	long bytes;
	char *packet;
	int ret;

	TRACE("Enter\n");

//	setup = _gSetup;
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[0], &info, sizeof(th_info), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], &tc, sizeof(th_comment), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], &op, sizeof(ogg_packet), 0);
	bytes = op.bytes;
	if (bytes > 0) {
		packet = (char*)malloc(bytes * sizeof(char));
		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[4], packet, bytes, 0);
	}
	op.packet = packet;
	ret = th_decode_headerin(&info, &tc, &setup, &op);
	_gSetup = setup;
	
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[0], &info, sizeof(th_info), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], &tc, sizeof(th_comment), 1);
//	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], setup, sizeof(th_setup_info), 1);

	free(packet);

	TRACE("Leave : ret(%d)\n", ret);
	return ret;
}

/* int th_decode_packetin (th_dec_ctx* _dec,
 *						   const ogg_packet* _op,
 *						   ogg_int64_t* _granpos)
 */
int qemu_th_decode_packetin (void)
{
	th_dec_ctx* dec;
	ogg_packet op;
	ogg_int64_t granpos;
	char *packet;
	long bytes;
	int ret;

	TRACE("Enter\n");

	if (_gDec) {
		dec = _gDec;
	}
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], &op, sizeof(ogg_packet), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], &granpos, sizeof(ogg_int64_t), 0);
	bytes = op.bytes;
	if (bytes > 0) {
		packet = (char*)malloc(bytes * sizeof(char));
		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], packet, bytes, 0);
	}
	op.packet = packet;

	ret = th_decode_packetin(dec, &op, &granpos);

	TRACE("Leave : ret(%d)\n", ret);
	return ret;
}

/* int th_decode_ycbcr_out (th_dec_ctx* _dec,
 *							th_ycbcr_buffer _ycbcr)
 */
int qemu_th_decode_ycbcr_out (void)
{
	th_dec_ctx* dec;
	th_ycbcr_buffer ycbcr;
	int ret, size;

	TRACE("Enter\n");

	if (_gDec) {
		dec = _gDec;
	}
	size = sizeof(th_dec_ctx);
	ret = th_decode_ycbcr_out(dec, ycbcr);
	
	TRACE("Leave : ret(%d)\n", ret);
	return ret;
}

/* void th_info_clear (th_info* _info) */
void qemu_th_info_clear (void)
{
	th_info* info;
	
	TRACE("Enter\n");

	th_info_clear(info);

	TRACE("Leave\n");
}

/* void th_comment_clear (th_comment* _tc) */
void qemu_th_comment_clear (void)
{
	th_comment* tc;

	TRACE("Enter\n");

	th_comment_clear(tc);

	TRACE("Leave\n");
}

/* void th_setup_free (th_setup_info* _setup) */
void qemu_th_setup_free (void)
{
	th_setup_info* setup;

	TRACE("Enter\n");

	th_setup_free(setup);

	TRACE("Leave\n");
}

/* void th_decode_free (th_dec_ctx* _dec) */
void qemu_th_decode_free (void)
{
	th_dec_ctx* dec;

	TRACE("Enter\n");

	th_decode_free(dec);

	TRACE("Leave\n");
}
#endif

static int abcd = 0;

static int codec_operate (uint32_t apiIndex, SVCodecState *state)
{
	int ret = -1;
	TRACE("%s : count :%d\n", __func__, ++abcd);
	switch (apiIndex) {
		/* FFMPEG API */
		case 1:
			qemu_av_register_all();
			break;
		case 2:
			ret = qemu_avcodec_open(state);
			break;
		case 3:
			ret = qemu_avcodec_close(state);
			break;
		case 4:
			qemu_avcodec_alloc_context();
			break;
		case 5:
			qemu_avcodec_alloc_frame();
			break;
		case 6:
			qemu_av_free(state);
			break;
		case 10:
			qemu_avcodec_get_context_defaults();
			break;
		case 11:
			qemu_avcodec_flush_buffers();
			break;
		case 12:
			ret = qemu_avcodec_default_get_buffer();
			break;
		case 13:
			qemu_avcodec_default_release_buffer();
			break;
#ifdef CODEC_THREAD
		case 20:
			ret = qemu_avcodec_decode_video(state);
			break;
		case 22:
			ret = qemu_avcodec_encode_video(state);
			break;
#else
		case 20:
		case 22:
			/* wake codec worker thread */
			wake_codec_wrkthread(state);
			TRACE("[%d]After wake_codec_wrkthread\n", __LINE__);
//			sleep_codec_wrkthread(state);
			break;
#endif
		case 24:
			qemu_av_picture_copy(state);
			break;
		case 30:
			qemu_av_parser_init();
			break;
		case 31:
			ret= qemu_av_parser_parse();
			break;
		case 32:
			qemu_av_parser_close();
			break;
		/* THEORA API */
/*		case 40:
			ret = qemu_th_decode_ctl();
			break;
		case 41:
			ret = qemu_th_decode_alloc();
			break;
		case 42:
			ret = qemu_th_decode_headerin();
			break;
		case 43:
			ret = qemu_th_decode_packetin();
			break;
		case 44:
			ret = qemu_th_decode_ycbcr_out();
			break;
		case 45:
			qemu_th_info_clear();
			break;
		case 46:
			qemu_th_comment_clear();
			break;
		case 47:
			qemu_th_setup_free();
			break;
		case 48:
			qemu_th_decode_free();
			break; */
		default:
			WARN("The api index does not exsit!. api index:%d\n", apiIndex);
	}
	return ret;
}

/*
 *	Virtual Codec Device Define
 */
static int codec_copy_info (SVCodecState *s)
{
	int i = 0;
	int paramSize = 0;
	uint8_t paramMax;

	if (!s) {
		ERR("SVCodecState is NULL\n");
		return -1;
	}

	paramMax = s->codecInfo.num;
	
	for (; i < paramMax; i++) {
		memcpy((uint8_t*)s->vaddr + paramSize,
				s->codecInfo.param[i],
				s->codecInfo.param_size[i]);
		paramSize += s->codecInfo.param_size[i];
	}

		
	return 0;
}

static uint32_t codec_read (void *opaque, target_phys_addr_t addr)
{
	int ret = -1;
	SVCodecState *state = (SVCodecState*)opaque;

	switch (addr) {
		case READY_TO_GET_DATA:
			ret = state->bstart;
			TRACE("GET_DATA ret:%d\n", ret);
			break;
		default:
			ERR("There is no avaiable command for svcodece\n");
	}
	return ret;
}

static int count = 0;
static int testcount = 0;
static void codec_write (void *opaque, target_phys_addr_t addr, uint32_t value)
{
	uint32_t offset;
	int ret = -1;
	SVCodecState *state = (SVCodecState*)opaque;

	offset = addr;
	switch (offset) {
		case FUNC_NUM:
			TRACE("TEST COUNT:%d\n", ++testcount);
			target_param->func_num = value;
			ret = codec_operate(value, state);
			count = 0;
			break;
		case IN_ARGS:
			target_param->in_args[count++] = value;
			break;
		case RET_STR:
			target_param->ret_args = value;
		case COPY_RESULT_DATA:
			codec_copy_info (state);
			sleep_codec_wrkthread(state);
			qemu_irq_lower(state->dev.irq[1]);
			break;
	}
}

static CPUReadMemoryFunc * const svcodec_io_readfn[3] = {
	codec_read,
	codec_read,
	codec_read,
};

static CPUWriteMemoryFunc * const svcodec_io_writefn[3] = {
	codec_write,
	codec_write,
	codec_write,
};

static void codec_mem_map (PCIDevice *dev, int region_num,
							pcibus_t addr, pcibus_t size, int type)
{
	SVCodecState *s = DO_UPCAST(SVCodecState, dev, dev);
	cpu_register_physical_memory(addr, size, s->vram_offset);
	s->mem_addr = addr;
}

static void codec_mmio_map (PCIDevice *dev, int region_num,
							pcibus_t addr, pcibus_t size, int type)
{
	SVCodecState *s = DO_UPCAST(SVCodecState, dev, dev);
	cpu_register_physical_memory(addr, size, s->svcodec_mmio);
	s->mmio_addr = addr;
}

void wake_codec_wrkthread(SVCodecState *s)
{
	pthread_mutex_lock(&s->thInfo.lock);
	s->bstart = true;
	s->index = target_param->func_num;
	if (pthread_cond_signal(&s->thInfo.cond)) {
		ERR("Failed to send a signal to the worker thread\n");
		return ;
	}
	pthread_mutex_unlock(&s->thInfo.lock);
}

void sleep_codec_wrkthread(SVCodecState *s)
{
	pthread_mutex_lock(&s->thInfo.lock);
	s->bstart = false;
	pthread_mutex_unlock(&s->thInfo.lock);
}


static int codec_thread_init(void *opaque)
{
	SVCodecState *s = (SVCodecState*)opaque;
	int ret;
	
	if (pthread_cond_init(&s->thInfo.cond, NULL)) {
		ERR("Failed to initialize thread conditional variable\n");
		return -1;
	}
	if (pthread_mutex_init(&s->thInfo.lock, NULL)) {
		ERR("Failed to initialize mutex variable\n");
		return -1;
	}

	ret = pthread_create(&s->thInfo.codec_thread, NULL,
						codec_worker_thread, s);
	if (ret != 0) {
		ERR("Failed to create a worker thread for codec device\n");
		return ret;
	}

	return ret;
}

void codec_thread_destroy(void *opaque)
{
	SVCodecState *s = (SVCodecState*)opaque;
	
	pthread_mutex_destroy(&s->thInfo.lock);
	pthread_cond_destroy(&s->thInfo.cond);

//	pthread_exit(NULL);
}

static int abc = 0;

static void* codec_worker_thread(void *opaque)
{
	SVCodecState *s = (SVCodecState*)opaque;
	
	pthread_mutex_lock(&s->thInfo.lock);
	while (1) {
		pthread_cond_wait(&s->thInfo.cond, &s->thInfo.lock);

		TRACE("wake_up worker thread :%d\n", abc++);
		if (s->index == 20) {
			qemu_avcodec_decode_video(s);
		} else if (s->index == 22) {
			qemu_avcodec_encode_video(s);
		}
		qemu_irq_raise(s->dev.irq[1]);
	}
	pthread_mutex_unlock(&s->thInfo.lock);
	
//	pthread_exit(NULL);
}

static int codec_initfn (PCIDevice *dev)
{
	SVCodecState *s = DO_UPCAST(SVCodecState, dev, dev);
	uint8_t *pci_conf = s->dev.config;

	target_param = (SVCodecParam*)qemu_malloc(sizeof(SVCodecParam));
	memset(target_param, 0x00, sizeof(SVCodecParam));
	
	pci_config_set_vendor_id(pci_conf, PCI_VENDOR_ID_SAMSUNG);
	pci_config_set_device_id(pci_conf, PCI_DEVICE_ID_VIRTUAL_CODEC);
	pci_config_set_class(pci_conf, PCI_CLASS_MULTIMEDIA_OTHER);
	pci_config_set_interrupt_pin(pci_conf, 2);

	s->vram_offset = qemu_ram_alloc(NULL, "codec.ram", SVCODEC_MEM_SIZE);
	s->vaddr = qemu_get_ram_ptr(s->vram_offset);

	s->svcodec_mmio = cpu_register_io_memory(svcodec_io_readfn, svcodec_io_writefn,
											s, DEVICE_LITTLE_ENDIAN);

	pci_register_bar(&s->dev, 0, SVCODEC_MEM_SIZE,
					PCI_BASE_ADDRESS_MEM_PREFETCH, codec_mem_map);
	pci_register_bar(&s->dev, 1, SVCODEC_REG_SIZE,
					PCI_BASE_ADDRESS_SPACE_MEMORY, codec_mmio_map);
#ifndef CODEC_THREAD 
	codec_thread_init(s);
#endif

	return 0;
}

int pci_codec_init (PCIBus *bus)
{
	pci_create_simple (bus, -1, QEMU_DEV_NAME);
	return 0;
}

static PCIDeviceInfo codec_info = {
	.qdev.name 		= QEMU_DEV_NAME,
	.qdev.desc 		= "Virtual codec device for emulator",
	.qdev.size 		= sizeof (SVCodecState),	
	.init			= codec_initfn,
};

static void codec_register (void)
{
	pci_qdev_register(&codec_info);
}
device_init(codec_register);
