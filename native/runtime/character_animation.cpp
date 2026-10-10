#include "character_animation.h"
#include <stdexcept>
#include <algorithm>
#include <string>
#include <cmath>
#include <cstring>
#include "../euler_rotation.h"
#include "../character_pose.h"
#include "../character_profiles.h"
#include "../character_aim.h"
#include "../point_light_receiver.h"
#include "../brute_movement.h"

static void require(bool ok,const char *what){if(!ok)throw std::runtime_error(what);}
NativeCharacterClipGraph::NativeCharacterClipGraph(const NativeFirstpersonClips &bank,
    const char *prefix,int32_t root_bone,const UCCharacterLocomotionPlan &plan,const float *brute_blend){
    static const char *actions[]={"Run","Walk","Backwalk","TurnLeft90","TurnLeft180",
        "TurnRight90","TurnRight180","StartRun","PushOff90Left","PushOff180Left",
        "PushOff90Right","PushOff180Right"};
    require(plan.base_action<3&&plan.turn_action<=12&&root_bone>=0&&
        (uint32_t)root_bone<bank.skeleton.bone_count,"Character locomotion graph domain");
    sequence=plan.turn_action!=12;turn_scale_=plan.turn_scale;
    if(sequence){
        //2A95B0 returns null when the requested turn/start action is absent.
        //2AF0B4 then takes2AF16C's base-only loop (Cop has no StartRun).
        uint32_t index=UINT32_MAX;
        require(!uc_animation_template_find_action(&bank.catalog,prefix,actions[plan.turn_action],&index),"Character optional initial action lookup");
        sequence=index!=UINT32_MAX;
    }
    for(unsigned i=0;i<(sequence?2u:1u);i++){
        unsigned action=sequence&&!i?plan.turn_action:plan.base_action;
        load_leaf(bank,prefix,root_bone,actions[action],i);
    }
    if(brute_blend&&plan.base_action==0){
        brute_blend_=brute_blend;unsigned base=sequence?1:0;
        //The factory creates three independent leaves, even though two share Run.
        uc_animation_sample_free(&leaves_[base].sample);leaves_[base].sample={};roots_[base]=0;
        load_leaf(bank,prefix,root_bone,"RunBatUp",base);
        load_leaf(bank,prefix,root_bone,"Run",base+1);load_leaf(bank,prefix,root_bone,"Run",base+2);
    }
}
NativeCharacterClipGraph::NativeCharacterClipGraph(const NativeFirstpersonClips &bank,
    const char *prefix,int32_t root_bone,const char *action){load_leaf(bank,prefix,root_bone,action,0);}
NativeCharacterClipGraph::NativeCharacterClipGraph(const NativeFirstpersonClips &bank,
    const char *prefix,int32_t root_bone,const char *base,const char *first){
    sequence=first!=nullptr;if(sequence)load_leaf(bank,prefix,root_bone,first,0);
    load_leaf(bank,prefix,root_bone,base,sequence?1:0);
}
NativeCharacterClipGraph::NativeCharacterClipGraph(std::shared_ptr<const NativeFirstpersonClip> clip,uint32_t bones,
    int32_t root,std::shared_ptr<UCCharacterClipCompletion> completion):direct_clip_(std::move(clip)),loop_completion_(std::move(completion)){
    require(direct_clip_&&loop_completion_,"Character direct loop owners");load_leaf(*direct_clip_,bones,root,0);
}
NativeCharacterClipGraph::NativeCharacterClipGraph(std::shared_ptr<const NativeFirstpersonClip> clip,uint32_t bones,
    int32_t root,bool hold):one_shot_(true),hold_last_(hold),direct_clip_(std::move(clip)){
    require(direct_clip_!=nullptr,"Character direct action owner");load_leaf(*direct_clip_,bones,root,0);
}
NativeCharacterClipGraph::NativeCharacterClipGraph(const NativeFirstpersonClips &bank,
    const char *prefix,int32_t root_bone,const char *action,float previous_phase){
    require(std::isfinite(previous_phase)&&previous_phase>=0&&previous_phase<1,"Character initial leaf phase");
    load_leaf(bank,prefix,root_bone,action,0);loop_.leaf_phase=previous_phase;
}
NativeCharacterClipGraph::NativeCharacterClipGraph(const NativeFirstpersonClips&bank,const char*prefix,
    int32_t root,const char*action,float scale,bool one_shot):turn_scale_(scale),one_shot_(one_shot){
    require(one_shot&&std::isfinite(scale),"Character one-shot root scale");load_leaf(bank,prefix,root,action,0);
}
void NativeCharacterClipGraph::load_leaf(const NativeFirstpersonClips &bank,const char *prefix,
    int32_t root_bone,const char *action,unsigned i){
    require(prefix&&action&&root_bone>=0&&(uint32_t)root_bone<bank.skeleton.bone_count,"Character action domain");
    uint32_t index=UINT32_MAX;
    require(uc_animation_template_find_action(&bank.catalog,prefix,action,&index)==UC_ANIMATION_OK&&index<bank.clips.size(),"Missing authored character action");
    load_leaf(*bank.clips[index],bank.skeleton.bone_count,root_bone,i);
}
void NativeCharacterClipGraph::load_leaf(const NativeFirstpersonClip &resource,uint32_t bones,int32_t root_bone,unsigned i){
    require(i<4&&root_bone>=0&&(uint32_t)root_bone<bones,"Character direct clip skeleton");
    auto &leaf=leaves_[i];leaf.resource=&resource;
    require(uc_animation_sample_load(leaf.resource->bytes.data(),leaf.resource->bytes.size(),&leaf.sample)==UC_ANIMATION_OK,"Character action sample load");
    const auto &clip=leaf.sample.clip;frequencies_[i]=clip.source_timing[0];
    for(uint32_t j=0;j<clip.track_count;j++){
        int32_t bone=clip.source_tracks[j];if(bone<0)bone=~bone;
        require((uint32_t)bone<bones,"Character action track outside skeleton");roots_[i]|=bone==root_bone;
    }
}
bool NativeCharacterClipGraph::advance(float delta){
    const float weights[5]={1,1,1,1,1};return advance(delta,weights);
}
bool NativeCharacterClipGraph::advance(float delta,const float weights[5]){
    uint8_t finished=0;
    const unsigned base=sequence?1:0;float timings[2]={frequencies_[0],frequencies_[1]};
    const double brute_frequency=brute_blend_?base_timing(0):0;const float previous_elapsed=sequence_.elapsed;const uint8_t previous_crossed=completion.crossed;
    if(brute_blend_)timings[base]=(float)brute_frequency;
    if(one_shot_)require(!uc_character_action_clip(&loop_,roots_[0],frequencies_[0],leaves_[0].sample.clip.source_timing[2],
        turn_scale_,delta,weights,(uint8_t)hold_last_,consumed,requests_,emitted_,&finished),"Character one-shot turn advance");
    else if(sequence)require(!uc_character_clip_sequence(&sequence_,&completion,roots_,timings,
        leaves_[0].sample.clip.source_timing[2],turn_scale_,delta,weights,consumed,requests_,emitted_,&finished),
        "Character sequence advance");
    else if(brute_blend_){
        //2B4C00 returns x87 precision;2B1EB0 narrows only after delta multiplication.
        float amount=(float)(brute_frequency*delta),unwrapped=loop_.phase+amount;
        require(std::isfinite(unwrapped)&&unwrapped>=0&&unwrapped<2147483648.f,"Brute loop phase");
        loop_.phase=(float)fmod((double)unwrapped,1.);completion.cycles+=(int32_t)unwrapped;completion.total+=amount;
        if(unwrapped>1)completion.crossed=1;
    }
    else require(!uc_character_clip_loop(&loop_,loop_completion_?loop_completion_.get():&completion,roots_[0],timings[0],delta,
        weights,consumed,requests_,emitted_),"Character loop advance");
    if(brute_blend_){
        uint32_t first;float factor;require(!uc_brute_movement_slider(*brute_blend_,&first,&factor),"Brute movement slider");
        float input[5],part[5],phase=loop_.phase;
        if(sequence){
            loop_weights(weights,input);float duration=(float)(1./timings[0]);
            float time=(float)(((double)sequence_.elapsed-duration)*brute_frequency),prior=(float)(((double)previous_elapsed-duration)*brute_frequency);
            phase=(float)((double)time-floor(time));completion.cycles=(int32_t)floor(time);completion.total=time;
            completion.crossed=(uint8_t)(previous_crossed||floor(time)!=floor(prior));
        }
        else std::copy_n(weights,5,input);
        float initial_used[5]{};if(sequence&&emitted_[0])initial_used[0]=roots_[0]?requests_[0].weight:0;
        std::fill_n(emitted_+base,3,uint8_t(0));
        for(unsigned side=0;side<2;side++){
            unsigned i=base+first+side;float w[5];for(unsigned j=0;j<5;j++)w[j]=input[j]*(side?1.f-factor:factor);
            require(!uc_character_clip_request(&leaf_previous_[i],roots_[i],frequencies_[i],delta,phase,w,1,part,requests_+i,emitted_+i),"Brute slider leaf request");
            //2B4890 returns the first child's consumption; its second emitted
            //pose is retained, but its consumption is not added by the source.
            if(!side)for(unsigned j=0;j<5;j++)consumed[j]=(0.f+part[j])+initial_used[j];
        }
    }
    UCAnimationEventQueue pending{};UCCharacterClipRequest requests[4]{};uint8_t roots[4]{};uint32_t count=0;
    //2BB240 walks head-to-tail, with wrapped annotation intervals split at1.
    for(unsigned i=0;i<leaf_count();i++)if(emitted_[i]){
        const auto &r=requests_[i];requests[count]=r;roots[count++]=roots_[i];
        UCAnimationAdvance intervals{};
        if(r.phase<r.previous){intervals.count=2;intervals.intervals[0]={r.previous,1,1};intervals.intervals[1]={0,r.phase,1};}
        else {intervals.count=1;intervals.intervals[0]={r.previous,r.phase,1};}
        require(uc_animation_collect_events(&leaves_[i].resource->annotations,&intervals,&pending)==UC_ANIMATION_OK,
            "Character graph annotation intervals");
    }
    require(!uc_character_clip_rotation(requests,roots,count,&rotation_scale),"Character graph root-yaw scale");
    events=pending;return finished!=0;
}
void NativeCharacterClipGraph::sample(NativeRenderer &renderer,NativeAnimatedModel &model,const UCAnimationBlendMath &math){
    UCAnimationBindingSample samples[4]{};size_t count=0;
    //2B5EC0 descends the linked list before sampling: tail-to-head.
    for(unsigned i=leaf_count();i--;)if(emitted_[i]){
        const auto &r=requests_[i];samples[count++]={&leaves_[i].sample,r.phase,r.weight,r.rate};
    }
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    model.sample(renderer,math,samples,count,identity,true);
}
void NativeCharacterClipGraph::append(std::vector<Request> &out){
    for(unsigned i=leaf_count();i--;)if(emitted_[i]){
        const auto&r=requests_[i];out.push_back({{&leaves_[i].sample,r.phase,r.weight,r.rate},
            &leaves_[i].resource->annotations,r.previous,r.root_scale,roots_[i]});
    }
}
double NativeCharacterClipGraph::base_timing(unsigned lane)const{
    unsigned base=sequence?1:0;if(!brute_blend_)return leaves_[base].sample.clip.source_timing[lane];
    uint32_t first;float weight;require(!uc_brute_movement_slider(*brute_blend_,&first,&weight),"Brute slider timing");
    double a=leaves_[base+first].sample.clip.source_timing[lane],b=leaves_[base+first+1].sample.clip.source_timing[lane];
    return(a-b)*weight+b;
}
void NativeCharacterClipGraph::loop_weights(const float*input,float*out)const{
    float blend=0;
    if(sequence){float phase=(float)((double)frequencies_[0]*sequence_.elapsed),normalized=(float)((double)frequencies_[0]*leaves_[0].sample.clip.source_timing[2]);blend=1;
        if((double)phase>1.-normalized){float x=(1.f-phase)/normalized;blend=x<=0?0:x>=1?1:(float)((1.-cos((double)x*3.1415927410125732))*.5);}}
    for(unsigned i=0;i<5;i++)out[i]=(1.f-blend)*input[i];
}


