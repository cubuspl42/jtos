.section .head0
mov $stack_top, %rsp
call kernel_start0
ret

.section .stack0
.align 16
stack_bottom:
.skip 0x4000 # 16 KiB
stack_top:
