savedcmd_/home/trung/bme280-driver/bme280/bme280.mod := printf '%s\n'   bme280.o | awk '!x[$$0]++ { print("/home/trung/bme280-driver/bme280/"$$0) }' > /home/trung/bme280-driver/bme280/bme280.mod
