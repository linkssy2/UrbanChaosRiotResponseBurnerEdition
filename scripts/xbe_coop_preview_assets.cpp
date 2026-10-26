#include "xbe_coop_preview_assets.h"
#include "xbe_hd_ui_filter.h"
extern "C" {
#include "native/archive_reader/archive_reader.h"
#include "native/animation_binding.h"
#include "native/material_asset.h"
#include "native/model_render.h"
#include "native/texture_asset/texture_asset.h"
#include "native/texture_sequence.h"
}
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <xmmintrin.h>

namespace trial_coop_preview {
namespace {
constexpr size_t ArchiveLimit=16*1024*1024,EntryLimit=2*1024*1024,PixelLimit=32*1024*1024;
constexpr const char* Models[]={
    "assets_characters\\cha_multiplayer\\cha_multiplayer_tzero\\cha_multiplayer_tzero_skin.amf",
    "assets_characters\\cha_multiplayer\\cha_multiplayer_burners\\cha_multiplayer_avenger_skin.amf",
    "assets_characters\\cha_multiplayer\\cha_multiplayer_burners\\cha_multiplayer_banger_skin.amf",
    "assets_characters\\cha_multiplayer\\cha_multiplayer_burners\\cha_multiplayer_magnum_skin.amf",
    "assets_characters\\cha_multiplayer\\cha_multiplayer_burners\\cha_multiplayer_prophet_skin.amf"};
constexpr const char* Stand="assets_characteranimation\\characterspecific\\cca_multiplayer\\cca_multiplayer_anim_stand.kfa";
constexpr const char* Lowered="assets_characteranimation\\characterspecific\\cca_multiplayer\\cca_multiplayer_anim_handgun_idle_lowered.kfa";
constexpr float Identity[]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
template<class T,auto Free>struct Owner {T value{};~Owner(){Free(&value);}};
using Bytes=Owner<UCArchiveBytes,uc_archive_bytes_free>;
void Require(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
std::string Key(const char* text){
    Require(text!=nullptr,"Missing preview resource name");std::string result;
    for(;*text;++text){
        unsigned char c=static_cast<unsigned char>(*text);
        Require(c<128&&result.size()<1023,"Invalid preview resource name");
        result+=c=='/'?'\\':c>='A'&&c<='Z'?char(c+32):char(c);
    }
    return result;
}
void Read(const UCArchive* archive,const char* name,Bytes& bytes){
    const auto key=Key(name);size_t found=SIZE_MAX;
    for(size_t i=0;i<uc_archive_count(archive);++i){
        const auto* entry=uc_archive_entry(archive,i);
        if(Key(reinterpret_cast<const char*>(entry->name))!=key)continue;
        Require(found==SIZE_MAX&&!entry->selection_tag&&entry->unpacked_bytes<=EntryLimit,
                "Ambiguous or oversized preview resource");found=i;
    }
    Require(found!=SIZE_MAX&&uc_archive_read(archive,found,UC_AR_LOCAL_PAYLOAD,&bytes.value)==UC_AR_OK,
            "Missing or invalid co-op preview resource");
}
struct Recipe {bool sequence=false;};
Recipe Material(const UCMaterialAsset& m){
    // These four exact 40-01 recipes differ only in dependency names. No source
    // shader is executed; unknown programs or setup instructions fail closed.
    uint64_t hash=1469598103934665603ull;size_t size=0;
    const auto add=[&](const uint8_t* p,size_t n){size+=n;for(size_t i=0;i<n;++i){hash^=p[i];hash*=1099511628211ull;}};
    add(m.header,sizeof m.header);
    for(uint32_t i=0;i<m.dependency_count;++i)add(&m.dependencies[i].kind,1);
    for(const auto& s:m.setup_program)add(s.data,s.size);
    for(uint32_t i=0;i<m.pass_count;++i){
        const auto& p=m.passes[i];add(p.header,sizeof p.header);
        for(const auto& s:{p.vertex_program,p.vertex_declaration,p.pixel_program})add(s.data,s.size);
    }
    if((hash==0x35a2b9321cce1f21ull&&size==756)||(hash==0x4f6efa7f088a0005ull&&size==757))return {};
    if(hash==0x9307ef1e96c3c6b6ull&&size==808)return {true};
    // Source8FF30 uses source-alpha blending for these direct visor/glasses/blade
    // textures. Preserve authored alpha; enclosing game alpha-test state varies.
    if(hash==0x9f754e75e0f0b16full&&size==84)return {};
    throw std::runtime_error("Unsupported co-op preview material recipe");
}
Image Texture(const UCArchive* archive,const char* name){
    Bytes bytes;Read(archive,name,bytes);Owner<UCTextureAsset,uc_texture_free> decoded;
    Require(uc_texture_decode(bytes.value.data,bytes.value.size,&decoded.value)==UC_TEXTURE_OK,
            "Unsupported co-op preview texture");
    const auto& t=decoded.value;
    Require(t.width<=256&&t.height<=256,"Oversized co-op preview texture");
    const unsigned format=t.format==UC_TEXTURE_BC1_UNORM?12:t.format==UC_TEXTURE_BC3_UNORM?15:0;
    Image image{t.width,t.height,{}};
    Require(uc_hd_ui::filter::DecodeNative(t.width,t.height,format,{t.mips[0].data,t.mips[0].slice_pitch},image.rgba),
            "Unsupported co-op preview texture format");
    for(size_t i=0;i<image.rgba.size();i+=4)std::swap(image.rgba[i],image.rgba[i+2]);
    return image;
}
Image MaterialImage(const UCArchive* archive,const UCMaterialAsset& m,const Recipe& recipe){
    Require(m.dependency_count&&m.dependencies[0].kind,"Missing preview material texture");
    const char* name=m.dependencies[0].name;
    if(!recipe.sequence)return Texture(archive,name);
    Bytes bytes;Read(archive,name,bytes);Owner<UCTextureSequence,uc_texture_sequence_free> sequence;
    Require(uc_texture_sequence_decode(bytes.value.data,bytes.value.size,&sequence.value)==UC_SEQUENCE_OK&&
            sequence.value.count<=16,"Unsupported preview texture sequence");
    Image first;
    for(uint32_t i=0;i<sequence.value.count;++i){
        auto image=Texture(archive,sequence.value.names[i]);if(!i)first=std::move(image);
    }
    // A representative frame0 is explicit here; gameplay owns its independently
    // selected per-player texture variation.
    return first;
}
void Tile(Model& model,unsigned image_index){
    auto& image=model.images[image_index];float low[2]{0,0},high[2]{1,1};
    for(const auto& triangle:model.triangles)if(triangle.image==image_index)
        for(auto index:triangle.indices)for(unsigned k=0;k<2;++k){
            const float uv=model.vertices[index].uv[k];Require(std::isfinite(uv)&&std::abs(uv)<=4,"Invalid preview UV");
            low[k]=std::min(low[k],uv);high[k]=std::max(high[k],uv);
        }
    const int start[2]={int(std::floor(low[0])),int(std::floor(low[1]))};
    const unsigned count[2]={unsigned(std::ceil(high[0])-start[0]),unsigned(std::ceil(high[1])-start[1])};
    Require(count[0]&&count[1]&&count[0]*image.width<=MaxImageDimension&&count[1]*image.height<=MaxImageDimension,
            "Oversized wrapped preview texture");
    if(count[0]!=1||count[1]!=1){
        Image tiled{image.width*count[0],image.height*count[1],{}};
        tiled.rgba.resize(size_t(tiled.width)*tiled.height*4);
        for(unsigned y=0;y<tiled.height;++y)for(unsigned x=0;x<tiled.width;++x)
            std::memcpy(tiled.rgba.data()+(size_t(y)*tiled.width+x)*4,
                        image.rgba.data()+(size_t(y%image.height)*image.width+x%image.width)*4,4);
        image=std::move(tiled);
    }
    // Each prepared corner belongs to one range/triangle, so remap it once.
    for(const auto& triangle:model.triangles)if(triangle.image==image_index)
        for(auto index:triangle.indices)for(unsigned k=0;k<2;++k)
            model.vertices[index].uv[k]=(model.vertices[index].uv[k]-start[k])/count[k];
}
}
bool Load(const std::filesystem::path& game,unsigned character,Model& out,std::string& error){
    const unsigned previous=_mm_getcsr();
    struct Restore {unsigned value;~Restore(){_mm_setcsr(value);}} restore{previous};_mm_setcsr(0x1f80);
    try {
        Require(character<std::size(Models),"Invalid co-op preview character");
        std::ifstream file(game/"coopcharacters.lst",std::ios::binary|std::ios::ate);
        Require(bool(file),"Co-op character archive is unavailable");const auto length=file.tellg();
        Require(length>=0xc8&&length<=std::streamoff(ArchiveLimit),"Invalid co-op character archive size");
        std::vector<uint8_t> data(static_cast<size_t>(length));file.seekg(0);
        Require(bool(file.read(reinterpret_cast<char*>(data.data()),std::streamsize(data.size()))),"Cannot read co-op character archive");
        UCArchive* raw=nullptr;Require(uc_archive_copy(data.data(),data.size(),&raw)==UC_AR_OK,"Invalid co-op character archive");
        std::unique_ptr<UCArchive,decltype(&uc_archive_close)> archive(raw,uc_archive_close);
        Bytes bytes;Read(archive.get(),Models[character],bytes);Owner<UCModel,uc_model_free> source;
        Require(uc_model_load(bytes.value.data,bytes.value.size,&source.value)==UC_MODEL_OK,"Unsupported co-op preview model");
        auto& m=source.value;
        Require(m.bone_count==73&&m.vertex_count<=8192&&m.batch_count<=128&&m.material_count<=MaxImages,
                "Unexpected co-op preview model layout");
        Bytes template_bytes,clip_bytes,upper_bytes;Read(archive.get(),m.animation_template,template_bytes);Read(archive.get(),Stand,clip_bytes);Read(archive.get(),Lowered,upper_bytes);
        Owner<UCAnimationTemplate,uc_animation_template_free> skeleton;
        Owner<UCAnimationSample,uc_animation_sample_free> clip;
        Owner<UCAnimationSample,uc_animation_sample_free> upper;
        Owner<UCAnimationBinding,uc_animation_binding_free> pose;
        Require(uc_animation_template_load(template_bytes.value.data,template_bytes.value.size,&skeleton.value)==UC_ANIMATION_OK&&
                uc_animation_sample_load(clip_bytes.value.data,clip_bytes.value.size,&clip.value)==UC_ANIMATION_OK&&
                uc_animation_sample_load(upper_bytes.value.data,upper_bytes.value.size,&upper.value)==UC_ANIMATION_OK&&
                uc_animation_binding_init(&m,&skeleton.value,&pose.value)==UC_ANIMATION_OK,
                "Unsupported co-op standing animation");
        Require(uc_animation_sample_phase(&clip.value,0)==UC_ANIMATION_OK&&clip.value.fraction==0&&
                uc_animation_sample_phase(&upper.value,0)==UC_ANIMATION_OK&&upper.value.fraction==0,"Standing pose is not an exact key");
        // Stand owns legs; the original lowered idle supplies upper-body tracks.
        // Full-weight exact keys copy samples without the interpolation table.
        UCAnimationBlendMath exact_key_math;
        std::fill(std::begin(exact_key_math.coefficients),std::end(exact_key_math.coefficients),std::numeric_limits<float>::quiet_NaN());
        const UCAnimationBindingSample samples[]={{&clip.value,0,1,1},{&upper.value,0,1,1}};
        Require(uc_animation_binding_sample_many(&pose.value,samples,std::size(samples),&exact_key_math)==UC_ANIMATION_OK,
                "Cannot pose co-op character");
        // E47A0 uses variant[seed%3]; all current co-op choices set seed0 and
        // head-variant0, including the other two stored head-variant values.
        constexpr const char* equipment_names[]={"ITEM_Head_01","ITEM_Head_02","ITEM_Head_03","ITEM_Arm_Left","ITEM_Arm_Right","ITEM_Chest","ITEM_Back","ITEM_Leg_Left","ITEM_Leg_Right"};
        UCModelEquipment equipment{{-1,-1,-1,-1,-1,-1,-1,-1,-1},{1,0,0,0,1,0,0,1,1}};
        for(unsigned slot=0;slot<std::size(equipment_names);++slot){
            for(uint32_t bone=0;bone<skeleton.value.bone_count;++bone)
                if(Key(skeleton.value.bone_names[bone])==Key(equipment_names[slot])){
                    Require(equipment.bones[slot]==-1,"Duplicate preview equipment bone");equipment.bones[slot]=int32_t(bone);
                }
            Require(equipment.bones[slot]>=0,"Missing preview equipment bone");
        }
        Require(uc_model_pose_equipment(&equipment,pose.value.local,m.bone_count)==0,"Cannot apply co-op preview appearance");
        Model result;
        for(uint32_t i=0;i<m.material_count;++i){
            Bytes material_bytes;Read(archive.get(),m.material_names[i],material_bytes);
            Owner<UCMaterialAsset,uc_material_free> material;
            Require(uc_material_decode(material_bytes.value.data,material_bytes.value.size,&material.value)==UC_MATERIAL_OK,
                    "Invalid co-op preview material");
            const auto recipe=Material(material.value);result.images.push_back(MaterialImage(archive.get(),material.value,recipe));
        }
        std::vector<uint8_t> draw(m.batch_count,1);
        for(uint32_t i=0;i<m.batch_count;++i)
            Require(m.batches[i].light_index==UINT16_MAX&&m.batches[i].flags[1],"Unexpected preview draw pass");
        Owner<UCModelRender,uc_model_render_free> prepared;
        Require(uc_model_render_init_lighting(&m,draw.data(),&prepared.value)==UC_MODEL_RENDER_OK&&
                prepared.value.vertex_count<=150000&&
                uc_model_render_update(&prepared.value,pose.value.local,Identity,nullptr,nullptr)==UC_MODEL_RENDER_OK,
                "Cannot prepare co-op character geometry");
        const auto& p=prepared.value;result.vertices.resize(p.vertex_count);
        float low[3]{INFINITY,INFINITY,INFINITY},high[3]{-INFINITY,-INFINITY,-INFINITY};
        for(uint32_t i=0;i<p.vertex_count;++i){
            auto& v=result.vertices[i];std::memcpy(v.position,p.vertices[i].position,sizeof v.position);
            std::memcpy(v.normal,p.vectors[i].normal,sizeof v.normal);std::memcpy(v.uv,p.vertices[i].uv,sizeof v.uv);v.color=p.vertices[i].color;
            float length2=0;for(float n:v.normal)length2+=n*n;
            Require(std::isfinite(length2),"Invalid co-op preview normal");
            if(length2>1e-12f)for(float& n:v.normal)n/=std::sqrt(length2);else v.normal[1]=1;
            for(unsigned k=0;k<3;++k){Require(std::isfinite(v.position[k]),"Invalid co-op preview position");low[k]=std::min(low[k],v.position[k]);high[k]=std::max(high[k],v.position[k]);}
        }
        for(uint32_t i=0;i<p.range_count;++i){const auto& range=p.ranges[i];
            for(uint32_t j=0;j<range.index_count;j+=3){
                Triangle t{{p.indices[range.first_index+j],p.indices[range.first_index+j+1],p.indices[range.first_index+j+2]},range.material_index};
                result.triangles.push_back(t);
            }
        }
        Require(!result.triangles.empty(),"Empty co-op preview geometry");
        float radius2=0;for(unsigned k=0;k<3;++k){result.center[k]=(low[k]+high[k])*.5f;radius2+=(high[k]-low[k])*(high[k]-low[k])*.25f;}
        result.radius=std::sqrt(radius2);Require(std::isfinite(result.radius)&&result.radius>0&&result.radius<10000,"Invalid co-op preview bounds");
        size_t pixels=0;for(unsigned i=0;i<result.images.size();++i){Tile(result,i);pixels+=result.images[i].rgba.size();Require(pixels<=PixelLimit,"Co-op preview texture budget exceeded");}
        out=std::move(result);error.clear();return true;
    } catch(const std::exception& failure){error=failure.what();return false;}
}
}
