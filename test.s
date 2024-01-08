csrrd status, %r0
csrrd cause, %r0
csrrd handler, %r0

csrwr %r0, status
csrwr %r0, cause
csrwr %r0, handler