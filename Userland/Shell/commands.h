#ifndef COMMANDS_H
#define COMMANDS_H

// External commands (not POSIX built-ins)

int clear(int argc, char *argv[]);
int help(int argc, char *argv[]);
int time(int argc, char *argv[]);
int font(int argc, char *argv[]);
int man(int argc, char *argv[]);
int regs(int argc, char *argv[]);

// ========== TEST COMMANDS ==========
// Thin wrappers around test functions from Userland/tests/

int testmm(int argc, char *argv[]);
int testprocesses(int argc, char *argv[]);
int testpriority(int argc, char *argv[]);
int testsemaphore(int argc, char *argv[]);

// a implementar
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

#endif // COMMANDS_H
