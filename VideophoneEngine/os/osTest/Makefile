all: ostest
.PHONY: all

.PHONY: clean
clean:
	rm ostest

ostest: ostest.c
	gcc -g -I../.. -DstiLINUX ostest.c -lpthread ../x86/release/libos.a -o ostest
#	gcc -g -I../.. -DstiLINUX ostest.c -lpthread ../../build/arm925t/debug/os/os.a -o ostest
# -C -E
