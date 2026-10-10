#include "character_retarget.h"
#include <math.h>
int uc_character_route_splice(const int32_t*nodes,uint32_t count,uint32_t cursor,
 int32_t endpoint,UCCharacterSpliceClear clear,void*owner,UCCharacterRouteSplice*out){
 if(!nodes||!count||count>1000000||cursor>=count||!clear||!out||endpoint< -1)return -1;
 if(endpoint==-1)return 0;
 for(uint32_t j=count-1;j>cursor;--j)if(nodes[j]==endpoint){
  UCCharacterRouteSplice r={cursor,j,0};*out=r;return 1;
 }
 for(uint32_t j=cursor+1;j<count;++j)if(nodes[j]!=-1){
  int result=clear(owner,j);if(result<0)return -1;if(result){
   UCCharacterRouteSplice r={cursor,j+1,1};*out=r;return 1;
  }
 }
 return 0;
}
int uc_character_route_near(int status,float radius,const float goal[3],const float waypoint[3]){
 if(status==0||status==1||status==4)return 1;
 if((status!=2&&status!=3)||!goal||!waypoint||!isfinite(radius)||radius<0||
    !isfinite(goal[0])||!isfinite(goal[2])||!isfinite(waypoint[0])||!isfinite(waypoint[2]))return -1;
 double x=(double)goal[0]-waypoint[0],z=(double)goal[2]-waypoint[2];
 return (double)radius>sqrt(z*z+x*x);
}
