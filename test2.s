.section prva_sekcija
lolo:
    .skip 3
    add %r1, %r2
    sub %r1, %r2
    ld [%r1 + 4], %r2
    ld 0x999, %r2
    call lolo
.section druga_sekcija
lolo2:
    .skip 3
    add %r1, %r2
    sub %r1, %r2
    ld [%r1 + 4], %r2
    ld 0x999, %r2
    call lolo2
.end

