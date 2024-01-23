.section prva_sekcija
main:
    .skip 3
    add %r1, %r2
    sub %r1, %r2
    ld [%r1 + 4], %r2
    ld 0x999, %r2
    call main
.section druga_sekcija
main2:
    .skip 3
    add %r1, %r2
    sub %r1, %r2
    ld [%r1 + 4], %r2
    ld 0x999, %r2
    call main
.end

