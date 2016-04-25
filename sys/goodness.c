#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lab1.h>
int goodness(int pid){
	int tmp;
	struct pentry *proc = &proctab[pid];
	if(proc->quantum <= 0)
		return 0;
	tmp = proc->quantum + proc->pprio;
	return tmp;
}
