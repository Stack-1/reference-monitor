obj-m += the_usctm.o
the_usctm-objs += usctm.o ./lib/vtpmo.o

all:
	@echo Compiling system table discoverer
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 
	@echo Mounting system table discoverer
	sudo make mount
	@echo Mounting completed correctly, syscalls table addres sasved in pseudofile
	make clean

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

mount:
	insmod the_usctm.ko
	

	

