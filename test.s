.section prva_sekcija
bgt %r1, %r2, 0x999
bgt %r1, %r2, main
main:
bgt %r1, %r2, f1
f1:
bgt %r1, %r2, 1
.end