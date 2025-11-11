// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <cursor.h>
#include <time.h>
#include <fonts.h>

#define TOGGLE_TICKS 9

static uint8_t IS_SHOWING = 0;

void toggleCursor(void);

int toggleSpeed(void) {
    return ticks_elapsed() / TOGGLE_TICKS;
}

void toggleCursor(void) {
    int toggle = toggleSpeed() % 2;
    if ((toggle == 1) && !IS_SHOWING) {
        showCursor();
        IS_SHOWING = 1;
    } else if (toggle == 0 && IS_SHOWING) {
        hideCursor();
        IS_SHOWING = 0;
    }
}
