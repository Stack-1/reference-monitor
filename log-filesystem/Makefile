obj-m += singlefilefs.o
singlefilefs-objs += file_system.o file.o dir.o

all:
	gcc singlefilemakefs.c -o singlefilemakefs
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules 
	make load-FS-driver
	make create-fs
	make mount-fs
	make clean

load-FS-driver:
	sudo insmod singlefilefs.ko

clean:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
	rm singlefilemakefs
	

create-fs:
	dd bs=4096 count=100 if=/dev/zero of=image
	./singlefilemakefs image
	sudo mkdir -p /opt/mount
        
mount-fs:
	sudo mount -o loop -t singlefilefs image /opt/mount/
