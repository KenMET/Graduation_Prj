cmd_arch/arm/boot/compressed/vmlinux := /home/ken/workspace/tools/arm-2009q3/bin/arm-none-linux-gnueabi-ld -EL    --defsym zreladdr=0x40008000 -p --no-undefined -X -T arch/arm/boot/compressed/vmlinux.lds arch/arm/boot/compressed/head.o arch/arm/boot/compressed/piggy.gzip.o arch/arm/boot/compressed/misc.o arch/arm/boot/compressed/decompress.o arch/arm/boot/compressed/lib1funcs.o -o arch/arm/boot/compressed/vmlinux 
