#if XA_MAJOR != 2
#error make sure this is up to date for future tests
#endif
#if XA_MINOR != 4
#error make sure this is up to date for future tests
#endif

	* = $1000

#ifdef BAD0
.assert w=w, "what the"
#endif

w

#ifdef BAD1
#error bad1
#endif

#ifdef BAD2
.assert *<>$1000, "everything is bad"
#endif

	lda #1

.assert *-2=w, "everything is really bad"
.assert w==w, "everything sucks"
.assert ((w & $ff00)>>8)=16, "everything is hideous"
#ifdef BAD3
.assert *==$1003, "everything is terrible"
#endif
#ifdef BAD4
.assert w!=w, "everything sucks and is terrible and hideous and bad"
#endif

#ifdef BAD5
#if XA_MAJOR != 1
#error I want a really old version
#endif
#endif

