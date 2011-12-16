
;
; FILE image.a
;

#include "cpu.inc"

    .text
    *=$C000

Foo:
    WDM(170)

