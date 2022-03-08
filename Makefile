ifeq ($(KERNELRELEASE), )

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -rf *.o *.ko .depend *.mod.o *.mod.c Module.* modules.*

.PHONY:modules clean

else

obj-m:=get_func_entry_args.o

endif
