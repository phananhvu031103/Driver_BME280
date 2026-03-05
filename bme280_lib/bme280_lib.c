#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include "bme280_lib.h" // File header cho thư viện
#define BME280_DEVICE "/dev/bme280"
#define BME280_IOCTL_MAGIC 'B'
#define BME280_READ_TEMPERATURE _IOR(BME280_IOCTL_MAGIC, 1, char[32])
#define BME280_READ_PRESSURE _IOR(BME280_IOCTL_MAGIC, 2, char[32])
#define BME280_READ_HUMIDITY _IOR(BME280_IOCTL_MAGIC, 3, char[32])

static int bme280_fd = -1; // File descriptor 

int bme280_init() {
    bme280_fd = open(BME280_DEVICE, O_RDWR);
    if (bme280_fd < 0) {
        perror("Failed to open BME280 device");
        return -errno;
    }
    return 0; 
}

int bme280_read_temperature(char *temperature) {
    if (bme280_fd < 0) {
        fprintf(stderr, "Device not initialized\n");
        return -ENODEV;
    }
    if (ioctl(bme280_fd, BME280_READ_TEMPERATURE, temperature) < 0) {
        perror("Failed to read temperature");
        return -errno;
    }
    return 0; 
}

int bme280_read_pressure(char *pressure) {
    if (bme280_fd < 0) {
        fprintf(stderr, "Device not initialized\n");
        return -ENODEV;
    }
    if (ioctl(bme280_fd, BME280_READ_PRESSURE, pressure) < 0) {
        perror("Failed to read pressure");
        return -errno;
    }
    return 0; 
}

int bme280_read_humidity(char *humidity) {
    if (bme280_fd < 0) {
        fprintf(stderr, "Device not initialized\n");
        return -ENODEV;
    }
    if (ioctl(bme280_fd, BME280_READ_HUMIDITY, humidity) < 0) {
        perror("Failed to read humidity");
        return -errno;
    }
    return 0; 
}

void bme280_close() {
    if (bme280_fd >= 0) {
        close(bme280_fd);
        bme280_fd = -1;
    }
}
