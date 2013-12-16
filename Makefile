obj-m += ramdisk_module.o
ramdisk_module-objs := ramdisk.o file_func.o rw.o ramdisk_module.o

GCC = gcc
CXXFLAGS = -DKL_DEBUG
KDIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)

all:
	make -C $(KDIR) CFLAGS=$(CXXFLAGS) SUBDIRS=$(PWD) modules
clean:
	make -C $(KDIR) SUBDIRS=$(PWD) clean







#user-level debug makefile
#main: Jiayi_main.o ramdisk.o file_func.o rw.o
#	gcc -o main Jiayi_main.o ramdisk.o file_func.o rw.o
#Jiayi_main.o: Jiayi_main.c ramdisk.h
#	gcc -c Jiayi_main.c
#ramdisk.o: ramdisk.c ramdisk_struct.h constant.h
#	gcc -c ramdisk.c
#file_func.o: file_func.c ramdisk.h  constant.h
#	gcc -c file_func.c
#rw.o: rw.c rw.h ramdisk_struct.h ramdisk.h
#	gcc -c rw.c

#clean:
#	rm *.o main
