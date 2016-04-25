#include <conf.h>
#include <kernel.h>
#include <sched.h>
#include <stdio.h>
#include <proc.h>
#include <lab1.h>
#define LOOP 50

int prA, prB, prC;
int proc(char c);
int main() {
    int i;
    int count = 0;
    char buf[8];
	kprintf("\nHELLO WORLD\n");
    for(i = 0; i < 10; i++){
        srand(i);
        printf("%d\n", rand());
    }

    /* Linux like Scheduler */  
    setschedclass(LINUXSCHED);
    resume(prA = create(proc, 2000, 15, "proc A", 1, 'A'));
    resume(prB = create(proc, 2000, 25, "proc B", 1, 'B'));
//    resume(prC = create(proc, 2000, 90, "proc C", 1, 'C'));
    while (count++ < LOOP) {
        kprintf("M");
        for (i = 0; i < 10000000; i++)
            ;
    }
    return 0;
}

int proc(char c) {
    int i;
    int count = 0;
    while (count++ < LOOP) {
        kprintf("%c", c);
        for (i = 0; i < 10000000; i++)
            ;
    }
    return 0;
}
