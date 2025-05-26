#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/mman.h>
#include <jpeglib.h>
#include <linux/fb.h>

#define YUYV_W 160
#define YUYV_H 120

#define MJPG_W 320
#define MJPG_H 240

//jpeg����
int read_JPEG_file(const char* jpeg_data, unsigned char* rgb_data, int size)
{
    struct jpeg_error_mgr jerr;
    struct jpeg_decompress_struct cinfo;
    cinfo.err = jpeg_std_error(&jerr);
    //����������󲢳�ʼ��
    jpeg_create_decompress(&cinfo);
    //ָ������Դ
    jpeg_mem_src(&cinfo, (unsigned char*)jpeg_data, size);
    //��ȡJPEG�ļ�ͷ
    jpeg_read_header(&cinfo, TRUE);
    //��ʼ����
    jpeg_start_decompress(&cinfo);
    //����洢һ�����ݵ��ڴ�ռ�
    int row_stride = cinfo.output_width * cinfo.output_components;
    unsigned char* buffer = (unsigned char*)malloc(row_stride);
    int i = 0;
    while(cinfo.output_scanline < cinfo.output_height)
    {
        //��ȡһ������
        jpeg_read_scanlines(&cinfo, &buffer, 1);
        //�����ݴ洢��RGB������
        memcpy(rgb_data + i * row_stride, buffer, row_stride);
        i++;

    }

    //�������  
    jpeg_finish_decompress(&cinfo);
    //�ͷŽ������
    jpeg_destroy_decompress(&cinfo);
    //�ͷ��ڴ�
    free(buffer);
    return 0;
}

//lcd��ʾrgb����
int fb_fd = 0;
unsigned int* fb_ptr = NULL;

void lcd_show_rgb(unsigned char* rgb_data, int width, int height, int lcd_width)
{
    unsigned int* p = fb_ptr;
    unsigned char* rgb = rgb_data;
    for(int i = 0; i < height; i++)
    {
        for(int j = 0; j < width; j++)
        {
            int index = (i * width + j) * 3;
            unsigned char r = rgb[index];
            unsigned char g = rgb[index + 1];
            unsigned char b = rgb[index + 2];

            // ARGB8888: alpha�̶�Ϊ0xFF
            p[i * lcd_width + j] = (0xFF << 24) | (r << 16) | (g << 8) | b;
        }
    }
}

// YUYVתRGB888
void yuyv_to_rgb888(unsigned char* yuyv, unsigned char* rgb, int width, int height)
{
    int i, j;
    int y0, u, y1, v;
    int r, g, b;
    int index = 0;

    for (i = 0; i < width * height * 2; i += 4)
    {
        y0 = yuyv[i + 0];
        u  = yuyv[i + 1] - 128;
        y1 = yuyv[i + 2];
        v  = yuyv[i + 3] - 128;

        // ��һ������
        r = y0 + 1.402 * v;
        g = y0 - 0.344136 * u - 0.714136 * v;
        b = y0 + 1.772 * u;
        rgb[index++] = r < 0 ? 0 : (r > 255 ? 255 : r);
        rgb[index++] = g < 0 ? 0 : (g > 255 ? 255 : g);
        rgb[index++] = b < 0 ? 0 : (b > 255 ? 255 : b);

        // �ڶ�������
        r = y1 + 1.402 * v;
        g = y1 - 0.344136 * u - 0.714136 * v;
        b = y1 + 1.772 * u;
        rgb[index++] = r < 0 ? 0 : (r > 255 ? 255 : r);
        rgb[index++] = g < 0 ? 0 : (g > 255 ? 255 : g);
        rgb[index++] = b < 0 ? 0 : (b > 255 ? 255 : b);
    }
}

