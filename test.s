.section prva_sekcija
main:
    ld $5, %r1
    ld $6, %r2
    sub %r1, %r2
    ld $2, %r3
    mul %r2, %r3
    halt
.end

