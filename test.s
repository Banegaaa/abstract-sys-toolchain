.global main
.section my_section_1
.word 8
main:
    bne %r1, %r2, a
    beq %r1, %r2, b
    bgt %r1, %r2, c
a:
    .word 1
b:
    .word 2
c:
    .word 3
