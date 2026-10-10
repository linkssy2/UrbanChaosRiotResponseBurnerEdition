#ifndef UC_HYBRID_FRONTEND_GUEST_H
#define UC_HYBRID_FRONTEND_GUEST_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { UC_HF_NONE=0,UC_HF_LIST=1,UC_HF_LEVELS=2,UC_HF_DIALOG=3,UC_HF_PROFILES=4,UC_HF_REWARD_BROWSER=5 };
enum { UC_HF_SELECT=1,UC_HF_ACTIVATE=2,UC_HF_BACK=3,UC_HF_LEFT=4,UC_HF_RIGHT=5,
       UC_HF_DIFFICULTY=6,UC_HF_REWARDS=7 };
enum { UC_HF_ROW_ACTIVATE=1,UC_HF_ROW_LEFT=2,UC_HF_ROW_RIGHT=4 };
/* Presentation only: input remains revoked while the original manager fades. */
#define UC_HF_TRANSITION 0x40000000u
typedef struct UcHybridFrontendRow {
    uint32_t row,index,table,enabled,actions,callback;
    char label[128],value[128];
    uint32_t artwork_id,completed; /* completed0=none,1..4=source difficulty+1 */
} UcHybridFrontendRow;
typedef struct UcHybridFrontendDetailText {
    uint32_t flags,color,font;
    float x,y;
    char prefix[32],suffix[64];
} UcHybridFrontendDetailText;
typedef struct UcHybridFrontendDetailIcon {
    uint32_t kind,color;
    float x,y;
} UcHybridFrontendDetailIcon;
typedef struct UcHybridFrontendSnapshot {
    uint64_t epoch;
    uint32_t screen,screen_vtable,owner,kind,selected,count,back_action,source_kind;
    char title[128];
    UcHybridFrontendRow rows[55];
    uint32_t text_count,icon_count;
    UcHybridFrontendDetailText texts[32];
    UcHybridFrontendDetailIcon icons[32];
    uint32_t level_actions,level_difficulty,level_medals;
} UcHybridFrontendSnapshot;
typedef struct UcHybridFrontendAction {
    uint64_t epoch;
    uint32_t screen,owner,row,index,kind;
} UcHybridFrontendAction;
/* Host copies snapshots; never retain the pointer or touch guest CPU/RAM.
 * kind NONE revokes presentation and queued actions. Labels are original
 * localization keys or literal dynamic text. Empty values/titles are unknown.
 * take_action atomically removes at most one queued, user-authored action. */
int ucrr_native_frontend_enabled(void);
void ucrr_native_frontend_publish(const UcHybridFrontendSnapshot *);
int ucrr_native_frontend_take_action(UcHybridFrontendAction *);
/* Movie callbacks own one native decoder. open declines unsupported/missing
 * assets without starting playback; return1 only after successful preparation.
 * tick returns1 while playing. draw runs at original87810, before later UI.
 * GPU draw must drain previous guest work and preserve borrowed target/state. */
int ucrr_native_frontend_movie_open(const char *path,int loop,float volume);
int ucrr_native_frontend_movie_tick(void);
void ucrr_native_frontend_movie_draw(void);
void ucrr_native_frontend_movie_close(void);
void ucrr_native_frontend_movie_volume(float volume);
/* One actual host click/confirm press, consumed only at the original qualified
 * movie screen query after its minimum skip delay and fade gates. */
int ucrr_native_frontend_movie_take_skip(void);
/* Generated entry seams:0=open875A0,1=tick87710,2=draw87810,
 * 3=close87420,4=volume874C0,5=screen query274740.
 * Return1 means guest return was completed. */
int uc_hybrid_frontend_movie(unsigned operation);
#ifdef __cplusplus
}
#endif
#endif
