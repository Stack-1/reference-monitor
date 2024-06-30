obj-m += the_stack_reference_monitor.o
the_stack_reference_monitor-objs += stack_reference_monitor.o lib/scth.o utils.o


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 
	sudo make mount
	make clean

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

mount:
	@read -p "Select a password: " password; \
	insmod the_stack_reference_monitor.ko syscalls_table_address=$$(cat /sys/module/the_usctm/parameters/sys_call_table_address) password="$$password"
