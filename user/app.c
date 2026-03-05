#include <stdio.h>
#include "bme280_lib.h"

int main() {
    char temp[32], pressure[32], humidity[32];

    if (bme280_init() != 0) {
        printf("Failed to initialize BME280\n");
        return -1;
    }

    if (bme280_read_temperature(temp) == 0) {
        printf("Temperature: %s \u00B0C\n", temp);
    }

    if (bme280_read_pressure(pressure) == 0) {
        printf("Pressure: %s Pa\n", pressure);
    }

    if (bme280_read_humidity(humidity) == 0) {
        printf("Humidity: %s %%\n", humidity);
    }

    bme280_close();

    return 0;
}
