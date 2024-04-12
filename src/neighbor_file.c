#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h> // 添加此头文件以获取 IF_NAMESIZE

#define MAX_LINE_LENGTH 256

void print_arp_table(FILE *file) {
    char line[MAX_LINE_LENGTH];

    // 打印表头
    printf("IP\t\tMAC\t\tInterface\tType\n");

    // 读取并解析每一行
    while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
        // 跳过表头行
        if (strstr(line, "IP address") != NULL)
            continue;

        char ip[16], mac[18], iface[IF_NAMESIZE], type[16];
        unsigned int mac_part1, mac_part2, mac_part3, mac_part4, mac_part5, mac_part6;
int count = sscanf(line, "%s %x %x %x:%x:%x:%x %*s %s %[^\n]", ip, &mac_part1, &mac_part2, &mac_part3, &mac_part4, &mac_part5, &mac_part6, type, iface);


        // 检查字段数量是否正确
        printf("sscanf count: %d\n", count);
        if (count == 9) {
            // 将 MAC 地址转换为字符串格式
            snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x", mac_part1, mac_part2, mac_part3, mac_part4, mac_part5, mac_part6);

            // 打印 IP、MAC、接口和类型信息
            printf("%s\t%s\t%s\t\t%s\n", ip, mac, iface, type);
        }
        else {
            printf("Error parsing line: %s\n", line);
        }
    }
}

int main() {
    FILE *file = fopen("/proc/net/arp", "r");
    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    print_arp_table(file);

    fclose(file);
    return EXIT_SUCCESS;
}

