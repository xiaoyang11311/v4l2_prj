#ifndef H264_ENCODE_H
#define H264_ENCODE_H

#include "log.h"
#include <stdint.h>
#include "../include/x264_config.h"
#include "../include/x264.h"
#include "yuyv_to_i420.h"

void h264_encoder_init(struct x264_param_t* param, struct x264_picture_t* pic_in, struct x264_t** encoder, int width, int height, int fps); //h264相关资源初始化

void h264_encode(struct x264_t* encoder, struct x264_picture_t* pic_in, struct x264_picture_t* pic_out, int* frame_num, unsigned char* YUYV_data, x264_nal_t **nals, int* i_nals, int width, int height); //开始编码

void h264_encoder_release(struct x264_t* encoder, struct x264_picture_t* pic_in); //释放h264相关资源

#endif