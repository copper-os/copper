#ifndef CONSOLE_CONSOLE_H
#define CONSOLE_CONSOLE_H

typedef struct {
    int state;
    int (*test)(void);
} Console;

int console_init(Console*);

int console_direct(void);

#endif
