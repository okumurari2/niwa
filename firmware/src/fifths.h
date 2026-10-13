/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 okumurari2 */
/* The key, and OCT- / OCT+ as the circle of fifths. From maku (maku.c maku_fifth / maku_resig), cut to one scale: major.
 * Pure integer functions, no state: the app keeps `root` (0..11, C = 0) and calls these.
 *
 * A step round the circle is a change of key signature, not a transposition: a note already sounding or played keeps its
 * pitch, and only the one note the signature changes moves (C -> G: F becomes F#; C -> F: B becomes Bb). So nothing that
 * was in the old key is out of the new one, and nothing jumps. Twelve steps are the identity. There is no scale choice. */
#ifndef FIFTHS_H
#define FIFTHS_H
#include <stdint.h>

#define FIFTHS_MASK 0xAB5u                       /* the major scale from its root: pitch classes 0 2 4 5 7 9 11 */

/* OCT+ (dir > 0): a fifth up, a sharp more. OCT- (dir < 0): a fifth down, a flat more. root and result: 0..11 */
static inline uint32_t fifths_step(uint32_t root, int dir) { return (root % 12u + (dir > 0 ? 7u : 5u)) % 12u; }

/* is MIDI note n in the key of `root`? */
static inline int fifths_in_key(uint32_t n, uint32_t root) { return (FIFTHS_MASK >> ((n + 120u - root) % 12u)) & 1u; }

/* the note n (a MIDI note, 0..127) when the key goes from `from` to `to` = one step by fifths_step. In the old key and in the
 * new one: it stays. Only the note the signature changes moves, one semitone, the way the signature goes (a step up the circle
 * raises it, a step down lowers it). Not in the old key (a note played off the scale): it stays as played. */
static inline uint32_t fifths_resig(uint32_t n, uint32_t from, uint32_t to)
{
    if (!fifths_in_key(n, from) || fifths_in_key(n, to))
        return n;
    if (n < 127u && (to + 12u - from) % 12u == 7u)
        return n + 1u;
    return n > 0u ? n - 1u : n;
}

/* the key signature of the major key on `root`: +n sharps, -n flats (6 = F#, not Gb) */
static inline int fifths_signature(uint32_t root)
{
    int v = (int)((root % 12u) * 7u % 12u);
    return v > 6 ? v - 12 : v;
}

/* scale degree d (0 = root, 7 = the octave, 14 = two octaves; any d >= 0) to a MIDI note, with `base` the root's note in the
 * lowest octave in use: the way to ask for a note that cannot be out of key */
static inline uint32_t fifths_degree(uint32_t base, uint32_t d)
{
    static const uint8_t S[7] = {0, 2, 4, 5, 7, 9, 11};
    return base + (d / 7u) * 12u + S[d % 7u];
}

#endif
