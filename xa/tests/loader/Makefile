
all: loader example test2 rom65
clean:
	rm -f loader test2 example a.o65 rom65

loader: loader.a65 file.def
	../../xa loader.a65 -o loader

example: test.a
	../../xa -R test.a -o example

test2: test2.a
	../../xa test2.a -o test2

rom65: test.a
	# ugly!
	( cd .. && ../mkrom.sh -O "-G" -S "-bd 1234" -R loader/rom65 loader/test.a loader/test.a  )
	../hextool -cmp=rom65.ok < rom65

