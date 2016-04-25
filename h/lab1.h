#define LINUXSCHED 1
#define MULTIQSCHED 2
#define XINUSCHED 3
#define REALQUEUE 1
#define NORMALQUEUE 0

#ifndef RAND_MAX
#define RAND_MAX ((int) ((unsigned) ~0 >> 1))
#endif

extern int schedulerClass;
int getSchedClass();
void setSchedClass(int);
int goodness(int);
int highestGoodness();
int linuxSched(int);
int multiSched(int);
