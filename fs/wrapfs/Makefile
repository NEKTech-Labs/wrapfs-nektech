WRAPFS_VERSION="0.1"

EXTRA_CFLAGS += -DWRAPFS_VERSION=\"$(WRAPFS_VERSION)\" -DNEKTECH_LOGGER

obj-$(CONFIG_WRAP_FS) += wrapfs.o

wrapfs-y := dentry.o file.o inode.o main.o super.o lookup.o mmap.o nektech_logger.o
KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

default:
                $(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:  
                $(MAKE) -C $(KERNELDIR) M=$(PWD) clean

