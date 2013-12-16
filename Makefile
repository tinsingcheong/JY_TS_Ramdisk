main: Jiayi_main.o ramdisk.o file_func.o rw.o
	gcc -o  main Jiayi_main.o ramdisk.o file_func.o rw.o
Jiayi_main.o: Jiayi_main.c ramdisk.h
	gcc -c -DUL_DEBUG Jiayi_main.c
ramdisk.o: ramdisk.c ramdisk_struct.h constant.h
	gcc -c -DUL_DEBUG ramdisk.c
file_func.o: file_func.c ramdisk.h  constant.h
	gcc -c -DUL_DEBUG file_func.c
rw.o: rw.c rw.h ramdisk_struct.h ramdisk.h
	gcc -c -DUL_DEBUG rw.c

clean:
	rm *.o main
