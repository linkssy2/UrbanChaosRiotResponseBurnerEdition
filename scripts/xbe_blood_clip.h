#pragma once
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
namespace blood_clip {
struct Point {float u,v;};
using Polygon=std::vector<Point>;
inline float cross(Point a,Point b,Point p){return (b.u-a.u)*(p.v-a.v)-(b.v-a.v)*(p.u-a.u);}
inline Point lerp(Point a,Point b,float t){return {a.u+(b.u-a.u)*t,a.v+(b.v-a.v)*t};}
inline void intersect(Polygon& a,const Polygon& b,Polygon& scratch){
 for(size_t i=0;i<b.size()&&!a.empty();++i){
  scratch.clear();Point p=a.back();float dp=cross(b[i],b[(i+1)%b.size()],p);
  for(Point q:a){float dq=cross(b[i],b[(i+1)%b.size()],q);
   if((dp>=0)!=(dq>=0))scratch.push_back(lerp(p,q,dp/(dp-dq)));
   if(dq>=0)scratch.push_back(q);p=q;dp=dq;
  }a.swap(scratch);
 }
}
template<class Probe> std::vector<Polygon> support(Probe probe){
 // shortcut: 8x8 surface sampling resolves ledges, not holes narrower than a cell; use collision triangles if such assets are encountered.
 constexpr int n=8;bool hits[n+1][n+1];
 for(int y=0;y<=n;++y)for(int x=0;x<=n;++x)hits[y][x]=probe(Point{float(x)/n,float(y)/n});
 std::vector<Polygon> result;int full=0;
 for(int y=0;y<n;++y)for(int x=0;x<n;++x){
  Point p[4]={{float(x)/n,float(y)/n},{float(x+1)/n,float(y)/n},{float(x+1)/n,float(y+1)/n},{float(x)/n,float(y+1)/n}};
  bool h[4]={hits[y][x],hits[y][x+1],hits[y+1][x+1],hits[y+1][x]};
  if(h[0]&&h[1]&&h[2]&&h[3]&&probe(lerp(p[0],p[2],.5f))){++full;result.push_back({p[0],p[1],p[2],p[3]});continue;}
  for(auto indices:{std::array<int,3>{0,1,2},std::array<int,3>{0,2,3}}){
   Polygon poly;
   for(int j=0;j<3;++j){int a=indices[j],b=indices[(j+1)%3];
    if(h[a])poly.push_back(p[a]);
    if(h[a]!=h[b]){Point inside=h[a]?p[a]:p[b],outside=h[a]?p[b]:p[a];
     for(int k=0;k<12;++k){Point mid=lerp(inside,outside,.5f);if(probe(mid))inside=mid;else outside=mid;}
     poly.push_back(inside);
    }
   }
   float area=0;for(size_t i=1;i+1<poly.size();++i)area+=cross(poly[0],poly[i],poly[i+1]);
   if(poly.size()>=3&&area>1e-10f){Point c{};for(auto v:poly){c.u+=v.u/poly.size();c.v+=v.v/poly.size();}if(probe(c))result.push_back(std::move(poly));}
  }
 }if(full==n*n)return {{{0,0},{1,0},{1,1},{0,1}}};return result;
}
}
