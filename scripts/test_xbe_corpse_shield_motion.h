// Included only by the offline fixture, never by the game runtime.
namespace corpse_bridge_test {
using corpse_shield::Matrix;
constexpr uint32_t world=0x601000;
constexpr uint32_t Owner(unsigned slot){return corpse_shield_test::owners[slot];}
constexpr uint32_t Render(unsigned index){return 0x620000+index*0x1000;}
constexpr uint32_t Resource(unsigned kind){return 0x630000+kind*0x1000;}
constexpr uint32_t Locals(unsigned index){return 0x640000+index*0x10000;}
constexpr uint32_t Authored(unsigned kind){return 0x680000+kind*0x3000;}
constexpr uint32_t Records(unsigned kind){return 0x690000+kind*0x5000;}
uint8_t* B(){return kernel::memory.data();}
void U(uint32_t p,uint32_t value){std::memcpy(B()+p,&value,4);}
void M(uint32_t p,const Matrix& m){std::memcpy(B()+p,m.data(),64);}
Matrix Identity(){Matrix m{};m[0]=m[5]=m[10]=m[15]=1;return m;}
Matrix Read(uint32_t p){Matrix m;std::memcpy(m.data(),B()+p,64);return m;}
Matrix Rotation(float angle){Matrix m=Identity();m[0]=m[10]=std::cos(angle);m[2]=-std::sin(angle);m[8]=std::sin(angle);return m;}
void Close(const Matrix& a,const Matrix& b,float tolerance=.0003f){for(unsigned i=0;i<16;++i)assert(std::abs(a[i]-b[i])<tolerance);}
Matrix NativeRest(unsigned i){Matrix p=Rotation(float(i)*.03f);p[12]=i*.2f;p[13]=i*.9f;p[14]=i*.1f;return p;}
void Seed(){
    using namespace corpse_shield;
    kernel::memory.fill(0);TrialCorpseShieldReset();held={};U(0x51c4dc,world);corpse_shield_test::enabled=true;
    Matrix anchor=Rotation(.4f);anchor[12]=5;anchor[14]=20;
    for(unsigned kind=0;kind<2;++kind){
        const unsigned start=first_joint[kind];const uint32_t resource=Resource(kind);
        U(resource,0x457008);U(resource+0xc,counts[kind]);U(resource+0x3c,Authored(kind));U(resource+0x40,Records(kind));
        std::memcpy(B()+resource+0x10,tags[kind],std::strlen(tags[kind])+1);
        for(unsigned i=0;i<counts[kind];++i){M(Authored(kind)+i*64,i==start-1?anchor:Identity());}
        for(unsigned i=0;i<15;++i){
            Matrix rest=Identity();const auto p=NativeRest(joints[i]);
            for(unsigned c=0;c<3;++c)rest[12+c]=p[12+c]+(c==1?8.f:3.f);
            Matrix bind=Product(rest,anchor),inverse_bind;assert(Inverse(bind,inverse_bind));
            M(Records(kind)+(start+i)*160+96,inverse_bind);
            const uint16_t parent=i?uint16_t(start):uint16_t(start-1);
            std::memcpy(B()+Records(kind)+(start+i)*160+0x16,&parent,2);
            if(i){const auto root=NativeRest(joints[0]);for(unsigned c=0;c<3;++c)rest[12+c]-=root[12+c]+(c==1?8.f:3.f);}
            M(Authored(kind)+(start+i)*64,rest);
        }
    }
    for(unsigned slot=0;slot<2;++slot){
        const uint32_t owner=Owner(slot),child=0x600200+slot*0x200;
        U(owner,0x46fbe8);U(owner+0x54,slot+100);U(owner+0x368,slot);
        U(corpse_shield_test::bodies[slot]+0x54,200+slot);U(child+0x54,300+slot);U(child+0x58,200+slot);
        for(unsigned kind=0;kind<2;++kind){
            const unsigned index=slot*2+kind;const uint32_t render=Render(index),wrapper=render+0x200,entity=kind?child:owner;
            U(entity+0x74,wrapper);U(wrapper,render);U(render,0x457880);U(render+0x68,Resource(kind));U(render+0x6c,Locals(index));
            Matrix w=Rotation(-.7f);w[12]=300+slot*100;w[13]=40;w[14]=-250;M(render+0x20,w);if(!kind)M(owner+0x6f0,w);
            std::memcpy(B()+Locals(index),B()+Authored(kind),counts[kind]*64);
            ready[index]={world,owner,slot+100,entity,kind?slot+300:slot+100,wrapper,render,Resource(kind)};
        }
        auto& h=held[slot];h.owner=owner;h.id=slot+100;h.physics.ragdoll=slot+1;h.posed=true;
        for(unsigned i=0;i<73;++i)h.pose[i]=h.rest[i]=NativeRest(i);
    }
}
}
void BridgeCases(){
    using namespace corpse_bridge_test;using namespace corpse_shield;
    Seed();
    for(unsigned slot=0;slot<2;++slot){
        assert(TrialCorpseShieldActive(B(),Owner(slot)));
        Matrix map,target,anchor,render;std::array<Matrix,15> rest;
        assert(Mapping(B(),ready[slot*2],0,held[slot],map,rest));
        // A new equipment transform must win over a stale render copy.
        auto current=Read(Owner(slot)+0x6f0);current[12]+=50;M(Owner(slot)+0x6f0,current);
        assert(Target(B(),ready[slot*2],held[slot],target));
        assert(Anchor(B(),ready[slot*2],0,anchor));render=Read(Owner(slot)+0x6f0);
        const auto display=Product(Product(map,anchor),render);
        const auto visual_chest=Product(held[slot].rest[5],display),physical_chest=Product(held[slot].rest[5],target);
        for(unsigned c=12;c<15;++c)assert(std::abs(visual_chest[c]-physical_chest[c])<.001f);
        for(unsigned row=0;row<3;++row){float length=0;for(unsigned c=0;c<3;++c)length+=target[row*4+c]*target[row*4+c];assert(std::abs(length-1)<.0001f);}
        for(unsigned kind=0;kind<2;++kind){const unsigned index=slot*2+kind;
            TrialCorpseShieldPose(B(),Render(index));
            for(unsigned i=0;i<counts[kind];++i)Close(Read(Locals(index)+i*64),Read(Authored(kind)+i*64));
        }
    }
    // Head and all eleven other physical bones each independently reach both
    // equipment views. A translation is scaled exactly, not a spring response.
    for(unsigned slot=0;slot<2;++slot)for(unsigned j=0;j<15;++j){
        Seed();held[slot].pose[joints[j]][12]+=10;
        for(unsigned kind=0;kind<2;++kind){const unsigned index=slot*2+kind,start=first_joint[kind];
            TrialCorpseShieldPose(B(),Render(index));
            Matrix anchor;assert(Anchor(B(),ready[index],kind,anchor));
            std::array<Matrix,15> composed;
            for(unsigned i=0;i<15;++i){composed[i]=Read(Locals(index)+(start+i)*64);
                composed[i]=Product(composed[i],i?composed[0]:anchor);
            }
            // Verify a point on the selected joint moved by the unscaled native displacement,
            // while a second player's pose and original holding arm stay intact.
            Matrix inverse_anchor;assert(Inverse(anchor,inverse_anchor));
            const auto relative=Product(composed[j],inverse_anchor);
            std::array<Matrix,15> rest;Matrix map;assert(Mapping(B(),ready[index],kind,held[slot],map,rest));
            for(unsigned c=0;c<3;++c)assert(std::abs(relative[12+c]-rest[j][12+c]-10*map[c])<.001f);
            for(unsigned i=0;i<start;++i)Close(Read(Locals(index)+i*64),Read(Authored(kind)+i*64));
            TrialCorpseShieldPose(B(),Render((1-slot)*2+kind));
            for(unsigned i=0;i<counts[kind];++i)Close(Read(Locals((1-slot)*2+kind)+i*64),Read(Authored(kind)+i*64));
            const auto previous=std::vector<uint8_t>(B()+Locals(index),B()+Locals(index)+counts[kind]*64);
            TrialCorpseShieldPose(B(),Render(index));assert(!std::memcmp(previous.data(),B()+Locals(index),previous.size()));
        }
    }
    // Native B9830 must apply the cache again after its animation update and
    // before hierarchy composition, without advancing the shared-world solver.
    Seed();held[0].pose[8][12]+=10;
    U(Render(0)+8,0x457830);U(0x457830+0x28,0xf0010);U(Render(0)+0x70,0x6f0000);B()[Resource(0)+0x49]=1;
    corpse_shield_test::pose_render=Render(0);corpse_shield_test::pose_compositions=0;
    corpse_shield_test::pose_animation_updates=0;corpse_shield_test::pose_require_motion=true;
    X86Context c{};c.ecx=Render(0)+8;c.esp=0x710000;U(c.esp,0x123456);U(c.esp+4,0);U(c.esp+8,0);
    NativePose(c,B());assert(c.esp==0x71000c&&corpse_shield_test::pose_compositions==1&&corpse_shield_test::pose_animation_updates==1);
    for(unsigned bad=0;bad<6;++bad){Seed();held[0].pose[8][12]+=10;
        if(bad==0)held[0].posed=false;
        if(bad==1)held[0].physics.ragdoll=0;
        if(bad==2)held[0].id=999;
        if(bad==3)U(Owner(0)+0x54,999);
        if(bad==4)U(Render(0)+0x68,0);
        if(bad==5)held[0].pose[8][0]=std::nanf("");
        TrialCorpseShieldPose(B(),Render(0));
        for(unsigned i=0;i<121;++i)Close(Read(Locals(0)+i*64),Read(Authored(0)+i*64));
        if(bad<5)assert(!TrialCorpseShieldActive(B(),Owner(0)));
    }
    Seed();corpse_shield_test::enabled=false;assert(!TrialCorpseShieldActive(B(),Owner(0)));
    std::puts("Native corpse pose retarget, chest scale, both-player/FP/TP isolation and animation overwrite checks passed");
}
