obj-m += the_stack_reference_monitor.o
the_stack_reference_monitor-objs += stack_reference_monitor.o syscall-mount/scth.o utils/utils.o utils/blacklist.o kprobes/kprobes.o  log/logger.o
password ?= $(shell bash ./ask_password.sh)


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 
	sudo make mount
	make clean

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

mount:
	@sudo insmod the_stack_reference_monitor.ko syscalls_table_address=$$(cat /sys/module/the_usctm/parameters/sys_call_table_address) password="$(password)"


