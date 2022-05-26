#include <console/console.h>

static int console_test(void) { return 1; }

int console_direct(void) { return 2; }

int console_init(Console* console)
{
    console->state = 10;
    console->test = &console_test;
    return 0;
}
