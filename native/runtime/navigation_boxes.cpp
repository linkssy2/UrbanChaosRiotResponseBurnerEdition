#include "navigation_boxes.h"
#include <algorithm>
#include <cmath>
#include <cstring>
using Polygon=NativeNavigationPolygon;
using Point=Polygon::Point;
static float(*rows(std::vector<Point>&p))[3]{return reinterpret_cast<float(*)[3]>(p.data());}
static bool valid(const Polygon&p){
 if(p.points.size()<3||p.points.size()>4096||!std::isfinite(p.height)||p.height<0)return false;
 for(const auto&q:p.points)for(float v:q)if(!std::isfinite(v)||fabsf(v)>1e6f)return false;return true;
}
static void bounds(Polygon&p){
 p.center={};for(const auto&q:p.points)for(unsigned j=0;j<3;j++)p.center[j]+=q[j];
 float inverse=(float)(1./(double)p.points.size());for(float&v:p.center)v*=inverse;
 float x=p.points[0][0]-p.center[0],z=p.points[0][2]-p.center[2];p.radius=(float)sqrt((double)z*z+(double)x*x);
}
int native_navigation_insert(Polygon&p,const Point&q){
 if(!valid(p)||p.points.size()==4096)return 1;for(float v:q)if(!std::isfinite(v)||fabsf(v)>1e6f)return 1;
 int first=-1,last=(int)p.points.size();bool previous=false;
 for(int i=0;i<(int)p.points.size();i++){
  const auto&a=p.points[i];const auto&b=p.points[(i+1)%p.points.size()];
  float nx=0.f-(b[2]-a[2]),nz=b[0]-a[0];float dot=(q[2]-a[2])*nz+(q[0]-a[0])*nx;dot+=0.f;
  if(dot<0){if(previous)last=i;previous=false;}else{if(!previous)first=i;previous=true;}
 }
 if(first<0)return 0;
 if(last-first>1){
  //23615..2369C. Preserve the original signed comparison and splice order.
  if(first>last){p.points.erase(p.points.begin(),p.points.begin()+last);first+=1-last;last=(int)p.points.size();}
  p.points.erase(p.points.begin()+first+1,p.points.begin()+last);
 }
 p.points.insert(p.points.begin()+first+1,q);return 0;
}
int native_navigation_merge(const Polygon&a,const Polygon&b,Polygon&out){
 if(!valid(a)||!valid(b)||a.allow_inside!=b.allow_inside)return 1;
 const Polygon&base=a.points.size()>b.points.size()?a:b;const Polygon&add=&base==&a?b:a;
 Polygon p=base;p.members=base.members+add.members;p.height=add.height>base.height?add.height:base.height;
 for(const auto&q:add.points)if(native_navigation_insert(p,q))return 1;
 bounds(p);out=std::move(p);return 0;
}
int native_navigation_overlap(const Polygon&a,const Polygon&b){
 if(!valid(a)||!valid(b))return -1;if(a.allow_inside!=b.allow_inside)return 0;
 float x=a.center[0]-b.center[0],z=a.center[2]-b.center[2];
 if(sqrt((double)z*z+(double)x*x)>(double)a.radius+b.radius+150.f)return 0;
 Point lo[2],hi[2];const Polygon*p[2]={&a,&b};
 for(unsigned k=0;k<2;k++){lo[k]=hi[k]=p[k]->points[0];for(size_t i=1;i<p[k]->points.size();i++)for(unsigned j=0;j<3;j++){float v=p[k]->points[i][j];if(lo[k][j]>v)lo[k][j]=v;else if(v>hi[k][j])hi[k][j]=v;}}
 for(unsigned j=0;j<3;j++)if(lo[0][j]>hi[1][j]||lo[1][j]>hi[0][j])return 0;return 1;
}
struct NativeNavigationBoxes::Group {
 Polygon polygon;std::vector<Point>collision,detour;NativeCharacterDetour::Obstacle published{};
};
NativeNavigationBoxes::NativeNavigationBoxes(NativeCharacterNavigationManager&m):manager_(m){}
NativeNavigationBoxes::~NativeNavigationBoxes(){while(!boxes_.empty())remove(boxes_.back()->box.id);}
int NativeNavigationBoxes::add(int32_t id,const float center[3],const float size[3],uint8_t flag){
 if(failed_||boxes_.size()>=4096)return 1;for(const auto&p:boxes_)if(p->box.id==id)return 1;
 auto p=std::make_unique<Entry>();if(uc_navigation_box(id,center,size,flag,&p->box))return 1;boxes_.push_back(std::move(p));return 0;
}
int NativeNavigationBoxes::publish(std::unique_ptr<Group>p){
 auto&v=p->polygon;uint32_t n=(uint32_t)v.points.size();std::vector<Point>directions(n*2);p->collision.resize(n*2);p->detour.resize(n*2);
 if(uc_character_obstacle_directions(rows(v.points),n,rows(directions),n*2)||
    uc_character_obstacle_expand(rows(v.points),n,rows(directions),40,rows(p->collision),n*2)||
    uc_character_obstacle_expand(rows(v.points),n,rows(directions),141,rows(p->detour),n*2))return 2;
 auto&o=p->published;o.identity=p.get();o.collision.expanded=rows(p->collision);o.collision.count=n*2;
 memcpy(o.collision.center,v.center.data(),12);o.collision.half_height=v.height;
 o.collision.bounding_radius=v.radius;o.collision.allow_inside=v.allow_inside;
 o.detour=rows(p->detour);o.detour_count=n*2;manager_.add_obstacle(o);groups_.push_back(std::move(p));return 0;
}
void NativeNavigationBoxes::erase(Group*g){
 if(!g)return;manager_.remove_obstacle(g);
 groups_.erase(std::remove_if(groups_.begin(),groups_.end(),[g](const auto&p){return p.get()==g;}),groups_.end());
}
int NativeNavigationBoxes::refresh(){
 if(failed_)return 2;
 for(auto&p:boxes_)if(!p->group){
  auto g=std::make_unique<Group>();auto&v=g->polygon;for(const auto&q:p->box.points)v.points.push_back({q[0],q[1],q[2]});
  memcpy(v.center.data(),p->box.center,12);v.radius=p->box.radius;v.height=p->box.height;v.allow_inside=p->box.allow_inside;
  Group*raw=g.get();if(publish(std::move(g))){failed_=true;return 2;}p->group=raw;
  for(const auto&q:boxes_)if(q.get()!=p.get()&&q->group&&q->group!=p->group){
   int overlap=native_navigation_overlap(p->group->polygon,q->group->polygon);if(overlap<0){failed_=true;return 2;}if(!overlap)continue;
   Group*a=p->group,*b=q->group;auto merged=std::make_unique<Group>();
   if(native_navigation_merge(b->polygon,a->polygon,merged->polygon)){failed_=true;return 2;}Group*next=merged.get();
   for(auto&e:boxes_)if(e->group==a||e->group==b)e->group=next;
   //24630 removes old entries in group-list order, then appends the result.
   for(size_t i=0;i<groups_.size();)if(groups_[i].get()==a||groups_[i].get()==b)erase(groups_[i].get());else ++i;
   if(publish(std::move(merged))){for(auto&e:boxes_)if(e->group==next)e->group=nullptr;failed_=true;return 2;}
  }
 }
 return 0;
}
int NativeNavigationBoxes::remove(int32_t id){
 auto p=std::find_if(boxes_.begin(),boxes_.end(),[id](const auto&e){return e->box.id==id;});if(p==boxes_.end())return 1;
 Group*g=(*p)->group;boxes_.erase(p);for(auto&e:boxes_)if(e->group==g)e->group=nullptr;erase(g);return 0;
}
const NativeCharacterDetour::Obstacle*NativeNavigationBoxes::group(int32_t id)const{
 for(const auto&p:boxes_)if(p->box.id==id)return p->group?&p->group->published:nullptr;return nullptr;
}
