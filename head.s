.section .bss
.align 16
stack_bottom:
.skip 0x80000 # 512 KiB
stack_top:

.section .head
.global _start
.type _start, @function
_start:
	# Setup new stack
	mov $stack_top, %esp

	# kernel_params is passed via %rdi
	call kernel_main

	cli
1:	hlt
	jmp 1b

.size _start, . - _start
