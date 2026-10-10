#include "order_hud.h"
#include "input_bindings.h"
#include "../texture_asset/texture_asset.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
extern "C" int native_hud_keycap(uint32_t*,unsigned,const char*,const char*);
namespace {
void require(bool yes,const char*message){if(!yes)throw std::runtime_error(message);}
std::string key(const char*n){require(n,"Missing order HUD material");std::string s(n);for(char&c:s){if(c=='/')c=92;if(c>='A'&&c<='Z')c+=32;}return s;}
struct Material{UCMaterialAsset data{};~Material(){uc_material_free(&data);}};
struct Texture{UCTextureAsset data{};~Texture(){uc_texture_free(&data);}};
bool equal(UCMaterialSpan a,UCMaterialSpan b){return a.size==b.size&&(!a.size||!std::memcmp(a.data,b.data,a.size));}
ComPtr<ID3D11ShaderResourceView> pixels(NativeRenderer&r,const uint32_t*p,unsigned w,unsigned h){
 UCTextureAsset t{};t.format=UC_TEXTURE_BGRA8_UNORM;t.width=w;t.height=h;t.mip_count=1;t.storage=(uint8_t*)p;t.storage_bytes=size_t(w)*h*4;t.mips[0]={w,h,w*4,w*h*4,(const uint8_t*)p};return r.upload(t);
}
}
void NativeOrderHud::load(const UCArchive*a,NativeRenderer&r){
 NativeOrderHud next;BytesOwner source;checked_resource(a,"hud\\hud_ammo_pistol.shader","e5e867aa24e66ecc186be07af9bb428faef646beef19f713d5cc608438910c40",source);
 Material reference;require(uc_material_decode(source.value.data,source.value.size,&reference.data)==UC_MATERIAL_OK,"Order HUD reference");
 for(size_t i=0;i<uc_archive_count(a);i++){
  auto*entry=uc_archive_entry(a,i);auto name=key((const char*)entry->name);
  if((name.rfind("hud\\hud_orders_",0)&&name.rfind("hud\\hud_icon_heal_",0))||name.size()<7||name.substr(name.size()-7)!=".shader"||name=="hud\\hud_orders_pad_xbox.shader")continue;
  Material material;require(uc_material_load(a,i,UC_AR_LOCAL_PAYLOAD,&material.data)==UC_MATERIAL_OK,"Order HUD material");const auto&m=material.data;const auto&ref=reference.data;
  require(!std::memcmp(m.header,ref.header,14)&&m.dependency_count==1&&m.dependencies[0].kind&&m.pass_count==1&&
   equal(m.setup_program[0],ref.setup_program[0])&&equal(m.setup_program[1],ref.setup_program[1]),"Unsupported order HUD recipe");
  const auto&p=m.passes[0];const auto&q=ref.passes[0];require(!std::memcmp(p.header,q.header,16)&&equal(p.vertex_program,q.vertex_program)&&equal(p.vertex_declaration,q.vertex_declaration)&&equal(p.pixel_program,q.pixel_program),"Unsupported order HUD shader");
  auto index=find_resource(a,m.dependencies[0].name);Texture texture;require(index!=SIZE_MAX&&uc_texture_load(a,index,UC_AR_LOCAL_PAYLOAD,&texture.data)==UC_TEXTURE_OK,"Order HUD texture");
  next.images_.emplace(name,Image{r.upload(texture.data),float(texture.data.width),float(texture.data.height)});
 }
 require(!next.images_.empty(),"Order HUD assets missing");uint32_t white=0xffffffff;next.white_=pixels(r,&white,1,1);next.update_key(r,VK_MBUTTON);*this=std::move(next);
}
ID3D11ShaderResourceView*NativeOrderHud::image(const char*material)const{auto i=images_.find(key(material));require(i!=images_.end(),"Unqualified heartbeat order image");return i->second.texture.Get();}
void NativeOrderHud::update_key(NativeRenderer&r,unsigned code){
 if(code==key_code_&&key_)return;const unsigned art=code==NativeInputBindings::wheel_up?0x100u:code==NativeInputBindings::wheel_down?0x101u:code;
 auto label=NativeInputBindings::key_name(code);std::vector<uint32_t> data(384*128);require(native_hud_keycap(data.data(),art,label.c_str(),"DRAG")!=0,"Order HUD keycap");auto view=pixels(r,data.data(),384,128);key_=std::move(view);key_code_=code;
}
void NativeOrderHud::draw(NativeRenderer&r,const std::vector<NativeOrderSprite>&sprites,bool held,float dx,float dy,int selected){
 require(sprites.size()<=8193,"Order HUD sprite bound");require(std::isfinite(dx)&&std::isfinite(dy)&&selected>=-1&&selected<4,"Invalid order HUD drag");require(key_&&white_,"Order HUD not loaded");vertices_.clear();views_.clear();
 const float scale=std::min(r.width/640.f,r.height/480.f),cx=r.width*.5f,cy=r.height*.62f;
 auto quad=[&](float x,float y,float w,float h,uint32_t color,ID3D11ShaderResourceView*texture){
  float xy[4][2]={{x,y},{x+w,y},{x+w,y+h},{x,y+h}},uv[4][2]={{0,0},{1,0},{1,1},{0,1}};
  for(unsigned i=0;i<4;i++){NativeVertex v{};v.position[0]=xy[i][0];v.position[1]=xy[i][1];v.color=color;memcpy(v.uv,uv[i],8);vertices_.push_back(v);}views_.push_back(texture);
 };
 bool hint=false,visible=false;for(const auto&s:sprites){
  require(std::isfinite(s.alpha)&&s.alpha>=0&&s.alpha<=1,"Order HUD alpha");for(float v:s.color)require(std::isfinite(v)&&v>=0&&v<=1,"Order HUD color");if(s.alpha==0)continue;
  if(s.slot==-1){require(key(s.material)=="hud\\hud_orders_pad_xbox.shader","Unsupported order HUD root");hint=true;continue;}
  require(s.slot>=0&&s.slot<4,"Invalid order HUD slot");auto i=images_.find(key(s.material));require(i!=images_.end(),"Unqualified order HUD image");const auto&image=i->second;visible=true;
  const float direction[4][2]={{0,-1},{0,1},{-1,0},{1,0}};float fit=std::min(64/image.width,32/image.height)*scale,w=image.width*fit,h=image.height*fit;
  float x=cx+direction[s.slot][0]*70*scale-w*.5f,y=cy+direction[s.slot][1]*45*scale-h*.5f;
  uint32_t color=(uint32_t(s.alpha*255)<<24)|(uint32_t(s.color[0]*255)<<16)|(uint32_t(s.color[1]*255)<<8)|uint32_t(s.color[2]*255);
  if(held&&selected==s.slot){const uint32_t line=0xc4bddb84;quad(x-3*scale,y-3*scale,w+6*scale,scale,line,white_.Get());quad(x-3*scale,y+h+2*scale,w+6*scale,scale,line,white_.Get());}
  quad(x,y,w,h,color,image.texture.Get());
 }
 if(hint||(held&&visible))quad(cx-48*scale,(held?cy+75*scale:r.height-53*scale),96*scale,32*scale,0xd9ffffff,key_.Get());
 if(held&&visible){
  float extent=std::max(std::fabs(dx),std::fabs(dy));if(extent>60){dx*=60/extent;dy*=60/extent;}dx*=.6f*scale;dy*=.6f*scale;
  float length=std::sqrt(dx*dx+dy*dy);if(length>0){float nx=-dy/length*scale,ny=dx/length*scale;quad(0,0,1,1,0xffbddb84,white_.Get());auto*v=vertices_.data()+vertices_.size()-4;
   float xy[4][2]={{cx+nx,cy+ny},{cx+dx+nx,cy+dy+ny},{cx+dx-nx,cy+dy-ny},{cx-nx,cy-ny}};for(unsigned i=0;i<4;i++){v[i].position[0]=xy[i][0];v[i].position[1]=xy[i][1];}}
  quad(cx+dx-2*scale,cy+dy-2*scale,4*scale,4*scale,0xffe8efda,white_.Get());
 }
 if(views_.empty())return;
 if(views_.size()>capacity_){capacity_=std::max(size_t(16),views_.size());std::vector<NativeVertex>v(capacity_*4);std::vector<uint32_t>indices(capacity_*6);const uint32_t q[]={0,1,2,0,2,3};for(size_t i=0;i<capacity_;i++)for(unsigned j=0;j<6;j++)indices[i*6+j]=uint32_t(i*4)+q[j];mesh_=r.upload(v.data(),v.size(),indices.data(),indices.size(),true);}
 vertices_.resize(capacity_*4);r.update(mesh_,vertices_.data(),vertices_.size());float matrix[16]={2.f/r.width,0,0,0,0,-2.f/r.height,0,0,0,0,1,0,-1,1,0,1},white[4]={1,1,1,1};NativeDrawState state;state.clamp_u=state.clamp_v=state.point_filter=state.depth_disabled=state.vertex_exposure=true;state.blend_mode=1;state.write_alpha=false;
 for(size_t i=0;i<views_.size();i++)r.draw(mesh_,matrix,white,views_[i],uint32_t(i*6),6,state);
}
