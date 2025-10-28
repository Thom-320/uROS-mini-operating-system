.section .text.start
.global _start
.global ctx_switch

_start:
    # Set stack pointer to kernel stack (aligned to 16 bytes)
    la sp, _stack_top
    andi sp, sp, -16
    
    # Jump to C kernel main
    call kmain
    
    # Halt if kmain returns
halt:
    wfi
    j halt

# Context switch: ctx_switch(context_t *from, context_t *to)
# a0 = from, a1 = to
# Offsets match context_t in uros.h:
# x1(ra)=0, x2(sp)=8, x3(gp)=16, x4(tp)=24, x5-x31, sstatus=248, sepc=256
.align 4
ctx_switch:
    # Save context to 'from' (a0)
    sd x1,  0(a0)      # ra
    sd x2,  8(a0)      # sp
    sd x3,  16(a0)     # gp
    sd x4,  24(a0)     # tp
    sd x5,  32(a0)     # t0
    sd x6,  40(a0)     # t1
    sd x7,  48(a0)     # t2
    sd x8,  56(a0)     # s0/fp
    sd x9,  64(a0)     # s1
    sd x10, 72(a0)     # a0
    sd x11, 80(a0)     # a1
    sd x12, 88(a0)     # a2
    sd x13, 96(a0)     # a3
    sd x14, 104(a0)    # a4
    sd x15, 112(a0)    # a5
    sd x16, 120(a0)    # a6
    sd x17, 128(a0)    # a7
    sd x18, 136(a0)    # s2
    sd x19, 144(a0)    # s3
    sd x20, 152(a0)    # s4
    sd x21, 160(a0)    # s5
    sd x22, 168(a0)    # s6
    sd x23, 176(a0)    # s7
    sd x24, 184(a0)    # s8
    sd x25, 192(a0)    # s9
    sd x26, 200(a0)    # s10
    sd x27, 208(a0)    # s11
    sd x28, 216(a0)    # t3
    sd x29, 224(a0)    # t4
    sd x30, 232(a0)    # t5
    sd x31, 240(a0)    # t6
    
    # Save sstatus and sepc
    csrr t0, sstatus
    sd t0, 248(a0)
    csrr t0, sepc
    sd t0, 256(a0)
    
    # Restore context from 'to' (a1)
    ld x1,  0(a1)      # ra
    ld x2,  8(a1)      # sp
    ld x3,  16(a1)     # gp
    ld x4,  24(a1)     # tp
    ld x5,  32(a1)     # t0
    ld x6,  40(a1)     # t1
    ld x7,  48(a1)     # t2
    ld x8,  56(a1)     # s0/fp
    ld x9,  64(a1)     # s1
    ld x10, 72(a1)     # a0
    ld x11, 80(a1)     # a1
    ld x12, 88(a1)     # a2
    ld x13, 96(a1)     # a3
    ld x14, 104(a1)    # a4
    ld x15, 112(a1)    # a5
    ld x16, 120(a1)    # a6
    ld x17, 128(a1)    # a7
    ld x18, 136(a1)    # s2
    ld x19, 144(a1)    # s3
    ld x20, 152(a1)    # s4
    ld x21, 160(a1)    # s5
    ld x22, 168(a1)    # s6
    ld x23, 176(a1)    # s7
    ld x24, 184(a1)    # s8
    ld x25, 192(a1)    # s9
    ld x26, 200(a1)    # s10
    ld x27, 208(a1)    # s11
    ld x28, 216(a1)    # t3
    ld x29, 224(a1)    # t4
    ld x30, 232(a1)    # t5
    ld x31, 240(a1)    # t6
    
    # Restore sstatus and sepc
    ld t0, 248(a1)
    csrw sstatus, t0
    ld t0, 256(a1)
    csrw sepc, t0
    
    # Ensure SP is 16-byte aligned
    andi sp, sp, -16
    
    ret

