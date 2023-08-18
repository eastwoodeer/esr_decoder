all: esr_decoder

esr_decoder: esr.c
	gcc -Werror esr.c -o $@

clean:
	rm -rf *.o esr_decoder
