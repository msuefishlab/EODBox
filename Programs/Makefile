Fishy: Fishy.p
	pasm -b Fishy.p

PRUClock: PRUClock.p
	pasm -b PRUClock.p

Fishy_v1:Fishy PRUClock Fishy_v1.c
	gcc Fishy_v1.c -o Fishy_v1 -lpthread -lprussdrv -lm

run: Fishy_v1
	./Fishy_v1