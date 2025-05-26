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

//jpeg解码
int read_JPEG_file(const char* jpeg_data, unsigned char* rgb_data, int size)
{
    struct jpeg_error_mgr jerr;
    struct jpeg_decompress_struct cinfo;
    cinfo.err = jpeg_std_error(&jerr);
    //创建解码对象并初始化
    jpeg_create_decompress(&cinfo);
    //指定输入源
    jpeg_mem_src(&cinfo, (unsigned char*)jpeg_data, size);
    //读取JPEG文件头
    jpeg_read_header(&cinfo, TRUE);
    //开始解码
    jpeg_start_decompress(&cinfo);
    //申请存储一行数据的内存空间
    int row_stride = cinfo.output_width * cinfo.output_components;
    unsigned char* buffer = (unsigned char*)malloc(row_stride);
    int i = 0;
    while(cinfo.output_scanline < cinfo.output_height)
    {
        //读取一行数据
        jpeg_read_scanlines(&cinfo, &buffer, 1);
        //将数据存储到RGB数据中
        memcpy(rgb_data + i * row_stride, buffer, row_stride);
        i++;

    }

    //解码完成  
    jpeg_finish_decompress(&cinfo);
    //释放解码对象
    jpeg_destroy_decompress(&cinfo);
    //释放内存
    free(buffer);
    return 0;
}

//lcd显示rgb数据
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

            // ARGB8888: alpha固定为0xFF
            p[i * lcd_width + j] = (0xFF << 24) | (r << 16) | (g << 8) | b;
        }
    }
}

// YUYV转RGB888
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

        // 第一个像素
        r = y0 + 1.402 * v;
        g = y0 - 0.344136 * u - 0.714136 * v;
        b = y0 + 1.772 * u;
        rgb[index++] = r < 0 ? 0 : (r > 255 ? 255 : r);
        rgb[index++] = g < 0 ? 0 : (g > 255 ? 255 : g);
        rgb[index++] = b < 0 ? 0 : (b > 255 ? 255 : b);

        // 第二个像素
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

    //打开摄像头设备设备
    int fd = open("/dev/video0", O_RDWR, 0);
    if(fd < 0)
    {
        perror("open file failed");
        return -1;
    }

    //打开lcd设备
    fb_fd = open("/dev/fb0", O_RDWR);
    if(fb_fd < 0)
    {
        perror("open fb0 failed");
        return -1;
    }
    
    int ret = 0;

    //读取fb0真实尺寸
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

    //fb映射
    fb_ptr = (unsigned int*)mmap(NULL, lcd_width*lcd_height*4, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0); 

/*     //查询设备信息
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

    printf("index=%d\n", v4l_fmtdesct.index);
    printf("flags=%d\n", v4l_fmtdesct.flags);
    printf("description=%s\n", v4l_fmtdesct.description);
    unsigned  char* p = (unsigned char*)&v4l_fmtdesct.pixelformat;
    printf("pixelformat=%c%c%c%c\n", p[0],p[1],p[2],p[3]);

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

    unsigned char rgb_data[width*height*3];
/*     while(1)
    {
        //采集数据
        ret = ioctl(fd, VIDIOC_DQBUF, &v4l_buffer);
        if(ret < 0)
        {
            perror("VIDIOC_DQBUF failed");
        }

        //保存为mjpg文件
        //FILE *file = fopen("my.jpg", "wb");
        //fwrite(mptr[v4l_buffer.index], v4l_buffer.length, 1, file);
        //fclose(file); 

        //在fb0上显示
        read_JPEG_file((const char*)mptr[v4l_buffer.index], rgb_data, v4l_buffer.length);
        lcd_show_rgb(rgb_data, width, height);

        ret = ioctl(fd, VIDIOC_QBUF, &v4l_buffer);
        if(ret < 0)
        {
            perror("VIDIOC_QBUF failed");
        }
    }  */

    //采集yuv数据
    //FILE *file = fopen("my.yuv", "wb");
    while(1)
    {
        //采集数据
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

    //释放lcd映射
    ret = munmap(fb_ptr, width*height*4);
    if(ret < 0)
    {
        perror("unmap fb0 failed");
    }

    //关闭文件
    close(fd);
    close(fb_fd);

    return 0;
}

