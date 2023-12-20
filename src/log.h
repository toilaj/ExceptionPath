//
// Created by root on 12/19/23.
//

#ifndef LWFW_LOG_H
#define LWFW_LOG_H
#include <stdio.h>
//2023-09-23 00:00:03 +0530 < info:2635:2692> dpid_session_cmd_count:4566
#define LOG(fmt, ...) printf("%s < %s:%d > "fmt"\n", __FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__)
#endif //LWFW_LOG_H
