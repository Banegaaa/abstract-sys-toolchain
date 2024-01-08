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
.end