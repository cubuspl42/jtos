ARCH			= $(shell uname -m | sed s,i[3456789]86,ia32,)

TARGET			= loader.efi

KERNEL_OBJS		= kernel.o

EFIINC			= /usr/local/include/efi
EFIINCS			= -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
LIB				= /usr/lib
EFILIB			= /usr/local/lib
EFI_CRT_OBJS	= $(EFILIB)/crt0-efi-$(ARCH).o
EFI_LDS			= $(EFILIB)/elf_$(ARCH)_efi.lds
CC				= /usr/bin/clang

CFLAGS				= -xc -std=gnu11 -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -Wall -Wextra -pedantic-errors
CFLAGS				+= $(CFLAGS-$@) $(EFIINCS)
CFLAGS-efi.o		+= -DGNU_EFI_USE_MS_ABI
CFLAGS-loader.o		+= -DGNU_EFI_USE_MS_ABI
CFLAGS-gfx.o		+= -DGNU_EFI_USE_MS_ABI
LDFLAGS				= -nostdlib
LDFLAGS				+= $(LDFLAGS-$@)
LDFLAGS-loader.so	+= -T $(EFI_LDS) -shared -Bsymbolic -L $(EFILIB) -L $(LIB) $(EFI_CRT_OBJS)

all: $(TARGET) copy kernel.text.bin

kernel.img: head.o kernel.o # make a flat kernel image
	ld -T kernel.ld -o $@ $^

kernelimg.o: kernel.img # wrap flat kernel image into an object
	objcopy -I binary -O elf64-x86-64 -B i386 $^ $@

loader.so: efi.o gfx.o loader.o kernelimg.o # inject kernel image into bootloader
	ld $(LDFLAGS) $^ -o $@ -lefi -lgnuefi

%.efi: %.so
	objcopy -j .text -j .sdata -j .data -j .dynamic \
		-j .dynsym  -j .rel -j .rela -j .reloc \
		--target=efi-app-$(ARCH) $^ $@

hda:
	mkdir -p hda/EFI/BOOT

copy: loader.efi hda
	cp loader.efi hda/EFI/BOOT/BOOTx64.efi

clean:
	rm -f *.efi *.o *.so *.img
	rm -rf hda