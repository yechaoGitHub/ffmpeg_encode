// ffmpeg_record.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <assert.h>
#include <stdio.h>

extern "C"
{
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libavformat/avformat.h"
}

#pragma comment (lib, "libavcodecd.lib")
#pragma comment (lib, "libavformatd.lib")

const char *out_file = "test.mp4";

int main()
{
	int ret = 0;
	const AVCodecID codec_id = AV_CODEC_ID_H264;

	AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	assert(codec != NULL);

	AVCodecContext *codec_ctx =	avcodec_alloc_context3(codec);
	assert(codec_ctx != NULL);

	codec_ctx->width = 352;
	codec_ctx->height = 288;
	codec_ctx->time_base.num = 1;
	codec_ctx->time_base.den = 25;
	codec_ctx->gop_size = 10;
	codec_ctx->max_b_frames = 0;
	codec_ctx->bit_rate = 400000;
	codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

	av_opt_set(codec_ctx->priv_data, "preset", "ultrafast", 0);
	av_opt_set(codec_ctx->priv_data, "tune", "zerolatency", 0);


	ret = avcodec_open2(codec_ctx, codec, NULL);
	assert(ret >= 0);

	AVFrame *frame = av_frame_alloc();
	assert(frame != NULL);

	frame->format = codec_ctx->pix_fmt;
	frame->width = codec_ctx->width;
	frame->height = codec_ctx->height;

	ret = av_image_alloc(frame->data, frame->linesize, codec_ctx->width, 
		codec_ctx->height, codec_ctx->pix_fmt, 16);
	assert(ret >= 0);

	AVPacket *pkt = av_packet_alloc();
	assert(pkt != NULL);

	av_init_packet(pkt);
	pkt->data = NULL;
	pkt->size = 0;


	AVFormatContext *fmt_ctx(NULL);
	ret = avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, "test.mp4");
	assert(ret >= 0);

	ret = avio_open(&fmt_ctx->pb, out_file, AVIO_FLAG_READ_WRITE);
	assert(ret >= 0);

	AVStream *video = avformat_new_stream(fmt_ctx, 0);
	assert(video != NULL);

	ret = avcodec_parameters_from_context(video->codecpar, codec_ctx);
	assert(ret >= 0);

	ret = avformat_write_header(fmt_ctx, NULL);
	assert(ret >= 0);

	int i = 0;
	

	FILE *yuv_file(NULL);
	fopen_s(&yuv_file, ".\\test(352_288).yuv", "rb+");
	assert(yuv_file != NULL);

	while (!feof(yuv_file)) 
	{
		fread(frame->data[0], 1, 352 * 288, yuv_file);
		fread(frame->data[1], 1, 352 * 288 / 4, yuv_file);
		fread(frame->data[2], 1, 352 * 288 / 4, yuv_file);

		frame->pts = i * (video->time_base.den) / ((video->time_base.num) * 25);

		int p = av_rescale_q(i, codec_ctx->time_base, video->time_base);

		ret = avcodec_send_frame(codec_ctx, frame);
		if (ret < 0) 
		{
			printf("send packet failed \n");
		}

		while (ret >= 0) 
		{
			ret = avcodec_receive_packet(codec_ctx, pkt);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) 
			{
				break;
			}

			pkt->stream_index = video->index;

			ret = av_interleaved_write_frame(fmt_ctx, pkt);
			if (ret < 0) 
			{
				printf("av_write_frame failed\n");
			}

			i++;
		}

		av_packet_unref(pkt);

	}
	
	av_write_trailer(fmt_ctx);

	fclose(yuv_file);
	avcodec_close(codec_ctx);
	avcodec_free_context(&codec_ctx);
	av_freep(&frame->data[0]);
	av_frame_free(&frame);

	return 0;
}



