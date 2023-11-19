PBI=6

#define NIB(x)  PBI+((x&1)>>0), \
                PBI+((x&2)>>1), \
                PBI+((x&4)>>2), \
                PBI+((x&8)>>3)
tab0
        .byt NIB(10),NIB(12),NIB(0), NIB(0)
        .byt NIB(10),NIB(12),NIB(15),NIB(0)
        .byt NIB(10),NIB(12),NIB(0), NIB(15)
        .byt NIB(10),NIB(12),NIB(15),NIB(15)

; oh well.
tab1
        .byt PBI+0,PBI+1,PBI+0,PBI+1,PBI+0,PBI+0,PBI+1,PBI+1
        .byt PBI+0,PBI+0,PBI+0,PBI+0,PBI+0,PBI+0,PBI+0,PBI+0
        .byt PBI+0,PBI+1,PBI+0,PBI+1,PBI+0,PBI+0,PBI+1,PBI+1
        .byt PBI+1,PBI+1,PBI+1,PBI+1,PBI+0,PBI+0,PBI+0,PBI+0
        .byt PBI+0,PBI+1,PBI+0,PBI+1,PBI+0,PBI+0,PBI+1,PBI+1
        .byt PBI+0,PBI+0,PBI+0,PBI+0,PBI+1,PBI+1,PBI+1,PBI+1
        .byt PBI+0,PBI+1,PBI+0,PBI+1,PBI+0,PBI+0,PBI+1,PBI+1
        .byt PBI+1,PBI+1,PBI+1,PBI+1,PBI+1,PBI+1,PBI+1,PBI+1

