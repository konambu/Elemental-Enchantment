/* Experimental: caller holds guard when modifying visual records. */
#define CB_LIMIT 64
typedef struct {void* character;void* scene;void* node;void* fx;void* fx2;Vec position;Quat rotation;float progress;U64 sample;int element;int secondary;} CBCharge;
typedef struct {void* projectile;void* source;void* scene;void* node;void* fx;void* fx2;Vec previous,origin,first,last,offset;U64 born;unsigned int shooter_index,shooter_serial,shot_thread;U64 shot_sequence;unsigned int frames,peak;int speed_done;int element;int secondary;int heading;int has_origin;} CBFlight;
typedef struct {DWORD thread;unsigned index,serial;U64 sequence;void* created[16];unsigned count;} CBShot;
static CBCharge cb_charges[CB_LIMIT];static CBFlight cb_flights[CB_LIMIT];static CBShot cb_shots[CB_LIMIT];static U64 cb_shot_sequence;static int cb_visual_enabled=1,cb_logged_charge,cb_logged_flight,cb_logged_speed,cb_logged_trace,cb_logged_no_node,cb_logged_no_ally,cb_logged_shoot,cb_logged_flight_fx;
static void (*original_ranged_update)(void*,float,const void*,const void*);
static void (*original_crossbow_shoot)(void*,void*,void*,int,const Vec*);
static void* (*original_harpoon_create)(void*,const void*,const void*);
static void (*original_projectile_trace)(void*,const Vec*,const Vec*);
static void (*original_projectile_destroy)(void*);
static Vec* (*cb_barrel)(void*,Vec*);
static float (*cb_normalise_vec)(Vec*);static float (*cb_normalise_quat)(Quat*);static void (*cb_particle_scale)(void*,const Vec*);
static void cb_clear_charge(CBCharge* c){if(c->fx2&&c->scene)particle_destroy(particle_manager(),c->fx2,c->scene);if(c->fx&&c->scene)particle_destroy(particle_manager(),c->fx,c->scene);if(c->node&&c->scene)node_destroy(c->scene,c->node);*c=(CBCharge){0};}
static int cb_diag_count;
static int cb_diag_int(char* b,int n,int x){char tmp[12];int k=0;if(x<0){b[n++]='-';x=-x;}do{tmp[k++]=(char)('0'+x%10);x/=10;}while(x&&k<11);while(k)b[n++]=tmp[--k];return n;}
static int cb_diag_label(char* b,int n,const char* s){while(*s)b[n++]=*s++;return n;}
static int cb_diag_vec(char* b,int n,Vec v){n=cb_diag_int(b,n,(int)v.x);b[n++]=',';n=cb_diag_int(b,n,(int)v.y);b[n++]=',';return cb_diag_int(b,n,(int)v.z);}
static void cb_diag_flight(CBFlight* f){char b[256];int n=0;if(cb_diag_count>=12||!f->projectile)return;++cb_diag_count;n=cb_diag_label(b,n,"CROSSBOW SAMPLE: element=");n=cb_diag_int(b,n,f->element);n=cb_diag_label(b,n," frames=");n=cb_diag_int(b,n,f->frames);n=cb_diag_label(b,n," peak=");n=cb_diag_int(b,n,f->peak);n=cb_diag_label(b,n," ms=");n=cb_diag_int(b,n,(int)(GetTickCount64()-f->born));n=cb_diag_label(b,n," origin=");n=cb_diag_vec(b,n,f->origin);n=cb_diag_label(b,n," first=");n=cb_diag_vec(b,n,f->first);n=cb_diag_label(b,n," last=");n=cb_diag_vec(b,n,f->last);b[n++]='\r';b[n++]='\n';b[n]=0;logline(b);}
static void cb_reset_flight(CBFlight* f){volatile unsigned char* p=(volatile unsigned char*)f;unsigned i;for(i=0;i<sizeof(*f);++i)p[i]=0;}
static void cb_clear_flight(CBFlight* f){cb_diag_flight(f);if(f->fx2&&f->scene)particle_destroy(particle_manager(),f->fx2,f->scene);if(f->fx&&f->scene)particle_destroy(particle_manager(),f->fx,f->scene);if(f->node&&f->scene)node_destroy(f->scene,f->node);cb_reset_flight(f);}
static void clear_crossbow_scene(void* scene){int i;for(i=0;i<CB_LIMIT;++i){if(cb_charges[i].scene==scene)cb_clear_charge(&cb_charges[i]);if(cb_flights[i].scene==scene)cb_clear_flight(&cb_flights[i]);}}
static void clear_crossbow_character(void* c){int i;for(i=0;i<CB_LIMIT;++i)if(cb_charges[i].character==c)cb_clear_charge(&cb_charges[i]);}
static void clear_crossbow_source(void* node){int i;for(i=0;i<CB_LIMIT;++i)if(cb_flights[i].source==node)cb_clear_flight(&cb_flights[i]);}
static void clear_crossbow_projectile(void* projectile){int i;for(i=0;i<CB_LIMIT;++i)if(cb_flights[i].projectile==projectile)cb_clear_flight(&cb_flights[i]);}
static const String* cb_template(int e){static String f={{0},21,21,0},l={{0},17,17,0},w={{"EE_Bolt_Wind"},12,15,0},a={{0},22,22,0},i={{"EE_Bolt_Ice"},11,15,0};f.text.heap="SB_Prototype_HandFire";l.text.heap="EE_Bolt_Lightning";a.text.heap="SB_Prototype_WaterHand";return e==5?&i:e==4?&a:e==3?&w:e==2?&l:&f;}
static void cb_update_hook(void* ranged,float dt,const void* target,const void* suggested){
    void *actor,*gun;float timer,max,progress;int state,i,slot=-1;Vec barrel;Quat aim={1,0,0,0};
    original_ranged_update(ranged,dt,target,suggested);if(!active||!ranged||internal==GetCurrentThreadId())return;
    __try {actor=*(void**)((unsigned char*)ranged+0x68);gun=*(void**)((unsigned char*)ranged+0x28);state=*(int*)ranged;timer=*(float*)((unsigned char*)ranged+0x78);max=*(float*)((unsigned char*)ranged+0x74);
        if(!actor||!gun||!character_is_player(actor)||state>2||state<0||!(max>.001f&&max<1000.0f)||!(timer>=0.0f&&timer<=max)||!cb_barrel)return;
        progress=1.0f-timer/max;if(progress<0)progress=0;if(progress>1)progress=1;cb_barrel(gun,&barrel);
        barrel.y+=1.0f; /* Lift the charge just above the crossbow. */
        if(!(barrel.x>-1000000&&barrel.x<1000000&&barrel.y>-1000000&&barrel.y<1000000&&barrel.z>-1000000&&barrel.z<1000000))return;
        __try {Quat q=*(Quat*)((unsigned char*)gun+0xf0);float q2=q.w*q.w+q.x*q.x+q.y*q.y+q.z*q.z;if(q2>.01f&&q2<4.0f){aim=q;cb_normalise_quat(&aim);}}__except(1){}
        AcquireSRWLockExclusive(&guard);__try {for(i=0;i<CB_LIMIT;++i){if(cb_charges[i].character==actor){slot=i;break;}if(slot<0&&!cb_charges[i].character)slot=i;}if(slot>=0){CBCharge* c=&cb_charges[slot];c->character=actor;c->position=barrel;c->rotation=aim;c->progress=progress;c->sample=GetTickCount64();if(!cb_logged_charge){cb_logged_charge=1;logline("CROSSBOW: first player reload sample.\r\n");}}}__finally{ReleaseSRWLockExclusive(&guard);}
    }__except(1){logline("CROSSBOW: reload sample skipped.\r\n");}
}
static void cb_trace_hook(void* projectile,const Vec* pos,const Vec* direction){
    int i,j,slot=-1;unsigned index,serial_number;void* node;original_projectile_trace(projectile,pos,direction);
    if(!active||!projectile||internal==GetCurrentThreadId())return;
    if(!cb_logged_trace){cb_logged_trace=1;logline("CROSSBOW: projectile trace called.\r\n");}
    __try {index=*(unsigned*)((unsigned char*)projectile+0x34);serial_number=*(unsigned*)((unsigned char*)projectile+0x38);node=*(void**)((unsigned char*)projectile+0x58);if(!node){if(!cb_logged_no_node){cb_logged_no_node=1;logline("CROSSBOW: projectile has no scene node at trace.\r\n");}return;}
        AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {for(i=0;i<SB_ALLY_LIMIT;++i)if(allies[i].character&&allies[i].element){const unsigned char* h=(const unsigned char*)character_handle(allies[i].character);if(h&&*(unsigned*)(h+0x14)==index&&*(unsigned*)(h+0x18)==serial_number)break;}
            if(i<SB_ALLY_LIMIT){for(j=0;j<CB_LIMIT;++j){if(cb_flights[j].projectile==projectile){cb_clear_flight(&cb_flights[j]);slot=j;break;}if(slot<0&&!cb_flights[j].projectile)slot=j;}if(slot>=0){CBFlight* f=&cb_flights[slot];f->projectile=projectile;f->source=node;f->born=GetTickCount64();f->shooter_index=index;f->shooter_serial=serial_number;f->shot_thread=GetCurrentThreadId();for(j=0;j<CB_LIMIT;++j)if(cb_shots[j].thread==f->shot_thread&&cb_shots[j].index==index&&cb_shots[j].serial==serial_number&&cb_shots[j].sequence>f->shot_sequence)f->shot_sequence=cb_shots[j].sequence;f->element=allies[i].element;f->secondary=allies[i].secondary;f->previous=pos?*pos:(Vec){0};if(!cb_logged_flight){cb_logged_flight=1;logline("CROSSBOW: first player projectile tracked.\r\n");}}}else if(!cb_logged_no_ally){cb_logged_no_ally=1;logline("CROSSBOW: projectile shooter did not match an enchanted ally.\r\n");}}
        __finally {internal=0;ReleaseSRWLockExclusive(&guard);}
    }__except(1){logline("CROSSBOW: projectile sample skipped.\r\n");}
}
static void* cb_create_hook(void* manager,const void* mesh,const void* material){
    void* projectile=original_harpoon_create(manager,mesh,material);int i;DWORD tid=GetCurrentThreadId();
    if(!active||!projectile||internal==tid)return projectile;
    AcquireSRWLockExclusive(&guard);__try {for(i=0;i<CB_LIMIT;++i)if(cb_shots[i].sequence&&cb_shots[i].thread==tid){CBShot* s=&cb_shots[i];if(s->count<16)s->created[s->count++]=projectile;break;}}__finally{ReleaseSRWLockExclusive(&guard);}
    return projectile;
}
static void cb_capture_created(CBShot* shot,int element,int secondary){
    unsigned n;int i;for(n=0;n<shot->count;++n)__try {
        void* projectile=shot->created[n];void* node=*(void**)((unsigned char*)projectile+0x58);unsigned index=*(unsigned*)((unsigned char*)projectile+0x34),serial_number=*(unsigned*)((unsigned char*)projectile+0x38);
        if(!node||index!=shot->index||serial_number!=shot->serial)continue;
        for(i=0;i<CB_LIMIT;++i)if(cb_flights[i].projectile==projectile)break;if(i<CB_LIMIT)continue;
        for(i=0;i<CB_LIMIT;++i)if(!cb_flights[i].projectile)break;
        if(i<CB_LIMIT){int j;CBFlight* f=&cb_flights[i];f->projectile=projectile;f->source=node;f->born=GetTickCount64();f->shooter_index=index;f->shooter_serial=serial_number;f->shot_thread=shot->thread;f->shot_sequence=shot->sequence;f->element=element;f->secondary=secondary;for(j=0;j<CB_LIMIT;++j)if(cb_charges[j].character&&GetTickCount64()-cb_charges[j].sample<500){const unsigned char* h=(const unsigned char*)character_handle(cb_charges[j].character);if(h&&*(unsigned*)(h+0x14)==index&&*(unsigned*)(h+0x18)==serial_number){f->origin=cb_charges[j].position;f->origin.y-=1.0f;f->has_origin=1;break;}}if(!cb_logged_flight){cb_logged_flight=1;logline("CROSSBOW: player projectile captured at creation.\r\n");}}
    }__except(1){logline("CROSSBOW: created projectile sample skipped.\r\n");}
}
static void cb_shoot_hook(void* gun,void* actor,void* target,int stat,const Vec* aim){
    int i,slot=-1,element=0,secondary=0;unsigned index=0,serial_number=0;U64 sequence=0;float speed=1.0f;int player=0;DWORD tid=GetCurrentThreadId();
    if(active&&!cb_logged_shoot){cb_logged_shoot=1;logline("CROSSBOW: shoot hook called.\r\n");}
    if(active&&actor&&internal!=tid)__try {if(character_is_player(actor)){const unsigned char* h=(const unsigned char*)character_handle(actor);void* stats=character_stats(actor);if(h&&stats){index=*(const unsigned*)(h+0x14);serial_number=*(const unsigned*)(h+0x18);AcquireSRWLockExclusive(&guard);__try {SBCharacterRecord* r=sb_persistence_bind(&persistence,stats,actor,index,serial_number,index^(serial_number*0x9e3779b9u),(unsigned)element_from_character(actor));if(r&&!r->read_only){speed=sb_affinity_attack_speed(&r->affinity,1.0f);player=1;for(i=0;i<CB_LIMIT;++i)if(!cb_shots[i].sequence){slot=i;break;}if(slot>=0){sequence=++cb_shot_sequence;if(!sequence)sequence=++cb_shot_sequence;cb_shots[slot].thread=tid;cb_shots[slot].index=index;cb_shots[slot].serial=serial_number;cb_shots[slot].sequence=sequence;cb_shots[slot].count=0;}}}__finally{ReleaseSRWLockExclusive(&guard);}}}}__except(1){player=0;}
    original_crossbow_shoot(gun,actor,target,stat,aim);
    if(player&&sequence){AcquireSRWLockExclusive(&guard);__try {if(slot>=0&&cb_shots[slot].sequence==sequence){for(i=0;i<SB_ALLY_LIMIT;++i)if(allies[i].character==actor){element=allies[i].element;secondary=allies[i].secondary;break;}if(element)cb_capture_created(&cb_shots[slot],element,secondary);cb_shots[slot].sequence=0;cb_shots[slot].count=0;}if(speed>1.0f&&speed<2.0f)for(i=0;i<CB_LIMIT;++i){CBFlight* f=&cb_flights[i];if(f->projectile&&!f->speed_done&&f->shot_sequence==sequence){float* velocity=(float*)((unsigned char*)f->projectile+0x44);__try {if(*velocity>0.0f&&*velocity<10000.0f){*velocity*=speed;f->speed_done=1;if(!cb_logged_speed){cb_logged_speed=1;logline("CROSSBOW: projectile speed bonus applied.\r\n");}}}__except(1){f->speed_done=1;}}}}__finally{ReleaseSRWLockExclusive(&guard);}}
}
static void cb_destroy_hook(void* projectile){if(active&&projectile&&internal!=GetCurrentThreadId()){AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try{clear_crossbow_projectile(projectile);}__finally{internal=0;ReleaseSRWLockExclusive(&guard);}}original_projectile_destroy(projectile);}
static int cb_create_fx(void* manager,void* scene,void** node,void** fx,void** fx2,int element,int secondary){void* root=trail_root?trail_root(scene,0):0;if(!root)return 0;*node=node_create(scene,0);if(!*node)return 0;node_add(root,*node);*fx=create_secondary_effect(manager,scene,*node,cb_template(element),0);if(!*fx)return 0;if(secondary)*fx2=create_secondary_effect(manager,scene,*node,cb_template(secondary),0);return 1;}
static Vec cb_align_position(CBFlight* f,Vec raw){Vec p;if(!f->frames){f->first=raw;if(f->has_origin){f->offset=(Vec){f->origin.x-raw.x,f->origin.y-raw.y,f->origin.z-raw.z};}}f->last=raw;++f->frames;p=(Vec){raw.x+f->offset.x,raw.y+f->offset.y,raw.z+f->offset.z};if(f->frames==1)f->previous=p;return p;}
static void cb_update_visuals_impl(void* manager,float dt){int i,j;U64 now=GetTickCount64();const float bounds[6]={0,0,0,25,25,25};
    for(i=0;i<CB_LIMIT;++i){CBCharge* c=&cb_charges[i];Ally* a=0;void* parent;float scale;Vec sz;
        if(!c->character)continue;for(j=0;j<SB_ALLY_LIMIT;++j)if(allies[j].character==c->character){a=&allies[j];break;}
        if(!a||!a->body||!a->element||now-c->sample>500){cb_clear_charge(c);continue;}parent=parent_node(a->body);if(!parent){cb_clear_charge(c);continue;}
        if(c->fx&&(c->element!=a->element||c->secondary!=a->secondary||c->scene!=node_scene(parent))){cb_clear_charge(c);continue;}
        if(!c->fx){c->scene=node_scene(parent);c->element=a->element;c->secondary=a->secondary;if(!c->scene||!cb_create_fx(manager,c->scene,&c->node,&c->fx,&c->fx2,c->element,c->secondary)){cb_clear_charge(c);continue;}}
        scale=.20f+.80f*c->progress;sz=(Vec){scale,scale,scale};node_scale(c->node,&sz);if(cb_particle_scale){float main_scale=scale*(c->element==1?1.0f:1.6f),sub_scale=scale*(c->secondary==1?1.0f:1.6f);Vec main_size={main_scale,main_scale,main_scale},sub_size={sub_scale,sub_scale,sub_scale};cb_particle_scale(c->fx,&main_size);if(c->fx2)cb_particle_scale(c->fx2,&sub_size);}node_position(c->node,&c->position);if(c->rotation.w||c->rotation.x||c->rotation.y||c->rotation.z)node_rotation(c->node,c->rotation.w,c->rotation.x,c->rotation.y,c->rotation.z);particle_update(c->fx,dt);set_aabb(c->fx,bounds);if(c->fx2){particle_update(c->fx2,dt);set_aabb(c->fx2,bounds);}
    }
    for(i=0;i<CB_LIMIT;++i){CBFlight* f=&cb_flights[i];Vec pos,back;Quat q;float d;
        if(!f->projectile)continue;if(!f->source||now-f->born>10000){cb_clear_flight(f);continue;}
        if(!trail_derive(f->source,&pos)){cb_clear_flight(f);continue;}pos=cb_align_position(f,pos);
        back=(Vec){f->previous.x-pos.x,f->previous.y-pos.y,f->previous.z-pos.z};d=cb_normalise_vec(&back);f->previous=pos;
        if(d>.01f&&d<2000){if(back.y<-.9999f)q=(Quat){0,1,0,0};else{q=(Quat){1+back.y,back.z,0,-back.x};cb_normalise_quat(&q);}f->heading=1;pos.x-=back.x*2.0f;pos.y-=back.y*2.0f;pos.z-=back.z*2.0f;}else q=(Quat){1,0,0,0};
        if(!f->fx){float main_scale=f->element==1?1.75f:2.8f,sub_scale=f->secondary==1?1.75f:2.8f;Vec size={2.0f,2.0f,2.0f},main_size={main_scale,main_scale,main_scale},sub_size={sub_scale,sub_scale,sub_scale};f->scene=node_scene(f->source);if(!f->scene||!cb_create_fx(manager,f->scene,&f->node,&f->fx,&f->fx2,f->element,f->secondary)){cb_clear_flight(f);continue;}node_position(f->node,&pos);if(f->heading)node_rotation(f->node,q.w,q.x,q.y,q.z);node_scale(f->node,&size);if(cb_particle_scale){cb_particle_scale(f->fx,&main_size);if(f->fx2)cb_particle_scale(f->fx2,&sub_size);}particle_local(f->fx,0);if(f->fx2)particle_local(f->fx2,0);if(f->element!=2||f->heading)particle_update(f->fx,.10f);if(f->fx2&&(f->secondary!=2||f->heading))particle_update(f->fx2,.10f);if(!cb_logged_flight_fx){cb_logged_flight_fx=1;logline("CROSSBOW: projectile effect created.\r\n");}}
        node_position(f->node,&pos);if(d>.01f&&d<2000)node_rotation(f->node,q.w,q.x,q.y,q.z);particle_update(f->fx,dt);if(particle_count){U64 count=particle_count(f->fx);if(count>f->peak)f->peak=(unsigned int)count;}set_aabb(f->fx,bounds);if(f->fx2){particle_update(f->fx2,dt);set_aabb(f->fx2,bounds);}
    }
}

static void cb_update_visuals(void* manager,float dt){int i;if(!cb_visual_enabled)return;__try {cb_update_visuals_impl(manager,dt);}__except(1){cb_visual_enabled=0;logline("CROSSBOW: visuals disabled after native error.\r\n");for(i=0;i<CB_LIMIT;++i){__try{cb_clear_charge(&cb_charges[i]);}__except(1){cb_charges[i]=(CBCharge){0};}__try{cb_clear_flight(&cb_flights[i]);}__except(1){cb_reset_flight(&cb_flights[i]);}}}}
