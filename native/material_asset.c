#include "material_asset.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static uint16_t read16(const uint8_t *p) { return (uint16_t)((uint16_t)p[0]|(uint16_t)p[1]<<8); }
static int take(const uint8_t *p,size_t size,size_t *at,size_t count,const uint8_t **out) {
    if(count>size-*at)return 0;
    *out=p+*at;*at+=count;return 1;
}
void uc_material_free(UCMaterialAsset *m) {
    if(m) { free(m->storage);free(m->dependencies);free(m->passes);memset(m,0,sizeof(*m)); }
}
UCMaterialStatus uc_material_decode(const void *data,size_t size,UCMaterialAsset *out) {
    const uint8_t *input=(const uint8_t *)data,*p;
    UCMaterialAsset m={0}; size_t at=14,name_at=size;
    UCMaterialStatus error=UC_MATERIAL_FORMAT;
    if(!out)return UC_MATERIAL_ARGUMENT;
    memset(out,0,sizeof(*out));
    if(!input)return UC_MATERIAL_ARGUMENT;
    if(size<14)return UC_MATERIAL_FORMAT;
    if(size>2u*1024u*1024u)return UC_MATERIAL_LIMIT;
    memcpy(m.header,input,14);m.source_flags=read16(input+4);m.effective_flags=m.source_flags;
    /* Original90E4F..90E7D normalization. No rendering behavior inferred. */
    if((m.effective_flags&1) && !(m.effective_flags&0x60))m.effective_flags|=0x142;
    if((m.effective_flags&3)==3)m.effective_flags|=0x140;
    m.dependency_count=input[7];m.pass_count=input[11];
    m.storage_bytes=size+(size_t)m.dependency_count*256;
    m.storage=(uint8_t *)malloc(m.storage_bytes);
    m.dependencies=(UCMaterialDependency *)calloc(m.dependency_count?m.dependency_count:1,sizeof(*m.dependencies));
    m.passes=(UCMaterialPass *)calloc(m.pass_count?m.pass_count:1,sizeof(*m.passes));
    if(!m.storage||!m.dependencies||!m.passes){error=UC_MATERIAL_MEMORY;goto fail;}
    memcpy(m.storage,input,size);input=m.storage;
    for(uint32_t i=0;i<m.dependency_count;++i) {
        UCMaterialDependency *d=&m.dependencies[i];
        if(!take(input,size,&at,3,&p))goto fail;
        d->kind=p[0];d->name_bytes=read16(p+1);
        /* Original loader uses a256B stack string and appends NUL. */
        if(!d->name_bytes || d->name_bytes>=256){error=UC_MATERIAL_UNSUPPORTED;goto fail;}
        if(!take(input,size,&at,d->name_bytes,&p))goto fail;
        if(memchr(p,0,d->name_bytes)){error=UC_MATERIAL_UNSUPPORTED;goto fail;}
        memcpy(m.storage+name_at,p,d->name_bytes);m.storage[name_at+d->name_bytes]=0;
        d->name=(const char *)m.storage+name_at;name_at+=d->name_bytes+1;
    }
    for(unsigned i=0;i<2;++i) {
        m.setup_program[i].size=read16(m.header+i*2);
        if(!take(input,size,&at,m.setup_program[i].size,&m.setup_program[i].data))goto fail;
    }
    for(uint32_t i=0;i<m.pass_count;++i) {
        UCMaterialPass *pass=&m.passes[i];
        if(!take(input,size,&at,16,&p))goto fail;
        memcpy(pass->header,p,16);pass->dependency_count=p[0];
        if(pass->dependency_count>4){error=UC_MATERIAL_UNSUPPORTED;goto fail;}
        for(unsigned j=0;j<pass->dependency_count;++j) {
            pass->dependency_indices[j]=p[j+1];
            if(p[j+1]>=m.dependency_count)goto fail;
            if(!m.dependencies[p[j+1]].kind){error=UC_MATERIAL_UNSUPPORTED;goto fail;}
        }
        pass->vertex_program.size=read16(p+8);
        pass->vertex_declaration.size=read16(p+10);
        pass->pixel_program.size=read16(p+12);
        if(!take(input,size,&at,pass->vertex_program.size,&pass->vertex_program.data) ||
           !take(input,size,&at,pass->vertex_declaration.size,&pass->vertex_declaration.data) ||
           !take(input,size,&at,pass->pixel_program.size,&pass->pixel_program.data))goto fail;
    }
    /* Optional resource indices used by original90E15/90E34. */
    if((m.header[8]!=255 && m.header[8]>=m.dependency_count) ||
       (m.header[9]!=255 && m.header[9]>=m.dependency_count))goto fail;
    if(at!=size)goto fail;
    *out=m;return UC_MATERIAL_OK;
fail:
    uc_material_free(&m);return error;
}
UCMaterialStatus uc_material_load(const UCArchive *archive,size_t index,
    UCArchiveSource source,UCMaterialAsset *out) {
    UCArchiveBytes b={0};UCMaterialStatus result;
    if(!out)return UC_MATERIAL_ARGUMENT;
    memset(out,0,sizeof(*out));
    if(uc_archive_read(archive,index,source,&b)!=UC_AR_OK)return UC_MATERIAL_ARCHIVE;
    result=uc_material_decode(b.data,b.size,out);uc_archive_bytes_free(&b);return result;
}
const char *uc_material_status(UCMaterialStatus status) {
    static const char *const names[]={"ok","argument","format","unsupported","limit","memory","archive"};
    return (unsigned)status<sizeof(names)/sizeof(names[0])?names[status]:"unknown";
}

