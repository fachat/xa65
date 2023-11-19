; test instructions that failed during our testing
; and test instructions in the tokenizer bordering mvn/mvp
w = $01
lda $d020
lsr
mvn w, $02
#ifdef OLD_SYNTAX
mvn $0201
mvn $0066
#else
mvn $02,$01
mvn $00,$66
#endif
mvp $03, w+3
#ifdef OLD_SYNTAX
mvp $0403
mvp $0088
#else
mvp $04,$03
mvp $00,$88
#endif
#ifdef OLD_SYNTAX
mvp $0403,$03
mvp $03,$0405
mvn $0403,$03
mvn $03,$0405
#endif
nop
.byt $5a, $b6
.word $b65a
