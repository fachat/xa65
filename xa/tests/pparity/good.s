#define FOO(a,b,c)
#define BAR(a,b,c) FOO(c,b,a)

#define FOO(a,b,c)
#define BAR(b,c) FOO("x",b,c)

#define FOO(a,b,c) .byt a,b,c
#define BAR(c) FOO("1", "2", c)

BAR("3")