struct NativeCharacterAnimationSchedule::Impl {
    enum Kind {Action,Approach,Locomotion,Wrapper,Fade,Transition,Combined,Stance,Face,Movement,Turn,Duck};
    struct Node {
        Kind kind=Action;Node *a=nullptr,*b=nullptr;bool interrupted=false;
        float duration=0,elapsed=0;
        NativeCharacterAnimationCallback *callback=nullptr,*first=nullptr;
        std::unique_ptr<NativeCharacterClipGraph> action;
        UCCharacterFace face{};UCGameRandom *random=nullptr;std::vector<uint8_t> phonemes;
        UCCharacterNear approach_state{};const NativeCharacterBody *body=nullptr;
        const NativeCharacterMoveTarget *target=nullptr;uint8_t first_pending=0,done=0,backwards=0;int32_t mode=0;
        UCCharacterSchedule nested{};std::shared_ptr<int32_t> status;
        NativeCharacterAnimationCallback **arrival=nullptr;bool status_pending=false,loop_profile=false,reload_profile=false,authored=false,hold_last=false;
    };
    const NativeFirstpersonClips &bank;std::string prefix;int32_t root;
    UCCharacterSchedule state{},stance{},face{},overlay{},turn{};UCCharacterScheduleServices services{};int32_t stance_mode=0;bool cop=false;
    bool brute=false;int32_t brute_mode=1;UCBruteMovementBlend brute_blend{};
    UCCharacterAim aim{};UCCharacterAimAngles aim_angles[2]{};
    NativeCharacterAnimationCallback *aim_task=nullptr,*stance_task=nullptr,*weapon_tasks[2]{};
    std::shared_ptr<int32_t> movement_status,turn_status;
    std::shared_ptr<int32_t> duck_status[3];
    std::shared_ptr<int32_t> cop_status[2]; //stance owner20 reload,24 shot
    std::vector<std::unique_ptr<Node>> nodes;
    std::unique_ptr<NativeCharacterClipGraph> baseline;
    std::vector<NativeCharacterClipGraph::Request> requests;
    UCAnimationEventQueue emitted{};float rotation=0,helper[4]{};bool busy=false;
    std::vector<UCCharacterNearOutput> modifiers;
    std::vector<UCCharacterLocomotionStep> steering_requests;
    std::vector<UCCharacterAimRequest> aim_requests;
    std::vector<UCCharacterProfileRequest> profile_requests;
    std::vector<UCCharacterFacePose> face_requests;
    UCCharacterSteering steering{};const NativeCharacterBody *steering_body=nullptr;
    float desired_heading=0;bool heading_emitted=false,steering_initialized=false,authored_emitted=false;
    UCCharacterFoot feet[2]{};uint32_t pelvis=0,pelvis_parent=0;
    std::unique_ptr<float[][16]> worlds;const UCModel *pose_model=nullptr;
    struct Guard {
        bool &busy;explicit Guard(bool &value):busy(value){require(!busy,"Character scheduler callback reentry");busy=true;}
        ~Guard(){busy=false;}
    };
    static Node *node(void *p){return static_cast<Node*>(p);}
    static void notify(void *p,bool cancelled){
        if(!p)return;auto *c=static_cast<NativeCharacterAnimationCallback*>(p);
        require(c->notify!=nullptr,"Character scheduler callback has no receiver");c->notify(c->owner,cancelled);
    }
    struct Completion {
        std::shared_ptr<int32_t> status;NativeCharacterAnimationCallback *outer;
        NativeCharacterAnimationCallback callback{this,[](void*p,bool cancelled){auto*c=static_cast<Completion*>(p);*c->status=cancelled?2:1;notify(c->outer,cancelled);delete c;}};
        Completion(std::shared_ptr<int32_t> value,NativeCharacterAnimationCallback*next):status(std::move(value)),outer(next){}
    };
    NativeCharacterAnimationCallback *completion(std::shared_ptr<int32_t>&status,NativeCharacterAnimationCallback*outer){
        status=std::make_shared<int32_t>(0);return &(new Completion(status,outer))->callback;
    }
    Node *make(Kind kind,Node *a=nullptr,Node *b=nullptr){
        require(nodes.size()<64,"Character scheduler graph limit");
        auto n=std::make_unique<Node>();n->kind=kind;n->a=a;n->b=b;
        Node *result=n.get();nodes.push_back(std::move(n));return result;
    }
    static uint8_t finished(Node *n){
        switch(n->kind){
        case Action:case Stance:case Face:return 0;
        case Locomotion:case Movement:return n->done;
        case Turn:return n->done&&!n->hold_last;
        case Approach:return n->approach_state.phase>=1;
        case Wrapper:return n->interrupted||finished(n->a);
        case Fade:return !n->a;
        case Transition:return finished(n->a);
        case Combined:return finished(n->a)&&finished(n->b);
        case Duck:return n->interrupted||(n->status&&*n->status!=0);
        }return 0;
    }
    static Node *control(Node *n){return n->kind==Wrapper||n->kind==Movement||n->kind==Duck?n:n->kind==Transition?control(n->a):nullptr;}
    static float blend(Node *n,bool out){
        if(n->kind==Approach)return .2f;
        if(n->kind==Face)return .4f;
        if(n->kind==Movement)return n->nested.current?blend(node(n->nested.current),out):0;
        if(n->kind==Action||n->kind==Locomotion||n->kind==Stance||n->kind==Turn)return out?n->action->blend_out():n->action->blend_in();
        if(n->kind==Wrapper||n->kind==Transition||n->kind==Duck)return blend(n->a,out);
        return 0;
    }
    void destroy(Node *n){
        if(!n)return;
        if(n->kind==Movement){destroy(node(n->nested.fading));destroy(node(n->nested.current));destroy(node(n->nested.pending));
            notify(n->nested.pending_callback,true);notify(n->nested.stop_callback,true);if(n->status_pending)*n->status=2;}
        if(n->kind==Turn&&n->status_pending)*n->status=2;
        destroy(n->a);destroy(n->b);notify(n->callback,true);notify(n->first,true);
        auto i=std::find_if(nodes.begin(),nodes.end(),[&](const auto&p){return p.get()==n;});
        require(i!=nodes.end(),"Character graph ownership mismatch");nodes.erase(i);
    }
    void sample(Node *n,float delta,const float*w,float*out){
        switch(n->kind){
        case Movement:{
            if(!n->done){
                if(w[0]==0)require(!uc_character_schedule_stop(&n->nested,&services,nullptr),"Movement occlusion stop");
                else if(!n->nested.current&&!n->nested.pending){
                    double x=(double)n->target->position[0]-n->body->world[12],z=(double)n->target->position[2]-n->body->world[14];
                    float distance=(float)sqrt(x*x+z*z);
                    if(distance<=40){if(n->arrival){auto*c=*n->arrival;*n->arrival=nullptr;notify(c,false);}}
                    else{
                        Node*child;
                        if(distance<100){float direction[3]={(float)sin(-(double)n->body->heading.value),0,(float)cos(-(double)n->body->heading.value)};
                            child=make_approach(*n->body,n->target->position,direction,nullptr,2);}
                        else child=make_locomotion(*n->body,*n->target,n->target->near_target!=0);
                        require(!uc_character_schedule_queue(&n->nested,&services,child,nullptr,1),"Nested movement queue");
                    }
                }
            }
            require(!uc_character_schedule_update(&n->nested,&services,delta,w,out),"Nested movement update");break;
        }
        case Face:{
            UCCharacterFacePose poses[8];uint32_t count=0;
            require(!uc_character_face_step(&n->face,n->random,n->phonemes.empty()?nullptr:n->phonemes.data(),
                (uint32_t)n->phonemes.size(),delta,w[0],poses,&count),"Character facial sample");
            require(face_requests.size()+count<=24,"Character facial receiver capacity");
            static const char*names[]={"Bip01 Head","LeftEyeBall","RightEyeBall","Jaw","LeftMouth","LeftMouthBot","RightMouth","RightMouthBot"};
            static const int32_t ids[]={8,13,14,15,18,19,20,21};
            for(uint32_t j=0;j<count;j++){
                const auto id=std::find(std::begin(ids),std::end(ids),poses[j].bone);require(id!=std::end(ids),"Character facial symbol");
                poses[j].bone=-1;for(uint32_t i=0;i<bank.skeleton.bone_count;i++)if(!_stricmp(bank.skeleton.bone_names[i],names[id-ids])){
                    require(poses[j].bone==-1,"Duplicate facial bone");poses[j].bone=(int32_t)i;}
                face_requests.push_back(poses[j]);
            }
            std::fill_n(out,5,0.f);break;
        }
        case Stance:{
            static const UCCharacterProfileRow alert[]={{6,.3f,.2f,1,15},{8,.86f,.86f,.86f,25},{13,1,1,1,999},{14,1,1,1,999}};
            static const UCCharacterProfileRow aiming[]={{6,1,1,1,15},{8,1,1,1,25},{13,1,1,1,99},{14,1,1,1,99}};
            float first[5],second[5],remaining[5];profile(1,n->mode==1?aiming:alert,4,w,first);
            for(unsigned i=0;i<5;i++)remaining[i]=w[i]-first[i];profile(0,nullptr,0,remaining,second);
            for(unsigned i=0;i<5;i++)remaining[i]-=second[i];
            n->action->advance(delta,remaining);n->action->append(requests);require(requests.size()<=32,"Character stance request limit");
            for(unsigned i=0;i<5;i++)out[i]=(first[i]+second[i])+n->action->consumed[i];break;
        }
        case Approach:{
            float inverse[16];require(uc_point_light_inverse(n->body->world,inverse)!=0,"Approach body inverse");
            UCCharacterNearOutput result{};
            require(!uc_character_near(&n->approach_state,n->body->world+12,n->body->heading.value,inverse,delta,w[0],&result),"Character final approach");
            if(result.completed){auto*c=n->first;n->first=nullptr;notify(c,false);}
            require(modifiers.size()<32,"Character modifier limit");modifiers.push_back(result);
            n->action->advance(delta,w);n->action->append(requests);
            require(requests.size()<=32,"Character animation request limit");std::fill_n(out,5,0.f);break;
        }
        case Locomotion:{
            UCCharacterLocomotionStep step{};const auto &goal=*n->target;
            require(!uc_character_locomotion_step(&n->done,n->first_pending,n->body->world+12,n->body->heading.value,
                goal.position,n->backwards,goal.override_heading,goal.relative_heading,n->mode,goal.mode,w[4],&step),"Character locomotion step");
            require(steering_requests.size()<32,"Character steering request limit");steering_requests.push_back(step);
            if(step.heading_emitted){desired_heading=step.goal_heading;heading_emitted=true;}
            if(n->action->advance(delta,w))n->first_pending=0;
            n->action->append(requests);require(requests.size()<=32,"Character animation request limit");
            std::copy_n(n->action->consumed,5,out);out[4]+=step.steering_weight;break;
        }
        case Turn:{
            float used[5]{};n->done=n->action->advance(delta,w);
            if(n->authored&&n->action->one_shot_active())authored_emitted=true;
            if(n->reload_profile&&n->action->one_shot_active()){static const UCCharacterProfileRow rows[]={{6,1,.2f,1,15},{8,1,1,1,25},{13,1,1,1,99},{14,1,1,1,99}};
                profile(1,rows,4,w,used);}
            if(n->done&&n->status_pending){*n->status=1;n->status_pending=false;}
            if(n->done&&n->first){auto*c=n->first;n->first=nullptr;notify(c,false);}
            n->action->append(requests);require(requests.size()<=32,"Character turn request limit");
            for(unsigned i=0;i<5;i++)out[i]=used[i]+n->action->consumed[i];break;}
        case Action:
            if(n->action->advance(delta,w)&&n->first){auto*c=n->first;n->first=nullptr;notify(c,false);}
            if(n->loop_profile){float loop_weights[5],used[5];n->action->loop_weights(w,loop_weights);profile(0,nullptr,0,loop_weights,used);
                for(unsigned i=0;i<5;i++)n->action->consumed[i]+=used[i];}
            n->action->append(requests);require(requests.size()<=32,"Character animation request limit");
            std::copy_n(n->action->consumed,5,out);break;
        case Wrapper:sample(n->a,delta,w,out);break;
        case Duck:
            if(!n->status&&finished(n->a))interrupt_duck(n,true);
            sample(n->a,delta,w,out);break;
        case Fade:{
            UCCharacterGraphFade f{n->a,n->duration,n->elapsed};
            require(!uc_character_graph_fade(&f,&services,delta,w,out),"Character fade update");
            n->a=node(f.current);n->elapsed=f.elapsed;break;
        }
        case Transition:{
            UCCharacterGraphTransition f{n->a,n->b,n->callback,n->duration,n->elapsed};
            require(!uc_character_graph_transition(&f,&services,delta,w,out),"Character transition update");
            n->a=node(f.current);n->b=node(f.previous);n->callback=static_cast<NativeCharacterAnimationCallback*>(f.callback);n->elapsed=f.elapsed;break;
        }
        case Combined:{
            float used[5],remaining[5],part[5];sample(n->a,delta,w,used);
            for(unsigned i=0;i<5;i++)remaining[i]=w[i]-used[i];sample(n->b,delta,remaining,part);
            for(unsigned i=0;i<5;i++)out[i]=used[i]+part[i];break;
        }
        }
    }
    void profile(unsigned channel,const UCCharacterProfileRow*rows,unsigned count,const float weights[5],float used[5]){
        UCCharacterProfileRequest request{};require(!uc_character_profile_request(channel,rows,count,0,weights,&request,used),"Character profile request");
        if(request.emitted){require(profile_requests.size()<32,"Character profile request limit");profile_requests.push_back(request);}
    }
    void interrupt_duck(Node*n,bool graceful){
        if(!graceful){n->interrupted=true;return;}if(n->status)return;
        auto graph=std::make_unique<NativeCharacterClipGraph>(bank,prefix.c_str(),root,"DucksForCoverEnd",1.f,true);
        auto*end=make(Turn);end->action=std::move(graph);end->first=completion(n->status,n->callback);n->callback=nullptr;
        auto*transition=make(Transition,end,n->a);transition->duration=blend(end,false);n->a=transition;
    }
    Node *make_approach(const NativeCharacterBody&body,const float target[3],const float direction[3],NativeCharacterAnimationCallback*completion,float rate){
        auto graph=std::make_unique<NativeCharacterClipGraph>(bank,prefix.c_str(),root,"StandIdle");
        auto*n=make(Approach);n->action=std::move(graph);n->body=&body;n->first=completion;
        std::copy_n(target,3,n->approach_state.target);std::copy_n(direction,3,n->approach_state.direction);n->approach_state.rate=rate;n->approach_state.callback_pending=completion!=nullptr;return n;
    }
    Node *make_locomotion(const NativeCharacterBody&body,const NativeCharacterMoveTarget&target,bool near_target){
        UCCharacterLocomotionPlan plan{};require(!uc_character_locomotion_plan(body.world+12,body.heading.value,target.position,target.override_heading,target.relative_heading,target.mode,(uint8_t)near_target,&plan),"Locomotion target plan");
        auto graph=std::make_unique<NativeCharacterClipGraph>(bank,prefix.c_str(),root,plan,brute?&brute_blend.current:nullptr);
        auto*n=make(Locomotion);n->action=std::move(graph);n->body=&body;n->target=&target;n->mode=target.mode;
        n->first_pending=n->action->has_initial_clip();n->backwards=(uint8_t)plan.backwards;steering_body=&body;return n;
    }
    void apply_pose(NativeAnimatedModel &model,const UCAnimationBlendMath &math,const float identity[16]){
        if(!pose_model){
            auto bone=[&](const char*name){uint32_t found=UINT32_MAX;
                for(uint32_t i=0;i<bank.skeleton.bone_count;i++)if(!_stricmp(bank.skeleton.bone_names[i],name)){
                    require(found==UINT32_MAX,"Duplicate character body bone");found=i;}
                require(found<model.source.bone_count,"Missing character body bone");return found;};
            pelvis=bone("Bip01 Pelvis");pelvis_parent=model.source.bones[pelvis].parent;
            feet[0].foot=bone("Bip01 L Foot");feet[0].shin=bone("Bip01 L Calf");feet[0].thigh=bone("Bip01 L Thigh");
            feet[1].foot=bone("Bip01 R Foot");feet[1].shin=bone("Bip01 R Calf");feet[1].thigh=bone("Bip01 R Thigh");
            worlds.reset(new float[model.source.bone_count][16]);
            require(uc_model_pose_world(&model.source,nullptr,identity,worlds.get())==UC_POSE_OK,"Character bind hierarchy");
            auto length=[&](uint32_t a,uint32_t b){float d[3];for(unsigned i=0;i<3;i++)d[i]=worlds[a][12+i]-worlds[b][12+i];
                return(float)sqrt(((double)d[2]*d[2]+(double)d[1]*d[1])+(double)d[0]*d[0]);};
            for(auto&f:feet){f.pelvis=pelvis;f.upper_length=length(f.thigh,f.shin);f.lower_length=length(f.foot,f.shin);}
            pose_model=&model.source;
        }
        require(pose_model==&model.source,"Character modifiers changed skeleton owner");
        if(steering_body&&!steering_initialized){
            static const char *names[]={"Scene Root","Bip01","Bip01 Pelvis","Bip01 Spine","Bip01 Spine1","Bip01 Spine2",
                "Bip01 Spine3","Bip01 Neck","Bip01 Head","Bip01 L Clavicle","Bip01 L UpperArm","Bip01 R Clavicle","Bip01 R UpperArm","LeftEyeBall","RightEyeBall"};
            for(unsigned j=0;j<15;j++){steering.bones[j]=-1;for(uint32_t i=0;i<model.source.bone_count;i++)
                if(_stricmp(bank.skeleton.bone_names[i],names[j])==0){require(steering.bones[j]==-1,"Duplicate steering bone");steering.bones[j]=(int32_t)i;}}
            for(unsigned j=0;j<15;j++)if(steering.bones[j]>=0){auto p=model.source.bones[steering.bones[j]].parent;
                for(unsigned k=0;k<15;k++)if(steering.bones[k]>=0&&(uint32_t)steering.bones[k]==p){steering.parents[j]=k;break;}}
            steering.pelvis=(int32_t)pelvis;steering.compensate=1;steering_initialized=true;
        }
        float bob[3]{};for(auto&f:feet){f.weight=0;f.yaw=0;std::fill_n(f.offset,3,0.f);}
        for(const auto&m:modifiers){
            if(m.helper_emitted)for(unsigned i=0;i<4;i++)helper[i]+=m.helper[i];
            for(unsigned i=0;i<3;i++){float scaled=m.bob[i]*m.weight;bob[i]+=scaled*m.weight;
                for(unsigned side=0;side<2;side++)feet[side].offset[i]=m.feet[side][i]-scaled;}
            for(unsigned side=0;side<2;side++){feet[side].weight=m.weight;feet[side].yaw=m.foot_yaw[side];}
        }
        require(uc_model_pose_world(&model.source,model.binding.local,identity,worlds.get())==UC_POSE_OK,"Character pre-modifier hierarchy");
        if(steering_body){
            uc_character_profiles_clear(steering.channel);
            //2B5EC0 recurses to the tail before processing each prepended
            //request: pose/profile accumulation retains emission order.
            for(const auto&r:profile_requests)require(!uc_character_profile_accumulate(steering.channel,&r),"Character profile accumulation");
            for(const auto&r:steering_requests)if(r.steering_emitted)
                require(!uc_character_steering_accumulate(steering.channel,1,r.steering_weight,r.steering,0),"Character steering accumulation");
            for(const auto&r:aim_requests)
                require(!uc_character_steering_accumulate(steering.channel,r.channel,r.weight,r.yaw,r.pitch),"Character aim accumulation");
            require(!uc_character_pose_steer(&steering,model.binding.local,worlds.get(),model.source.bone_count,1.f/30,&steering_body->heading_delta,&math),"Character torso modifier");
        }
        require(!uc_character_pose_translate(model.binding.local,worlds.get(),model.source.bone_count,pelvis,pelvis_parent,bob),"Character pelvis modifier");
        for(auto&f:feet)require(!uc_character_pose_foot(model.binding.local,worlds.get(),model.source.bone_count,&f,&math),"Character foot modifier");
    }
    Impl(const NativeFirstpersonClips &bank_,const char *prefix_,int32_t root_):bank(bank_),prefix(prefix_),root(root_){
        nodes.reserve(64);requests.reserve(32);services.owner=this;
        services.finished=[](void*,void*p){return finished(node(p));};
        services.interruptor=[](void*,void*p)->void*{return control(node(p));};
        services.interrupt=[](void*owner,void*p,uint8_t graceful){auto*n=node(p);
            if(n->kind==Duck){static_cast<Impl*>(owner)->interrupt_duck(n,graceful!=0);return;}
            if(n->kind==Movement){n->done=1;if(n->status_pending){*n->status=1;n->status_pending=false;}
                if(n->arrival){auto*c=*n->arrival;*n->arrival=nullptr;notify(c,false);}}
            else n->interrupted=true;}; //2AE310/2B85C0 ignore the byte.
        services.destroy=[](void*p,void*g){static_cast<Impl*>(p)->destroy(node(g));};
        services.transition=[](void*p,void*a,void*b,void*c)->void*{
            auto&s=*static_cast<Impl*>(p);Node*n=s.make(Transition,node(a),node(b));
            n->duration=blend(n->a,false);n->callback=static_cast<NativeCharacterAnimationCallback*>(c);return n;
        };
        services.fade=[](void*p,void*a)->void*{if(!a)return nullptr;auto&s=*static_cast<Impl*>(p);Node*n=s.make(Fade,node(a));n->duration=blend(n->a,true);return n;};
        services.combine=[](void*p,void*a,void*b)->void*{return static_cast<Impl*>(p)->make(Combined,node(a),node(b));};
        services.notify=[](void*,void*c){notify(c,false);};services.cancel=[](void*,void*c){notify(c,true);};
        services.sample=[](void*p,void*g,float dt,const float*w,float*out){static_cast<Impl*>(p)->sample(node(g),dt,w,out);};
        services.wrap=[](void*p,void*g)->void*{return static_cast<Impl*>(p)->make(Wrapper,node(g));};
    }
    ~Impl(){
        destroy(node(turn.fading));destroy(node(turn.current));destroy(node(turn.pending));notify(turn.pending_callback,true);notify(turn.stop_callback,true);
        destroy(node(face.fading));destroy(node(face.current));destroy(node(face.pending));
        destroy(node(overlay.fading));destroy(node(overlay.current));destroy(node(overlay.pending));notify(overlay.pending_callback,true);notify(overlay.stop_callback,true);
        destroy(node(stance.fading));destroy(node(stance.current));destroy(node(stance.pending));notify(stance.pending_callback,true);notify(stance.stop_callback,true);
        notify(aim_task,true);notify(stance_task,true);for(auto*t:weapon_tasks)notify(t,true);
        destroy(node(state.fading));destroy(node(state.current));destroy(node(state.pending));
        notify(state.pending_callback,true);notify(state.stop_callback,true);
    }
};
NativeCharacterAnimationSchedule::NativeCharacterAnimationSchedule(const NativeFirstpersonClips &bank,
    const char *prefix,int32_t root):p_(std::make_unique<Impl>(bank,prefix,root)){}
