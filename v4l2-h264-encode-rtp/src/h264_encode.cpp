#include "h264_encode.h"

void h264_encoder_init(struct x264_param_t* param, struct x264_picture_t* pic_in, struct x264_t** encoder, int width, int height, int fps)
{
    if (*encoder == NULL)
    {
        // 设置编码参数
        x264_param_default_preset(param, "veryfast", "zerolatency");

        param->i_width = width;
        param->i_height = height;
        param->i_fps_num = fps;
        param->i_fps_den = 1;
        param->i_csp = X264_CSP_I420;

        param->rc.i_rc_method = X264_RC_ABR;
        param->rc.i_bitrate = 800;
        param->rc.i_vbv_max_bitrate = 800;
        param->rc.i_vbv_buffer_size = 800;

        param->b_repeat_headers = 1;
        param->b_annexb = 1; 
        param->i_keyint_max = 30;
        param->i_threads = 1; 

        // 打开编码器
        x264_param_apply_profile(param, "baseline");
        *encoder = x264_encoder_open(param);
        if(*encoder == NULL)
        {
            LOGE("Failed to open encoder");
            return;
        }

        if (x264_picture_alloc(pic_in, param->i_csp, param->i_width, param->i_height) < 0) 
        {
            LOGE("x264_picture_alloc failed");
            return;
        }
    }
}

void h264_encode(struct x264_t* encoder, struct x264_picture_t* pic_in, struct x264_picture_t* pic_out, int* frame_num, unsigned char* YUYV_data, x264_nal_t **nals, int* i_nals, int width, int height)
{
    // 转换 YUYV 数据为 I420 格式
    yuyv_to_i420_with_stride(YUYV_data,
        pic_in->img.plane[0], pic_in->img.i_stride[0],
        pic_in->img.plane[1], pic_in->img.i_stride[1],
        pic_in->img.plane[2], pic_in->img.i_stride[2],
        width, height);

    //时间戳设置
    pic_in->i_pts = *frame_num;
    printf("frame_num: %d\n", *frame_num);
    x264_encoder_encode(encoder, nals, i_nals, pic_in, pic_out);
    *frame_num += 1;
}

void h264_encoder_release(struct x264_t* encoder, struct x264_picture_t* pic_in)
{
    if (encoder != NULL)
    {
        x264_encoder_close(encoder);
        encoder = NULL;
    }

    //释放图片
    x264_picture_clean(pic_in);
}