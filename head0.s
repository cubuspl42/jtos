.section .head0
mov $stack_top, %rsp
call kernel_start0
hlt

.section .stack
.align 16
stack_bottom:
.skip 0x10000 # 64 KiB
stack_top: