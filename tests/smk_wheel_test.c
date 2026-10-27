/* SPDX-License-Identifier: GPL-3.0-only
 * FMini-1 SMK-37 port: the wheels' readings (firmware/src/smk_wheel.h, main.c smk_wheels).
 * The hardware: the pitch wheel rests at its centre (~535 raw, travel ~445..640), the mod wheel rests at its
 * bottom (up reads lower, ~629 -> 474). The main loop reads them thousands of times a second, so this runs the
 * functions at that rate too: the bug it guards against was a centre that followed the wheel as fast as these
 * calls come -- a slow touch dragged it along, the spring's return landed outside the dead band, and a bend (a
 * transposition) stayed on the selected track. */
#include <stdio.h>
#include <string.h>
#include "../firmware/src/smk_wheel.h"

static uint32_t bad;
static void check(const char *what, int ok)
{
    printf("%s %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok)
        bad++;
}

/* the main loop's rate: smk_wheels runs in a busy wait, several times a millisecond */
#define BUSY 10u
static int32_t busy(smk_pw_t *w, int32_t p, uint32_t *now, uint32_t ms)
{
    int32_t b = 0;
    uint32_t i;
    for (i = 0; i < ms; i++) {
        uint32_t k;
        for (k = 0; k < BUSY; k++)
            b = smk_pw_bend(w, p, *now);
        (*now)++;
    }
    return b;
}

int main(void)
{
    smk_pw_t w;
    smk_mw_t m;
    uint32_t now = 1000, i;
    int32_t b;
    /* a slow touch: up 40 counts at 20 ms a count (2 s), then the spring brings it back at once */
    memset(&w, 0, sizeof w);
    check("pitch at rest: no bend", busy(&w, 535, &now, 100) == 0);
    b = 0;
    for (i = 1; i <= 40u; i++)
        b = busy(&w, 535 + (int32_t)i, &now, 20);
    check("a slow touch up does not move the rest (the drift is in time) and bends less than the far 40 counts",
          b > 0 && b < 8191);
    check("released after a slow touch: back to no bend (no transposition)", busy(&w, 535, &now, 20) == 0);
    check("and it stays there", busy(&w, 535, &now, 2000) == 0);
    /* the full travel: up to 640 and back, down to 445 and back */
    check("pushed fully up: the full bend 8191", busy(&w, 640, &now, 20) == 8191);
    check("released: no bend", busy(&w, 535, &now, 20) == 0);
    check("pushed fully down: the full bend -8192", busy(&w, 445, &now, 20) == -8192);
    check("released: no bend again", busy(&w, 535, &now, 20) == 0);
    check("the centre kept: 30 counts up is well under the full bend", (b = busy(&w, 565, &now, 20)) > 0 && b < 4000);
    check("released: no bend", busy(&w, 535, &now, 20) == 0);
    /* a wheel that rests off its captured centre (5 counts: inside the dead band) still reads no bend, and the
     * centre follows it in time */
    check("resting 5 counts off: no bend", busy(&w, 540, &now, 20) == 0);
    check("and after a second of resting: still no bend", busy(&w, 540, &now, 1000) == 0);
    /* the mod wheel: 629 at rest, 474 at the top */
    memset(&m, 0, sizeof m);
    check("mod wheel at rest: 0", smk_mw_val(&m, 629) == 0);
    check("mod wheel halfway (551): about the middle", (b = smk_mw_val(&m, 551)) > 50 && b < 80);
    check("mod wheel fully up (474): 127", smk_mw_val(&m, 474) == 127);
    check("mod wheel lower than the rest (a noise spike): still 0", smk_mw_val(&m, 640) == 0);
    check("back at rest: 0", smk_mw_val(&m, 629) == 0);
    printf("smk_wheel: %u failing check(s)\n", bad);
    return bad ? 1 : 0;
}
