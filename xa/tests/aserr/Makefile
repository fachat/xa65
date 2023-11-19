
default: test.s
	@echo expected to fail....
	../../xa -DBAD0 test.s || exit 0 && exit 1
	../../xa -DBAD1 test.s || exit 0 && exit 1
	../../xa -DBAD2 test.s || exit 0 && exit 1
	../../xa -DBAD3 test.s || exit 0 && exit 1
	../../xa -DBAD4 test.s || exit 0 && exit 1
	../../xa -DBAD5 test.s || exit 0 && exit 1
	@echo no more failures!
	../../xa test.s
	../hextool -cmp=ok < a.o65

clean:
	rm -f a.o65

