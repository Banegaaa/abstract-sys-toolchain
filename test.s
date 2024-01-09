.section prva_sekcija
    .skip 10
a:
    .skip 30
.section druga_sekcija
    .equ b, 2
    .word a, 0xa, b, 0xb, a, 0xc, b, 0xd, a, 0xf
.end