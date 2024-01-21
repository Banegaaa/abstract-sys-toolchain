.section prva_sekcija
main:
    add %r1, %r2
    sub %r1, %r2
    ld [%r1 + 4], %r2
    ld 0x999, %r2
    call main
.end

# NOT FUCKING WORKING BRO
