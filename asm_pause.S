    .section .text
    .globl asm_pause
    .type asm_pause, @function

asm_pause:
    beqz a0, .Ldone
.Lloop:
    addi a0, a0, -1
    bnez a0, .Lloop
.Ldone:
    ret
