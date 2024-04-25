
all: 
	ipcrm -a
	gcc -o process process.c
	gcc -o master master.c
	gcc -o sched sched.c
	gcc -o mmu mmu.c
	./master

clean:
	rm process master sched mmu result.txt
