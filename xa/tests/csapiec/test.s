	.text

c0        =*-1

#ifdef BUG
	.data
#endif

cmdt      .asc "$",0
cow       .asc "@",0
c2        .asc "rename",0
c3        .asc "scratch",0
c4        .asc "copy",0
c5        .asc "new",0
c6        .asc "validate",0
c7        .asc "initialize",0
;c8        .asc "rmdir",0
;c9        .asc "mkdir",0
;c10       .asc "chdir",0
c11       .asc "assign",0
c12       .asc "cd",0
c13       .asc "rd",0
c14       .asc "md",0
c15       .asc "drv",0      ; iec-bus-unit 

cmda    .byt cmdt-c0
    	.byt <c0
        .byt <cow-c0
        .word <cow-c0
        .byt c2-c0, c3-c0, c4-c0, c5-c0, c6-c0, c7-c0 /*,c8-c0*/
          .byt /*c9-c0,c10-c0,*/ c11-c0, c12-c0, c13-c0, c14-c0, c15-c0, 0

