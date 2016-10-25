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

CFLAGS				= -O0 -xc -std=gnu11 -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -Wall -Wextra -pedantic-errors -DGNU_EFI_USE_MS_ABI
CFLAGS				+= $(CFLAGS-$@) $(EFIINCS)
CFLAGS-efi.o		+= -DGNU_EFI_USE_MS_ABI
CFLAGS-loader.o		+= -DGNU_EFI_USE_MS_ABI
CFLAGS-gfx.o		+= -DGNU_EFI_USE_MS_ABI
LDFLAGS				= -nostdlib -znocombreloc
LDFLAGS				+= $(LDFLAGS-$@)
LDFLAGS-loader.so	+= -T $(EFI_LDS) -shared -Bsymbolic -L $(EFILIB) -L $(LIB) $(EFI_CRT_OBJS)

all: $(TARGET) copy

fontppm.o: font.ppm
	objcopy -I binary -O elf64-x86-64 -B i386 $^ $@

kernel.elf.img: common.o head.o kernel.o serial.o fontppm.o console.o paging.o pagealloc.o
	ld -T kernel.ld -o $@ $^

kernel.img: kernel.elf.img
	objcopy -O binary $^ $@

kernelimg.o: kernel.img
	objcopy -I binary -O elf64-x86-64 -B i386 $^ $@

loader.so: efi.o gfx.o loader.o serial.o kernelimg.o
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