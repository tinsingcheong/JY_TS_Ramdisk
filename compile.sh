#!/bin/bash

make
gcc ramdisk_user.c test_file.c -o test
insmod Ramdisk.ko
