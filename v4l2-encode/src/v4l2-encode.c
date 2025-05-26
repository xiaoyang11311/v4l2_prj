#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/mman.h>
#include "../include/x264_config.h"
#include "../include/x264.h"

#define YUYV_W 160
#define YUYV_H 120
#define YUYV_FPS 30


//YUYV转I420
void yuyv_to_i420_with_stride(unsigned char* yuyv,
                              unsigned char* y_plane, int y_stride,
                              unsigned char* u_plane, int u_stride,
                              unsigned char* v_plane, int v_stride,
                              int width, int height)
{
    for (int y = 0; y < height; y++) {
        unsigned char* y_row = y_plane + y * y_stride;
        unsigned char* u_row = u_plane + (y / 2) * u_stride;
        unsigned char* v_row = v_plane + (y / 2) * v_stride;

        for (int x = 0; x < width; x += 2) {
            int index = (y * width + x) * 2;

            unsigned char y0 = yuyv[index + 0];
            unsigned char u  = yuyv[index + 1];
            unsigned char y1 = yuyv[index + 2];
            unsigned char v  = yuyv[index + 3];

            y_row[x]     = y0;
            y_row[x + 1] = y1;

            if (y % 2 == 0) {  // 每 2 行采一行 U/V
                u_row[x / 2] = u;
                v_row[x / 2] = v;
            }
        }
    }
}

static x264_t *encoder = NULL;
static x264_param_t param;
static x264_picture_t pic_in, pic_out;
static int frame_num = 0;

//h264_encoder初始化
void h264_encoder_init()
{
    if (encoder == NULL)
    {
        // 设置编码参数
        x264_param_default_preset(&param, "veryfast", "zerolatency");

        param.i_width = YUYV_W;
        param.i_height = YUYV_H;
        param.i_fps_num = YUYV_FPS;
        param.i_fps_den = 1;
        param.i_csp = X264_CSP_I420;

        param.rc.i_rc_method = X264_RC_ABR;
        param.rc.i_bitrate = 800;
        param.rc.i_vbv_max_bitrate = 800;
        param.rc.i_vbv_buffer_size = 800;

        param.b_repeat_headers = 1;
        param.b_annexb = 1; //带起始码的数据
        param.i_keyint_max = 30;
        param.i_threads = 1; 

        // 打开编码器
        x264_param_apply_profile(&param, "baseline");
        encoder = x264_encoder_open(&param);
        if(encoder == NULL)
        {
            fprintf(stderr, "Failed to open encoder\n");
            return;
        }

        if (x264_picture_alloc(&pic_in, param.i_csp, param.i_width, param.i_height) < 0) {
            fprintf(stderr, "x264_picture_alloc failed\n");
            return;
        }
    }
}

//x264编码
void yuv_to_h264(unsigned char* YUYV_data, unsigned char* h264_data, int width, int height, int* size)
{
    // 转换 YUYV 数据为 I420 格式
    yuyv_to_i420_with_stride(YUYV_data,
        pic_in.img.plane[0], pic_in.img.i_stride[0],
        pic_in.img.plane[1], pic_in.img.i_stride[1],
        pic_in.img.plane[2], pic_in.img.i_stride[2],
        width, height);

    // 编码
    x264_nal_t *nals;
    int i_nals = 0;
    int i_size = 0;

    //时间戳设置
    pic_in.i_pts = frame_num++;
    printf("frame_num: %d\n", frame_num);
    i_size = x264_encoder_encode(encoder, &nals, &i_nals, &pic_in, &pic_out);

    for(int i = 0; i < i_nals; ++i)
    {
        printf("nal_type = %d\n", nals[i].i_type);
        switch (nals[i].i_type)
        {
            case NAL_SPS:
                printf("NAL_SPS\n");
                break;
            case NAL_PPS:
                printf("NAL_PPS\n");
                break;
            case NAL_SLICE_IDR:
                printf("NAL_SLICE_IDR\n");
                break;
            case NAL_SLICE:
                printf("NAL_SLICE\n");
                break;
            default:
                printf("other nal type\n");
                break;
        }
    }
    printf("nal_count = %d\n", i_nals);
    printf("nal_size = %d\n", i_size);  

    if (i_size > 0 && h264_data != NULL)
    {
        int offset = 0;
        for (int i = 0; i < i_nals; ++i)
        {
            memcpy(h264_data + offset, nals[i].p_payload, nals[i].i_payload);
            offset += nals[i].i_payload;
        }
        *size = offset;
    }
    else
    {
        *size = 0;
    }


}

