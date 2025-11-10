#ifndef SHELL_INTERNAL_H
#define SHELL_INTERNAL_H

typedef struct {
    char *name;
    int (*func)(int argc, char *argv[]);
    char *description;
    int isBuiltIn;
} Command;

extern Command commands[];

#endif
