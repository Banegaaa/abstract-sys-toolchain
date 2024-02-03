.section prva_sekcija
main:
    ld $3, %r1
    ld $8, %r2
    shr %r1, %r2
main2:
    halt
.end

