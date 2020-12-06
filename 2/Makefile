all : ssu_shell pps ttop

ssu_shell : ssu_shell.c
	gcc -o ssu_shell ssu_shell.c

pps : pps.c
	gcc -o pps pps.c

ttop : ttop.c
	gcc -o ttop ttop.c -lncurses

clean :
	rm ssu_shell
	rm pps
	rm ttop
