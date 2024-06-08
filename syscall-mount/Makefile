obj-m += sys_calls_ref_monitor.o
sys-calls-ref-monitorobjs += sys_calls_ref_monitor.o 

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 
	sudo make mount
	make clean

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

mount:
	insmod sys_calls_ref_monitor.ko