NativeCharacterAnimationSchedule::~NativeCharacterAnimationSchedule()=default;
void NativeCharacterAnimationSchedule::initialize_baseline(const NativeCharacterBody &body,UCVectorRandom &random,uint32_t character_class){
    auto&s=*p_;Impl::Guard guard(s.busy);
    require(((character_class==1&&s.prefix=="BurnerBrute_")||(character_class==2&&s.prefix=="BurnerBlazer_")||(character_class==10&&s.prefix=="Cop_")||(character_class==11&&s.prefix=="Paramedic_"))&&
        !s.baseline&&!s.steering_body&&s.nodes.empty(),"Character baseline class or initialization order");
    s.cop=character_class==10;s.brute=character_class==1;
    // Reuse the shared draw's exact state transition. The body uses the unsigned
    // lane0 scale; converting the signed result back would add rounding error.
    UCVectorRandom next=random;float discard[4];
    require(!uc_vector_random_signed(&next,1,discard),"Character baseline random stream");
    float previous=next.values[next.first*4]*(1.f/4000000.f);
    auto baseline=std::make_unique<NativeCharacterClipGraph>(s.bank,s.prefix.c_str(),s.root,"StandIdle",previous);
    s.baseline=std::move(baseline);s.steering_body=&body;random=next;
}
void NativeCharacterAnimationSchedule::queue(const UCCharacterLocomotionPlan &plan,
    NativeCharacterAnimationCallback *completion,NativeCharacterAnimationCallback *first,bool interrupt){
    auto&s=*p_;Impl::Guard guard(s.busy);
    auto graph=std::make_unique<NativeCharacterClipGraph>(s.bank,s.prefix.c_str(),s.root,plan,s.brute?&s.brute_blend.current:nullptr);
    if(!graph->has_initial_clip()&&first){Impl::notify(first,true);first=nullptr;}
    auto*n=s.make(Impl::Action);n->action=std::move(graph);n->first=first;
    require(!uc_character_schedule_queue(&s.state,&s.services,n,completion,(uint8_t)interrupt),"Character scheduler queue");
}
void NativeCharacterAnimationSchedule::stop(NativeCharacterAnimationCallback *completion){
    auto&s=*p_;Impl::Guard guard(s.busy);
    require(!uc_character_schedule_stop(&s.state,&s.services,completion),"Character scheduler stop");
}
std::shared_ptr<UCCharacterClipCompletion> NativeCharacterAnimationSchedule::queue_loop(std::shared_ptr<const NativeFirstpersonClip> clip){
    auto&s=*p_;Impl::Guard guard(s.busy);auto completion=std::make_shared<UCCharacterClipCompletion>();
    auto graph=std::make_unique<NativeCharacterClipGraph>(std::move(clip),s.bank.skeleton.bone_count,s.root,completion);
    auto*n=s.make(Impl::Action);n->action=std::move(graph);
    require(!uc_character_schedule_queue(&s.state,&s.services,n,nullptr,1),"Character direct loop queue");return completion;
}
void NativeCharacterAnimationSchedule::queue_clip(std::shared_ptr<const NativeFirstpersonClip> clip,
    NativeCharacterAnimationCallback*completion,bool hold,bool authored){
    auto&s=*p_;Impl::Guard guard(s.busy);
    auto graph=std::make_unique<NativeCharacterClipGraph>(std::move(clip),s.bank.skeleton.bone_count,s.root,hold);
    auto*n=s.make(Impl::Turn);n->action=std::move(graph);n->first=completion;n->authored=authored;n->hold_last=hold;
    require(!uc_character_schedule_queue(&s.state,&s.services,n,nullptr,1),"Character direct action queue");
}
bool NativeCharacterAnimationSchedule::authored_motion()const{return p_->authored_emitted;}
void NativeCharacterAnimationSchedule::queue_action(const char*name,NativeCharacterAnimationCallback*completion,bool overlay){
    auto&s=*p_;Impl::Guard guard(s.busy);require(name&&*name,"Character named action");
    auto graph=std::make_unique<NativeCharacterClipGraph>(s.bank,s.prefix.c_str(),s.root,name,1.f,true);
    auto*n=s.make(Impl::Turn);n->action=std::move(graph);n->first=completion;
    require(!uc_character_schedule_queue(overlay?&s.overlay:&s.state,&s.services,n,nullptr,1),"Character one-shot action queue");
}
void NativeCharacterAnimationSchedule::queue_duck(bool already_low,NativeCharacterAnimationCallback*started){
    auto&s=*p_;Impl::Guard guard(s.busy);
    if(s.duck_status[1]&&*s.duck_status[1]==0){Impl::notify(started,true);return;}
    auto*first=s.completion(s.duck_status[0],started);
    auto*loop=s.completion(s.duck_status[1],nullptr);
    auto*last=s.completion(s.duck_status[2],nullptr);
    if(already_low){Impl::notify(first,true);first=nullptr;}
    auto graph=std::make_unique<NativeCharacterClipGraph>(s.bank,s.prefix.c_str(),s.root,"DucksForCoverLoop",already_low?nullptr:"DucksForCoverStart");
    auto*action=s.make(Impl::Action);action->action=std::move(graph);action->first=first;action->callback=loop;action->loop_profile=true;
    auto*duck=s.make(Impl::Duck,action);duck->callback=last;
    require(!uc_character_schedule_queue(&s.state,&s.services,duck,nullptr,!already_low),"Character cover sequence queue");
}
void NativeCharacterAnimationSchedule::queue_face(const uint8_t *phonemes,uint32_t ticks,UCGameRandom &random){
    auto&s=*p_;Impl::Guard guard(s.busy);require((phonemes||!ticks)&&ticks<=1000000,"Character phoneme span");
    auto*n=s.make(Impl::Face);if(ticks)n->phonemes.assign(phonemes,phonemes+ticks);n->random=&random;
    require(!uc_character_face_init(&n->face,&random),"Character face constructor");
    require(!uc_character_schedule_queue(&s.face,&s.services,n,nullptr,1),"Character face queue");
}
void NativeCharacterAnimationSchedule::stop_face(){auto&s=*p_;Impl::Guard guard(s.busy);
    require(!uc_character_schedule_stop(&s.face,&s.services,nullptr),"Character face stop");}
