obj-m += sys_calls_mount.o
sys-calls-ref-monitorobjs += sys_calls_mount.o 

all:
	cd syscall-table-discoverer/ && make
	@echo Starting compilation of reference monitor
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 
	sudo make mount
	@echo Reference monitor mounted and working
	make clean

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

mount:
	@read -p "Select a password: " password; \
	insmod sys_calls_mount.ko the_syscall_table=$$(cat /sys/module/the_usctm/parameters/sys_call_table_address) password="$$password"
