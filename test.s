.global main
.section my_section_1
.word 8
main:
    call a
a:
    .word 1
    jmp b
b:
    .word 2
    ret
    add %r1, %r2
    sub %r1, %r2
    mul %r1, %r2
    div %r1, %r2
    pop %r1
    push %r1
    and %r1, %r2
    or %r1, %r2
    xor %r1, %r2
    not %r1
.end