const std::vector<UCCharacterFacePose> &NativeCharacterAnimationSchedule::face_poses()const{return p_->face_requests;}
void NativeCharacterAnimationSchedule::cop_stance(int32_t mode,bool transition,bool already_low){
    auto&s=*p_;Impl::Guard guard(s.busy);require(s.cop&&(mode==0||mode==1||mode==4),"Cop stance owner or mode");
    if(s.stance_mode==mode)return;
    const char*base=mode==1?"TargetFront":mode==4?"TargetFrontAlert":nullptr;
    const char*first=nullptr;
    auto exists=[&](const char*name){uint32_t index=UINT32_MAX;require(!uc_animation_template_find_action(&s.bank.catalog,s.prefix.c_str(),name,&index),"Cop stance action lookup");return index!=UINT32_MAX;};
    if(base&&!exists(base))base=nullptr;
    if(transition){
        const bool low_transition=s.stance_mode==0&&mode==1;
        if(already_low&&low_transition&&exists("TransitionAimCrouch"))first="TransitionAimCrouch";
        //2AA249: a crouched entry with no crouched table slot has no prelude.
        if(!first&&(!already_low||low_transition)){
            if(s.stance_mode==0&&mode==1)first="TransitionAim";
            else if(s.stance_mode==0&&mode==4)first="TransitionAim_RelaxedToAlert";
            else if(s.stance_mode==1&&mode==4)first="TransitionAim_AimingToAlert";
            else if(s.stance_mode==4&&mode==1)first="TransitionAim_AlertToAiming";
            if(first&&!exists(first))first=nullptr;
        }
    }
    if(!base&&!first)require(!uc_character_schedule_stop(&s.stance,&s.services,nullptr),"Cop stance stop");
    else{
        //Actual Cop0/1/4 base clips exist; optional transition-only graphs for
        //other model catalogs require their own finite profile-loop owner.
        require(base!=nullptr,"Cop stance missing base with transition");
        auto graph=std::make_unique<NativeCharacterClipGraph>(s.bank,s.prefix.c_str(),s.root,base,first);
        auto*n=s.make(Impl::Stance);n->action=std::move(graph);n->mode=mode;
        require(!uc_character_schedule_queue(&s.stance,&s.services,n,nullptr,1),"Cop stance queue");
    }
    s.stance_mode=mode;
}
void NativeCharacterAnimationSchedule::cop_shoot(NativeCharacterAnimationCallback*callback){
    auto&s=*p_;Impl::Guard guard(s.busy);require(s.cop,"Cop shot owner");
    auto*completion=s.completion(s.cop_status[1],callback);
    const char*name=s.stance_mode==1?"TargetFrontFire":nullptr;
    if(!name){Impl::notify(completion,true);return;}
    auto*n=s.make(Impl::Turn);n->action=std::make_unique<NativeCharacterClipGraph>(s.bank,s.prefix.c_str(),s.root,name,1.f,true);n->first=completion;
    require(!uc_character_schedule_queue(&s.overlay,&s.services,n,nullptr,1),"Cop shot queue");
}
void NativeCharacterAnimationSchedule::cop_reload(NativeCharacterAnimationCallback*callback){
    auto&s=*p_;Impl::Guard guard(s.busy);require(s.cop,"Cop reload owner");
    auto*completion=s.completion(s.cop_status[0],callback);const char*name="Reload";
    if(s.stance_mode==0){uint32_t index=UINT32_MAX;require(!uc_animation_template_find_action(&s.bank.catalog,s.prefix.c_str(),"ReloadRelaxed",&index),"Cop reload lookup");
        if(index!=UINT32_MAX)name="ReloadRelaxed";}
    auto*n=s.make(Impl::Turn);n->action=std::make_unique<NativeCharacterClipGraph>(s.bank,s.prefix.c_str(),s.root,name,1.f,true);n->first=completion;n->reload_profile=true;
    require(!uc_character_schedule_queue(&s.overlay,&s.services,n,nullptr,1),"Cop reload queue");
}
bool NativeCharacterAnimationSchedule::cop_stance_busy()const{
    require(p_->cop,"Cop stance status owner");return p_->cop_status[0]&&*p_->cop_status[0]==0; //2ACDE0
}
int32_t NativeCharacterAnimationSchedule::cop_stance_mode()const{require(p_->cop,"Cop stance mode owner");return p_->stance_mode;}
void NativeCharacterAnimationSchedule::brute_movement_mode(int32_t mode){
    auto&s=*p_;Impl::Guard guard(s.busy);require(s.brute&&mode>=0&&mode<=2,"Brute movement mode owner");s.brute_mode=mode;
}
int32_t NativeCharacterAnimationSchedule::cop_weapon_mode()const{
    require(p_->cop&&!p_->weapon_tasks[0]&&!p_->weapon_tasks[1],"Cop weapon-mode task status is unconnected");return 0; //2A9B10: both class10+18 task slots startNULL.
}
void NativeCharacterAnimationSchedule::aim(const float point[3],uint32_t mode,NativeCharacterAnimationCallback*completion){auto&s=*p_;Impl::Guard guard(s.busy);
    require(s.steering_body&&point&&(mode<=4),"Character aim mode or body");
    for(unsigned i=0;i<3;i++)require(std::isfinite(point[i]),"Character aim point");
    s.aim.enabled=1;std::copy_n(point,3,s.aim.point);s.aim.mode=mode;Impl::notify(s.aim_task,true);s.aim_task=completion;}
