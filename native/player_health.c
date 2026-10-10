#include "player_health.h"
#include <math.h>
int uc_player_health_set(float *current,float *maximum,float value){
    if(!current||!maximum||current==maximum||!isfinite(*maximum)||*maximum<0||
       *maximum>65536||!isfinite(value)||value<0||value>65536)return 0;
    *current=value;if(value>*maximum)*maximum=value;return 1;
}
