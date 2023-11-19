
all: loader example test2 rom65

loader: loader.a65 file.def
	../xa loader.a65 -o loader

clean:
	rm -f loader test2 example a.o65 rom65

example: test.a
	../xa -R test.a -o example

test2: test2.a
	../xa test2.a -o test2

rom65: test.a
	../mkrom.sh -O "-G" -S "-bd 1234" -R rom65 test.a test.a 

