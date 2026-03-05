# Các biến cấu hình
CC = gcc
CFLAGS = -fPIC          # Tùy chọn để biên dịch mã nguồn thành mã đối tượng chia sẻ
LDFLAGS = -shared       # Tùy chọn để tạo thư viện chia sẻ (.so)

# Các tên file
SRC = bme280_lib.c      # Tên file mã nguồn
OBJ = bme280_lib.o      # Tên file đối tượng
LIBRARY = libbme280.so  # Tên thư viện xuất ra

# Mục tiêu mặc định (tạo thư viện)
all: $(LIBRARY)

# Quy trình biên dịch .c thành .o
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

# Quy trình tạo thư viện .so
$(LIBRARY): $(OBJ)
	$(CC) $(LDFLAGS) -o $(LIBRARY) $(OBJ)

# Mục tiêu dọn dẹp
clean:
	rm -f $(OBJ) $(LIBRARY)