int main()
{
    int width = YUYV_W;
    int height = YUYV_H;

    //������ͷ�豸�豸
    int fd = open("/dev/video0", O_RDWR, 0);
    if(fd < 0)
    {
        perror("open file failed");
        return -1;
    }

    //��lcd�豸
    fb_fd = open("/dev/fb0", O_RDWR);
    if(fb_fd < 0)
    {
        perror("open fb0 failed");
        return -1;
    }
    
    int ret = 0;

    //��ȡfb0��ʵ�ߴ�
    struct fb_var_screeninfo var;
    ret = ioctl(fb_fd, FBIOGET_VSCREENINFO, &var);
    if(ret < 0)
    {
        perror("FBIOGET_VSCREENINFO failed");
        close(fb_fd);
        return -1;
    }
    int lcd_width = var.xres_virtual;
    int lcd_height = var.yres_virtual;
    printf("fb0 width=%d, height=%d\n", var.xres_virtual, var.yres_virtual);

    //fbӳ��
    fb_ptr = (unsigned int*)mmap(NULL, lcd_width*lcd_height*4, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0); 

/*     //��ѯ�豸��Ϣ
    struct v4l2_capability v4l_capability;
    ret = ioctl(fd, VIDIOC_QUERYCAP, &v4l_capability);
    if (ret == -1) {
        perror("VIDIOC_QUERYCAP failed");
        close(fd);
        return -1;
    }
    printf("%s\r\n", v4l_capability.driver);
    printf("%s\r\n", v4l_capability.card);
    printf("%s\r\n", v4l_capability.bus_info);
    printf("Version: %u.%u.%u\n", 
        (v4l_capability.version >> 16) & 0xFF,
        (v4l_capability.version >> 8) & 0xFF,
        v4l_capability.version & 0xFF);
    printf("Capabilities: 0x%08X\n", v4l_capability.capabilities);
    printf("Device Caps: 0x%08X\n", v4l_capability.device_caps); */

    
    //��ѯ֧�ֵĸ�ʽ
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

    printf("index=%d\n", v4l_fmtdesct.index);
    printf("flags=%d\n", v4l_fmtdesct.flags);
    printf("description=%s\n", v4l_fmtdesct.description);
    unsigned  char* p = (unsigned char*)&v4l_fmtdesct.pixelformat;
    printf("pixelformat=%c%c%c%c\n", p[0],p[1],p[2],p[3]);

    //���ø�ʽ
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

    //�����ڴ�
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

    //ӳ���ڴ�
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
        

        //�������
        ret = ioctl(fd, VIDIOC_QBUF, &v4l_buffer);
        if(ret < 0)
        {
            perror("VIDIOC_QBUF failed");
        }
    }


    //��ʼ�ɼ�
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd, VIDIOC_STREAMON, &type);
    if(ret < 0)
    {
        perror("VIDIOC_STREAMON failed");
    }
    printf("start capture...\n");

    unsigned char rgb_data[width*height*3];
/*     while(1)
    {
        //�ɼ�����
        ret = ioctl(fd, VIDIOC_DQBUF, &v4l_buffer);
        if(ret < 0)
        {
            perror("VIDIOC_DQBUF failed");
        }

        //����Ϊmjpg�ļ�
        //FILE *file = fopen("my.jpg", "wb");
        //fwrite(mptr[v4l_buffer.index], v4l_buffer.length, 1, file);
        //fclose(file); 

        //��fb0����ʾ
        read_JPEG_file((const char*)mptr[v4l_buffer.index], rgb_data, v4l_buffer.length);
        lcd_show_rgb(rgb_data, width, height);

        ret = ioctl(fd, VIDIOC_QBUF, &v4l_buffer);
        if(ret < 0)
        {
            perror("VIDIOC_QBUF failed");
        }
    }  */

    //�ɼ�yuv����
    //FILE *file = fopen("my.yuv", "wb");
    while(1)
    {
        //�ɼ�����
        ret = ioctl(fd, VIDIOC_DQBUF, &v4l_buffer);
        if(ret < 0)
        {
            perror("VIDIOC_DQBUF failed");
        } 

        //fwrite(mptr[v4l_buffer.index], v4l_buffer.length, 1, file);

        yuyv_to_rgb888(mptr[v4l_buffer.index], rgb_data, width, height);
        lcd_show_rgb(rgb_data, width, height, lcd_width);

        ret = ioctl(fd, VIDIOC_QBUF, &v4l_buffer);
        if(ret < 0)
        {
            perror("VIDIOC_QBUF failed");
        }
    }
    //fclose(file);

    //ֹͣ�ɼ�
    ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
    if(ret < 0)
    {
        perror("IDIOC_STREAMOFF failed");
    }
    printf("stop capture...\n");

    //�ͷ�ӳ��
    for(int i = 0; i < 4; i++)
    {
        ret = munmap(mptr[i], buf_size[i]);
        if(ret < 0)
        {
            perror("unmap failed");
        }
    }

    //�ͷ�lcdӳ��
    ret = munmap(fb_ptr, width*height*4);
    if(ret < 0)
    {
        perror("unmap fb0 failed");
    }

    //�ر��ļ�
    close(fd);
    close(fb_fd);

    return 0;
}

