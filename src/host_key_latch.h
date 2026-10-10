#ifndef UCRR_HOST_KEY_LATCH_H
#define UCRR_HOST_KEY_LATCH_H
#include <stdint.h>
#include <string.h>

/* Same-thread host message pump and guest action query. One short press is
 * shared by matching queries in one outer frontend update, including nested
 * updates; outside an update, a query consumes it. Repeats add no press edges.
 * Expiration prevents startup/loading input leaking into a later screen. */
typedef struct UCRR_KEY_LATCH {
    uint64_t at[256];
    unsigned char pending[256], active[256];
    unsigned depth;
} UCRR_KEY_LATCH;
/* Windows virtual-key codes from the existing keyboard/gamepad mapping.
 * Only discrete frontend actions; analog gameplay remains level-based. */
static unsigned ucrr_menu_action_binding(uint32_t action)
{
    /* Guest physical 1A/1B are White/Black; host rows 5/4 are White/Black. */
    static const unsigned bindings[] = {0,1,2,3,5,4,9,8,12,13,14,15,6,7,10,11};
    return action >= 0x16 && action <= 0x25 ? bindings[action-0x16] : 25;
}
static void ucrr_key_clear(UCRR_KEY_LATCH *s) { memset(s, 0, sizeof(*s)); }
static void ucrr_key_record(UCRR_KEY_LATCH *s, unsigned key, int repeat, uint64_t now)
{
    if (key < 256 && !repeat) { s->at[key] = now; s->pending[key] = 1; }
}
static void ucrr_key_begin(UCRR_KEY_LATCH *s, uint64_t now, int focused)
{
    if (!focused) ucrr_key_clear(s);
    if (s->depth++ != 0) return;
    for (unsigned key = 0; key < 256; ++key) {
        s->active[key] = s->pending[key] && now - s->at[key] <= 250u;
        s->pending[key] = 0;
    }
}
static void ucrr_key_end(UCRR_KEY_LATCH *s)
{
    if (s->depth && --s->depth == 0) memset(s->active, 0, sizeof(s->active));
}
static int ucrr_key_take(UCRR_KEY_LATCH *s, unsigned key, uint64_t now, int focused)
{
    int pending;
    if (!focused) { ucrr_key_clear(s); return 0; }
    if (key >= 256) return 0;
    if (s->depth) return s->active[key];
    pending = s->pending[key]; s->pending[key] = 0;
    return pending && now - s->at[key] <= 250u;
}
#endif