void NativeCharacterAnimationSchedule::look_at(const float point[3]){auto&s=*p_;Impl::Guard guard(s.busy);
    require(s.steering_body&&point,"Character look body");for(unsigned i=0;i<3;i++)require(std::isfinite(point[i]),"Character look point");
    s.aim.look_enabled=1;std::copy_n(point,3,s.aim.look_point);}
void NativeCharacterAnimationSchedule::stop_look(){auto&s=*p_;Impl::Guard guard(s.busy);s.aim.look_enabled=0;}
void NativeCharacterAnimationSchedule::stop_aim(){auto&s=*p_;Impl::Guard guard(s.busy);
    s.aim.enabled=0;Impl::notify(s.aim_task,true);s.aim_task=nullptr;}
void NativeCharacterAnimationSchedule::queue_approach(const NativeCharacterBody &body,const float target[3],
    const float direction[3],NativeCharacterAnimationCallback *completion,float rate){
    auto&s=*p_;Impl::Guard guard(s.busy);require(target&&direction,"Character approach target");
    require(std::isfinite(rate)&&rate>0,"Character approach rate");
    for(unsigned i=0;i<3;i++)require(std::isfinite(target[i])&&std::isfinite(direction[i]),"Character approach coordinates");
    auto*n=s.make_approach(body,target,direction,completion,rate);
    require(!uc_character_schedule_queue(&s.state,&s.services,n,nullptr,1),"Character approach queue");
}
void NativeCharacterAnimationSchedule::queue_locomotion(const NativeCharacterBody &body,const NativeCharacterMoveTarget &target,bool near_target){
    auto&s=*p_;Impl::Guard guard(s.busy);
    require(s.prefix=="BurnerBrute_"||s.prefix=="BurnerBlazer_"||s.prefix=="Cop_"||s.prefix=="Paramedic_","Locomotion body profiles require class1/2/10/11 factory");
    require(!s.steering_body||s.steering_body==&body,"Locomotion changed body owner");
    auto*n=s.make_locomotion(body,target,near_target);
    require(!uc_character_schedule_queue(&s.state,&s.services,n,nullptr,1),"Locomotion queue");
}
void NativeCharacterAnimationSchedule::queue_movement(const NativeCharacterBody&body,const NativeCharacterMoveTarget&target,NativeCharacterAnimationCallback**arrival){
    auto&s=*p_;Impl::Guard guard(s.busy);require(arrival&&(!s.steering_body||s.steering_body==&body),"Movement body and arrival storage");
    if(s.movement_status&&*s.movement_status==0)return;
    auto*n=s.make(Impl::Movement);n->body=&body;n->target=&target;n->arrival=arrival;n->status=std::make_shared<int32_t>(0);n->status_pending=true;s.movement_status=n->status;
    require(!uc_character_schedule_queue(&s.state,&s.services,n,nullptr,1),"Outer movement queue");
}
bool NativeCharacterAnimationSchedule::movement_busy()const{return p_->movement_status&&*p_->movement_status==0;}
const float *NativeCharacterAnimationSchedule::heading_override()const{return p_->heading_emitted?&p_->desired_heading:nullptr;}
bool NativeCharacterAnimationSchedule::empty()const{return !p_->state.current&&!p_->state.pending;}
float NativeCharacterAnimationSchedule::rotation_scale()const{return p_->rotation;}
const UCAnimationEventQueue &NativeCharacterAnimationSchedule::events()const{return p_->emitted;}
const float *NativeCharacterAnimationSchedule::movement_helper()const{return p_->helper;}
NativeCharacterBody::NativeCharacterBody(const float position[3],float yaw){
    require(position&&std::isfinite(yaw)&&fabsf(yaw)<=1e6f,"Character body origin");
    for(unsigned i=0;i<3;i++)require(std::isfinite(position[i]),"Character body position");
    float origin[3];uc_collision_to_physics(position,origin);uc_player_reset(&physics,origin);
    heading.value=heading.target=yaw;
    require(!uc_character_heading(&heading,1.f/30),"Character initial heading");
    uc_euler_rotation(0,heading.value,0,world);std::memcpy(world+12,position,12);
}
void NativeCharacterBody::step(const NativeAnimatedModel &model,float rotation_scale,
    const UCCollisionAsset &collision,const UCSolidBlockGeometry *const *blocks,size_t count,
    const float helper[4],const float displacement[3],const float *heading_override,bool authored){
    constexpr float delta=1.f/30;float movement[16],desired[3],yaw;
    require(!uc_animation_binding_movement(&model.binding,delta,movement),"Character body root extraction");
    require(!uc_character_motion(movement,world+12,heading.value,rotation_scale,helper,displacement,desired,&yaw),"Character body motion");
    //2AB250 sets current heading to composed root yaw, then supplies that same
    // angle to the target setter unless2B62B0 replaces it with the helper yaw.
    auto next_heading=heading;next_heading.value=yaw;next_heading.target=heading_override?*heading_override:yaw;
    require(!uc_character_heading(&next_heading,delta),"Character body heading");
    float target[3];uc_collision_to_physics(desired,target);
    float planar[3];for(unsigned i=0;i<3;i++)planar[i]=(target[i]-physics.position[i])/delta;
    auto next=physics;
    require((authored?uc_character_step_authored_blocks(&collision,blocks,count,planar,&next):
        uc_character_step_blocks(&collision,blocks,count,planar,&next))==UC_PLAYER_OK,"Character body collision");
    float transform[16],speed[3];uc_euler_rotation(0,next_heading.value,0,transform);
    uc_collision_to_render(next.position,transform+12);
    for(unsigned i=0;i<3;i++)speed[i]=(transform[12+i]-world[12+i])/delta;
    float difference=next_heading.value-heading.value;
    constexpr double pi=3.1415927410125732,tau=6.2831854820251465;
    heading_delta=difference>0?(float)(fmod((double)difference+pi,tau)-pi):(float)-(fmod(pi-(double)difference,tau)-pi);
    physics=next;heading=next_heading;std::memcpy(world,transform,sizeof world);std::memcpy(velocity,speed,sizeof velocity);
}
const std::vector<NativeCharacterClipGraph::Request> &NativeCharacterAnimationSchedule::advance(float delta){
    auto&s=*p_;Impl::Guard guard(s.busy);s.requests.clear();s.modifiers.clear();s.steering_requests.clear();s.aim_requests.clear();s.profile_requests.clear();s.face_requests.clear();s.heading_emitted=s.authored_emitted=false;std::fill_n(s.helper,4,0.f);
    if(s.brute)require(!uc_brute_movement_blend(&s.brute_blend,s.brute_mode,delta),"Brute movement blend update");
    float remaining[5]={1,1,1,1,1},used[5];
    //2B15D0: face+30, overlay+38, stance+3C, foreground+40, profiles+44, idle+4C.
    for(auto*schedule:{&s.face,&s.overlay,&s.stance,&s.state}){
        require(!uc_character_schedule_update(schedule,&s.services,delta,remaining,used),"Character scheduler update");
        for(unsigned i=0;i<5;i++)remaining[i]-=used[i];
    }
    if(s.steering_body){
        //2B9230 is layer+48, between foreground+40 and profiles+44. Its
        //primary angle leaf always consumes lane4, including neutral angles.
        //The distinct speech-look leaf consumes lane3 only while enabled.
        //2B9230 owns a separate scheduler and a retained36280 completion.
        //A new turn is chosen only after that actual clip task has completed.
        if(remaining[0]>0&&s.aim_angles[0].enabled&&!(s.aim.mode&4)){
            if(!s.turn_status||*s.turn_status!=0){uint32_t action;float scale;
                require(!uc_character_turn_plan(s.aim_angles[0].yaw,(uint8_t)((s.aim.mode>>1)&1),&action,&scale),"Character body turn selection");
                if(action){static const char*names[]={nullptr,"TurnLeft180","TurnLeft90","TurnRight180","TurnRight90"};
                    auto graph=std::make_unique<NativeCharacterClipGraph>(s.bank,s.prefix.c_str(),s.root,names[action],scale,true);
                    auto*n=s.make(Impl::Turn);n->action=std::move(graph);n->status=std::make_shared<int32_t>(0);n->status_pending=true;s.turn_status=n->status;
                    require(!uc_character_schedule_queue(&s.turn,&s.services,n,nullptr,1),"Character body turn queue");
                }else{auto*c=s.aim_task;s.aim_task=nullptr;Impl::notify(c,false);}
            }
        }else require(!uc_character_schedule_stop(&s.turn,&s.services,nullptr),"Character body turn stop");
        float turn_used[5],angle_input[5];require(!uc_character_schedule_update(&s.turn,&s.services,delta,remaining,turn_used),"Character body turn update");
        for(unsigned i=0;i<5;i++)angle_input[i]=remaining[i]-turn_used[i];
        UCCharacterAimRequest angles[2];uint32_t count=0;
        require(!uc_character_aim_layer(&s.aim,s.aim_angles,angle_input,angles,&count,used),"Character automatic aim layer");
        s.aim_requests.insert(s.aim_requests.end(),angles,angles+count);for(unsigned i=0;i<5;i++)remaining[i]-=turn_used[i]+used[i];
        static const UCCharacterProfileRow first[]={{8,1,1,0,5},{13,1,1,0,99},{14,1,1,0,99}};
        static const UCCharacterProfileRow second[]={{6,.2f,.1f,0,3},{8,.86f,.86f,0,10},{13,1,1,0,99},{14,1,1,0,99}};
        s.profile(1,second,4,remaining,used);for(unsigned i=0;i<5;i++)remaining[i]-=used[i];
        s.profile(0,first,3,remaining,used);for(unsigned i=0;i<5;i++)remaining[i]-=used[i];
    }
    if(s.baseline){
        // Last of2B15D0's12 ordered layers. Earlier foreground controllers
        // consume their weights first; idle advances even when fully occluded.
        s.baseline->advance(delta,remaining);s.baseline->append(s.requests);
        require(s.requests.size()<=32,"Character baseline request limit");
    }
    UCCharacterClipRequest roots[32]{};uint8_t present[32]{};
    UCAnimationEventQueue events{};size_t count=s.requests.size();
    // Requests were prepended while evaluating. Reverse emission order restores
    // actual head-to-tail annotations and recursive root-scale accumulation.
    for(size_t i=0;i<count;i++){
        const auto&r=s.requests[count-i-1];roots[i]={r.sample.weight,r.previous,r.sample.phase,r.sample.root_factor,r.root_scale};present[i]=r.has_root;
        UCAnimationAdvance spans{};
        if(r.sample.phase<r.previous){spans.count=2;spans.intervals[0]={r.previous,1,1};spans.intervals[1]={0,r.sample.phase,1};}
        else {spans.count=1;spans.intervals[0]={r.previous,r.sample.phase,1};}
        require(!uc_animation_collect_events(r.annotations,&spans,&events),"Scheduled character annotation collection");
    }
    require(!uc_character_clip_rotation(roots,present,(uint32_t)count,&s.rotation),"Scheduled character root rotation");
    s.emitted=events;return s.requests;
}
void NativeCharacterAnimationSchedule::update(NativeRenderer &renderer,NativeAnimatedModel &model,
    const UCAnimationBlendMath &math,float delta){
    auto&s=*p_;
    if(s.steering_body){
        std::fill_n(s.aim_angles,2,UCCharacterAimAngles{});
        if(s.aim.enabled||s.aim.look_enabled){int32_t head=-1;
            for(uint32_t i=0;i<s.bank.skeleton.bone_count;i++)if(!_stricmp(s.bank.skeleton.bone_names[i],"Bip01 Head")){require(head==-1,"Duplicate aim head");head=(int32_t)i;}
            require(head>=0&&(uint32_t)head<model.source.bone_count&&model.prepared.ready&&model.prepared.world_valid[head],"Character aim previous pose");
            require(!uc_character_aim_solve(&s.aim,s.steering_body->world,model.prepared.world[head],s.steering_body->heading.value,s.aim_angles),"Character aim solver");
        }
    }
    advance(delta);Impl::Guard guard(s.busy);
    UCAnimationBindingSample samples[32]{};size_t count=s.requests.size();
    for(size_t i=0;i<count;i++)samples[i]=s.requests[i].sample;
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    if(s.modifiers.empty()&&!s.steering_body&&s.face_requests.empty())model.sample(renderer,math,samples,count,identity,true);
    else{
        model.prepared.ready=0;
        require(uc_animation_binding_sample_graph(&model.binding,samples,count,&math)==UC_ANIMATION_OK,"Character modified graph sampling");
        if(!s.modifiers.empty()||s.steering_body)s.apply_pose(model,math,identity);
        if(!s.face_requests.empty())require(!uc_character_face_apply(model.binding.local,model.source.bone_count,s.face_requests.data(),(uint32_t)s.face_requests.size()),"Character facial pose receiver");
        model.prepare(renderer,model.binding.local,identity);
    }
}

