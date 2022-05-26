#include <console/console.h>

int _start(void)
{
    Console console;
    consle_init(&console);

    int val = console.test();
    if (val != 1) {
        return val;
    }

    return console_direct();
}
