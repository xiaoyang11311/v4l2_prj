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
#include <setjmp.h>

#define MJPG_W 320
#define MJPG_H 240

//jpeg������
struct my_error_mgr
{
    struct jpeg_error_mgr pub;  // "public" fields
    jmp_buf setjmp_buffer;      // for return to caller
};
typedef struct my_error_mgr* my_error_ptr;
void my_error_exit(j_common_ptr cinfo)
{
    my_error_ptr myerr = (my_error_ptr)cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    //���ص�setjmp
    longjmp(myerr->setjmp_buffer, 1);
}


//jpeg����
int read_JPEG_file(const char* jpeg_data, unsigned char* rgb_data, int size)
{
    int ret = 0;
    struct my_error_mgr jerr;
    struct jpeg_decompress_struct cinfo;
    
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    
    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        return -1; // ����ʧ��
    }
    
    //����������󲢳�ʼ��
    jpeg_create_decompress(&cinfo);
    //ָ������Դ
    jpeg_mem_src(&cinfo, (unsigned char*)jpeg_data, size);
    //��ȡJPEG�ļ�ͷ
    jpeg_read_header(&cinfo, TRUE);
	 if(ret < 0) 
	 {
		printf("Not a jpg frame.\n");
		return -2;
     }
    //��ʼ����
    jpeg_start_decompress(&cinfo);
    //����洢һ�����ݵ��ڴ�ռ�
    int row_stride = cinfo.output_width * cinfo.output_components;
    printf("output_components=%d, row_stride=%d\n", cinfo.output_components, row_stride);
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

int main()
{
    int width = MJPG_W;
    int height = MJPG_H;

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

    //��ѯ֧�ֵĸ�ʽ
    struct v4l2_fmtdesc v4l_fmtdesct;
    v4l_fmtdesct.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l_fmtdesct.index = 1;
 
    ret = ioctl(fd, VIDIOC_ENUM_FMT, &v4l_fmtdesct);
    if(ret < 0)
    {
        perror("VIDIOC_ENUM_FMT failed");
        close(fd);
        return -1;
    }

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
    while(1)
    {
        //�ɼ�����
        ret = ioctl(fd, VIDIOC_DQBUF, &v4l_buffer);
        if(ret < 0)
        {
            perror("VIDIOC_DQBUF failed");
        }

        //��fb0����ʾ
        read_JPEG_file((const char*)mptr[v4l_buffer.index], rgb_data, v4l_buffer.length);
        lcd_show_rgb(rgb_data, width, height, lcd_width);

        ret = ioctl(fd, VIDIOC_QBUF, &v4l_buffer);
        if(ret < 0)
        {
            perror("VIDIOC_QBUF failed");
        }
    } 

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


