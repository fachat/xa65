# 1 "over.c"
# 1 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 1 "<command-line>" 2
# 1 "over.c"
# 13 "over.c"
 lda #1
 jmp buggy
 rts

# 1 "over.h" 1




fuzz lda #5
 sta $0400
 bne fuzz
 rts

 lda @$c0c0c0
# 18 "over.c" 2
