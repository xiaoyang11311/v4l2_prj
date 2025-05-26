#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <string.h>
using namespace std;

static string gettime()
{
    const char* time_fmt = "%Y-%m-%d %H:%M:%S";
    time_t t = time(NULL);
    char time_str[64];
    strftime(time_str, sizeof(time_str), time_fmt, localtime(&t));

    return time_str;
}

#define LOGI(format, ...) fprintf(stderr, "[INFO]%s [%s:%d %s()] " format "\n", gettime().data(),__FILE__,__LINE__,__func__ ,##__VA_ARGS__)
#define LOGE(format, ...) fprintf(stderr, "[ERROR]%s [%s:%d %s()] " format "\n", gettime().data(),__FILE__,__LINE__,__func__ ,##__VA_ARGS__)

#endif