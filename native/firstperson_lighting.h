#ifndef UC_FIRSTPERSON_LIGHTING_H
#define UC_FIRSTPERSON_LIGHTING_H
#include <string.h>
/* Default gameplay owner26D230 ->9EBE3/87270. This is the private camera
 * manager, not the world light list. Menu fade26C600 is a separate owner. */
typedef struct UCFirstpersonLighting {float direction[3][4],color[3][4],ambient[4];} UCFirstpersonLighting;
static void uc_firstperson_lighting_defaults(UCFirstpersonLighting*out){
    static const float directions[3][4]={
        {.2357214242219925f,-.22495105862617493f,.9454271197319031f,-0.f},
        {.8191520571708679f,0.f,-.5735764503479004f,0.f},
        {-.1835751086473465f,.9781476259231567f,.09760861843824387f,0.f}};
    static const unsigned char colors[3][3]={{245,249,250},{239,242,41},{0,150,255}};
    memset(out,0,sizeof(*out));memcpy(out->direction,directions,sizeof(directions));
    for(unsigned i=0;i<3;i++){for(unsigned k=0;k<3;k++)out->color[i][k]=(float)colors[i][k]*(1.f/255.f);out->color[i][3]=1;}
}
/* PC presentation correction: retain the three-light shape and luminance.
 * ponytail: neutral private lights avoid the exaggerated native weapon cast;
 * replace this calibration only after matched CONTROL material-composition proof.
 * Source defaults above remain unchanged for original-behavior verification. */
static void uc_firstperson_lighting_neutral(UCFirstpersonLighting*out){
    uc_firstperson_lighting_defaults(out);
    for(unsigned i=0;i<3;i++){
        float y=out->color[i][0]*.2126f+out->color[i][1]*.7152f+out->color[i][2]*.0722f;
        for(unsigned k=0;k<3;k++)out->color[i][k]=y;
    }
}
#endif
