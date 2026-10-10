#ifndef UC_CHECKPOINT_MESSAGE_H
#define UC_CHECKPOINT_MESSAGE_H
#include "font.h"
/*136C50 ->69280 ->4AB90 ->4A290: type15 notification text domain,
 * also used by71200's resolved team-join notification.
 * Literal resolved English text, no prefix/suffix, dedup ID or delayed sound.
 * Other provider message types deliberately are not admitted here. */
typedef struct UCCheckpointMessageRow {float age;char text[128];} UCCheckpointMessageRow;
typedef struct UCCheckpointMessages {UCCheckpointMessageRow rows[5];uint32_t count;uint8_t changed;} UCCheckpointMessages;
typedef struct UCCheckpointMessagePlan {float x,y,scale,blur;} UCCheckpointMessagePlan;
int uc_checkpoint_message_add(UCCheckpointMessages *,const char *,uint8_t *accepted);
int uc_checkpoint_message_step(UCCheckpointMessages *,float delta);
/*4BA80/4A460/17E40/171A0: original640-wide provider coordinates. The original
 * empty prefix/suffix still contribute one space each to the measured width. */
int uc_checkpoint_message_plan(const UCCheckpointMessages *,const UCFont *,float width,UCCheckpointMessagePlan *);
#endif
