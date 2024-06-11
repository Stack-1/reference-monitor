MAKE = make -C

all:
	$(MAKE) /lib/modules/$(shell uname -r)/build M=$(PWD)/syscall-table-discoverer modules
	$(MAKE) /lib/modules/$(shell uname -r)/build M=$(PWD)/reference-monitor modules

clean:
	$(MAKE) syscall-table-discoverer/ clean
	$(MAKE) reference-monitor/ clean

mount: 
	$(MAKE) syscall-table-discoverer/ mount
	$(MAKE) reference-monitor/ mount

unmount:
	rmmod the_usctm
	rmmod stack_reference_monitor

