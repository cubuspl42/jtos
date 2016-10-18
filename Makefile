ARCH			= $(shell uname -m | sed s,i[3456789]86,ia32,)

OBJS			= main.o
TARGET			= hello.efi

KERNEL_OBJS		= kernel.o

EFIINC			= /usr/local/include/efi
EFIINCS			= -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
LIB				= /usr/lib
EFILIB			= /usr/local/lib
EFI_CRT_OBJS	= $(EFILIB)/crt0-efi-$(ARCH).o
EFI_LDS			= $(EFILIB)/elf_$(ARCH)_efi.lds
CC				= /usr/bin/clang

CFLAGS				= -xc -std=gnu11 -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -Wall -Wextra
CFLAGS				+= $(CFLAGS-$@)
CFLAGS-main.o		+= $(EFIINCS) -DGNU_EFI_USE_MS_ABI
LDFLAGS				= -nostdlib
LDFLAGS				+= $(LDFLAGS-$@)
LDFLAGS-hello.so	+= -T $(EFI_LDS) -shared -Bsymbolic -L $(EFILIB) -L $(LIB) $(EFI_CRT_OBJS)

all: $(TARGET) copy kernel.text.bin

kernel.text.bin: kernel.o
	objcopy -O binary --only-section=.text $^ $@

hello.so: $(OBJS)
	ld $(LDFLAGS) $(OBJS) -o $@ -lefi -lgnuefi

%.efi: %.so
	objcopy -j .text -j .sdata -j .data -j .dynamic \
		-j .dynsym  -j .rel -j .rela -j .reloc \
		--target=efi-app-$(ARCH) $^ $@

hda:
	mkdir -p hda

copy: hello.efi hda
	cp hello.efi hda/

clean:
	rm -f *.efi *.o *.so
	rm -rf hda