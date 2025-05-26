#ifndef YUYV_TO_I420_H
#define YUYV_TO_I420_H

#include "log.h"

void yuyv_to_i420_with_stride(unsigned char* yuyv,
                              unsigned char* y_plane, int y_stride,
                              unsigned char* u_plane, int u_stride,
                              unsigned char* v_plane, int v_stride,
                              int width, int height);

#endif