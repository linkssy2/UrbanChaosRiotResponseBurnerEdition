#include "collision_asset.h"
#include "ragdoll_asset.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define LIMIT (256u*1024u*1024u)
#define NONE UINT32_MAX
typedef struct Section { uint32_t base, local, global, virtuals, end; } Section;
typedef struct Fixup { uint32_t source, target; } Fixup;
typedef struct Reader {
    const uint8_t *data; size_t size, pos, allocated;
    Section sections[5]; Fixup *fixups; uint32_t fixup_count;
    UCCollisionStatus status;
} Reader;
static uint32_t u32(const uint8_t *p) { return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }
static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0]|(p[1]<<8)); }
static const uint8_t *span(Reader *r, size_t at, size_t bytes) {
    if(at>r->size||bytes>r->size-at){r->status=UC_COLLISION_FORMAT;return NULL;}
    return r->data+at;
}
static int number(Reader *r, uint32_t *out) {
    const uint8_t *p=span(r,r->pos,4);if(!p)return 0;*out=u32(p);r->pos+=4;return 1;
}
static int alloc(Reader *r, void **out, size_t count, size_t stride) {
    if(count>(LIMIT-r->allocated)/stride){r->status=UC_COLLISION_LIMIT;return 0;}
    if(!count)return 1;
    *out=malloc(count*stride);if(!*out){r->status=UC_COLLISION_MEMORY;return 0;}
    r->allocated+=count*stride;return 1;
}
#define NEED(x) do {if(!(x))return 0;}while(0)
#define VALID(x) do {if(!(x)){r->status=UC_COLLISION_FORMAT;return 0;}}while(0)
#define SUPPORT(x) do {if(!(x)){r->status=UC_COLLISION_UNSUPPORTED;return 0;}}while(0)
static int fix_compare(const void *a,const void *b) {
    uint32_t x=((const Fixup*)a)->source,y=((const Fixup*)b)->source;return (x>y)-(x<y);
}
static uint32_t pointer(const Reader *r,uint32_t at) {
    Fixup key={at,0};const Fixup *f=bsearch(&key,r->fixups,r->fixup_count,sizeof(*f),fix_compare);
    return f?f->target:NONE;
}
static int named(Reader *r,uint32_t at,const char *name) {
    size_t n=strlen(name)+1;const uint8_t *p=span(r,at,n);return p&&!memcmp(p,name,n);
}
static int packfile(Reader *r) {
    const uint8_t *h=span(r,0,304);NEED(h);
    SUPPORT(u32(h)==0x57E0E057&&u32(h+4)==0x10C0C010&&!memcmp(h+8,"snap",4));
    SUPPORT(u32(h+12)==2&&u32(h+16)==0x01000104&&u32(h+20)==5);
    const char *tags[]={"__classnames__","__classindex__","__dataindex__","__data__","__types__"};
    uint32_t count=0;
    for(unsigned i=0;i<5;i++) {
        const uint8_t *s=h+64+i*48;Section *q=&r->sections[i];
        SUPPORT(!memcmp(s,tags[i],strlen(tags[i])+1));
        q->base=u32(s+20);q->local=u32(s+24);q->global=u32(s+28);q->virtuals=u32(s+32);q->end=u32(s+40);
        VALID(q->base>=304&&q->local<=q->global&&q->global<=q->virtuals&&q->virtuals<=q->end);
        NEED(span(r,q->base,q->end));
        VALID((q->global-q->local)%8==0&&(q->virtuals-q->global)%4==0);
        /* Tables are padded to sixteen bytes with all-ones records. */
        count+=(q->global-q->local)/8+(q->virtuals-q->global)/12;
        if(i)VALID(r->sections[i-1].base+r->sections[i-1].end==q->base);
    }
    VALID(r->sections[4].base+r->sections[4].end==r->size);
    NEED(alloc(r,(void**)&r->fixups,count,sizeof(Fixup)));
    for(unsigned i=0;i<5;i++) {
        Section *s=&r->sections[i];
        for(unsigned kind=0;kind<2;kind++) {
            uint32_t at=kind?s->global:s->local,end=kind?s->virtuals:s->global,step=kind?12:8;
            for(;end-at>=step;at+=step) {
                const uint8_t *p=r->data+s->base+at;uint32_t from=u32(p),section=kind?u32(p+4):i,to=u32(p+(kind?8:4));
                if(from==NONE)continue;
                VALID(section<5&&s->local>=4&&from<=s->local-4&&to<r->sections[section].local);
                r->fixups[r->fixup_count++]=(Fixup){s->base+from,r->sections[section].base+to};
            }
            for(;at<end;at++)VALID(r->data[s->base+at]==255);
        }
    }
    qsort(r->fixups,r->fixup_count,sizeof(Fixup),fix_compare);
    for(unsigned i=1;i<r->fixup_count;i++)VALID(r->fixups[i-1].source!=r->fixups[i].source);
    return 1;
}
static uint32_t class_named(Reader *r,const char *name) {
    Section *s=&r->sections[1];
    if(s->local%8)return NONE;
    for(uint32_t at=0;at<s->local;at+=8) {
        const uint8_t *p=r->data+s->base+at;uint32_t sec=u32(p),off=u32(p+4);
        if(sec==NONE&&off==NONE)continue;
        if(sec>=5||off>r->sections[sec].local||r->sections[sec].local-off<36)return NONE;
        uint32_t object=r->sections[sec].base+off;
        if(named(r,pointer(r,object),name))return object;
        if(r->status)return NONE;
    }
    return NONE;
}
static int member(Reader *r,uint32_t object,const char *name,unsigned type,unsigned subtype,unsigned offset) {
    const uint8_t *c=span(r,object,36);NEED(c);uint32_t count=u32(c+28),table=pointer(r,object+24);
    VALID(count<100);NEED(span(r,table,count*20));
    for(uint32_t i=0;i<count;i++) {
        const uint8_t *m=r->data+table+i*20;
        if(named(r,pointer(r,table+i*20),name)) {
            SUPPORT(m[12]==type&&m[13]==subtype&&u16(m+14)==0&&u16(m+18)==offset);return 1;
        }
        if(r->status)return 0;
    }
    r->status=UC_COLLISION_UNSUPPORTED;return 0;
}
static int schema(Reader *r) {
    const char *classes[]={"hkMoppBvTreeShape","hkBvTreeShape","hkStorageMeshShape","hkMeshShape","SubpartStorage","Subpart","hkMoppCode"};
    const unsigned sizes[]={20,16,80,64,72,48,48};
    for(unsigned i=0;i<7;i++) {
        uint32_t c=class_named(r,classes[i]);const uint8_t *p=span(r,c,36);NEED(p);SUPPORT(u32(p+8)==sizes[i]);
    }
    uint32_t a=class_named(r,"hkMoppBvTreeShape");NEED(member(r,a,"code",20,25,16));
    a=class_named(r,"hkBvTreeShape");NEED(member(r,a,"shapeCollection",20,25,12));
    a=class_named(r,"hkStorageMeshShape");NEED(member(r,a,"storage",22,20,64));
    a=class_named(r,"hkMeshShape");NEED(member(r,a,"scaling",12,0,16));NEED(member(r,a,"numBitsForSubpartIndex",7,0,32));
    NEED(member(r,a,"subparts",22,25,36));NEED(member(r,a,"radius",11,0,48));
    a=class_named(r,"SubpartStorage");
    const char *names[]={"vertices","indices16","indices32","materialIndices","materials","materialIndices16"};
    const unsigned types[]={11,6,8,4,8,6};
    for(unsigned i=0;i<6;i++)NEED(member(r,a,names[i],22,types[i],i*12));
    a=class_named(r,"Subpart");
    const char *fields[]={"vertexStriding","numVertices","indexStriding","numTriangles","materialIndexStriding","materialStriding","numMaterials"};
    const unsigned offsets[]={4,8,20,24,32,40,44};
    for(unsigned i=0;i<7;i++)NEED(member(r,a,fields[i],7,0,offsets[i]));
    NEED(member(r,a,"stridingType",24,0,16));NEED(member(r,a,"materialIndexStridingType",24,0,17));
    a=class_named(r,"hkMoppCode");NEED(member(r,a,"info",25,0,16));NEED(member(r,a,"data",22,4,32));
    return 1;
}
static int array(Reader *r,uint32_t object,uint32_t offset,uint32_t stride,uint32_t *data,uint32_t *count) {
    Section *s=&r->sections[3];
    VALID(object>=s->base&&(uint64_t)object+offset+12<=s->base+s->local);
    const uint8_t *p=span(r,(size_t)object+offset,12);NEED(p);*count=u32(p+4);*data=pointer(r,object+offset);
    VALID(*count<20000000u&&*count<=(u32(p+8)&0x3FFFFFFF));
    if(!*count){VALID(*data==NONE);return 1;}
    VALID(*data>=s->base&&(uint64_t)*data+(uint64_t)*count*stride<=s->base+s->local);
    NEED(span(r,*data,(size_t)*count*stride));return 1;
}
static const char *object_name(Reader *r,uint32_t object) {
    Section *s=&r->sections[3];
    for(uint32_t at=s->virtuals;s->end-at>=12;at+=12) {
        const uint8_t *p=r->data+s->base+at;
        uint32_t from=u32(p),section=u32(p+4),to=u32(p+8);
        if(from==NONE)continue;
        VALID(from<s->local&&section<5&&to<r->sections[section].local);
        if(s->base+from==object){
            const uint8_t *name=span(r,r->sections[section].base+to,1);NEED(name);
            VALID(memchr(name,0,r->sections[section].local-to));return (const char*)name;
        }
    }
    r->status=UC_COLLISION_UNSUPPORTED;return 0;
}
static int object_class(Reader *r,uint32_t object,const char *name){
    const char *actual=object_name(r,object);NEED(actual);SUPPORT(!strcmp(actual,name));return 1;
}
static int model_shape(Reader *r,UCModelCollisionShape *out) {
    NEED(packfile(r));SUPPORT(u32(r->data+24)==3&&u32(r->data+28)==0);
    const Section *s=&r->sections[3];uint32_t root=s->base,leaf=root;
    const uint8_t *p=span(r,root,16);NEED(p);VALID(s->local>=16);
    out->root_user_data=u32(p+8);
    for(unsigned i=0;i<4;i++)out->transform[i*5]=1;
    out->rotation[3]=1;
    const char *name=object_name(r,root);NEED(name);
    if(!strcmp(name,"hkTransformShape")){
        out->root_kind=25;
        NEED(object_class(r,root,"hkTransformShape"));VALID(s->local>=96);
        uint32_t c=class_named(r,"hkTransformShape");p=span(r,c,36);NEED(p);SUPPORT(u32(p+8)==96);
        NEED(member(r,c,"childShape",20,25,12));NEED(member(r,c,"rotation",13,0,16));NEED(member(r,c,"transform",18,0,32));
        memcpy(out->rotation,r->data+root+16,16);memcpy(out->transform,r->data+root+32,64);leaf=pointer(r,root+12);
    }
    VALID(leaf>=root&&(uint64_t)leaf+16<=root+s->local);name=object_name(r,leaf);NEED(name);uint32_t bytes=0;
    if(!strcmp(name,"hkBoxShape")){out->kind=1;bytes=32;}
    else if(!strcmp(name,"hkSphereShape")){out->kind=2;bytes=16;}
    else if(!strcmp(name,"hkCapsuleShape")){out->kind=3;bytes=48;}
    else SUPPORT(0);
    if(leaf==root){static const uint32_t kinds[]={0,7,4,8};out->root_kind=kinds[out->kind];}
    NEED(object_class(r,leaf,name));VALID((uint64_t)leaf+bytes<=root+s->local);
    uint32_t c=class_named(r,name);p=span(r,c,36);NEED(p);SUPPORT(u32(p+8)==bytes);
    if(out->kind==1)NEED(member(r,c,"halfExtents",12,0,16));
    if(out->kind==3){NEED(member(r,c,"vertexA",12,0,16));NEED(member(r,c,"vertexB",12,0,32));}
    c=class_named(r,"hkConvexShape");p=span(r,c,36);NEED(p);SUPPORT(u32(p+8)==16);NEED(member(r,c,"radius",11,0,12));
    out->child_user_data=u32(r->data+leaf+8);memcpy(&out->radius,r->data+leaf+12,4);
    if(bytes>16)memcpy(out->dimensions,r->data+leaf+16,bytes-16);
    VALID(isfinite(out->radius)&&out->radius>=0);
    for(unsigned i=0;i<16;i++)VALID(isfinite(out->transform[i]));
    for(unsigned i=0;i<4;i++)VALID(isfinite(out->rotation[i]));
    for(unsigned i=0;i<8;i++)VALID(isfinite(out->dimensions[i]));
    if(out->kind==1)for(unsigned i=0;i<3;i++)VALID(out->dimensions[i]>=0);
    return 1;
}

