# jtos
jtos is an EFI-compatible operating system for x86-64 architecture.

![jtos inside QEMU](https://github.com/cubuspl42/jtos/blob/master/screenshot.png)

## Features

* Terminal
* printing time (in hex)
* echo mode

## Compiling

Currently jtos can be compiled only on Linux. Provided `Makefile` will probably work out of the box only on Ubuntu.

Dependencies:

* clang
* gnu-efi

## Running

jtos' QEMU image can be run on QEMU on Linux, OS X and possibly more.

`./run.sh`

Dependencies:
* QEMU
* OVMF
