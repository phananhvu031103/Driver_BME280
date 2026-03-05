#ifndef BME280_LIB_H
#define BME280_LIB_H

int bme280_init();
int bme280_read_temperature(char *temperature);
int bme280_read_pressure(char *pressure);
int bme280_read_humidity(char *humidity);
void bme280_close();

#endif 
