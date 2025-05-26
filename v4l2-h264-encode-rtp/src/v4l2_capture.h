#ifndef V4L2_CAPTURE_H
#define V4L2_CAPTURE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include "log.h"

int open_dev(string dev_name); //打开设备，返回文件描述符
int inquery_v4l2_fmt(int fd, struct v4l2_fmtdesc* fmt); //查询v4l2支持的格式，0是YUYV
int set_v4l2_fmt(int fd, int width, int height, struct v4l2_format* fmt); //设置摄像头的格式
int v4l2_buf_mmap(int fd, unsigned char* mptr[4], unsigned int buf_size[4], struct v4l2_buffer* v4l_buffer); //v4l2_buf内存映射，映射到用户空间
void capture_start(int fd); //开始采集
int capture_stop(int fd, unsigned char* mptr[4], unsigned int buf_size[4]); //停止采集，取消映射，关闭文件

#endif