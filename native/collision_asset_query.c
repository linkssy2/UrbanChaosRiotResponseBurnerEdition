#include "collision_asset_query.h"
#include "collision_box_axes.h"
#include <math.h>
#include <string.h>

typedef struct V { double x,y,z; } V;
static V add(V a,V b){return (V){a.x+b.x,a.y+b.y,a.z+b.z};}
static V sub(V a,V b){return (V){a.x-b.x,a.y-b.y,a.z-b.z};}
static V mul(V a,double s){return (V){a.x*s,a.y*s,a.z*s};}
static double dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static V cross(V a,V b){return (V){a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
static V vector(const float *p){return (V){p[0],p[1],p[2]};}
static void store(float *p,V v){p[0]=(float)v.x;p[1]=(float)v.y;p[2]=(float)v.z;}
static int finite3(const float *p){return p&&isfinite(p[0])&&isfinite(p[1])&&isfinite(p[2]);}
static double clamp(double a){return a<0?0:a>1?1:a;}
void uc_collision_to_physics(const float r[3],float p[3]) {
    float x=r[0]*0.01f,y=r[2]*0.01f,z=r[1]*0.01f;p[0]=x;p[1]=y;p[2]=z;
}
void uc_collision_to_render(const float p[3],float r[3]) {
    float x=p[0]*100.0f,y=p[2]*100.0f,z=p[1]*100.0f;r[0]=x;r[1]=y;r[2]=z;
}
static int valid(const UCCollisionAsset *a) {
    return a&&finite3(a->scaling)&&isfinite(a->radius)&&a->radius>=0&&
        (!a->triangle_count||(a->triangles&&a->vertices));
}
static void triangle(const UCCollisionAsset *a,uint32_t i,V p[3]) {
    for(unsigned j=0;j<3;j++) {
        const float *v=a->vertices[a->triangles[i].vertex[j]];
        p[j]=(V){(double)v[0]*a->scaling[0],(double)v[1]*a->scaling[1],(double)v[2]*a->scaling[2]};
    }
}
static V minimum(V a,V b){return (V){a.x<b.x?a.x:b.x,a.y<b.y?a.y:b.y,a.z<b.z?a.z:b.z};}
static V maximum(V a,V b){return (V){a.x>b.x?a.x:b.x,a.y>b.y?a.y:b.y,a.z>b.z?a.z:b.z};}
static int overlaps(const V p[3],V lo,V hi) {
    V low=minimum(p[0],minimum(p[1],p[2])),high=maximum(p[0],maximum(p[1],p[2]));
    return low.x<=hi.x&&low.y<=hi.y&&low.z<=hi.z&&high.x>=lo.x&&high.y>=lo.y&&high.z>=lo.z;
}
static int skip_block(const UCCollisionAsset*a,uint32_t i,V lo,V hi){
    if(i%32||!a->query_bounds||memcmp(a->scaling,a->query_scaling,sizeof a->scaling))return 0;
    const double*b=a->query_bounds[i/32];
    return b[0]>hi.x||b[1]>hi.y||b[2]>hi.z||b[3]<lo.x||b[4]<lo.y||b[5]<lo.z;
}
static int ray(V from,V delta,const V p[3],double *fraction,V *normal) {
    V e1=sub(p[1],p[0]),e2=sub(p[2],p[0]),h=cross(delta,e2);
    double det=dot(e1,h);if(det==0)return 0;
    V s=sub(from,p[0]),q=cross(s,e1);
    double u=dot(s,h)/det,v=dot(delta,q)/det,t=dot(e2,q)/det;
    if(u<0||v<0||u+v>1||t<0||t>1)return 0;
    V n=cross(e1,e2);double d=dot(n,n);if(d==0)return 0;
    *normal=mul(n,(dot(n,delta)>0?-1:1)/sqrt(d));*fraction=t;return 1;
}
int uc_collision_raycast(const UCCollisionAsset *a,const float from[3],const float to[3],UCCollisionRayHit *hit) {
    if(!valid(a)||!finite3(from)||!finite3(to)||!hit)return -1;
    V origin=vector(from),end=vector(to),delta=sub(end,origin),lo=minimum(origin,end),hi=maximum(origin,end);double closest=1;int found=0;
    UCCollisionRayHit result={0};
    for(uint32_t i=0;i<a->triangle_count;i++) {
        if(skip_block(a,i,lo,hi)){i+=31;continue;}
        V p[3],n;double t;triangle(a,i,p);if(!overlaps(p,lo,hi))continue;
        if(ray(origin,delta,p,&t,&n)&&(!found||t<closest)) {
            found=1;closest=t;result.fraction=(float)t;result.triangle=i;
            store(result.position,add(origin,mul(delta,t)));store(result.normal,n);
        }
    }
    if(found)*hit=result;return found;
}
static V closest_triangle(V x,const V p[3]) {
    V ab=sub(p[1],p[0]),ac=sub(p[2],p[0]),ap=sub(x,p[0]);
    double d1=dot(ab,ap),d2=dot(ac,ap);if(d1<=0&&d2<=0)return p[0];
    V bp=sub(x,p[1]);double d3=dot(ab,bp),d4=dot(ac,bp);if(d3>=0&&d4<=d3)return p[1];
    double vc=d1*d4-d3*d2;if(vc<=0&&d1>=0&&d3<=0)return add(p[0],mul(ab,d1/(d1-d3)));
    V cp=sub(x,p[2]);double d5=dot(ab,cp),d6=dot(ac,cp);if(d6>=0&&d5<=d6)return p[2];
    double vb=d5*d2-d1*d6;if(vb<=0&&d2>=0&&d6<=0)return add(p[0],mul(ac,d2/(d2-d6)));
    double va=d3*d6-d5*d4;
    if(va<=0&&d4>=d3&&d5>=d6)return add(p[1],mul(sub(p[2],p[1]),(d4-d3)/((d4-d3)+(d5-d6))));
    double inverse=1/(va+vb+vc);return add(p[0],add(mul(ab,vb*inverse),mul(ac,vc*inverse)));
}
static void closest_segments(V a,V b,V c,V d,V *on_ab,V *on_cd) {
    V u=sub(b,a),v=sub(d,c),r=sub(a,c);double aa=dot(u,u),ee=dot(v,v),f=dot(v,r),s=0,t=0;
    if(aa==0&&ee==0){*on_ab=a;*on_cd=c;return;}
    if(aa==0)t=clamp(f/ee);
    else {
        double cc=dot(u,r);
        if(ee==0)s=clamp(-cc/aa);
        else {
            double bb=dot(u,v),denom=aa*ee-bb*bb;
            if(denom>0)s=clamp((bb*f-cc*ee)/denom);
            t=(bb*s+f)/ee;
            if(t<0){t=0;s=clamp(-cc/aa);}else if(t>1){t=1;s=clamp((bb-cc)/aa);}
        }
    }
    *on_ab=add(a,mul(u,s));*on_cd=add(c,mul(v,t));
}
static double segment_triangle(V a,V b,const V p[3],V *axis,V *surface) {
    double t;V n;
    if(ray(a,sub(b,a),p,&t,&n)){*axis=*surface=add(a,mul(sub(b,a),t));return 0;}
    *axis=a;*surface=closest_triangle(a,p);V q=sub(*axis,*surface);double best=dot(q,q);
    V y=closest_triangle(b,p);q=sub(b,y);double d=dot(q,q);
    if(d<best){best=d;*axis=b;*surface=y;}
    for(unsigned i=0;i<3;i++) {
        V x;closest_segments(a,b,p[i],p[(i+1)%3],&x,&y);q=sub(x,y);d=dot(q,q);
        if(d<best){best=d;*axis=x;*surface=y;}
    }
    return best;
}
size_t uc_collision_capsule_contacts(const UCCollisionAsset *a,const float end_a[3],const float end_b[3],float radius,UCCollisionContact *contacts,size_t capacity) {
    if(!valid(a)||!finite3(end_a)||!finite3(end_b)||!isfinite(radius)||radius<0||(capacity&&!contacts))return SIZE_MAX;
    V start=vector(end_a),end=vector(end_b);double reach=(double)radius+a->radius;size_t count=0;
    V padding={reach,reach,reach},lo=sub(minimum(start,end),padding),hi=add(maximum(start,end),padding);
    for(uint32_t i=0;i<a->triangle_count;i++) {
        if(skip_block(a,i,lo,hi)){i+=31;continue;}
        V p[3],axis,surface;triangle(a,i,p);if(!overlaps(p,lo,hi))continue;
        V n=cross(sub(p[1],p[0]),sub(p[2],p[0]));double nn=dot(n,n);
        if(nn==0)continue;
        double square=segment_triangle(start,end,p,&axis,&surface);if(square>reach*reach)continue;
        double distance=sqrt(square);
        if(distance>0)n=mul(sub(axis,surface),1/distance);
        else n=mul(n,(dot(n,sub(mul(add(start,end),0.5),p[0]))<0?-1:1)/sqrt(nn));
        if(count<capacity) {
            UCCollisionContact *c=&contacts[count];store(c->axis_point,axis);store(c->surface_point,surface);store(c->normal,n);
            c->penetration=(float)(reach-distance);c->triangle=i;
        }
        count++;
    }
    return count;
}

int uc_collision_block_contact(const UCSolidBlockGeometry *b,const float start[3],const float end[3],float radius,UCCollisionContact *out) {
    if(!b||!out||!finite3(start)||!finite3(end)||!finite3(b->center)||!finite3(b->half)||
       !isfinite(radius)||radius<0||!isfinite(b->radius)||b->radius<0)return -1;
    double axes[3][3];if(uc_collision_box_axes(b->rotation,axes))return -1;
    double a[3]={0},d[3]={0},h[3],cuts[8]={0,1};unsigned count=2;
    for(unsigned i=0;i<3;i++){
        h[i]=b->half[i];if(h[i]<0)return -1;
        for(unsigned k=0;k<3;k++){a[i]+=((double)start[k]-b->center[k])*axes[i][k];d[i]+=((double)end[k]-start[k])*axes[i][k];}
        if(d[i]!=0)for(int side=-1;side<=1;side+=2){double t=(side*h[i]-a[i])/d[i];if(t>0&&t<1)cuts[count++]=t;}
    }
    for(unsigned i=1;i<count;i++){double t=cuts[i];unsigned j=i;while(j&&cuts[j-1]>t){cuts[j]=cuts[j-1];--j;}cuts[j]=t;}
    double best=HUGE_VAL,best_t=0;
    // The segment-to-box distance is quadratic between its face crossings.
    for(unsigned interval=1;interval<count;interval++){
        double lo=cuts[interval-1],hi=cuts[interval],mid=(lo+hi)*.5,dd=0,ad=0;
        for(unsigned i=0;i<3;i++){double p=a[i]+mid*d[i];if(p < -h[i]||p>h[i]){double face=p<0?-h[i]:h[i];dd+=d[i]*d[i];ad+=(a[i]-face)*d[i];}}
        double t=dd?-ad/dd:lo;if(t<lo)t=lo;if(t>hi)t=hi;
        double square=0;for(unsigned i=0;i<3;i++){double p=a[i]+t*d[i],gap=fmax(fabs(p)-h[i],0);square+=gap*gap;}
        if(square<best){best=square;best_t=t;}
    }
    double reach=(double)radius+b->radius;if(best>reach*reach)return 0;
    double axis[3],surface[3],normal[3],depth;
    for(unsigned i=0;i<3;i++){axis[i]=a[i]+best_t*d[i];surface[i]=fmax(-h[i],fmin(h[i],axis[i]));normal[i]=axis[i]-surface[i];}
    if(best>1e-24){double distance=sqrt(best);depth=reach-distance;for(unsigned i=0;i<3;i++)normal[i]/=distance;}
    else {
        unsigned selected=0;double side=1;depth=HUGE_VAL;
        for(unsigned i=0;i<3;i++)for(int s=-1;s<=1;s+=2){
            double near=s>0?fmin(a[i],a[i]+d[i]):fmax(a[i],a[i]+d[i]);
            double amount=h[i]-s*near+reach;if(amount<depth){depth=amount;selected=i;side=s;best_t=(s>0?d[i]>=0:d[i]<0)?0:1;}
        }
        for(unsigned i=0;i<3;i++){axis[i]=a[i]+best_t*d[i];surface[i]=fmax(-h[i],fmin(h[i],axis[i]));normal[i]=0;}
        surface[selected]=side*h[selected];normal[selected]=side;
    }
    UCCollisionContact result={0};result.triangle=UINT32_MAX;result.penetration=(float)depth;
    for(unsigned k=0;k<3;k++){
        double pa=b->center[k],ps=pa,n=0;
        for(unsigned i=0;i<3;i++){pa+=axis[i]*axes[i][k];ps+=surface[i]*axes[i][k];n+=normal[i]*axes[i][k];}
        result.axis_point[k]=(float)pa;result.surface_point[k]=(float)ps;result.normal[k]=(float)n;
    }
    if(!isfinite(result.penetration)||!finite3(result.axis_point)||!finite3(result.surface_point)||!finite3(result.normal))return -1;
    *out=result;return 1;
}
