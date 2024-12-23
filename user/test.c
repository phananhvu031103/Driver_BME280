#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

uint8_t read_buf[31];

int main() {
    int fd;
    char any;
    printf("*********************************\n");
    printf("*******Test BME280 Driver*******\n");
    fd = open("/dev/bme280", O_RDWR);
    if (fd < 0) {
        printf("Cannot open device file...\n");
        return 0;
    }
    while (1) {
		printf("*********************************\n");
        printf("Enter Anything to read Data: ");
        scanf("%c", &any);
        while (getchar() != '\n');
        read(fd, read_buf, 31);
        printf("Data = %s", read_buf);
    }
    close(fd);
    return 0;
}
