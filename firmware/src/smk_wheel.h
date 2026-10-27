/* SPDX-License-Identifier: GPL-3.0-only
 * FMini-1 SMK-37 port: Copyright (C) 2026 M. Geisler (genkigenki/makkuro) */
/* The SMK-37's wheels: a raw ADC reading -> the pitch bend (-8192..8191) and the mod wheel (0..127), as main.c
 * smk_wheels plays them on the selected track. No hardware here: the host test (tests/smk_wheel_test.c) runs it.
 * Both calibrate themselves: the pitch wheel's rest is its centre (the first reading taken), its range widens to the
 * extremes the wheel has reached (the travel is short, ~445..640 raw), the mod wheel's bottom is its rest (up reads
 * lower). The pitch centre follows a resting wheel slowly, at most one count per SMK_PW_DRIFT_MS and in time, not in
 * calls: the main loop calls this thousands of times a second, so a counter of calls is a centre that follows as fast
 * as the wheel moves -- a slow touch drags it along (no bend) and the spring's return then lands outside the dead
 * band, where the centre never follows again: a bend stays on the selected track and the keys sound transposed. */
#pragma once
#include <stdint.h>

#define SMK_PW_DEAD 12            /* raw counts around the centre: no bend */
#define SMK_PW_SPAN0 80           /* the pitch range assumed until the wheel goes further */
#define SMK_PW_DRIFT_MS 250u      /* the centre follows a resting wheel by one count per this */
#define SMK_MW_LO 485             /* the mod wheel's range assumed until it goes further */
#define SMK_MW_HI 615
#define SMK_MW_DEAD 12

typedef struct {
    int32_t pc, plo, phi;         /* pitch: the centre, full down, full up (raw) */
    uint32_t drift;               /* when the centre last followed a resting wheel */
    uint8_t start;
} smk_pw_t;

typedef struct {
    int32_t lo, hi;               /* mod: full up, the bottom (its rest) */
    uint8_t start;
} smk_mw_t;

/* the pitch bend for reading p (raw) at now (ms) */
static int32_t smk_pw_bend(smk_pw_t *w, int32_t p, uint32_t now)
{
    int32_t d, b, up, dn;
    if (!w->start) {
        w->start = 1;
        w->pc = p;
        w->plo = p - SMK_PW_SPAN0;
        w->phi = p + SMK_PW_SPAN0;
        w->drift = now;
    }
    if (p < w->plo)
        w->plo = p;
    if (p > w->phi)
        w->phi = p;
    d = p - w->pc;
    if (d > -SMK_PW_DEAD && d < SMK_PW_DEAD) {
        if (now - w->drift >= SMK_PW_DRIFT_MS) {           /* resting: the centre follows a slow drift */
            w->pc += d > 0 ? 1 : d < 0 ? -1 : 0;
            w->drift = now;
        }
        return 0;
    }
    w->drift = now;                                        /* (bent: the drift waits until it rests again) */
    up = w->phi - w->pc - SMK_PW_DEAD;
    dn = w->pc - w->plo - SMK_PW_DEAD;
    b = d > 0 ? (d - SMK_PW_DEAD) * 8191 / (up > 1 ? up : 1) : (d + SMK_PW_DEAD) * 8192 / (dn > 1 ? dn : 1);
    return b < -8192 ? -8192 : b > 8191 ? 8191 : b;
}

/* the mod wheel for reading m (raw): 0..127, up (a lower reading) = more */
static int32_t smk_mw_val(smk_mw_t *w, int32_t m)
{
    int32_t v, span;
    if (!w->start) {
        w->start = 1;
        w->lo = m < SMK_MW_LO ? m : SMK_MW_LO;
        w->hi = m > SMK_MW_HI ? m : SMK_MW_HI;
    }
    if (m < w->lo)
        w->lo = m;
    if (m > w->hi)
        w->hi = m;
    span = w->hi - w->lo - SMK_MW_DEAD;
    v = w->hi - m <= SMK_MW_DEAD ? 0 : (w->hi - m - SMK_MW_DEAD) * 127 / (span > 1 ? span : 1);
    return v < 0 ? 0 : v > 127 ? 127 : v;
}
