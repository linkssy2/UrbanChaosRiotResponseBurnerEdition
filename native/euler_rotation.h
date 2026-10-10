#ifndef UC_EULER_ROTATION_H
#define UC_EULER_ROTATION_H
#include <math.h>
#include <string.h>
/* Original2D3DC0 with translation(0,0,0,1). Retain mixed single/double
 * stores and Euler convention; callers admit finite angles and RN/no FTZ. */
static inline void uc_euler_rotation(float x,float y,float z,float m[16]){
    float sx=(float)sin(x),sy=(float)sin(y),sz=(float)sin(z),cx=(float)cos(x),cy=(float)cos(y);
    double dz=cos(z);float cz=(float)dz;
    memset(m,0,64);m[0]=(float)(dz*cy);m[1]=(float)(-((double)cy*sz));m[2]=sy;
    m[4]=(cz*sy)*sx+cx*sz;m[5]=cz*cx-(sz*sy)*sx;m[6]=(float)(-((double)cy*sx));
    m[8]=sz*sx-(cz*cx)*sy;m[9]=(cx*sz)*sy+cz*sx;m[10]=cy*cx;m[15]=1;
}
#endif
