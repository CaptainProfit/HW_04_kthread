# Путь к исходникам ядра (может потребоваться изменить)
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
# Текущая директория с исходниками
PWD := $(shell pwd)

CLANG_FORMAT_VERS ?= 19
CLANG_FORMAT := clang-format-$(CLANG_FORMAT_VERS)
CLANG_FORMAT_FLAGS += -i
FORMAT_FILES := $(SRC_DIR)/*.c

# Цель по умолчанию — сборка модулей
kbuild:
	make -C $(KERNEL_DIR) M=$(PWD) modules

unload:
	sudo rmmod kernel_rw

load: kbuild
	sudo insmod ./kernel_rw.ko

check-kernel: load

check-user: user
	./user_rw

format:
	$(CLANG_FORMAT) $(CLANG_FORMAT_FLAGS) $(FORMAT_FILES)

user:
	g++ -g user_rw.c -lpthread -o user_rw

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
