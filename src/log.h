//
// Created by root on 12/19/23.
//

#ifndef LWFW_LOG_H
#define LWFW_LOG_H
#include <stdio.h>
#define LOG(fmt, ...) printf("%s [%s]:%d "fmt"\n", __FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__)
#endif //LWFW_LOG_H