//h264_encoder释放
void h264_encoder_release()
{
    if (encoder != NULL)
    {
        x264_encoder_close(encoder);
        encoder = NULL;
    }

    //释放图片
    x264_picture_clean(&pic_in);
}

int main()
{
    int width = YUYV_W;
    int height = YUYV_H;

    //打开摄像头设备设备
    int fd = open("/dev/video0", O_RDWR, 0);
    if(fd < 0)
    {
        perror("open file failed");
        return -1;
    }

    int ret = 0;
    
    //查询支持的格式
    struct v4l2_fmtdesc v4l_fmtdesct;
    v4l_fmtdesct.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l_fmtdesct.index = 0;
 
    ret = ioctl(fd, VIDIOC_ENUM_FMT, &v4l_fmtdesct);
    if(ret < 0)
    {
        perror("VIDIOC_ENUM_FMT failed");
        close(fd);
        return -1;
    }

    //设置格式
    struct v4l2_format v4l_format;
    v4l_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l_format.fmt.pix.width = width;
    v4l_format.fmt.pix.height = height;
    v4l_format.fmt.pix.pixelformat = v4l_fmtdesct.pixelformat;
    ret = ioctl(fd, VIDIOC_S_FMT, &v4l_format);
    if(ret < 0)
    {
        perror("VIDIOC_S_FMT failed");
        close(fd);
        return -1;
    }

    //申请内存
    struct v4l2_requestbuffers v4l_requestbuffers;
    v4l_requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l_requestbuffers.memory = V4L2_MEMORY_MMAP;
    v4l_requestbuffers.count = 4;
    ret = ioctl(fd, VIDIOC_REQBUFS, &v4l_requestbuffers);
    if(ret < 0)
    {
        perror("VIDIOC_REQBUFS failed");
        close(fd);
        return -1;
    }

    //映射内存
    unsigned char* mptr[4];
    unsigned int buf_size[4];

    struct v4l2_buffer v4l_buffer;
    v4l_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    for(int i = 0; i < 4; i++)
    {
        v4l_buffer.index = i;
        ret = ioctl(fd, VIDIOC_QUERYBUF, &v4l_buffer);
        if(ret < 0)
        {
            perror("VIDIOC_QUERYBUF failed");
        }

        mptr[i] = mmap(NULL, v4l_buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, v4l_buffer.m.offset);
        buf_size[i] = v4l_buffer.length;
        

        //加入队列
        ret = ioctl(fd, VIDIOC_QBUF, &v4l_buffer);
        if(ret < 0)
        {
            perror("VIDIOC_QBUF failed");
        }
    }


    //开始采集
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd, VIDIOC_STREAMON, &type);
    if(ret < 0)
    {
        perror("VIDIOC_STREAMON failed");   
    }
    printf("start capture...\n");

    unsigned char* h264_data = malloc(width * height * 3 / 2);
    int size = 0;
    FILE* fp = fopen("h264.h264", "wb+");
    if(fp == NULL)
    {
        perror("open file failed");
        return -1;
    }

    h264_encoder_init();

    while(1)
    {
        //采集数据
        ret = ioctl(fd, VIDIOC_DQBUF, &v4l_buffer);
        if(ret < 0)
        {
            perror("VIDIOC_DQBUF failed");
        } 

        //数据处理
        yuv_to_h264(mptr[v4l_buffer.index], h264_data, width, height, &size);
        printf("h264 data size: %d\n", size);
        //保存数据
        fwrite(h264_data, 1, size, fp);
        fflush(fp);

        ret = ioctl(fd, VIDIOC_QBUF, &v4l_buffer);
        if(ret < 0)
        {
            perror("VIDIOC_QBUF failed");
        }
    }
    
    h264_encoder_release();

    //释放h264_data
    free(h264_data);
    
    //关闭文件
    fclose(fp);

    //停止采集
    ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
    if(ret < 0)
    {
        perror("IDIOC_STREAMOFF failed");
    }
    printf("stop capture...\n");

    //释放映射
    for(int i = 0; i < 4; i++)
    {
        ret = munmap(mptr[i], buf_size[i]);
        if(ret < 0)
        {
            perror("unmap failed");
        }
    }

    //关闭摄像头
    close(fd);

    return 0;
}