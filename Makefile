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

CFLAGS			=	-O0 -xc -std=gnu11 -fno-stack-protector -fshort-wchar -mno-red-zone -Wall -Wextra -pedantic-errors \
					-DGNU_EFI_USE_MS_ABI -DGNU_EFI_USE_EXTERNAL_STDARG
CFLAGS				+= $(CFLAGS-$@) $(EFIINCS)
CFLAGS-efi.o		+= -DGNU_EFI_USE_MS_ABI -fpic
CFLAGS-loader.o		+= -DGNU_EFI_USE_MS_ABI -fpic
CFLAGS-gfx.o		+= -DGNU_EFI_USE_MS_ABI -fpic
CFLAGS-common.o		+= -mcmodel=large
CFLAGS-console.o	+= -mcmodel=large
CFLAGS-start0.o		+= -mcmodel=large
CFLAGS-serial.o		+= -mcmodel=large
CFLAGS-kernel.o		+= -mcmodel=large
CFLAGS-idt.o		+= -mcmodel=large
LDFLAGS				= -nostdlib -znocombreloc
LDFLAGS				+= $(LDFLAGS-$@)
LDFLAGS-loader.so	+= -T $(EFI_LDS) -shared -Bsymbolic -L $(EFILIB) -L $(LIB) $(EFI_CRT_OBJS)

all: disk.img

fontppm.o: font.ppm
	objcopy -I binary -O elf64-x86-64 -B i386 $^ $@

kernel1.elf.img: head1.o common.o kernel.o serial.o fontppm.o console.o idt.o idt_s.o
	ld -T kernel1.ld -o $@ $^

kernel1.img: kernel1.elf.img
	objcopy -O binary $^ $@

kernel1img.o: kernel1.img
	objcopy -I binary -O elf64-x86-64 -B i386 $^ $@

serial-loader.o: serial.c
	$(CC) $(CFLAGS) -fpic -c -o $@ $^

kernel0.elf.img: head0.o common.o serial.o paging.o pagealloc.o start0.o kernel1img.o
	ld -T kernel0.ld -o $@ $^

kernel0.img: kernel0.elf.img
	objcopy -O binary $^ $@

kernel0img.o: kernel0.img
	objcopy -I binary -O elf64-x86-64 -B i386 $^ $@

loader.so: efi.o loader.o serial-loader.o kernel0img.o
	ld $(LDFLAGS) $^ -o $@ -lefi -lgnuefi

%.efi: %.so
	objcopy -j .text -j .sdata -j .data -j .dynamic \
		-j .dynsym  -j .rel -j .rela -j .reloc \
		--target=efi-app-$(ARCH) $^ $@

disk.img: loader.efi
	dd if=/dev/zero of=$@ bs=1k count=1440
	mformat -i $@ -f 1440 ::
	mmd -i $@ ::/EFI
	mmd -i $@ ::/EFI/BOOT
	mcopy -i $@ $^ ::/EFI/BOOT/BOOTx64.EFI

OVMF.fd:
	cp /usr/share/ovmf/OVMF.fd .

run-qemu: disk.img OVMF.fd
	./run.sh

clean:
	rm -f *.efi *.o *.so *.img
	rm -rf hda
