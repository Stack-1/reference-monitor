obj-m += the_usctm.o
the_usctm-objs += usctm.o ./lib/vtpmo.o

all:
	@echo Compiling system table discoverer
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules 
	@echo Mounting system table discoverer
	sudo make mount
	@echo Mounting completed correctly, syscalls table addres sasved in pseudofile
	make clean

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

mount:
	insmod the_usctm.ko
	

	