void uc_material_catalog_free(UCNativeMaterialCatalog *c) {
    if(c){free(c->materials);free(c->storage);memset(c,0,sizeof(*c));}
}
UCMaterialStatus uc_material_catalog_decode(const void *data,size_t size,UCNativeMaterialCatalog *out) {
    const uint8_t *p=(const uint8_t *)data,*record,*name,*texture;
    UCNativeMaterialCatalog c={0};size_t at=44;
    if(!out)return UC_MATERIAL_ARGUMENT;
    memset(out,0,sizeof(*out));
    if(!p)return UC_MATERIAL_ARGUMENT;
    if(size<44||(memcmp(p,"UCMAT006",8)&&memcmp(p,"UCMAT007",8)&&memcmp(p,"UCMAT008",8)&&memcmp(p,"UCMAT009",8)))return UC_MATERIAL_FORMAT;
    if(size>16u*1024u*1024u)return UC_MATERIAL_LIMIT;
    c.count=(uint32_t)p[8]|(uint32_t)p[9]<<8|(uint32_t)p[10]<<16|(uint32_t)p[11]<<24;
    if(c.count>100000 || c.count>(size-44)/50)return UC_MATERIAL_LIMIT;
    c.storage=(uint8_t*)malloc(size);
    c.materials=(UCNativeMaterial*)calloc(c.count?c.count:1,sizeof(*c.materials));
    if(!c.storage||!c.materials){uc_material_catalog_free(&c);return UC_MATERIAL_MEMORY;}
    c.storage_bytes=size;memcpy(c.storage,p,size);p=c.storage;
    memcpy(c.source_archive_sha256,p+12,32);
    for(uint32_t i=0;i<c.count;++i) {
        UCNativeMaterial *m=&c.materials[i];uint16_t n,t;
        if(!take(p,size,&at,p[7]>='8'?56:48,&record))goto bad;
        uint8_t features=record[11]&15;
        m->rotor_uv=(record[11]>>4)&1;
        if(record[11]>(p[7]=='9'?31:p[7]>='7'?15:7))goto bad;
        n=read16(record+2);t=read16(record+4);
        if(record[0]>4||record[1]>6||record[6]>1||record[7]>3||!n||n>1024||!t||t>256)goto bad;
        if((record[0]!=0 && (record[1]!=0||(record[0]==3?t!=1:t<2))) || (record[0]==0 && (record[1]==0||t!=1)))goto bad;
        if(record[0]==3&&(record[6]||record[7]||record[8]||record[9]||record[10]||features>1))goto bad;
        if(record[8]>4||record[8]==3||record[9]>1||(record[10]>1&&record[10]!=14&&record[10]!=15)||features>(p[7]>='7'?15:7))goto bad;
        m->geometry=(UCNativeMaterialGeometry)(features&1);
        m->alpha_mode=(UCNativeMaterialAlpha)((features>>1)&1);
        m->color_write=(UCNativeMaterialColorWrite)((features>>2)&1);
        m->texture_selection=(UCNativeMaterialTextureSelection)(features>>3);
        if(m->geometry) {
            if(record[0]==3) {
                if(features!=1||record[12]!=255||record[13]!=255||record[14]!=255||record[15]!=255)goto bad;
            } else if(record[0]==4) {
                if(record[7]||record[8]!=1||record[9]||record[10]||!((features==1&&!record[6])||(features==9&&record[6]==1)))goto bad;
            } else {
            if((record[0]!=1&&record[0]!=2)||
               (record[10]&&!(record[0]==1&&!record[6]&&!record[7]&&(record[8]==1||record[8]==2)&&record[9]==1&&record[10]==15&&features==1))||
               (record[7]&&!(record[7]==3&&record[8]==4&&!record[6])))goto bad;
            if(record[0]==2) {
                if(features!=1||!((!record[6]&&!record[8]&&!record[9])||
                    (record[8]>=1&&record[8]<=4&&record[8]!=3&&record[9]==1&&record[15]==255)))goto bad;
            } else if(record[8]) {
                if(record[8]>2&&record[8]!=4)goto bad;
                if(record[9]!=1||!((features==1)||(!record[6]&&record[8]==1&&features==3)))goto bad;
            /* Exact cleaver handle: two-sided skin, white vertex alpha and
             * temporary RGBA writes (source flags12). */
            } else if(record[9]||(record[6]&&features!=1&&!(features==7&&!record[7])))goto bad;
            }
        } else if(features && !(features==2&&record[0]==1&&record[6]==0&&record[7]==0&&record[8]==1&&record[9]==1&&record[10]==0) &&
                  (features!=4||record[0]!=1||record[6]||record[7]||record[8]||record[9]||record[10]))goto bad;
        if(p[7]>='8')memcpy(m->uv_period,record+48,8);
        for(unsigned axis=0;axis<2;++axis)if(!isfinite(m->uv_period[axis]) ||
            (m->uv_period[axis]!=0 && (fabsf(m->uv_period[axis])<1e-6f||fabsf(m->uv_period[axis])>1e6f)))goto bad;
        if((m->uv_period[0]||m->uv_period[1]) && !((!m->geometry&&record[0]==2&&record[8]==2&&record[9]==1&&!record[7]&&!record[10])||
            (m->geometry&&(record[0]==1||record[0]==2)&&record[7]==0&&record[8]==2&&record[9]==1&&record[10]==0&&record[11]==1)||
            (record[0]==1&&!record[6]&&!record[7]&&record[9]==1&&record[11]==1&&m->uv_period[0]==0&&
             ((record[8]==4&&!record[10]&&m->uv_period[1]==.5f)||(record[8]==2&&record[10]==15&&m->uv_period[1]==1.f)))||
            (record[0]==1&&record[6]==0&&record[7]==0&&record[8]==1&&record[9]==1&&record[10]==0&&record[11]==3&&m->uv_period[0]==10&&m->uv_period[1]==0)))goto bad;
        if(m->rotor_uv&&(record[0]!=1||record[6]!=1||record[7]||record[8]!=1||record[9]!=1||record[10]||features!=1||m->uv_period[0]||m->uv_period[1]))goto bad;
        m->blend_mode=(UCNativeMaterialBlend)record[8];m->vertex_exposure=record[9];m->depth_bias=record[10];
        m->constant_argb=(uint32_t)record[12]|(uint32_t)record[13]<<8|(uint32_t)record[14]<<16|(uint32_t)record[15]<<24;
        if(record[0]==0&&(record[6]||record[7]||m->constant_argb))goto bad;
        if(record[0]==1&&m->constant_argb!=0xffffffffu&&!(record[11]==3&&record[8]==1&&record[9]==1&&m->constant_argb==0x66666666u&&m->uv_period[0]==10&&m->uv_period[1]==0))goto bad;
        /* Only authored combinations currently recovered by the converter. */
        if(record[0]==4) {
            if(record[7]||record[8]!=1||record[9]||record[10]||(record[11]!=(uint8_t)m->geometry&&!(record[11]==9&&record[6]==1))||m->constant_argb!=UINT32_MAX||m->uv_period[0]||m->uv_period[1])goto bad;
        } else if(m->geometry) { /* Skinned combinations checked above. */
        } else if(record[8]==2) {
            if(record[7]||record[9]!=1||(m->constant_argb>>24)!=255)goto bad;
            if(!((record[0]==1&&!record[6]&&record[10]==14)||(record[0]==2&&record[10]==0)))goto bad;
        } else if(record[8]==4) {
            if((record[0]!=1&&record[0]!=2)||record[6]||record[7]||record[9]!=1||record[10]||(m->constant_argb>>24)!=255)goto bad;
        } else if(record[8]==1) {
            if((record[0]!=1&&record[0]!=2)||record[9]!=1||(m->constant_argb>>24)!=255)goto bad;
            if(record[0]==1&&(record[7]||(record[10]&&record[10]!=15)))goto bad;
            if(!record[6]&&(record[7]||(record[10]&&record[10]!=15)))goto bad;
            if(!((record[7]==1&&record[10]==0)||(record[7]==0&&(record[10]<=1||(record[0]==1&&record[10]==15)))))goto bad;
        } else if(record[9]||record[10]||(record[6]&&record[7])||record[7]==2||(record[0]==2&&record[7]))goto bad;
        if(!take(p,size,&at,n,&name)||!take(p,size,&at,t,&texture))goto bad;
        if(name[n-1]||texture[t-1]||memchr(name,0,n-1)||memchr(texture,0,t-1))goto bad;
        if(m->texture_selection && (t<5||texture[t-5]!='.'||
           (texture[t-4]!='i'&&texture[t-4]!='I')||(texture[t-3]!='f'&&texture[t-3]!='F')||
           (texture[t-2]!='l'&&texture[t-2]!='L')))goto bad;
        if(m->geometry&&record[0]!=3&&!m->texture_selection && (t<5||texture[t-5]!='.'||
           (texture[t-4]!='t'&&texture[t-4]!='T')||(texture[t-3]!='g'&&texture[t-3]!='G')||
           (texture[t-2]!='a'&&texture[t-2]!='A')))goto bad;
        m->kind=(UCNativeMaterialKind)record[0];m->unsupported_reason=record[1];
        m->cull_mode=(UCNativeMaterialCull)record[6];m->address_v=(UCNativeMaterialAddressV)(record[7]&1);
        m->address_u=(UCNativeMaterialAddressU)(record[7]>>1);
        memcpy(m->source_sha256,record+16,32);m->name=(const char*)name;m->texture_name=(const char*)texture;
    }
    if(at!=size)goto bad;
    *out=c;return UC_MATERIAL_OK;
bad:
    uc_material_catalog_free(&c);return UC_MATERIAL_FORMAT;
}
const char *uc_material_unsupported_reason(uint32_t reason) {
    static const char *const names[]={"supported","setup script","material header/context",
        "pass bindings","vertex program","vertex declaration","pixel program"};
    return reason<sizeof(names)/sizeof(names[0])?names[reason]:"unknown";
}