static int model_record(Reader *r,UCModelCollisionShape *out){
    uint32_t bytes;NEED(number(r,&bytes));const uint8_t *p=span(r,r->pos,bytes);NEED(p);
    Reader sub={0};sub.data=p;sub.size=bytes;
    int ok=model_shape(&sub,out);free(sub.fixups);
    if(!ok){r->status=sub.status;return 0;}
    r->pos+=bytes;
    /*2A7E60 rounds the FILE cursor after each packfile to four bytes. */
    size_t pad=(4-r->pos%4)%4;NEED(span(r,r->pos,pad));r->pos+=pad;
    static const uint32_t suffix[]={36,12,6};out->metadata_size=suffix[out->family];
    p=span(r,r->pos,out->metadata_size);NEED(p);memcpy(out->metadata,p,out->metadata_size);r->pos+=out->metadata_size;
    return 1;
}
static int rag_class(Reader*r,const char*name,uint32_t size,uint32_t*out){
    uint32_t c=class_named(r,name);const uint8_t*p=span(r,c,36);NEED(p);SUPPORT(u32(p+8)==size);*out=c;return 1;
}
static int rag_basis(Reader*r,uint32_t owner,const char*name,const char*const*fields,unsigned count){
    uint32_t members=pointer(r,owner+24),n=u32(r->data+owner+28);NEED(span(r,members,(size_t)n*20));
    for(uint32_t i=0;i<n;i++)if(named(r,pointer(r,members+i*20),name)){
        uint32_t c=pointer(r,members+i*20+4);const uint8_t*p=span(r,c,36);NEED(p);SUPPORT(u32(p+8)==count*16);
        for(unsigned j=0;j<count;j++)NEED(member(r,c,fields[j],12,0,j*16));return 1;
    }SUPPORT(0);
}
static int rag_schema(Reader*r){
    uint32_t c;
    NEED(rag_class(r,"hkPhysicsSystem",64,&c));
    NEED(member(r,c,"rigidBodies",22,20,8));NEED(member(r,c,"constraints",22,20,20));
    NEED(member(r,c,"actions",22,20,32));NEED(member(r,c,"phantoms",22,20,44));
    NEED(rag_class(r,"hkRigidBody",196,&c));NEED(rag_class(r,"hkWorldObject",80,&c));
    NEED(member(r,c,"name",20,2,16));NEED(member(r,c,"collidable",25,0,20));
    NEED(rag_class(r,"hkCdBody",16,&c));NEED(member(r,c,"shape",20,25,0));
    NEED(rag_class(r,"hkEntity",196,&c));NEED(member(r,c,"motion",20,25,80));NEED(member(r,c,"material",25,0,88));
    NEED(rag_class(r,"hkBoxMotion",256,&c));NEED(member(r,c,"inertiaAndMassInv",12,0,240));
    NEED(rag_class(r,"hkRigidMotion",240,&c));NEED(member(r,c,"motionState",25,0,16));NEED(member(r,c,"massInv",11,0,192));
    NEED(member(r,c,"linearDamping",11,0,200));NEED(member(r,c,"angularDamping",11,0,204));
    NEED(member(r,c,"linearVelocity",12,0,208));NEED(member(r,c,"angularVelocity",12,0,224));
    NEED(rag_class(r,"hkMotionState",176,&c));NEED(member(r,c,"transform",18,0,0));
    NEED(member(r,c,"sweptTransform",25,0,64));
    NEED(member(r,c,"maxLinearVelocity",11,0,164));NEED(member(r,c,"maxAngularVelocity",11,0,168));
    NEED(rag_class(r,"hkSweptTransform",80,&c));NEED(member(r,c,"centerOfMassLocal",12,0,64));
    NEED(rag_class(r,"hkMaterial",12,&c));NEED(member(r,c,"friction",11,0,4));NEED(member(r,c,"restitution",11,0,8));
    NEED(rag_class(r,"hkCapsuleShape",48,&c));NEED(member(r,c,"vertexA",12,0,16));NEED(member(r,c,"vertexB",12,0,32));
    NEED(rag_class(r,"hkConvexShape",16,&c));NEED(member(r,c,"radius",11,0,12));
    NEED(rag_class(r,"hkConstraintInstance",40,&c));NEED(member(r,c,"name",20,2,28));NEED(member(r,c,"data",20,25,12));
    /* entities is the sole fixed-size array member; member() intentionally
     * admits only scalar fields for the other collision asset readers. */
    uint32_t members=pointer(r,c+24),count=u32(r->data+c+28);int found=0;
    NEED(span(r,members,(size_t)count*20));
    for(uint32_t i=0;i<count;i++)if(named(r,pointer(r,members+i*20),"entities")){
        const uint8_t*m=r->data+members+i*20;SUPPORT(m[12]==20&&m[13]==25&&u16(m+14)==2&&u16(m+18)==16);found=1;
    }SUPPORT(found);
    NEED(rag_class(r,"hkRagdollConstraintData",144,&c));NEED(member(r,c,"anglesAreNotStoredInHavok30Format",1,0,12));
    NEED(member(r,c,"basisA",25,0,16));NEED(member(r,c,"basisB",25,0,64));
    const char*rag_fields[]={"pivot","planeAxis","twistAxis"};
    NEED(rag_basis(r,c,"basisA",rag_fields,3));NEED(rag_basis(r,c,"basisB",rag_fields,3));
    const char*limits[]={"coneMinAngle","planeMinAngle","planeMaxAngle","twistMinAngle","twistMaxAngle","maxFrictionTorque"};
    for(unsigned i=0;i<6;i++)NEED(member(r,c,limits[i],11,0,112+i*4));
    NEED(rag_class(r,"hkLimitedHingeConstraintData",144,&c));NEED(member(r,c,"basisA",25,0,32));NEED(member(r,c,"basisB",25,0,96));
    const char*hinge_a[]={"pivot","axle","perpToAxle1","perpToAxle2"},*hinge_b[]={"pivot","axle","perp2FreeAxis"};
    NEED(rag_basis(r,c,"basisA",hinge_a,4));NEED(rag_basis(r,c,"basisB",hinge_b,3));
    NEED(member(r,c,"minAngle",11,0,12));NEED(member(r,c,"maxAngle",11,0,16));NEED(member(r,c,"maxFrictionTorque",11,0,20));
    return 1;
}
static int rag_object(Reader*r,uint32_t at,uint32_t size,const char*type){
    Section*s=&r->sections[3];VALID(at>=s->base&&(uint64_t)at+size<=s->base+s->local);return object_class(r,at,type);
}
static int rag_name(Reader*r,uint32_t at,char out[64]){
    Section*s=&r->sections[3];VALID(at>=s->base&&at<s->base+s->local);
    size_t n=0;while(n<64&&at+n<s->base+s->local&&r->data[at+n])n++;
    VALID(n&&n<64&&at+n<s->base+s->local);memcpy(out,r->data+at,n+1);
    for(size_t i=0;i<n;i++)VALID((unsigned char)out[i]>=32&&(unsigned char)out[i]<127);
    return 1;
}
static int rag_floats(Reader*r,uint32_t at,float*out,uint32_t count){
    const uint8_t*p=span(r,at,(size_t)count*4);NEED(p);memcpy(out,p,(size_t)count*4);
    for(uint32_t i=0;i<count;i++)VALID(isfinite(out[i]));return 1;
}
static int ragdoll(Reader*r,UCRagdollAsset*out){
    NEED(packfile(r));SUPPORT(u32(r->data+24)==3&&u32(r->data+28)==0);NEED(rag_schema(r));
    uint32_t root=r->sections[3].base,bodies,joints,unused,n;
    NEED(rag_object(r,root,64,"hkPhysicsSystem"));
    NEED(array(r,root,8,4,&bodies,&out->body_count));NEED(array(r,root,20,4,&joints,&out->joint_count));
    SUPPORT(out->body_count&&out->body_count<=64&&out->joint_count==out->body_count-1);
    NEED(array(r,root,32,4,&unused,&n));SUPPORT(!n);NEED(array(r,root,44,4,&unused,&n));SUPPORT(!n);
    NEED(alloc(r,(void**)&out->bodies,out->body_count,sizeof(*out->bodies)));
    NEED(alloc(r,(void**)&out->joints,out->joint_count,sizeof(*out->joints)));
    memset(out->bodies,0,out->body_count*sizeof(*out->bodies));if(out->joint_count)memset(out->joints,0,out->joint_count*sizeof(*out->joints));
    uint32_t addresses[64],parents[64];
    for(uint32_t i=0;i<out->body_count;i++){
        uint32_t a=addresses[i]=pointer(r,bodies+i*4);parents[i]=i;UCRagdollBody*b=out->bodies+i;
        NEED(rag_object(r,a,196,"hkRigidBody"));NEED(rag_name(r,pointer(r,a+16),b->name));
        for(uint32_t j=0;j<i;j++)VALID(a!=addresses[j]&&strcmp(b->name,out->bodies[j].name));
        uint32_t m=pointer(r,a+80),s=pointer(r,a+20);NEED(rag_object(r,m,256,"hkBoxMotion"));NEED(rag_object(r,s,48,"hkCapsuleShape"));
        NEED(rag_floats(r,m+16,b->transform,16));NEED(rag_floats(r,s+16,b->capsule_a,4));NEED(rag_floats(r,s+32,b->capsule_b,4));
        NEED(rag_floats(r,s+12,&b->radius,1));NEED(rag_floats(r,m+240,b->inertia_and_mass_inv,4));
        NEED(rag_floats(r,m+144,b->center_of_mass,4));
        NEED(rag_floats(r,m+192,&b->mass_inv,1));NEED(rag_floats(r,m+200,&b->linear_damping,1));NEED(rag_floats(r,m+204,&b->angular_damping,1));
        NEED(rag_floats(r,m+180,&b->max_linear_velocity,1));NEED(rag_floats(r,m+184,&b->max_angular_velocity,1));
        NEED(rag_floats(r,m+208,b->linear_velocity,4));NEED(rag_floats(r,m+224,b->angular_velocity,4));
        NEED(rag_floats(r,a+92,&b->friction,1));NEED(rag_floats(r,a+96,&b->restitution,1));
        VALID(b->radius>0&&b->mass_inv>0&&b->linear_damping>=0&&b->angular_damping>=0&&b->max_linear_velocity>0&&b->max_angular_velocity>0&&b->friction>=0&&b->restitution>=0);
        VALID(b->transform[3]==0&&b->transform[7]==0&&b->transform[11]==0&&b->transform[15]==1);
        for(unsigned j=0;j<4;j++)VALID(b->inertia_and_mass_inv[j]>0);
    }
    for(uint32_t i=0;i<out->joint_count;i++){
        uint32_t a=pointer(r,joints+i*4);UCRagdollJoint*j=out->joints+i;NEED(rag_object(r,a,40,"hkConstraintInstance"));
        NEED(rag_name(r,pointer(r,a+28),j->name));
        for(unsigned k=0;k<2;k++){uint32_t b=pointer(r,a+16+k*4);j->body[k]=NONE;for(uint32_t l=0;l<out->body_count;l++)if(addresses[l]==b)j->body[k]=l;VALID(j->body[k]!=NONE);}
        uint32_t aa=j->body[0],bb=j->body[1];while(parents[aa]!=aa)aa=parents[aa];while(parents[bb]!=bb)bb=parents[bb];VALID(aa!=bb);parents[aa]=bb;
        uint32_t d=pointer(r,a+12);const char*type=object_name(r,d);NEED(type);
        if(!strcmp(type,"hkRagdollConstraintData")){
            j->kind=1;NEED(rag_object(r,d,144,type));SUPPORT(r->data[d+12]==1);
            NEED(rag_floats(r,d+16,j->basis[0],12));NEED(rag_floats(r,d+64,j->basis[1],12));
            NEED(rag_floats(r,d+112,j->limits,5));NEED(rag_floats(r,d+132,&j->max_friction_torque,1));
            VALID(j->limits[0]>=0&&j->limits[0]<=3.141593f&&j->limits[1]<=j->limits[2]&&j->limits[3]<=j->limits[4]);
        }else if(!strcmp(type,"hkLimitedHingeConstraintData")){
            j->kind=2;NEED(rag_object(r,d,144,type));NEED(rag_floats(r,d+32,j->basis[0],16));NEED(rag_floats(r,d+96,j->basis[1],12));
            NEED(rag_floats(r,d+12,j->limits,2));NEED(rag_floats(r,d+20,&j->max_friction_torque,1));VALID(j->limits[0]<=j->limits[1]);
        }else SUPPORT(0);
        VALID(j->max_friction_torque>=0);
    }
    return 1;
}
void uc_ragdoll_asset_free(UCRagdollAsset*out){if(out){free(out->bodies);free(out->joints);memset(out,0,sizeof(*out));}}
UCCollisionStatus uc_ragdoll_asset_load(const void*data,size_t size,UCRagdollAsset*out){
    if(!data||!out||out->bodies||out->joints||out->body_count||out->joint_count)return UC_COLLISION_ARGUMENT;
    if(size>LIMIT)return UC_COLLISION_LIMIT;
    if(size<4||u32(data)!=size-4)return UC_COLLISION_FORMAT;
    Reader r={0};r.data=(const uint8_t*)data+4;r.size=size-4;UCRagdollAsset result={0};
    int ok=ragdoll(&r,&result);free(r.fixups);
    if(!ok){uc_ragdoll_asset_free(&result);return r.status?r.status:UC_COLLISION_FORMAT;}
    *out=result;return UC_COLLISION_OK;
}
void uc_model_collision_free(UCModelCollisionAsset *out){
    if(!out)return;free(out->shapes);free(out->group_object_counts);memset(out,0,sizeof(*out));
}
UCCollisionStatus uc_model_collision_load(const void *data,size_t size,int is_textured,UCModelCollisionAsset *out){
    if(!data||!out||(is_textured!=0&&is_textured!=1)||out->shapes||out->group_object_counts||out->shape_count||out->group_count)return UC_COLLISION_ARGUMENT;
    if(size>LIMIT)return UC_COLLISION_LIMIT;
    Reader r={0};r.data=data;r.size=size;UCModelCollisionAsset result={0};uint32_t capacity=0;
    if(!number(&r,&result.group_count))goto fail;
    if(result.group_count>100000){r.status=UC_COLLISION_LIMIT;goto fail;}
    if(!alloc(&r,(void**)&result.group_object_counts,result.group_count,4))goto fail;
    for(uint32_t g=0;g<result.group_count;g++){
        uint32_t objects;if(!number(&r,&objects))goto fail;result.group_object_counts[g]=objects;
        if(objects>100000){r.status=UC_COLLISION_LIMIT;goto fail;}
        for(uint32_t o=0;o<objects;o++)for(uint32_t family=0;family<3;family++){
            uint32_t flag,count;if(!number(&r,&flag))goto fail;
            if(flag>1){r.status=UC_COLLISION_UNSUPPORTED;goto fail;}if(!flag)continue;
            if(!number(&r,&count))goto fail;
            if(count>100000-result.shape_count){r.status=UC_COLLISION_LIMIT;goto fail;}
            if(count>capacity-result.shape_count){
                capacity=result.shape_count+count;
                if(capacity>(LIMIT-r.allocated)/sizeof(*result.shapes)){r.status=UC_COLLISION_LIMIT;goto fail;}
                void *next=realloc(result.shapes,(size_t)capacity*sizeof(*result.shapes));
                if(!next){r.status=UC_COLLISION_MEMORY;goto fail;}result.shapes=next;
            }
            for(uint32_t i=0;i<count;i++){
                UCModelCollisionShape *shape=&result.shapes[result.shape_count++];memset(shape,0,sizeof(*shape));
                shape->group=g;shape->object=o;shape->family=family;
                if(!model_record(&r,shape))goto fail;
            }
        }
        if(!is_textured){uint32_t extra;if(!number(&r,&extra))goto fail;if(extra){r.status=UC_COLLISION_UNSUPPORTED;goto fail;}}
    }
    /* The .mtxthk exporter retains a zero final world-shape marker even though
     *22861A skips that field for this extension. Accept only that known trailer. */
    if(is_textured&&size-r.pos==4){uint32_t unused;if(!number(&r,&unused))goto fail;if(unused){r.status=UC_COLLISION_UNSUPPORTED;goto fail;}}
    if(r.pos!=size){r.status=UC_COLLISION_FORMAT;goto fail;}
    *out=result;return UC_COLLISION_OK;
fail:
    uc_model_collision_free(&result);return r.status?r.status:UC_COLLISION_FORMAT;
}
static int collision(Reader *r,UCCollisionAsset *out) {
    NEED(packfile(r));NEED(schema(r));
    uint32_t root=r->sections[3].base,mesh=pointer(r,root+12),code=pointer(r,root+16);
    SUPPORT(u32(r->data+24)==3&&u32(r->data+28)==0);
    NEED(object_class(r,root,"hkMoppBvTreeShape"));NEED(object_class(r,mesh,"hkStorageMeshShape"));NEED(object_class(r,code,"hkMoppCode"));
    NEED(span(r,root,20));NEED(span(r,mesh,80));NEED(span(r,code,48));
    out->shape_user_data=u32(r->data+root+8);out->mesh_user_data=u32(r->data+mesh+8);
    out->disable_welding=r->data[mesh+12];memcpy(out->scaling,r->data+mesh+16,16);memcpy(&out->radius,r->data+mesh+48,4);
    out->subpart_index_bits=u32(r->data+mesh+32);VALID(out->subpart_index_bits>0&&out->subpart_index_bits<32);
    for(unsigned i=0;i<4;i++)VALID(isfinite(out->scaling[i]));VALID(isfinite(out->radius)&&out->radius>=0);
    uint32_t parts,storage,nstorage;NEED(array(r,mesh,36,48,&parts,&out->subpart_count));NEED(array(r,mesh,64,4,&storage,&nstorage));
    SUPPORT(out->subpart_count==nstorage);VALID(nstorage<100000);
    NEED(alloc(r,(void**)&out->subparts,nstorage,sizeof(*out->subparts)));
    for(unsigned pass=0;pass<2;pass++) {
        uint32_t vertex=0,triangle=0;
        for(uint32_t i=0;i<nstorage;i++) {
            const uint8_t *part=r->data+parts+i*48;uint32_t obj=pointer(r,storage+i*4),arrays[6],counts[6];
            NEED(span(r,obj,72));
            const unsigned strides[]={4,2,4,1,4,2};
            for(unsigned j=0;j<6;j++)NEED(array(r,obj,j*12,strides[j],&arrays[j],&counts[j]));
            uint32_t nv=u32(part+8),nt=u32(part+24);
            SUPPORT(u32(part+4)==12&&part[16]==1&&part[17]==2&&u32(part+20)==6&&u32(part+32)==2&&u32(part+40)==4);
            VALID(nv<=1000000-vertex&&nt<=2000000-triangle&&counts[0]==nv*3&&counts[1]==nt*3);
            SUPPORT(!counts[2]&&!counts[3]);VALID(counts[4]==u32(part+44)&&counts[5]==nt);
            if(!pass)out->subparts[i]=(UCCollisionSubpart){vertex,nv,triangle,nt};
            else {
                if(nv)memcpy(out->vertices+vertex,r->data+arrays[0],(size_t)nv*12);
                for(uint32_t v=0;v<nv;v++)for(unsigned a=0;a<3;a++)VALID(isfinite(out->vertices[vertex+v][a]));
                for(uint32_t t=0;t<nt;t++) {
                    UCCollisionTriangle *tri=&out->triangles[triangle+t];
                    for(unsigned a=0;a<3;a++){uint32_t ix=u16(r->data+arrays[1]+t*6+a*2);VALID(ix<nv);tri->vertex[a]=vertex+ix;}
                    tri->material_index=u16(r->data+arrays[5]+t*2);VALID(tri->material_index<counts[4]);
                    tri->material=u32(r->data+arrays[4]+tri->material_index*4);tri->subpart=i;tri->local_triangle=t;
                }
            }
            vertex+=nv;triangle+=nt;
        }
        if(!pass){out->vertex_count=vertex;out->triangle_count=triangle;NEED(alloc(r,(void**)&out->vertices,vertex,12));NEED(alloc(r,(void**)&out->triangles,triangle,sizeof(*out->triangles)));}
    }
    uint32_t bytes;NEED(array(r,code,32,1,&bytes,&out->mopp_size));
    memcpy(out->mopp_offset,r->data+code+16,16);for(unsigned i=0;i<4;i++)VALID(isfinite(out->mopp_offset[i]));
    NEED(alloc(r,(void**)&out->mopp_bytes,out->mopp_size,1));if(out->mopp_size)memcpy(out->mopp_bytes,r->data+bytes,out->mopp_size);
    return 1;
}
void uc_collision_free(UCCollisionAsset *a) {
    if(!a)return;free(a->vertices);free(a->triangles);free(a->subparts);free(a->sector_object_counts);free(a->mopp_bytes);free(a->query_bounds);memset(a,0,sizeof(*a));
}
UCCollisionStatus uc_collision_load(const void *data,size_t size,UCCollisionAsset *out) {
    if(!data||!out)return UC_COLLISION_ARGUMENT;
    if(out->vertices||out->triangles||out->subparts||out->sector_object_counts||out->mopp_bytes||out->query_bounds)return UC_COLLISION_ARGUMENT;
    if(size>LIMIT)return UC_COLLISION_LIMIT;
    Reader r={0};r.data=data;r.size=size;uint32_t groups,count,flag,bytes;
    if(!number(&r,&groups))goto fail;
    if(groups>100000){r.status=UC_COLLISION_LIMIT;goto fail;}
    out->sector_count=groups;
    if(!alloc(&r,(void**)&out->sector_object_counts,groups,4))goto fail;
    for(uint32_t i=0;i<groups;i++) {
        if(!number(&r,&count))goto fail;out->sector_object_counts[i]=count;
        if(count>100000){r.status=UC_COLLISION_LIMIT;goto fail;}
        for(uint32_t j=0;j<count;j++)for(unsigned k=0;k<3;k++) {
            if(!number(&r,&flag))goto fail;
            if(flag){r.status=UC_COLLISION_UNSUPPORTED;goto fail;}
        }
    }
    if(!number(&r,&flag))goto fail;
    if(!flag){if(r.pos==size)return UC_COLLISION_OK;r.status=UC_COLLISION_FORMAT;goto fail;}
    if(flag!=1){r.status=UC_COLLISION_UNSUPPORTED;goto fail;}
    if(!number(&r,&bytes))goto fail;
    if(bytes!=size-r.pos){r.status=UC_COLLISION_FORMAT;goto fail;}
    r.data+=r.pos;r.size=bytes;r.pos=0;
    if(!collision(&r,out))goto fail;
    if(out->triangle_count){
        if(!alloc(&r,(void**)&out->query_bounds,(out->triangle_count+31)/32,sizeof(*out->query_bounds)))goto fail;
        memcpy(out->query_scaling,out->scaling,sizeof out->scaling);
        for(uint32_t i=0;i<out->triangle_count;i++)for(unsigned j=0;j<3;j++)for(unsigned k=0;k<3;k++){
            double v=(double)out->vertices[out->triangles[i].vertex[j]][k]*out->scaling[k];
            double*b=out->query_bounds[i/32];
            if(i%32==0&&j==0)b[k]=b[k+3]=v;
            else{if(v<b[k])b[k]=v;if(v>b[k+3])b[k+3]=v;}
        }
    }
    free(r.fixups);return UC_COLLISION_OK;
fail:
    free(r.fixups);uc_collision_free(out);return r.status?r.status:UC_COLLISION_FORMAT;
}
const char *uc_collision_status(UCCollisionStatus s) {
    const char *names[]={"ok","argument","format","unsupported","limit","memory"};return (unsigned)s<6?names[s]:"unknown";
}