NativeCharacterMovement::NativeCharacterMovement(const NativeFirstpersonClips &bank,const char *prefix,int32_t root,const NativeCharacterBody &body)
    :body_(body),graph(bank,prefix,root){}
NativeCharacterMovement::~NativeCharacterMovement(){notify(true);}
void NativeCharacterMovement::notify(bool cancelled){
    auto*c=completion_;completion_=nullptr;if(c){bool was_busy=busy_;busy_=true;try{c->notify(c->owner,cancelled);}catch(...){busy_=was_busy;throw;}busy_=was_busy;}
}
void NativeCharacterMovement::request(const NativeCharacterMoveTarget &target,NativeCharacterAnimationCallback *completion){
    require(!busy_,"Movement callback reentry");require(!completion||completion->notify,"Movement completion receiver");
    for(float x:target.position)require(std::isfinite(x)&&fabsf(x)<=1e6f,"Movement target position");
    require(std::isfinite(target.relative_heading)&&fabsf(target.relative_heading)<=1e6f,"Movement target heading");
    notify(true);target_=target;completion_=completion;graph.queue_movement(body_,target_,&completion_);
}
void NativeCharacterMovement::stop(){
    require(!busy_,"Movement callback reentry");graph.stop();
}
void NativeCharacterMovement::update(NativeRenderer &renderer,NativeAnimatedModel &model,const UCAnimationBlendMath &math,float delta){
    require(!busy_&&delta==1.f/30,"Movement task update interval");busy_=true;
    try{
        graph.update(renderer,model,math,delta);
    }catch(...){busy_=false;throw;}busy_=false;
}
