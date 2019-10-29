; test instructions that failed during our testing
; and test instructions in the tokenizer bordering mvn/mvp
w = $01
lda $d020
lsr
mvn w, $02
mvn $0201
mvn $0066
mvp $03, w+3
mvp $0403
mvp $0088
nop
.byt $5a, $b6
.word $b65a
