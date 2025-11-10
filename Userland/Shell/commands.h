#ifndef COMMANDS_H
#define COMMANDS_H

int clear(int argc, char *argv[]);
int help(int argc, char *argv[]);
int time(int argc, char *argv[]);
int font(int argc, char *argv[]);
int man(int argc, char *argv[]);
int regs(int argc, char *argv[]);

int divzero(int argc, char *argv[]);
int invop(int argc, char *argv[]);

int testmm(int argc, char *argv[]);
int testprocesses(int argc, char *argv[]);
int testpriority(int argc, char *argv[]);
int testsemaphore(int argc, char *argv[]);
int testsync(int argc, char *argv[]);
int tnosync(int argc, char *argv[]);

#define MVAR_MAX_READERS 10
#define MVAR_MAX_WRITERS 10

int mem(int argc, char *argv[]);
int ps(int argc, char *argv[]);
int loop(int argc, char *argv[]);
int kill(int argc, char *argv[]);
int nice(int argc, char *argv[]);
int block(int argc, char *argv[]);
int cat(int argc, char *argv[]);
int wc(int argc, char *argv[]);
int filter(int argc, char *argv[]);
int mvar(int argc, char *argv[]);

#endif
