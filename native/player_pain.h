#ifndef UC_PLAYER_PAIN_H
#define UC_PLAYER_PAIN_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCPlayerPain { float channel[2][3]; } UCPlayerPain;
/*69110/69150/69180 and complete691B0. Delta is actual1F9010, stable during
 * the call. Completing blur service is48E10(-150,600,amplitude), in channel
 * order, including a final zero after decay. No health/damage policy here. */
void uc_player_pain_init(UCPlayerPain *);
int uc_player_pain_trigger(UCPlayerPain *,unsigned channel);
int uc_player_pain_step(UCPlayerPain *,float delta,void *owner,
 int (*blur)(void *,float start,float end,float amplitude));
#ifdef __cplusplus
}
#endif
#endif
