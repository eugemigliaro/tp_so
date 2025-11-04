#ifndef SHELL_INTERNAL_H
#define SHELL_INTERNAL_H

typedef struct {
    char *name;
    int (*func)(int argc, char *argv[]);
    char *description;
    int isBuiltIn;  // 1 for built-in commands, 0 for external
} Command;

// Declare commands array as extern so commands.c can access it
extern Command commands[];

#endif // SHELL_INTERNAL_H