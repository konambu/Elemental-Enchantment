/* Elemental Enchantment runtime: engine hooks and shared runtime state. */
typedef unsigned long DWORD;
int _fltused=0;
typedef unsigned long long U64;
typedef void* MODULE;
typedef struct { union { char small[16]; const char* heap; } text; U64 size,capacity,allocator; } String;
typedef struct { float x,y,z; } Vec;
typedef struct { float w,x,y,z; } Quat;
#ifdef EE_ENABLE_PERSISTENCE
#include "elemental_persistence.h"
#include "elemental_gamedata.h"
#endif
#include "elemental_boss_profiles.h"
#ifdef EE_ENABLE_UI
#include "elemental_mygui.h"
#include "elemental_localization.h"
#include "elemental_mcm_strings.h"
#endif
typedef struct { void* owner; void* blade; void* tag; void* scene; void* parent; void* node; void* effect; void* effect2; void* frost;void* frost_scene;void* frost_node;Quat frost_rotation;Vec frost_offset;int frost_bone,frost_attached; U64 born; float length;float applied_scale; int pole; int element;int secondary;int level;
#ifdef EE_ENABLE_TRAIL
    void* ribbon;void* ribbon_node;void* ribbon_root;Vec trail_direction;int trail_has_direction;float trail_idle;int trail_element,trail_linked,trail_attached;
#endif
} Entry;
__declspec(dllimport) MODULE __stdcall GetModuleHandleA(const char*);
__declspec(dllimport) void* __stdcall GetProcAddress(MODULE,const char*);
__declspec(dllimport) DWORD __stdcall GetModuleFileNameA(MODULE,char*,DWORD);
__declspec(dllimport) unsigned int __stdcall GetPrivateProfileIntA(const char*,const char*,int,const char*);
__declspec(dllimport) DWORD __stdcall GetPrivateProfileStringA(const char*,const char*,const char*,char*,DWORD,const char*);
__declspec(dllimport) DWORD __stdcall GetCurrentThreadId(void);
__declspec(dllimport) U64 __stdcall GetTickCount64(void);
__declspec(dllimport) int __stdcall QueryPerformanceCounter(U64*);
__declspec(dllimport) void __stdcall AcquireSRWLockExclusive(void*);
__declspec(dllimport) void __stdcall ReleaseSRWLockExclusive(void*);
__declspec(dllimport) void* __stdcall CreateFileA(const char*,DWORD,DWORD,void*,DWORD,DWORD,void*);
__declspec(dllimport) int __stdcall WriteFile(void*,const void*,DWORD,DWORD*,void*);
__declspec(dllimport) int __stdcall CloseHandle(void*);
#define EE_TRACK_LIMIT 512
#define EE_ALLY_LIMIT 512
static Entry entries[EE_TRACK_LIMIT];
typedef struct {void* character;void* body;void* left;void* right;int unarmed;int element;int attacking;int secondary;int level;int melee_weapon;float hit_glow;} Ally;
static Ally allies[EE_ALLY_LIMIT];
/* A conservative history filter: false positives cost a scan, false negatives
 * are impossible because bits are set before a character enters allies. */
static unsigned int ally_seen_bits[2048];
static unsigned int ally_seen_bit(void* character) {unsigned int x=(unsigned int)((U64)character>>4);x^=x>>16;x*=0x7feb352du;x^=x>>15;return x&65535u;}
static int ally_seen_maybe(void* character) {unsigned int bit=ally_seen_bit(character);return (ally_seen_bits[bit>>5]&(1u<<(bit&31)))!=0;}
static void ally_seen_remember(void* character) {unsigned int bit=ally_seen_bit(character);ally_seen_bits[bit>>5]|=1u<<(bit&31);}
typedef struct {void* character;void* body;void* bone;void* scene;void* parent;void* node;void* effect;void* effect2;float applied_scale;int element;int secondary;} HandEffect;
static HandEffect hand_effects[EE_ALLY_LIMIT*2];
static void* guard; static DWORD internal; static int active,logged_full; static U64 serial;
static char log_path[1024]; static MODULE self;
static char settings_path[1024]; static int all_aptitudes; static float effect_multiplier=1.0f; static unsigned int xp_tenths=10,ui_scale_tenths=10; static int show_trails=1,show_impacts=1;
#ifdef EE_ENABLE_PERSISTENCE
/* Called only when an unsaved player record is first created, under guard. */
static unsigned int fresh_aptitude_seed(unsigned int seed){static unsigned int sequence;U64 ticks=0;QueryPerformanceCounter(&ticks);++sequence;return seed^(unsigned int)ticks^(unsigned int)(ticks>>32)^(unsigned int)GetTickCount64()^(sequence*0x9e3779b9u);}
#endif
#ifdef EE_ENABLE_UI
static void select_game_locale(void);
#endif
typedef struct MCM_Api MCM_Api;
struct MCM_Api {
    int version; void* impl;
    void (__cdecl *beginMod)(MCM_Api*,const char*,const char*,const char*,void (__cdecl *)(void));
    void (__cdecl *section)(MCM_Api*,const char*);
    void (__cdecl *toggle)(MCM_Api*,const char*,const char*,const char*,const char*,int,unsigned);
    void (__cdecl *integer)(MCM_Api*,const char*,const char*,const char*,const char*,int,int,int,unsigned);
    void (__cdecl *number)(MCM_Api*,const char*,const char*,const char*,const char*,float,float,float,int,unsigned);
    void (__cdecl *choice)(MCM_Api*,const char*,const char*,const char*,const char*,const char*,const char* const*,const char* const*,int,unsigned);
};
static unsigned int parse_multiplier_tenths(const char* s,unsigned int maximum){unsigned int whole=0,decimal=0,value;int digits=0;if(!s)return 10;while(*s==' '||*s=='\t')++s;while(*s>='0'&&*s<='9'){if(whole>1000)return maximum;whole=whole*10u+(unsigned int)(*s++-'0');digits=1;}if(!digits)return 10;if(*s=='.'){++s;if(*s>='0'&&*s<='9')decimal=(unsigned int)(*s++-'0');while(*s>='0'&&*s<='9')++s;}while(*s==' '||*s=='\t')++s;if(*s)return 10;value=whole*10u+decimal;return value>maximum?maximum:value;}
static void __cdecl reload_settings(void) {
    char m[16];if(settings_path[0])GetPrivateProfileStringA("General","EffectMultiplier","1",m,sizeof(m),settings_path);else{m[0]='1';m[1]=0;}
    all_aptitudes=settings_path[0] && GetPrivateProfileIntA("General","AllAptitudes100",0,settings_path)!=0;
    effect_multiplier=(float)parse_multiplier_tenths(m,50u)/10.0f;
    if(settings_path[0])GetPrivateProfileStringA("General","ExperienceMultiplier","1",m,sizeof(m),settings_path);else{m[0]='1';m[1]=0;}
    xp_tenths=parse_multiplier_tenths(m,200u);
    if(settings_path[0])GetPrivateProfileStringA("General","UIScale","1",m,sizeof(m),settings_path);else{m[0]='1';m[1]=0;}
    ui_scale_tenths=parse_multiplier_tenths(m,20u);if(ui_scale_tenths<8)ui_scale_tenths=8;
    show_trails=!settings_path[0]||GetPrivateProfileIntA("General","ShowWeaponTrails",1,settings_path)!=0;
    show_impacts=!settings_path[0]||GetPrivateProfileIntA("General","ShowImpactEffects",1,settings_path)!=0;
#ifdef EE_ENABLE_UI
    ee_ui_view_set_effect_multiplier(effect_multiplier);
    ee_ui_view_set_scale(ui_scale_tenths);
#endif
}
__declspec(dllexport) void __cdecl MCM_Describe(MCM_Api* api) {
    if(!api || api->version<1 || !api->beginMod || !api->section || !api->toggle || !settings_path[0])return;
#ifdef EE_ENABLE_UI
    select_game_locale();
#endif
    api->beginMod(api,"ElementalEnchantment","Elemental Enchantment",settings_path,reload_settings);
#ifdef EE_ENABLE_UI
    {const EEMcmText* t=ee_mcm_text(ee_locale_get()->id);api->section(api,ee_locale_get()->mcm_section);
     api->toggle(api,"General","AllAptitudes100",ee_locale_get()->mcm_label,ee_locale_get()->mcm_tip,0,0);
     if(api->number)api->number(api,"General","ExperienceMultiplier",t->xp,t->xp_tip,1.0f,0.0f,20.0f,1,0);
     api->section(api,t->section);
     if(api->number)api->number(api,"General","EffectMultiplier",t->strength,t->strength_tip,1.0f,0.0f,5.0f,1,0);
     api->toggle(api,"General","ShowWeaponTrails",t->trails,t->trails_tip,1,0);
     api->toggle(api,"General","ShowImpactEffects",t->impact,t->impact_tip,1,0);
     api->section(api,t->ui_section);
     if(api->number)api->number(api,"General","UIScale",t->ui_scale,t->ui_scale_tip,1.0f,0.8f,2.0f,1,0);}
#else
    api->section(api,"Aptitude");
    api->toggle(api,"General","AllAptitudes100","All elemental aptitudes 100%","All characters gain elemental experience at 100% speed.",0,0);
    if(api->number)api->number(api,"General","ExperienceMultiplier","Elemental XP multiplier","Adjust Elemental XP gain. Aptitudes still apply; 0 stops XP gain.",1.0f,0.0f,20.0f,1,0);
    api->section(api,"Effects");
    if(api->number)api->number(api,"General","EffectMultiplier","Effect strength (x)","0: visuals only; 1: normal; 5: five times.",1.0f,0.0f,5.0f,1,0);
    api->toggle(api,"General","ShowWeaponTrails","Weapon trails","Show weapon trails while attacking.",1,0);
    api->toggle(api,"General","ShowImpactEffects","Impact effects","Show elemental bursts on hits from level 40.",1,0);
    api->section(api,"Interface");
    if(api->number)api->number(api,"General","UIScale","Enchantment panel scale","Change the size of the enchantment panel.",1.0f,0.8f,2.0f,1,0);
#endif
}

static void clear_crossbow_scene(void*);
static void clear_crossbow_character(void*);
static void clear_crossbow_source(void*);
static void clear_crossbow_projectile(void*);
static void impact_clear_scene(void*);
static void impact_clear_parent(void*);
static void impact_queue_character(void*,int,int);
static int impact_queue_position(void*,const Vec*,int,int);
static int crossbow_impact_sample(void*,void**,Vec*);
static void* entity_vtable;
static void* (*entity_mesh)(void*); static const String* (*resource_name)(void*);
static void* (*mesh_bounds)(void*); static const Vec* (*bounds_maximum)(void*);
static void* (*parent_node)(void*); static void* (*node_scene)(void*);
static const Vec* (*tag_position)(void*); static const Quat* (*tag_rotation)(void*);
static void* (*particle_manager)(void); static void* (*particle_template)(void*,const String*);
static void* (*particle_create)(void*,const String*,const String*,void*);
static void (*particle_destroy)(void*,void*,void*); static void (*particle_start)(void*);
static void (*particle_update)(void*,float); static void (*particle_local)(void*,unsigned char);
static U64 (*particle_count)(void*);
static void (*particle_bounds)(void*,unsigned char,float);
static void* (*particle_technique)(void*,U64); static void* (*technique_emitter)(void*,U64); static void (*box_height)(void*,float);
static void (*set_aabb)(void*,const float*); static void (*set_queue)(void*,unsigned char);
static void* (*node_create)(void*,int); static void (*node_destroy)(void*,void*);
static void* (*frost_entity_create)(void*,const void*,int);
static void (*frost_entity_material)(void*,const String*,const String*);
static unsigned char (*frost_get_visible)(void*);
static void (*frost_set_visible)(void*,unsigned char);
static void (*frost_cast_shadows)(void*,unsigned char);
static void (*node_add)(void*,void*); static void (*node_position)(void*,const Vec*);
static void (*node_scale)(void*,const Vec*);
static void (*node_rotation)(void*,float,float,float,float); static void (*node_attach)(void*,void*);
static void* (*original_attach)(void*,const void*,void*,const void*,const void*);
static void (*original_detach)(void*,void*); static void (*original_detach_all)(void*);
static void (*original_clear)(void*); static void (*original_destroy_entity)(void*,void*);
static void (*original_destroy_node)(void*,void*); static void (*original_scene_detach)(void*,void*); static void (*original_gpu)(void*,float);
static void (*original_free_tag)(void*,void*); static void (*original_free_object)(void*,void*);
#ifdef EE_ENABLE_TRAIL
static void* (*trail_root)(void*,int);static void* (*trail_create)(void*);static void (*trail_destroy)(void*,void*);
static void (*trail_add)(void*,void*);static void (*trail_remove)(void*,void*);static void (*trail_colour_change)(void*,U64,float,float,float,float);
static void* (*trail_derive)(void*,Vec*);
static void* trail_get_colour;static void* trail_get_width;static void* trail_get_material;
static void* (*trail_character_combat)(void*);static int (*trail_combat_state)(void*);
static void trail_cleanup(Entry*);
static void trail_tick(Entry*,const Vec*,const Quat*,float);
#endif
static void (*original_character_update)(void*); static void (*original_character_destroy)(void*);
static unsigned char (*character_is_player)(void*); static void* (*character_appearance)(void*); static void* (*appearance_body)(void*);
static const void* (*character_handle)(void*);
static unsigned char (*character_combat)(void*,unsigned char,unsigned char);
static void* (*character_current_weapon)(void*);static int (*weapon_category)(void*);
static void* (*appearance_attached)(void*,const String*); static void* (*appearance_skeleton)(void*); static void* (*skeleton_bone)(void*,const String*);
#ifdef EE_ENABLE_PERSISTENCE
static EEPersistence persistence;
static EEGameDataAPI gamedata_api;
static void* (*character_stats)(void*);
static void (*original_stats_load)(void*,void*);
static void (*original_stats_save)(void*,void*);
static void (*original_stats_destroy)(void*);
static void (*original_character_set_handle)(void*,const void*);
static int (*original_character_hit)(void*,int,void*,void*,void*,int);
static void* (*original_crossbow_wound)(void*,unsigned char,int,void*,int*,void*,int*,void*);
#ifdef ELEMENTAL_NATIVE_TEST
static float (*original_attack_speed)(void*);
static float (*original_block_speed)(void*);
#endif
static float (*original_calculate_combat_speed)(void*,float,float,unsigned char);
#endif
/* Caller holds guard. Entries contain identities only; no stored character is dereferenced. */
static Ally* find_ally(void* body) {int i;if(!body)return 0;for(i=0;i<EE_ALLY_LIMIT;++i)if(allies[i].body==body&&allies[i].character)return &allies[i];return 0;}
#ifdef ELEMENTAL_NATIVE_TEST
static int is_ally_body(void* body) {Ally* a=find_ally(body);return a&&a->element!=0;}
#endif
/* Character::getCurrentWeapon is virtual. 0x3c0 is the slot documented by the
 * same KenshiLib headers used for the verified game build. */
static void* current_weapon_virtual(void* character){void** table=character?*(void***)character:0;return table&&table[0x3c0/sizeof(void*)]?((void* (*)(void*))table[0x3c0/sizeof(void*)])(character):0;}
/* 0: ranged/unknown, 1: empty hands, 2: drawn melee weapon. */
static int melee_context(void* character)
{void* weapon;int category;if(!character||!character_current_weapon||!weapon_category||!character_combat)return 0;weapon=character_current_weapon(character);if(!weapon)return character_combat(character,0,1)?0:1;category=weapon_category(weapon);if(category==5)return character_combat(character,0,1)?0:1;return (category>=0&&category<=4)||category==8?2:0;}
static unsigned int affinity_seed(unsigned int index,unsigned int serial_number) {unsigned int x=index^(serial_number*0x9e3779b9u);x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;if((x>>1)%5u==0)return 4u;if((x>>1)%5u==1)return 5u;return ((x>>1)%3u==0)?3u:((x&1u)?2u:1u);}
/* Temporary test assignment derived from a character handle. It is not saved
 * and may change after an import or other handle reassignment. */
static int element_from_character(void* character) {const unsigned char* h=(const unsigned char*)character_handle(character);if(!h)return 1;return (int)affinity_seed(*(const unsigned int*)(h+0x14),*(const unsigned int*)(h+0x18));}
static Ally* set_ally(void* character,void* body,int is_player,void* left,void* right,int unarmed,int element) {int i,slot=-1;if(!character)return 0;if(is_player&&body)ally_seen_remember(character);for(i=0;i<EE_ALLY_LIMIT;++i){if(allies[i].character==character){slot=i;break;}if(slot<0 && !allies[i].character)slot=i;}if(slot>=0){if(is_player&&body){
    if(allies[slot].character!=character){allies[slot].secondary=0;allies[slot].level=0;allies[slot].hit_glow=0.0f;allies[slot].attacking=0;allies[slot].melee_weapon=0;}
#ifdef EE_ENABLE_PERSISTENCE
    allies[slot].element=element;
#else
    if(allies[slot].character!=character)allies[slot].element=element;
#endif
    allies[slot].character=character;allies[slot].body=body;allies[slot].left=left;allies[slot].right=right;allies[slot].unarmed=unarmed;return &allies[slot];}else if(allies[slot].character==character){allies[slot].character=0;allies[slot].body=0;allies[slot].left=0;allies[slot].right=0;allies[slot].unarmed=0;allies[slot].element=0;}}return 0;}
static void forget_body(void* body) {int i;for(i=0;i<EE_ALLY_LIMIT;++i)if(allies[i].body==body){allies[i].character=0;allies[i].body=0;}}
typedef struct {const char* message;U64 tick;} LogLimit;
static LogLimit log_limits[64];static void* log_guard;
static void logline(const char* s) {
    DWORD n=0,w;void* f;unsigned i,slot=(unsigned)(((U64)s>>4)&63u);U64 now=GetTickCount64();int allowed=0;
    AcquireSRWLockExclusive(&log_guard);
    for(i=0;i<64;++i){LogLimit* entry=&log_limits[(slot+i)&63u];if(!entry->message||entry->message==s){if(!entry->message||now-entry->tick>=60000){entry->message=s;entry->tick=now;allowed=1;}break;}}
    ReleaseSRWLockExclusive(&log_guard);if(!allowed)return;
    while(s[n])++n;f=CreateFileA(log_path,4,3,0,4,0x80,0);if(f!=(void*)(U64)-1){WriteFile(f,s,n,&w,0);CloseHandle(f);}
}
/* Cache profile IDs for loaded characters. Caller holds guard; cached pointer
 * values are only compared, never dereferenced. */
typedef struct { void* character; unsigned int index, generation; int boss_profile; } BossDiagSeen;
static BossDiagSeen boss_diag_seen[4096];
static unsigned int boss_diag_count;
static int boss_diag_character(void* character) {
    const unsigned char* h; void* data; const String* sid; unsigned int i,index,generation,slot;
    int profile=0;
    if(!character)return 0;
    h=(const unsigned char*)character_handle(character); if(!h)return 0;
    index=*(const unsigned int*)(h+0x14); generation=*(const unsigned int*)(h+0x18);
    slot=(index^(generation*2654435761u))&4095u;
    for(i=0;i<64;++i) { BossDiagSeen* seen=&boss_diag_seen[(slot+i)&4095u];
        if(seen->character==character && seen->index==index && seen->generation==generation)return seen->boss_profile;
        if(!seen->character)break;
    }
    if(i==64||boss_diag_count>=3072){for(i=0;i<4096;++i)boss_diag_seen[i].character=0;boss_diag_count=0;i=0;}
    slot=(slot+i)&4095u;
    data=*(void**)((unsigned char*)character+0x40); if(!data)return 0;
    sid=(const String*)((unsigned char*)data+0x58);
    if(!sid->size || sid->size>160 || sid->capacity<sid->size)return 0;
    { const char* text=sid->capacity<16?sid->text.small:sid->text.heap;
      unsigned int j;
      for(j=0;text && j<ee_boss_profile_count();++j) {
        const EEBossProfile* boss_entry=ee_boss_profile_at(j);const char* target=boss_entry?boss_entry->id:0;
        for(i=0;i<sid->size && target[i] && text[i]==target[i];++i){}
        if(i==sid->size && !target[i]){ profile=(int)j+1; break; }
      }
    }
    boss_diag_seen[slot].character=character;boss_diag_seen[slot].index=index;boss_diag_seen[slot].generation=generation;boss_diag_seen[slot].boss_profile=profile;++boss_diag_count;
    return profile;
}
#ifdef EE_ENABLE_TRAIL
#include "elemental_trail_runtime.inl"
#endif
static int eq(const String* s,const char* text) { U64 i; const char* p; if(!s || s->size>1024 || s->capacity<s->size)return 0; p=s->capacity<16?s->text.small:s->text.heap; if(!p)return 0; for(i=0;i<s->size && text[i];++i)if(p[i]!=text[i])return 0; return i==s->size && !text[i]; }
static int has(const String* s,const char* part) { U64 i,j;const char* p; if(!s || !s->size || s->size>1024 || s->capacity<s->size)return 0;p=s->capacity<16?s->text.small:s->text.heap;if(!p)return 0;for(i=0;i<s->size;++i) {for(j=0;part[j] && i+j<s->size;++j) {char x=p[i+j];if(x>='A' && x<='Z')x+='a'-'A';if(x!=part[j])break;}if(!part[j])return 1;}return 0; }
static int blade_name(const String* s) {return has(s,"katana")||has(s,"sword")||has(s,"sabre")||has(s,"blade");}
static void frost_clear(Entry* e){void* obj=e->frost;void* scene=e->frost_scene;void* node=e->frost_node;int attached=e->frost_attached;e->frost=0;e->frost_scene=0;e->frost_node=0;e->frost_attached=0;if(obj&&scene){if(attached&&node)original_scene_detach(node,obj);original_destroy_entity(scene,obj);}if(node&&scene)node_destroy(scene,node);}
static void frost_update(Entry* e,int wanted){
    void* mesh;void* obj;const Vec* p;const Quat* q;String material={{"EE_IceFrost"},11,15,0},group={{"Autodetect"},10,15,0};
    if(!wanted){if(e->frost||e->frost_node)frost_clear(e);return;}
    __try {
        p=e->tag?tag_position(e->tag):0;q=e->tag?tag_rotation(e->tag):0;
        if(!p||!q)return;
        if(e->frost){if(frost_get_visible&&frost_set_visible){unsigned char visible=frost_get_visible(e->blade);if(frost_get_visible(e->frost)!=visible)frost_set_visible(e->frost,visible);}if(e->frost_node){node_position(e->frost_node,p);node_rotation(e->frost_node,q->w,q->x,q->y,q->z);}return;}
        if(!e->frost_bone||!e->scene||!e->parent||!frost_entity_create||!frost_entity_material||!frost_get_visible||!frost_set_visible||!frost_cast_shadows||!original_scene_detach)return;
        mesh=entity_mesh(e->blade);if(!mesh||!*(void**)mesh)return;
        e->frost_node=node_create(e->scene,0);if(!e->frost_node)return;e->frost_scene=e->scene;
        node_add(e->parent,e->frost_node);node_position(e->frost_node,p);node_rotation(e->frost_node,q->w,q->x,q->y,q->z);
        obj=frost_entity_create(e->scene,mesh,0);if(!obj){frost_clear(e);return;}
        e->frost=obj;
        frost_entity_material(obj,&material,&group);
        frost_cast_shadows(obj,0);node_attach(e->frost_node,obj);e->frost_attached=1;frost_set_visible(obj,frost_get_visible(e->blade));
    } __except(1) {__try {frost_clear(e);} __except(1) {e->frost=0;e->frost_scene=0;e->frost_node=0;e->frost_attached=0;}e->frost_bone=0;logline("FROST: disabled for an unsupported weapon.\r\n");}
}
static void cleanup(Entry* e) {
    if(e->frost||e->frost_node)frost_clear(e);
#ifdef EE_ENABLE_TRAIL
    trail_cleanup(e);
#endif
    if(e->effect2 && e->scene)particle_destroy(particle_manager(),e->effect2,e->scene);if(e->effect && e->scene)particle_destroy(particle_manager(),e->effect,e->scene);if(e->node && e->scene)node_destroy(e->scene,e->node);e->effect2=0;e->secondary=0;e->effect=0;e->node=0;e->scene=0;e->parent=0;e->element=0;e->applied_scale=0.0f;}
static void erase(Entry* e) {cleanup(e);e->owner=0;e->blade=0;e->tag=0;}
static void clear_hand(HandEffect* h) {if(h->effect2 && h->scene)particle_destroy(particle_manager(),h->effect2,h->scene);if(h->effect && h->scene)particle_destroy(particle_manager(),h->effect,h->scene);if(h->node && h->scene)node_destroy(h->scene,h->node);h->character=0;h->body=0;h->bone=0;h->scene=0;h->parent=0;h->node=0;h->effect=0;h->effect2=0;h->secondary=0;h->element=0;h->applied_scale=0.0f;}
static void clear_character_hands(void* character) {int i;if(!character)return;for(i=0;i<EE_ALLY_LIMIT*2;++i)if(hand_effects[i].character==character)clear_hand(&hand_effects[i]);}
static void* attach_hook(void* owner,const void* bone,void* object,const void* orientation,const void* offset) {
    void* tag=original_attach(owner,bone,object,orientation,offset);int i,pole;void** mesh;void* box;const Vec* max;float length;
    if(!active||internal==GetCurrentThreadId()||!tag||!object)return tag;
    __try {if(*(void**)object!=entity_vtable)return tag;
        mesh=(void**)entity_mesh(object);if(!mesh || !*mesh)return tag;
        if(!eq((const String*)bone,"Bip01 Prop2") && !eq((const String*)bone,"Bip01 Prop1") &&
           !(eq((const String*)bone,"Bip01 L Hand") && (has(resource_name(*mesh),"_bare") || blade_name(resource_name(*mesh)))))return tag;
        if(has(resource_name(*mesh),"sheath") || has(resource_name(*mesh),"crossbow") || has(resource_name(*mesh),"arcane_focus") || has(resource_name(*mesh),"holster"))return tag;
        box=mesh_bounds(*mesh);max=box?bounds_maximum(box):0;length=max?max->y:10.0f;pole=has(resource_name(*mesh),"polearm")||has(resource_name(*mesh),"spear");
        AcquireSRWLockExclusive(&guard);for(i=0;i<EE_TRACK_LIMIT;++i)if(entries[i].blade==object)break;
        if(i==EE_TRACK_LIMIT){for(i=0;i<EE_TRACK_LIMIT;++i)if(!entries[i].blade){entries[i].owner=owner;entries[i].blade=object;entries[i].tag=tag;entries[i].born=GetTickCount64();entries[i].length=(length>2 && length<80)?length:10.0f;entries[i].pole=pole;entries[i].frost_bone=eq((const String*)bone,"Bip01 Prop1")?1:eq((const String*)bone,"Bip01 Prop2")?2:3;if(orientation&&offset){entries[i].frost_rotation=*(const Quat*)orientation;entries[i].frost_offset=*(const Vec*)offset;}else entries[i].frost_bone=0;break;}if(i==EE_TRACK_LIMIT && !logged_full){logged_full=1;logline("CAPACITY: all weapon tracking slots are in use.\r\n");}}
        ReleaseSRWLockExclusive(&guard);
    } __except(1) {logline("SKIP: blade metadata unavailable.\r\n");} return tag;
}
static void detach_hook(void* owner,void* blade) {int i;if(internal==GetCurrentThreadId()){original_detach(owner,blade);return;}AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {for(i=0;i<EE_TRACK_LIMIT;++i)if(entries[i].owner==owner && entries[i].blade==blade)erase(&entries[i]);} __finally {internal=0;ReleaseSRWLockExclusive(&guard);}original_detach(owner,blade);}
static void detach_all_hook(void* owner) {int i;if(internal==GetCurrentThreadId()){original_detach_all(owner);return;}AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {for(i=0;i<EE_TRACK_LIMIT;++i)if(entries[i].owner==owner)erase(&entries[i]);} __finally {internal=0;ReleaseSRWLockExclusive(&guard);}original_detach_all(owner);}
static void clear_hook(void* scene) {int i;if(internal==GetCurrentThreadId()){original_clear(scene);return;}AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {impact_clear_scene(scene);clear_crossbow_scene(scene);for(i=0;i<EE_TRACK_LIMIT;++i)if(entries[i].blade && (!entries[i].scene || entries[i].scene==scene))erase(&entries[i]);for(i=0;i<EE_ALLY_LIMIT*2;++i)if(hand_effects[i].scene==scene)clear_hand(&hand_effects[i]);for(i=0;i<EE_ALLY_LIMIT;++i){allies[i].character=0;allies[i].body=0;}} __finally {internal=0;ReleaseSRWLockExclusive(&guard);}original_clear(scene);}
static void entity_hook(void* scene,void* object) {int i;if(internal==GetCurrentThreadId()){original_destroy_entity(scene,object);return;}AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {for(i=0;i<EE_TRACK_LIMIT;++i)if(entries[i].owner==object || entries[i].blade==object)erase(&entries[i]);for(i=0;i<EE_ALLY_LIMIT*2;++i)if(hand_effects[i].body==object)clear_hand(&hand_effects[i]);forget_body(object);} __finally {internal=0;ReleaseSRWLockExclusive(&guard);}original_destroy_entity(scene,object);}
static void node_hook(void* scene,void* node) {int i;if(internal==GetCurrentThreadId()){original_destroy_node(scene,node);return;}AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {impact_clear_parent(node);clear_crossbow_source(node);for(i=0;i<EE_TRACK_LIMIT;++i)if(entries[i].parent==node)erase(&entries[i]);for(i=0;i<EE_ALLY_LIMIT*2;++i)if(hand_effects[i].parent==node)clear_hand(&hand_effects[i]);} __finally {internal=0;ReleaseSRWLockExclusive(&guard);}original_destroy_node(scene,node);}
static void scene_detach_hook(void* node,void* object) {int i;if(internal==GetCurrentThreadId()){original_scene_detach(node,object);return;}AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {for(i=0;i<EE_TRACK_LIMIT;++i)if(entries[i].owner==object)cleanup(&entries[i]);for(i=0;i<EE_ALLY_LIMIT*2;++i)if(hand_effects[i].body==object)clear_hand(&hand_effects[i]);} __finally {internal=0;ReleaseSRWLockExclusive(&guard);}original_scene_detach(node,object);}
static void free_tag_hook(void* skeleton,void* tag) {int i;if(internal==GetCurrentThreadId()){original_free_tag(skeleton,tag);return;}AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {for(i=0;i<EE_TRACK_LIMIT;++i)if(entries[i].tag==tag && tag)erase(&entries[i]);} __finally {internal=0;ReleaseSRWLockExclusive(&guard);}original_free_tag(skeleton,tag);}
static void free_object_hook(void* skeleton,void* object) {int i;if(internal==GetCurrentThreadId()){original_free_object(skeleton,object);return;}AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {for(i=0;i<EE_TRACK_LIMIT;++i)if(entries[i].blade==object && object)erase(&entries[i]);} __finally {internal=0;ReleaseSRWLockExclusive(&guard);}original_free_object(skeleton,object);}
#ifdef EE_ENABLE_PERSISTENCE
static int failed_save_read(void* context,int out[3]) {(void)context;(void)out;return -1;}
/* Caller holds guard. Player saves stay untouched; non-player profiles are
 * transient and never inserted into persistence or awarded experience. */
static int combat_affinity_locked(void* character,void* stats,EEAffinity* out)
{
    const unsigned char* h; unsigned int index,serial_number; int boss;
    if(!character||!out)return 0;
    if(character_is_player(character)) {
        EECharacterRecord* r;
        h=(const unsigned char*)character_handle(character);if(!stats||!h)return 0;
        index=*(const unsigned int*)(h+0x14);serial_number=*(const unsigned int*)(h+0x18);
        r=ee_persistence_bind(&persistence,stats,character,index,serial_number,index^(serial_number*0x9e3779b9u),(unsigned int)element_from_character(character));
        if(!r||r->read_only||!r->affinity.selected)return 0;
        *out=r->affinity;return 1;
    }
    boss=boss_diag_character(character);if(!boss)return 0;
    { const EEBossProfile* p=ee_boss_profile_at((unsigned int)(boss-1));unsigned int l;if(!p)return 0;l=(unsigned int)p->level;
      if(!ee_affinity_create(out,0,(unsigned int)p->primary))return 0;
      out->xp[0]=(unsigned short)(4u*(20u*l+l*(l-1u)));
      out->secondary=(unsigned int)p->secondary;
    }
    return out->selected!=0;
}
static float element_speed(void* stats,float base,int attacking)
{
    float result=base;if(!active||!stats||internal==GetCurrentThreadId())return base;
    __try {void* character=*(void**)((unsigned char*)stats+0x10);
        if(character&&melee_context(character)){EEAffinity a;AcquireSRWLockExclusive(&guard);
            __try {if(combat_affinity_locked(character,stats,&a))result=attacking?ee_affinity_attack_speed_scaled(&a,base,effect_multiplier):ee_affinity_combat_speed_scaled(&a,base,effect_multiplier);}
            __finally {ReleaseSRWLockExclusive(&guard);}
        }
    } __except(1) {logline("WIND: speed bonus skipped after native error.\r\n");}
    return result;
}
static float calculate_combat_speed_hook(void* stats,float weight,float skill,unsigned char is_block){float base=original_calculate_combat_speed(stats,weight,skill,is_block);return element_speed(stats,base,is_block?0:1);}
#ifdef ELEMENTAL_NATIVE_TEST
static float attack_speed_hook(void* stats){float base=original_attack_speed(stats);return element_speed(stats,base,1);}
static float block_speed_hook(void* stats){float base=original_block_speed(stats);return element_speed(stats,base,0);}
#endif
static int character_hit_hook(void* target,int direction,void* damage,void* attacker,void* technique,int combo)
{
    int result,has_bonus=0;EEDamage original,modified,bonus;
    if(active&&damage&&attacker&&target&&attacker!=target&&internal!=GetCurrentThreadId())__try {
        if(melee_context(attacker)){void* stats=character_stats(attacker);EEAffinity a;AcquireSRWLockExclusive(&guard);
            __try {if(combat_affinity_locked(attacker,stats,&a)){original=*(EEDamage*)damage;modified=original;has_bonus=ee_affinity_damage_scaled(&a,&modified,effect_multiplier);}}
            __finally {ReleaseSRWLockExclusive(&guard);}
        }
    } __except(1) {has_bonus=0;logline("EFFECT: hit bonus skipped after native error.\r\n");}
    if(has_bonus){bonus.cut=modified.cut-original.cut;bonus.blunt=modified.blunt-original.blunt;bonus.pierce=modified.pierce-original.pierce;bonus.stun=modified.stun-original.stun;bonus.bleed=modified.bleed-original.bleed;bonus.armour=modified.armour-original.armour;
        result=original_character_hit(target,direction,&modified,attacker,technique,combo);
        __try {EEDamage* d=(EEDamage*)damage;d->cut=modified.cut-bonus.cut;d->blunt=modified.blunt-bonus.blunt;d->pierce=modified.pierce-bonus.pierce;d->stun=modified.stun-bonus.stun;d->bleed=modified.bleed-bonus.bleed;d->armour=modified.armour-bonus.armour;} __except(1) {logline("EFFECT: output copy skipped after native error.\r\n");}
    }else result=original_character_hit(target,direction,damage,attacker,technique,combo);
    if(active&&attacker&&target&&attacker!=target&&(result==1||result==2||result==6)&&internal!=GetCurrentThreadId())__try {
        if(character_is_player(attacker)&&melee_context(attacker)){void* stats=character_stats(attacker);const unsigned char* h=(const unsigned char*)character_handle(attacker);
            if(stats&&h){unsigned int index=*(const unsigned int*)(h+0x14),serial=*(const unsigned int*)(h+0x18);AcquireSRWLockExclusive(&guard);
                __try {EECharacterRecord* r=ee_persistence_bind(&persistence,stats,attacker,index,serial,index^(serial*0x9e3779b9u),(unsigned int)element_from_character(attacker));if(r&&!r->read_only&&ee_affinity_award_hit_rate_scaled(&r->affinity,all_aptitudes,xp_tenths)){int k;if(show_impacts&&ee_affinity_level(&r->affinity,0)>=40){impact_queue_character(target,(int)r->affinity.selected,(int)r->affinity.secondary);for(k=0;k<EE_ALLY_LIMIT;++k)if(allies[k].character==attacker){allies[k].hit_glow=.18f;break;}}}}
                __finally {ReleaseSRWLockExclusive(&guard);}
            }
        }
    } __except(1) {logline("XP: hit ignored after native error.\r\n");}
    return result;
}
static void* crossbow_wound_hook(void* medical,unsigned char low,int direction,void* damage,int* material,void* attacker,int* attackdir,void* bolt)
{
    int applied=0,impact_element=0,impact_secondary=0;EEDamage modified,original,bonus;void* result;void* stats=0;void* impact_scene=0;Vec impact_position={0};unsigned int index=0,serial_number=0;
    if(active&&bolt&&damage&&attacker&&internal!=GetCurrentThreadId())__try {
        {int player=character_is_player(attacker);const unsigned char* h=player?(const unsigned char*)character_handle(attacker):0;void* character_stat=character_stats(attacker);EEAffinity a;
            if(player&&character_stat&&h){stats=character_stat;index=*(const unsigned int*)(h+0x14);serial_number=*(const unsigned int*)(h+0x18);}
            AcquireSRWLockExclusive(&guard);
            __try {if(combat_affinity_locked(attacker,character_stat,&a)){float pierce;if(show_impacts&&player&&ee_affinity_level(&a,0)>=40&&crossbow_impact_sample(bolt,&impact_scene,&impact_position)){impact_element=(int)a.selected;impact_secondary=(int)a.secondary;}original=*(EEDamage*)damage;modified=original;pierce=modified.pierce>0.0f?modified.pierce:0.0f;modified.cut+=pierce;applied=ee_affinity_damage_scaled(&a,&modified,effect_multiplier);modified.cut-=pierce;}}
            __finally {ReleaseSRWLockExclusive(&guard);}
        }
    } __except(1){applied=0;stats=0;logline("CROSSBOW: damage skipped after error.\r\n");}
    if(applied){bonus.cut=modified.cut-original.cut;bonus.blunt=modified.blunt-original.blunt;bonus.pierce=modified.pierce-original.pierce;bonus.stun=modified.stun-original.stun;bonus.bleed=modified.bleed-original.bleed;bonus.armour=modified.armour-original.armour;
        result=original_crossbow_wound(medical,low,direction,&modified,material,attacker,attackdir,bolt);
        __try {EEDamage* d=(EEDamage*)damage;d->cut=modified.cut-bonus.cut;d->blunt=modified.blunt-bonus.blunt;d->pierce=modified.pierce-bonus.pierce;d->stun=modified.stun-bonus.stun;d->bleed=modified.bleed-bonus.bleed;d->armour=modified.armour-bonus.armour;}__except(1){}
    }else result=original_crossbow_wound(medical,low,direction,damage,material,attacker,attackdir,bolt);
    if(bolt&&active&&internal!=GetCurrentThreadId()){AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try{void* live_scene=0;Vec live_position;if(result&&impact_scene&&impact_element&&crossbow_impact_sample(bolt,&live_scene,&live_position)&&live_scene==impact_scene)impact_queue_position(live_scene,&live_position,impact_element,impact_secondary);clear_crossbow_projectile(bolt);}__finally{internal=0;ReleaseSRWLockExclusive(&guard);}}
    if(result&&stats&&attacker&&bolt&&active)__try {AcquireSRWLockExclusive(&guard);
        __try {EECharacterRecord* r=ee_persistence_bind(&persistence,stats,attacker,index,serial_number,index^(serial_number*0x9e3779b9u),(unsigned int)element_from_character(attacker));if(r&&!r->read_only)ee_affinity_award_hit_rate_scaled(&r->affinity,all_aptitudes,xp_tenths);}
        __finally {ReleaseSRWLockExclusive(&guard);}
    }__except(1){logline("CROSSBOW: hit XP skipped after error.\r\n");}
    return result;
}
static void stats_load_hook(void* stats,void* data) {
    if(active&&stats&&data&&internal!=GetCurrentThreadId())__try {EEGameDataContext c={data,&gamedata_api};EESaveBackend b={&c,ee_gamedata_read,ee_gamedata_write};int status;
        AcquireSRWLockExclusive(&guard);__try {status=ee_persistence_stage(&persistence,stats,&b);} __finally {ReleaseSRWLockExclusive(&guard);}
        if(status==EE_SAVE_UNSUPPORTED)logline("SAVE: unknown affinity version retained without changes.\r\n");
    } __except(1) {
        /* A fault while reading must not turn unknown saved data into a new,
         * writable default. Stage a read-only sentinel without touching data. */
        EESaveBackend failed={0,failed_save_read,0};
        AcquireSRWLockExclusive(&guard);
        __try {ee_persistence_stage(&persistence,stats,&failed);} __finally {ReleaseSRWLockExclusive(&guard);}
        logline("SAVE: affinity read failed; preserving data as read-only.\r\n");
    }
    original_stats_load(stats,data);
}
static void stats_save_hook(void* stats,void* data) {
    original_stats_save(stats,data);
    if(active&&stats&&data&&internal!=GetCurrentThreadId())__try {void* character=*(void**)((unsigned char*)stats+0x10);const unsigned char* h=character?(const unsigned char*)character_handle(character):0;
        if(h){EEGameDataContext c={data,&gamedata_api};EESaveBackend b={&c,ee_gamedata_read,ee_gamedata_write};int status;
            AcquireSRWLockExclusive(&guard);__try {status=ee_persistence_save(&persistence,character,*(const unsigned int*)(h+0x14),*(const unsigned int*)(h+0x18),&b);} __finally {ReleaseSRWLockExclusive(&guard);}
            if(status==EE_SAVE_ERROR||status==EE_SAVE_UNSUPPORTED)logline("SAVE: affinity was not written.\r\n");
        }
    } __except(1) {logline("SAVE: skipped affinity write after error.\r\n");}
}
static void stats_destroy_hook(void* stats) {if(internal!=GetCurrentThreadId()){AcquireSRWLockExclusive(&guard);ee_persistence_forget(&persistence,stats,0);ReleaseSRWLockExclusive(&guard);}original_stats_destroy(stats);}
static void character_set_handle_hook(void* character,const void* next) {
    int started=0;unsigned int old_index=0,old_serial=0;const unsigned char* h;
    if(active&&character&&internal!=GetCurrentThreadId())__try {h=(const unsigned char*)character_handle(character);if(h){old_index=*(const unsigned int*)(h+0x14);old_serial=*(const unsigned int*)(h+0x18);AcquireSRWLockExclusive(&guard);started=ee_character_handle_begin(&persistence.characters,character,old_index,old_serial);ReleaseSRWLockExclusive(&guard);}} __except(1) {started=0;}
    original_character_set_handle(character,next);
    if(started){unsigned int index=old_index,serial_number=old_serial;__try {h=(const unsigned char*)character_handle(character);if(h){index=*(const unsigned int*)(h+0x14);serial_number=*(const unsigned int*)(h+0x18);}} __except(1) {logline("SAVE: handle continuity check failed.\r\n");}AcquireSRWLockExclusive(&guard);ee_character_handle_end(&persistence.characters,character,index,serial_number);ReleaseSRWLockExclusive(&guard);}
}
#endif
static void character_update_hook(void* character) {
    void* app=0;void* body=0;void* skeleton=0;void* left=0;void* right=0;int player=0,unarmed=0,element=1;
    int secondary=0,level=0,melee=0,boss=0,skip_untracked=0;
#ifdef EE_ENABLE_TRAIL
    int attacking=0;
#endif
#ifdef EE_ENABLE_PERSISTENCE
    void* stats=0;unsigned int index=0,serial_number=0;int has_handle=0;
#endif
    String hands={{"hands"},5,15,0},lb={{"Bip01 L Hand"},12,15,0},rb={{"Bip01 R Hand"},12,15,0};
    if(active && internal!=GetCurrentThreadId())__try {
        player=character_is_player(character);
        if(!player){AcquireSRWLockExclusive(&guard);__try {__try {boss=boss_diag_character(character);} __except(1) {boss=0;}skip_untracked=!boss&&!ally_seen_maybe(character);} __finally {ReleaseSRWLockExclusive(&guard);}}
        if(!skip_untracked){
        if(player||boss){const EEBossProfile* profile=boss?ee_boss_profile_at((unsigned int)(boss-1)):0;element=player?element_from_character(character):(profile?profile->primary:1);melee=melee_context(character);
#ifdef EE_ENABLE_PERSISTENCE
            {const unsigned char* h=(const unsigned char*)character_handle(character);if(h){index=*(const unsigned int*)(h+0x14);serial_number=*(const unsigned int*)(h+0x18);has_handle=1;}stats=character_stats(character);}
#endif
            app=character_appearance(character);if(app){body=appearance_body(app);unarmed=melee==1 && character_combat(character,1,0) && !appearance_attached(app,&hands);if(unarmed){skeleton=appearance_skeleton(app);if(skeleton){left=skeleton_bone(skeleton,&lb);right=skeleton_bone(skeleton,&rb);}}
            }
#ifdef EE_ENABLE_TRAIL
            if(trail_character_combat&&trail_combat_state&&character_combat(character,1,0)){void* combat=trail_character_combat(character);if(combat)attacking=trail_combat_state(combat)==0;}
#endif
        }
        AcquireSRWLockExclusive(&guard);
        __try {
            if(boss){const EEBossProfile* profile=ee_boss_profile_at((unsigned int)(boss-1));if(profile){secondary=profile->secondary;level=profile->level;}}
#ifdef EE_ENABLE_PERSISTENCE
            if(player&&body&&has_handle){EECharacterRecord* r=ee_persistence_bind(&persistence,stats,character,index,serial_number,index^(serial_number*0x9e3779b9u),(unsigned int)element);if(r){element=(int)r->affinity.selected;if(!r->read_only){secondary=(int)r->affinity.secondary;level=(int)ee_affinity_level(&r->affinity,0);}}}
#endif
            {Ally* ally=set_ally(character,body,player||boss,left,right,unarmed,element);
            if(ally){ally->secondary=secondary;ally->level=level;ally->melee_weapon=melee==2;
#ifdef EE_ENABLE_TRAIL
                ally->attacking=attacking;
#endif
            }}
        } __finally {ReleaseSRWLockExclusive(&guard);}}
    } __except(1) {logline("SKIP: player identity or hand bones unavailable.\r\n");}original_character_update(character);
}
static void character_destroy_hook(void* character) {if(internal!=GetCurrentThreadId()){AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {clear_crossbow_character(character);clear_character_hands(character);set_ally(character,0,0,0,0,0,0);
#ifdef EE_ENABLE_PERSISTENCE
    ee_persistence_forget(&persistence,0,character);
#endif
    } __finally {internal=0;ReleaseSRWLockExclusive(&guard);}}original_character_destroy(character);}
#ifdef EE_ENABLE_UI
#include "elemental_ui_runtime.inl"
#endif
static void* create_secondary_effect(void* manager,void* scene,void* node,const String* tmpl,float height)
{
    String name={{"EE_S00000000"},12,15,0};unsigned int n=(unsigned int)++serial,j;void* effect;void* tech;void* emitter;
    if(!scene||!node||!particle_template(manager,tmpl))return 0;
    for(j=0;j<8;++j)name.text.small[4+j]="0123456789abcdef"[(n>>(28-j*4))&15];
    effect=particle_create(manager,&name,tmpl,scene);if(!effect)return 0;
    if(height>0.0f){tech=particle_technique(effect,0);emitter=tech?technique_emitter(tech,0):0;if(!emitter){particle_destroy(manager,effect,scene);return 0;}box_height(emitter,height);if(tmpl->size==27||tmpl->size==23){tech=particle_technique(effect,1);emitter=tech?technique_emitter(tech,0):0;if(emitter)box_height(emitter,height);}if(tmpl->size==23){tech=particle_technique(effect,2);emitter=tech?technique_emitter(tech,0):0;if(emitter)box_height(emitter,height);}}
    node_attach(node,effect);particle_local(effect,1);set_queue(effect,84);particle_start(effect);particle_bounds(effect,0,0);return effect;
}
#include "elemental_impact_visual.inl"
#include "elemental_crossbow_visual.inl"
static float visual_scale(int level,float glow)
{float scale=level>=100?1.15f:(level>=80?1.1f:(level>=40?1.0f:(level>=20?.9f:.8f)));return glow>0.0f?scale*1.3f:scale;}
static const String* element_template(int element,const String* fire,const String* lightning,const String* wind,const String* water,const String* ice)
{
    switch(element){case 5:return ice;case 4:return water;case 3:return wind;case 2:return lightning;default:return fire;}
}
static void update_hands(void* manager,float dt) {
    int i,k;const float bounds[6]={0,0,0,20,20,20};String fire={{0},21,21,0},light={{0},26,26,0},wind={{0},21,21,0},water={{0},22,22,0},ice={{0},20,20,0};fire.text.heap="SB_Prototype_HandFire";light.text.heap="SB_Prototype_LightningHand";wind.text.heap="SB_Prototype_WindHand";water.text.heap="SB_Prototype_WaterHand";ice.text.heap="SB_Prototype_IceHand";
    for(i=0;i<EE_ALLY_LIMIT;++i)for(k=0;k<2;++k){Ally* a=&allies[i];HandEffect* h=&hand_effects[i*2+k];void* bone=k?a->right:a->left;void* parent;const Vec* p;const Quat* q;
        if(!a->character||!a->body||!a->unarmed||!a->element||!bone){if(h->effect)clear_hand(h);continue;}
        parent=parent_node(a->body);if(!parent){if(h->effect)clear_hand(h);continue;}
        if(h->effect && (h->character!=a->character||h->body!=a->body||h->bone!=bone||h->parent!=parent||h->element!=a->element))clear_hand(h);
        if(!h->effect){String name={{"EE_H00000000"},12,15,0};unsigned int n=(unsigned int)++serial,j;h->character=a->character;h->body=a->body;h->bone=bone;h->parent=parent;h->scene=node_scene(parent);if(!h->scene){clear_hand(h);continue;}
            const String* tmpl=element_template(a->element,&fire,&light,&wind,&water,&ice);if(!particle_template(manager,tmpl)){clear_hand(h);continue;}h->element=a->element;for(j=0;j<8;++j)name.text.small[4+j]="0123456789abcdef"[(n>>(28-j*4))&15];h->effect=particle_create(manager,&name,tmpl,h->scene);h->node=node_create(h->scene,0);if(!h->effect||!h->node){clear_hand(h);continue;}
            node_add(parent,h->node);node_attach(h->node,h->effect);particle_local(h->effect,1);set_queue(h->effect,84);particle_start(h->effect);particle_bounds(h->effect,0,0);
        }
        if(h->effect2&&h->secondary!=a->secondary){particle_destroy(manager,h->effect2,h->scene);h->effect2=0;h->secondary=0;}
        if(a->secondary&&!h->effect2){const String* second=element_template(a->secondary,&fire,&light,&wind,&water,&ice);h->effect2=create_secondary_effect(manager,h->scene,h->node,second,0);if(h->effect2)h->secondary=a->secondary;}
        {float scale=visual_scale(a->level,a->hit_glow);if(scale!=h->applied_scale){Vec size={scale,scale,scale};node_scale(h->node,&size);h->applied_scale=scale;}}
        p=tag_position(bone);q=tag_rotation(bone);if(p&&q){node_position(h->node,p);node_rotation(h->node,q->w,q->x,q->y,q->z);}particle_update(h->effect,dt);set_aabb(h->effect,bounds);
        if(h->effect2){particle_update(h->effect2,dt);set_aabb(h->effect2,bounds);}
    }
}
static void update(float dt) {
    int i; void* manager; const float bounds[6]={0,0,0,60,60,60};
    String fire={{0},17,17,0},light={{0},27,27,0},wind={{0},22,22,0},water={{0},23,23,0},ice={{0},21,21,0};fire.text.heap="SB_Prototype_Fire";light.text.heap="SB_Prototype_LightningBlade";wind.text.heap="SB_Prototype_WindBlade";water.text.heap="SB_Prototype_WaterBlade";ice.text.heap="SB_Prototype_IceBlade";
    if(dt<0 || dt>.1f)dt=.016f;manager=particle_manager();if(!manager || !particle_template(manager,&fire))return;
    AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();
    __try {for(i=0;i<EE_TRACK_LIMIT;++i)if(entries[i].blade) {
        Entry* e=&entries[i];Ally* ally=find_ally(e->owner);void* parent;const Vec* p;const Quat* q;int element=ally?ally->element:0,secondary=ally?ally->secondary:0;const String* tmpl=element_template(element,&fire,&light,&wind,&water,&ice);e->level=ally?ally->level:0;
        if(!element||!ally->melee_weapon){if(e->effect)cleanup(e);if(!ally&&GetTickCount64()-e->born>5000)erase(e);continue;}if(e->effect && e->element!=element)cleanup(e);
        parent=parent_node(e->owner);
        if(!parent) {cleanup(e);continue;}if(e->parent && e->parent!=parent)cleanup(e);
        if(!e->effect) {String name={{"EE_F00000000"},12,15,0};unsigned int n=(unsigned int)++serial,j;
            e->scene=node_scene(parent);if(!e->scene)continue;e->parent=parent;
            for(j=0;j<8;++j)name.text.small[4+j]="0123456789abcdef"[(n>>(28-j*4))&15];
            if(!particle_template(manager,tmpl)){cleanup(e);continue;}e->element=element;e->effect=particle_create(manager,&name,tmpl,e->scene);e->node=node_create(e->scene,0);
            if(!e->effect || !e->node){cleanup(e);continue;}
            {float start=e->length*(e->pole?.45f:.12f),end=e->length*.98f;void* tech=particle_technique(e->effect,0);void* emitter=tech?technique_emitter(tech,0):0;if(!emitter){cleanup(e);continue;}box_height(emitter,end-start);if(element==2||element==4){tech=particle_technique(e->effect,1);emitter=tech?technique_emitter(tech,0):0;if(emitter)box_height(emitter,end-start);}if(element==4){tech=particle_technique(e->effect,2);emitter=tech?technique_emitter(tech,0):0;if(emitter)box_height(emitter,end-start);}node_add(parent,e->node);node_attach(e->node,e->effect);particle_local(e->effect,1);}
            set_queue(e->effect,84);particle_start(e->effect);particle_bounds(e->effect,0,0);
        }
        if(e->effect2&&e->secondary!=secondary){particle_destroy(manager,e->effect2,e->scene);e->effect2=0;e->secondary=0;}
        if(secondary&&!e->effect2){float start=e->length*(e->pole?.45f:.12f),end=e->length*.98f;const String* second=element_template(secondary,&fire,&light,&wind,&water,&ice);e->effect2=create_secondary_effect(manager,e->scene,e->node,second,end-start);if(e->effect2)e->secondary=secondary;}
#ifdef EE_ICE_OVERLAY_EXPERIMENTAL
        frost_update(e,element==5||secondary==5);
#endif
        {float scale=visual_scale(e->level,ally->hit_glow);if(scale!=e->applied_scale){Vec size={scale,scale,scale};node_scale(e->node,&size);e->applied_scale=scale;}}
        p=tag_position(e->tag);q=tag_rotation(e->tag);if(p && q){float start=e->length*(e->pole?.45f:.12f),end=e->length*.98f,off=(start+end)*.5f-5.0f;Vec adjusted={p->x+2*(q->x*q->y-q->w*q->z)*off,p->y+(1-2*(q->x*q->x+q->z*q->z))*off,p->z+2*(q->y*q->z+q->w*q->x)*off};node_position(e->node,&adjusted);node_rotation(e->node,q->w,q->x,q->y,q->z);
#ifdef EE_ENABLE_TRAIL
            if(show_trails&&e->level>=20&&ally->attacking)trail_tick(e,p,q,dt);else trail_finish(e,dt);
#endif
        }
        particle_update(e->effect,dt);set_aabb(e->effect,bounds);
        if(e->effect2){particle_update(e->effect2,dt);set_aabb(e->effect2,bounds);}
    }update_hands(manager,dt);impact_update(manager,dt);cb_update_visuals(manager,dt);for(i=0;i<EE_ALLY_LIMIT;++i)if(allies[i].hit_glow>0.0f){allies[i].hit_glow-=dt;if(allies[i].hit_glow<0.0f)allies[i].hit_glow=0.0f;}} __finally {internal=0;ReleaseSRWLockExclusive(&guard);}
}
#ifdef EE_FROST_RELOAD_DEV
typedef struct {void* object;void* info;} EEFrostSharedTexture;
static EEFrostSharedTexture frost_dev_texture;
static int frost_dev_disabled;
static void frost_dev_text(char** p,const char* s){while(*s)*(*p)++=*s++;}
static void frost_dev_num(char** p,unsigned v){char b[12];unsigned n=0;do{b[n++]=(char)('0'+v%10);v/=10;}while(v&&n<11);while(n)*(*p)++=b[--n];}
static void frost_dev_report(void){
    MODULE ogre=GetModuleHandleA("OgreMain_x64.dll");unsigned char (*queue)(void*);unsigned char (*visible)(void*);int i,count=0;
    *(void**)(&queue)=GetProcAddress(ogre,"?getRenderQueueGroup@MovableObject@Ogre@@QEBAEXZ");
    *(void**)(&visible)=GetProcAddress(ogre,"?getVisible@MovableObject@Ogre@@QEBA_NXZ");
    AcquireSRWLockExclusive(&guard);__try {for(i=0;i<EE_TRACK_LIMIT&&count<12;++i){Entry* e=&entries[i];char line[512],*p=line;void** m;const String* s;const char* n;U64 j;
        if(!e->blade||(e->element!=5&&e->secondary!=5))continue;
        __try {
            frost_dev_text(&p,"FROST DIAG: mesh=");m=(void**)entity_mesh(e->blade);s=m&&*m?resource_name(*m):0;n=s?(s->capacity<16?s->text.small:s->text.heap):0;
            if(n&&s->size<=160)for(j=0;j<s->size;++j)*p++=n[j];else frost_dev_text(&p,"unknown");
            frost_dev_text(&p," bone=");frost_dev_num(&p,(unsigned)e->frost_bone);frost_dev_text(&p," layer=");frost_dev_num(&p,e->frost?1:0);frost_dev_text(&p," attached=");frost_dev_num(&p,(unsigned)e->frost_attached);
            if(queue&&visible&&e->frost){frost_dev_text(&p," queue_blade=");frost_dev_num(&p,queue(e->blade));frost_dev_text(&p," queue_layer=");frost_dev_num(&p,queue(e->frost));frost_dev_text(&p," visible_blade=");frost_dev_num(&p,visible(e->blade));frost_dev_text(&p," visible_layer=");frost_dev_num(&p,visible(e->frost));}
            *p++='\r';*p++='\n';*p=0;{DWORD written;void* file=CreateFileA(log_path,4,3,0,4,0x80,0);if(file!=(void*)(U64)-1){WriteFile(file,line,(DWORD)(p-line),&written,0);CloseHandle(file);}}++count;
        }__except(1){}
    }}__finally {ReleaseSRWLockExclusive(&guard);}
    if(!count)logline("FROST DIAG: no currently tracked ice weapon.\r\n");
}
static int frost_dev_reload(void){
    MODULE ogre=GetModuleHandleA("OgreMain_x64.dll");void* manager;
    void* (*instance)(void);void* (*get_texture)(void*,EEFrostSharedTexture*,const String*,const String*);void (*reload)(void*);
    String name={{0},15,15,0},group={{"Autodetect"},10,15,0};const char* n="EE_IceFrost.png";unsigned i;
    if(!ogre||frost_dev_disabled)return 0;
    *(void**)(&instance)=GetProcAddress(ogre,"?getSingletonPtr@TextureManager@Ogre@@SAPEAV12@XZ");
    *(void**)(&get_texture)=GetProcAddress(ogre,"?getByName@TextureManager@Ogre@@QEAA?AV?$SharedPtr@VTexture@Ogre@@@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0@Z");
    *(void**)(&reload)=GetProcAddress(ogre,"?reload@Resource@Ogre@@UEAAXXZ");
    if(!instance||!get_texture||!reload)return 0;
    __try {
        if(!frost_dev_texture.object){manager=instance();if(!manager)return 0;for(i=0;i<15;++i)name.text.small[i]=n[i];name.text.small[15]=0;get_texture(manager,&frost_dev_texture,&name,&group);}
        if(!frost_dev_texture.object)return 0;reload(frost_dev_texture.object);logline("FROST DEV: texture reloaded.\r\n");return 1;
    } __except(1) {frost_dev_disabled=1;logline("FROST DEV: reload disabled after native error.\r\n");return 0;}
}
static void frost_dev_poll(void){static U64 next;static int seen=-1;U64 now=GetTickCount64();int serial;if(!settings_path[0]||now<next)return;next=now+1000;serial=(int)GetPrivateProfileIntA("Developer","FrostReloadSerial",0,settings_path);if(seen<0){seen=serial;return;}if(serial!=seen&&frost_dev_reload()){seen=serial;frost_dev_report();}}
#ifdef ELEMENTAL_NATIVE_TEST
__declspec(dllexport) int probeTestFrostReload(void){return frost_dev_reload();}
#endif
#endif
static void gpu_hook(void* world,float dt) {original_gpu(world,dt);if(active)__try {
#ifdef EE_FROST_RELOAD_DEV
    frost_dev_poll();
#endif
    update(dt);} __except(1) {active=0;logline("DISABLED: enchantment visual update failed.\r\n");}}
static int distinct_hook_targets(void* a,void* b){return a&&b&&a!=b;}
static int module_matches(MODULE m,unsigned int stamp,unsigned int size) {unsigned char* b=(unsigned char*)m;unsigned int p;if(!b || *(unsigned short*)b!=0x5a4d)return 0;p=*(unsigned int*)(b+0x3c);return *(unsigned int*)(b+p)==0x4550 && *(unsigned int*)(b+p+8)==stamp && *(unsigned int*)(b+p+80)==size;}
__declspec(dllexport) void startPlugin(void) {
    MODULE ogre=GetModuleHandleA("OgreMain_x64.dll"),pu=GetModuleHandleA("Plugin_ParticleUniverse_x64.dll"),lib=GetModuleHandleA("KenshiLib.dll"),game=GetModuleHandleA("kenshi_x64.exe");
    int ok=1;DWORD i,last=0;void* stub;void* gpu_stub;void* gpu_target;void* target;void* char_update;void* char_destroy;int (*hook)(void*,void*,void**);long long (*address)(void*);
#ifdef EE_ENABLE_PERSISTENCE
    void* stats_load;void* stats_save;void* stats_destroy;void* char_set_handle;void* char_hit;void* combat_speed_calculate;void* crossbow_wound;
#endif
#ifdef EE_ENABLE_UI
    MODULE mygui=GetModuleHandleA("MyGUIEngine_x64.dll");void* ui_update;void* ui_clear;void* ui_destroy;
#endif
    if(!self || !GetModuleFileNameA(self,log_path,sizeof(log_path)))return;for(i=0;log_path[i];++i)if(log_path[i]=='\\'||log_path[i]=='/')last=i;
    for(i=0;i<=last;++i)settings_path[i]=log_path[i];
    {const char* name="ElementalEnchantment.ini";for(i=0;name[i] && last+1+i<sizeof(settings_path)-1;++i)settings_path[last+1+i]=name[i];settings_path[last+1+i]=0;}
    reload_settings();
    {const char* name="ElementalEnchantment.log";for(i=0;name[i];++i)log_path[last+1+i]=name[i];log_path[last+1+i]=0;}
    if(!lib || !module_matches(ogre,0x5ca5f929,0x9c9000) || !module_matches(pu,0x5ca5faa4,0x718000) || !(module_matches(game,0x6602d59d,0x232d000)||module_matches(game,0x65d604d7,0x232c000))) {logline("INACTIVE: unreviewed game or engine build.\r\n");return;}
#ifdef EE_ENABLE_UI
    if(!mygui){logline("INACTIVE: MyGUI module missing.\r\n");return;}
#endif
#define API(f,m,s) *(void**)(&f)=GetProcAddress(m,s);if(!f)ok=0
    API(hook,lib,"?AddHook@KenshiLib@@YA?AW4HookStatus@1@PEAX0PEAPEAX@Z");API(address,lib,"?GetRealAddress@KenshiLib@@YA_JPEAX@Z");
    if(!ok)return;
#ifdef EE_ENABLE_UI
    stub=GetProcAddress(lib,"?getInstance@LocaleManager@@SAPEAV1@XZ");locale_manager_instance=stub?(void*)(U64)address(stub):0;
    stub=GetProcAddress(lib,"?getCurrentLocale@LocaleManager@@QEBAPEAVLocaleInfo@@XZ");locale_current=stub?(void*)(U64)address(stub):0;
#endif
#define GAMEAPI(f,s) stub=GetProcAddress(lib,s);*(void**)(&f)=stub?(void*)(U64)address(stub):0;if(!f)ok=0
    GAMEAPI(character_is_player,"?isPlayerCharacter@Character@@QEBA_NXZ");GAMEAPI(character_appearance,"?getAppearance@Character@@QEAAPEAVAppearanceBase@@XZ");GAMEAPI(appearance_body,"?getBody@AppearanceBase@@QEBAPEAVEntity@Ogre@@XZ");
    GAMEAPI(character_handle,"?getHandle@RootObjectBase@@QEBAAEBVhand@@XZ");
#ifdef EE_ENABLE_TRAIL
    stub=GetProcAddress(lib,"?getCombatClass@Character@@QEBAPEAVCombatClass@@XZ");*(void**)(&trail_character_combat)=stub?(void*)(U64)address(stub):0;
    stub=GetProcAddress(lib,"?getCombatState@CombatClass@@QEBA?AW4swordStateEnum@@XZ");*(void**)(&trail_combat_state)=stub?(void*)(U64)address(stub):0;
    if(!trail_character_combat||!trail_combat_state)logline("TRAIL: combat state unavailable; trails disabled.\r\n");
#endif
#ifdef EE_ENABLE_PERSISTENCE
    ee_persistence_init(&persistence);
    ee_characters_set_seed_provider(fresh_aptitude_seed);
    GAMEAPI(character_stats,"?getStats@Character@@QEAAPEAVCharStats@@XZ");
    GAMEAPI(gamedata_api.list_exists,"?listExists@GameData@@QEBA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z");
    GAMEAPI(gamedata_api.list_size,"?getListSize@GameData@@QEBAHAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z");
    GAMEAPI(gamedata_api.list_name,"?getFromList@GameData@@QEBAAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBV23@H@Z");
    GAMEAPI(gamedata_api.list_values,"?getValueFromList@GameData@@QEBAAEBVTripleInt@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@H@Z");
    GAMEAPI(gamedata_api.find_reference,"?getGameDataReferenceObject@GameData@@QEAAPEAVGameDataReference@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0@Z");
    GAMEAPI(gamedata_api.add_reference,"?addToList@GameData@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0HHH@Z");
    stub=GetProcAddress(lib,"?updateStats@CharStats@@QEAAXPEAVGameData@@@Z");stats_load=stub?(void*)(U64)address(stub):0;
    stub=GetProcAddress(lib,"?serialise@CharStats@@QEAAXPEAVGameData@@@Z");stats_save=stub?(void*)(U64)address(stub):0;
    stub=GetProcAddress(lib,"?_DESTRUCTOR@CharStats@@QEAAXXZ");stats_destroy=stub?(void*)(U64)address(stub):0;
    stub=GetProcAddress(lib,"?_NV_setHandle@Character@@QEAAXAEBVhand@@@Z");char_set_handle=stub?(void*)(U64)address(stub):0;
    stub=GetProcAddress(lib,"?_NV_hitByMeleeAttack@Character@@QEAA?AW4HitMaterialType@@W4CutDirection@@AEAVDamages@@PEAV1@PEAVCombatTechniqueData@@H@Z");char_hit=stub?(void*)(U64)address(stub):0;
    combat_speed_calculate=0;
    if(module_matches(game,0x65d604d7,0x232c000))__try {
        static const unsigned char prologue[]={0x48,0x8b,0xc4,0x48,0x81,0xec,0x98,0,0,0,0x0f,0x29,0x70,0xe8};
        const unsigned char* code=(const unsigned char*)game+0x882bb0;unsigned j;void* mapped=0;
        stub=GetProcAddress(lib,"?calculateAttackOrBlockSpeed@CharStats@@QEAAMMM_N@Z");if(stub)mapped=(void*)(U64)address(stub);
        for(j=0;j<sizeof(prologue)&&code[j]==prologue[j];++j){}
        if((j==sizeof(prologue)||code[0]==0xe9)&&mapped==(void*)code&&*((const unsigned char*)game+0x897802)==0xe8&&*((const unsigned char*)game+0x897906)==0xe8)combat_speed_calculate=(void*)code;
    }__except(1){combat_speed_calculate=0;}
    if(!stats_load||!stats_save||!stats_destroy||!char_set_handle||!char_hit)ok=0;
#endif
#ifdef EE_ENABLE_UI
    GAMEAPI(stats_window_width,"?getWidth@GUIWindow@@UEBAHXZ");GAMEAPI(stats_window_height,"?getHeight@GUIWindow@@UEBAHXZ");GAMEAPI(stats_window_visible,"?_NV_isVisible@CharacterStatsWindow@@QEBA_NXZ");
    stub=GetProcAddress(lib,"?_NV_update@CharacterStatsWindow@@QEAAXXZ");ui_update=stub?(void*)(U64)address(stub):0;
    stub=GetProcAddress(lib,"?_NV_clear@CharacterStatsWindow@@QEAAXXZ");ui_clear=stub?(void*)(U64)address(stub):0;
    stub=GetProcAddress(lib,"?_DESTRUCTOR@CharacterStatsWindow@@QEAAXXZ");ui_destroy=stub?(void*)(U64)address(stub):0;
    if(!ui_update||!ui_clear||!ui_destroy)ok=0;
    API(mygui_api.gui_instance,mygui,"?getInstancePtr@?$Singleton@VGui@MyGUI@@@MyGUI@@SAPEAVGui@2@XZ");
    API(mygui_api.create_widget,mygui,"?createWidgetT@Widget@MyGUI@@QEAAPEAV12@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0AEBU?$TCoord@H@types@2@UAlign@2@0@Z");
    API(mygui_api.set_font,mygui,"?setFontName@TextBox@MyGUI@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z");
    *(void**)(&mygui_api.get_font_height)=GetProcAddress(mygui,"?getFontHeight@TextBox@MyGUI@@QEAAHXZ");
    *(void**)(&mygui_api.set_font_height)=GetProcAddress(mygui,"?setFontHeight@TextBox@MyGUI@@UEAAXH@Z");
    API(mygui_api.set_style,mygui,"?setWidgetStyle@Widget@MyGUI@@QEAAXUWidgetStyle@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z");
    API(mygui_api.set_size,mygui,"?setSize@Widget@MyGUI@@QEAAXHH@Z");
    *(void**)(&mygui_api.set_position)=GetProcAddress(mygui,"?setPosition@Widget@MyGUI@@QEAAXHH@Z");
    API(mygui_api.set_property,mygui,"?setProperty@Widget@MyGUI@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0@Z");
    API(mygui_api.destroy_widget,mygui,"?destroyWidget@Gui@MyGUI@@QEAAXPEAVWidget@2@@Z");
    API(mygui_api.set_visible,mygui,"?setVisible@Widget@MyGUI@@UEAAX_N@Z");
    API(mygui_api.set_caption,mygui,"?setCaptionWithReplacing@TextBox@MyGUI@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z");
    API(mygui_api.set_selected,mygui,"?setStateSelected@Button@MyGUI@@QEAAX_N@Z");
    API(mygui_api.canvas_created,mygui,"?isTextureCreated@Canvas@MyGUI@@QEBA_NXZ");
    API(mygui_api.canvas_create,mygui,"?createTexture@Canvas@MyGUI@@QEAAXHHW4TextureResizeMode@12@UTextureUsage@2@UPixelFormat@2@@Z");
    API(mygui_api.canvas_width,mygui,"?getTextureRealWidth@Canvas@MyGUI@@QEBAHXZ");API(mygui_api.canvas_height,mygui,"?getTextureRealHeight@Canvas@MyGUI@@QEBAHXZ");
    API(mygui_api.canvas_lock,mygui,"?lock@Canvas@MyGUI@@QEAAPEAXUTextureUsage@2@@Z");API(mygui_api.canvas_unlock,mygui,"?unlock@Canvas@MyGUI@@QEAAXXZ");
    API(input_focus,mygui,"?getMouseFocusWidget@InputManager@MyGUI@@QEBAPEAVWidget@2@XZ");API(widget_parent,mygui,"?getParent@Widget@MyGUI@@QEBAPEAV12@XZ");
    ee_mygui_backend(&mygui_api,&ui_api);
#endif
    GAMEAPI(character_combat,"?isInCombatMode@Character@@QEBA_N_N0@Z");GAMEAPI(appearance_attached,"?getAttachedEntity@AppearanceBase@@QEBAPEAVAttachedEntity@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z");GAMEAPI(appearance_skeleton,"?getSkeleton@AppearanceBase@@QEBAPEAVOldSkeletonInstance@Ogre@@XZ");
    GAMEAPI(weapon_category,"?getCategory@Weapon@@QEBA?AW4WeaponCategory@@XZ");character_current_weapon=current_weapon_virtual;
    API(skeleton_bone,ogre,"?getBone@Skeleton@Ogre@@UEBAPEAVOldBone@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z");
    stub=GetProcAddress(lib,"?_NV_update@Character@@QEAAXXZ");char_update=stub?(void*)(U64)address(stub):0;
    stub=GetProcAddress(lib,"?_DESTRUCTOR@Character@@QEAAXXZ");char_destroy=stub?(void*)(U64)address(stub):0;
    if(!char_update||!char_destroy)ok=0;
    entity_vtable=GetProcAddress(ogre,"??_7Entity@Ogre@@6BMovableObject@1@@");if(!entity_vtable)ok=0;
    API(entity_mesh,ogre,"?getMesh@Entity@Ogre@@QEBAAEBV?$SharedPtr@VMesh@Ogre@@@2@XZ");
    *(void**)(&frost_entity_create)=GetProcAddress(ogre,"?createEntity@SceneManager@Ogre@@UEAAPEAVEntity@2@AEBV?$SharedPtr@VMesh@Ogre@@@2@W4SceneMemoryMgrTypes@2@@Z");
    *(void**)(&frost_entity_material)=GetProcAddress(ogre,"?setMaterialName@Entity@Ogre@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0@Z");
    *(void**)(&frost_get_visible)=GetProcAddress(ogre,"?getVisible@MovableObject@Ogre@@QEBA_NXZ");
    *(void**)(&frost_set_visible)=GetProcAddress(ogre,"?setVisible@MovableObject@Ogre@@QEAAX_N@Z");
    *(void**)(&frost_cast_shadows)=GetProcAddress(ogre,"?setCastShadows@MovableObject@Ogre@@QEAAX_N@Z");
    API(mesh_bounds,ogre,"?getBounds@Mesh@Ogre@@QEBAAEBVAxisAlignedBox@2@XZ");API(bounds_maximum,ogre,"?getMaximum@AxisAlignedBox@Ogre@@QEBAAEBVVector3@2@XZ");
    API(resource_name,ogre,"?getName@Resource@Ogre@@UEBAAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ");
    API(parent_node,ogre,"?getParentSceneNode@MovableObject@Ogre@@QEBAPEAVSceneNode@2@XZ");API(node_scene,ogre,"?getCreator@SceneNode@Ogre@@QEBAPEAVSceneManager@2@XZ");
    API(tag_position,ogre,"?_getDerivedPosition@OldNode@Ogre@@UEBAAEBVVector3@2@XZ");API(tag_rotation,ogre,"?_getDerivedOrientation@OldNode@Ogre@@UEBAAEBVQuaternion@2@XZ");
    API(particle_manager,pu,"?getSingletonPtr@ParticleSystemManager@ParticleUniverse@@SAPEAV12@XZ");
    API(particle_template,pu,"?getParticleSystemTemplate@ParticleSystemManager@ParticleUniverse@@QEAAPEAVParticleSystem@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z");
    API(particle_create,pu,"?createParticleSystem@ParticleSystemManager@ParticleUniverse@@QEAAPEAVParticleSystem@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0PEAVSceneManager@Ogre@@@Z");
    API(particle_destroy,pu,"?destroyParticleSystem@ParticleSystemManager@ParticleUniverse@@QEAAXPEAVParticleSystem@2@PEAVSceneManager@Ogre@@@Z");
    API(particle_start,pu,"?start@ParticleSystem@ParticleUniverse@@QEAAXXZ");API(particle_update,pu,"?_update@ParticleSystem@ParticleUniverse@@QEAAXM@Z");
    *(void**)(&particle_count)=GetProcAddress(pu,"?getNumberOfEmittedParticles@ParticleSystem@ParticleUniverse@@QEAA_KXZ");
    API(particle_technique,pu,"?getTechnique@ParticleSystem@ParticleUniverse@@QEBAPEAVParticleTechnique@2@_K@Z");API(technique_emitter,pu,"?getEmitter@ParticleTechnique@ParticleUniverse@@QEBAPEAVParticleEmitter@2@_K@Z");API(box_height,pu,"?setHeight@BoxEmitter@ParticleUniverse@@QEAAXM@Z");
    API(particle_local,pu,"?setKeepLocal@ParticleSystem@ParticleUniverse@@QEAAX_N@Z");
    API(particle_bounds,pu,"?setBoundsAutoUpdated@ParticleSystem@ParticleUniverse@@QEAAX_NM@Z");API(set_aabb,ogre,"?setLocalAabb@MovableObject@Ogre@@QEAAXUAabb@2@@Z");API(set_queue,ogre,"?setRenderQueueGroup@MovableObject@Ogre@@UEAAXE@Z");
    API(node_create,ogre,"?createSceneNode@SceneManager@Ogre@@UEAAPEAVSceneNode@2@W4SceneMemoryMgrTypes@2@@Z");API(node_destroy,ogre,"?destroySceneNode@SceneManager@Ogre@@UEAAXPEAVSceneNode@2@@Z");API(node_add,ogre,"?addChild@Node@Ogre@@QEAAXPEAV12@@Z");
    API(node_position,ogre,"?setPosition@Node@Ogre@@QEAAXAEBVVector3@2@@Z");API(node_scale,ogre,"?setScale@Node@Ogre@@QEAAXAEBVVector3@2@@Z");API(node_rotation,ogre,"?setOrientation@Node@Ogre@@QEAAXMMMM@Z");API(node_attach,ogre,"?attachObject@SceneNode@Ogre@@QEAAXPEAVMovableObject@2@@Z");
#ifdef EE_ENABLE_TRAIL
    if(!trail_resolve(ogre))logline("TRAIL: optional ribbon API unavailable.\r\n");
#endif
    gpu_stub=GetProcAddress(lib,"?_NV_mainLoop_GPUSensitiveStuff@GameWorld@@QEAAXM@Z");if(!ok||!gpu_stub){logline("INACTIVE: required native API missing.\r\n");return;}
    *(void**)(&cb_normalise_vec)=GetProcAddress(ogre,"?normalise@Vector3@Ogre@@QEAAMXZ");
    *(void**)(&cb_normalise_quat)=GetProcAddress(ogre,"?normalise@Quaternion@Ogre@@QEAAMXZ");
    *(void**)(&cb_particle_scale)=GetProcAddress(pu,"?setScale@ParticleSystem@ParticleUniverse@@QEAAXAEBVVector3@Ogre@@@Z");
    stub=GetProcAddress(lib,"?_NV_getBarrelPos@GunClass@@QEAA?AVVector3@Ogre@@XZ");cb_barrel=stub?(void*)(U64)address(stub):0;
    gpu_target=(void*)(U64)address(gpu_stub);
#ifdef EE_ENABLE_PERSISTENCE
    stub=GetProcAddress(lib,"?addWound@MedicalSystem@@QEAAPEAVGameData@@_NW4CutDirection@@AEAVDamages@@AEAHPEAVRootObject@@AEAW4Enum@AttackDirection@@PEAVHarpoon@@@Z");crossbow_wound=stub?(void*)(U64)address(stub):0;
    if(!distinct_hook_targets(gpu_target,crossbow_wound)){logline("INACTIVE: hook targets invalid or overlapping.\r\n");return;}
#endif
#define HOOK(m,s,d,o) target=GetProcAddress(m,s);if(!target||hook(target,d,(void**)&o)!=0||!o)ok=0
    HOOK(ogre,"?attachObjectToBone@Entity@Ogre@@QEAAPEAVTagPoint@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@PEAVMovableObject@2@AEBVQuaternion@2@AEBVVector3@2@@Z",attach_hook,original_attach);
    HOOK(ogre,"?detachObjectFromBone@Entity@Ogre@@QEAAXPEAVMovableObject@2@@Z",detach_hook,original_detach);
    HOOK(ogre,"?detachAllObjectsFromBone@Entity@Ogre@@QEAAXXZ",detach_all_hook,original_detach_all);
    HOOK(ogre,"?clearScene@SceneManager@Ogre@@UEAAXXZ",clear_hook,original_clear);
    HOOK(ogre,"?destroyEntity@SceneManager@Ogre@@UEAAXPEAVEntity@2@@Z",entity_hook,original_destroy_entity);
    HOOK(ogre,"?destroySceneNode@SceneManager@Ogre@@UEAAXPEAVSceneNode@2@@Z",node_hook,original_destroy_node);
    HOOK(ogre,"?detachObject@SceneNode@Ogre@@QEAAXPEAVMovableObject@2@@Z",scene_detach_hook,original_scene_detach);
    HOOK(ogre,"?freeTagPoint@OldSkeletonInstance@Ogre@@QEAAXPEAVTagPoint@2@@Z",free_tag_hook,original_free_tag);
    HOOK(ogre,"?freeTagPoint@OldSkeletonInstance@Ogre@@QEAAXPEAVMovableObject@2@@Z",free_object_hook,original_free_object);
    if(!char_update||hook(char_update,character_update_hook,(void**)&original_character_update)!=0||!original_character_update)ok=0;
    if(!char_destroy||hook(char_destroy,character_destroy_hook,(void**)&original_character_destroy)!=0||!original_character_destroy)ok=0;
    /* Flight addresses are enabled only on the specifically verified executable. */
    if(module_matches(game,0x65d604d7,0x232c000)&&cb_barrel&&cb_normalise_vec&&cb_normalise_quat&&trail_root&&trail_derive){
        unsigned char* code=(unsigned char*)game;void* trace=(void*)(code+0x4354d0);void* destroy=(void*)(code+0x435610);void* create=(void*)(code+0x439f50);void* shoot;
        stub=GetProcAddress(lib,"?updateT@RangedCombatClass@@QEAAXMAEBVhand@@0@Z");target=stub?(void*)(U64)address(stub):0;
        stub=GetProcAddress(lib,"?shoot@GunClass@@QEAAXPEAVCharacter@@PEAVRootObject@@W4StatsEnumerated@@AEBVVector3@Ogre@@@Z");shoot=stub?(void*)(U64)address(stub):0;
        if(shoot&&shoot!=gpu_target&&shoot!=target&&shoot!=trace&&shoot!=destroy&&shoot!=create&&target&&target!=gpu_target&&trace!=gpu_target&&destroy!=gpu_target&&create!=gpu_target&&target!=trace&&target!=destroy&&target!=create&&trace!=destroy&&trace!=create&&destroy!=create){
            if(hook(target,cb_update_hook,(void**)&original_ranged_update)!=0||!original_ranged_update)ok=0;
            if(hook(shoot,cb_shoot_hook,(void**)&original_crossbow_shoot)!=0||!original_crossbow_shoot)ok=0;
            if(hook(create,cb_create_hook,(void**)&original_harpoon_create)!=0||!original_harpoon_create)ok=0;
            if(hook(trace,cb_trace_hook,(void**)&original_projectile_trace)!=0||!original_projectile_trace)ok=0;
            if(hook(destroy,cb_destroy_hook,(void**)&original_projectile_destroy)!=0||!original_projectile_destroy)ok=0;
        }else logline("CROSSBOW: unverified flight entry points; visuals disabled.\r\n");
    }else logline("CROSSBOW: flight unsupported on this executable.\r\n");
#ifdef EE_ENABLE_PERSISTENCE
    if(!stats_load||hook(stats_load,stats_load_hook,(void**)&original_stats_load)!=0||!original_stats_load)ok=0;
    if(!stats_save||hook(stats_save,stats_save_hook,(void**)&original_stats_save)!=0||!original_stats_save)ok=0;
    if(!stats_destroy||hook(stats_destroy,stats_destroy_hook,(void**)&original_stats_destroy)!=0||!original_stats_destroy)ok=0;
    if(!char_set_handle||hook(char_set_handle,character_set_handle_hook,(void**)&original_character_set_handle)!=0||!original_character_set_handle)ok=0;
    if(!char_hit||hook(char_hit,character_hit_hook,(void**)&original_character_hit)!=0||!original_character_hit)ok=0;
    if(!crossbow_wound||hook(crossbow_wound,crossbow_wound_hook,(void**)&original_crossbow_wound)!=0||!original_crossbow_wound)ok=0;
    if(!combat_speed_calculate||hook(combat_speed_calculate,calculate_combat_speed_hook,(void**)&original_calculate_combat_speed)!=0||!original_calculate_combat_speed)logline("SPEED: unverified calculation; melee speed bonuses disabled.\r\n");
#endif
#ifdef EE_ENABLE_UI
    if(!ui_update||hook(ui_update,stats_window_update_hook,(void**)&original_stats_window_update)!=0||!original_stats_window_update)ok=0;
    if(!ui_clear||hook(ui_clear,stats_window_clear_hook,(void**)&original_stats_window_clear)!=0||!original_stats_window_clear)ok=0;
    if(!ui_destroy||hook(ui_destroy,stats_window_destroy_hook,(void**)&original_stats_window_destroy)!=0||!original_stats_window_destroy)ok=0;
    HOOK(mygui,"?injectMousePress@InputManager@MyGUI@@QEAA_NHHUMouseButton@2@@Z",mouse_press_hook,original_mouse_press);
    if(mygui_api.set_position){HOOK(mygui,"?injectMouseMove@InputManager@MyGUI@@QEAA_NHHH@Z",mouse_move_hook,original_mouse_move);HOOK(mygui,"?injectMouseRelease@InputManager@MyGUI@@QEAA_NHHUMouseButton@2@@Z",mouse_release_hook,original_mouse_release);}
#endif
    target=gpu_target;if(!ok||!target||hook(target,gpu_hook,(void**)&original_gpu)!=0||!original_gpu){logline("INACTIVE: native hooks unavailable.\r\n");return;}
    active=1;
}
int __stdcall DllMain(MODULE module,DWORD reason,void* unused) {(void)unused;if(reason==1)self=module;return 1;}
#ifdef ELEMENTAL_NATIVE_TEST
/* Only exported in the isolated engine test build; never in the game package. */
static void* impact_test_appearance(void* target){return target?(void*)11:0;}
static void* impact_test_body(void* appearance){return appearance?(void*)12:0;}
static void* impact_test_parent(void* body){return body?(void*)13:0;}
static void* impact_test_scene(void* node){return node?(void*)123:0;}
static void* impact_test_derived(void* node,Vec* out){if(!node)return 0;*out=(Vec){1,2,3};return out;}
__declspec(dllexport) int probeTestImpactPositions(void){int i;Vec p;void* scene=0;for(i=0;i<EE_IMPACT_LIMIT;++i)impacts[i]=(EEImpact){0};character_appearance=impact_test_appearance;appearance_body=impact_test_body;parent_node=impact_test_parent;node_scene=impact_test_scene;trail_derive=impact_test_derived;particle_manager=0;impact_queue_character((void*)1,4,5);if(impacts[0].scene!=(void*)123||impacts[0].parent!=(void*)13||impacts[0].position.y!=10||impacts[0].element!=4||impacts[0].secondary!=5)return 1;impact_clear_parent((void*)13);if(impacts[0].scene)return 3;cb_flights[0]=(CBFlight){0};cb_flights[0].projectile=(void*)20;cb_flights[0].source=(void*)13;cb_flights[0].frames=1;cb_flights[0].offset=(Vec){10,20,30};if(!crossbow_impact_sample((void*)20,&scene,&p)||scene!=(void*)123||p.x!=11||p.y!=22||p.z!=33)return 2;cb_flights[0]=(CBFlight){0};return 0;}
__declspec(dllexport) int probeTestImpactQueue(void){Vec p={1,2,3};int i;void* scene=(void*)123;for(i=0;i<EE_IMPACT_LIMIT;++i)impacts[i]=(EEImpact){0};if(impact_queue_position(scene,&p,0,0)||impact_queue_position(0,&p,1,0))return 1;for(i=0;i<EE_IMPACT_LIMIT;++i)if(!impact_queue_position(scene,&p,1,2))return 2;if(impact_queue_position(scene,&p,1,0))return 3;particle_manager=0;impact_clear_scene(scene);for(i=0;i<EE_IMPACT_LIMIT;++i)if(impacts[i].scene)return 4;for(i=1;i<=5;++i)if(!impact_template(i))return 5;return 0;}
__declspec(dllexport) int probeTestImpactSetting(void){Vec p={1,2,3};int i;for(i=0;i<EE_IMPACT_LIMIT;++i)impacts[i]=(EEImpact){0};particle_manager=0;show_impacts=0;if(impact_queue_position((void*)123,&p,1,0))return 1;show_impacts=1;if(!impact_queue_position((void*)123,&p,1,0))return 2;show_impacts=0;impact_update(0,.016f);if(impacts[0].scene)return 3;show_impacts=1;return 0;}
__declspec(dllexport) float probeTestMultiplierFromIni(const char* path){unsigned i;if(!path)return -1.0f;for(i=0;i<sizeof(settings_path)-1&&path[i];++i)settings_path[i]=path[i];settings_path[i]=0;reload_settings();return effect_multiplier;}
__declspec(dllexport) int probeTestImpactSettingFromIni(const char* path){if(!path)return -1;probeTestMultiplierFromIni(path);return show_impacts;}
__declspec(dllexport) int probeTestXPSettingFromIni(const char* path){if(!path)return -1;probeTestMultiplierFromIni(path);return (int)xp_tenths;}
__declspec(dllexport) int probeTestUIScaleFromIni(const char* path){if(!path)return -1;probeTestMultiplierFromIni(path);return (int)ui_scale_tenths;}
__declspec(dllexport) int probeTestXPScaling(void){EEAffinity base,a,b;int i,record[3];for(i=0;i<10000;++i){ee_affinity_create(&base,(unsigned)i,1);if(base.rates[0]==60)break;}if(i==10000)return 1;
    a=base;a.xp_fraction=37;if(!ee_affinity_award_hit_rate_scaled(&a,0,0)||a.xp[0]||a.xp_fraction!=37)return 2;
    a=base;for(i=0;i<25;++i)ee_affinity_award_hit_rate_scaled(&a,0,5);if(a.xp[0]!=30||a.xp_fraction)return 3;
    a=base;for(i=0;i<25;++i)ee_affinity_award_hit_rate_scaled(&a,0,10);if(a.xp[0]!=60||a.xp_fraction)return 4;
    a=base;for(i=0;i<25;++i)ee_affinity_award_hit_rate_scaled(&a,0,20);if(a.xp[0]!=120||a.xp_fraction)return 5;
    a=base;for(i=0;i<25;++i)ee_affinity_award_hit_rate_scaled(&a,1,20);if(a.xp[0]!=200||a.rates[0]!=60)return 6;
    a=base;ee_affinity_award_hit_rate_scaled(&a,0,5);if(a.xp[0]!=1||a.xp_fraction!=20||!ee_affinity_encode(&a,record)||!ee_affinity_decode(&b,record)||b.xp[0]!=a.xp[0]||b.xp_fraction!=a.xp_fraction)return 7;
    a=base;a.xp[0]=47599;ee_affinity_award_hit_rate_scaled(&a,1,200);if(a.xp[0]!=47600||a.xp_fraction)return 8;
    a=base;ee_affinity_select(&a,0);if(ee_affinity_award_hit_rate_scaled(&a,1,200)||a.xp[0])return 9;
    a=base;for(i=0;i<25;++i)ee_affinity_award_hit_rate_scaled(&a,0,1);if(a.xp[0]!=6||a.xp_fraction)return 10;
    a=base;for(i=0;i<25;++i)ee_affinity_award_hit_rate_scaled(&a,0,200);if(a.xp[0]!=1200||a.xp_fraction)return 11;
    return 0;}
__declspec(dllexport) int probeTestEffectMultiplier(void)
{
    EEAffinity a;EEDamage original={30,10,0,0,1,0},zero,normal,five,half;int i,j;
    for(i=1;i<=5;++i){
        if(!ee_affinity_create(&a,42,(unsigned int)i))return 1;a.xp[0]=47600;
        zero=normal=five=original;
        if(ee_affinity_damage_scaled(&a,&zero,0))return 2;
        if(ee_affinity_damage(&a,&normal)!=ee_affinity_damage_scaled(&a,&five,5))return 3;
        half=original;ee_affinity_damage_scaled(&a,&half,0.5f);
        {float* o=(float*)&original;float* z=(float*)&zero;float* n=(float*)&normal;float* f=(float*)&five;
         for(j=0;j<6;++j){float diff=f[j]-(o[j]+5.0f*(n[j]-o[j]));float hd=((float*)&half)[j]-(o[j]+0.5f*(n[j]-o[j]));if(z[j]!=o[j]||diff<-.001f||diff>.001f||hd<-.001f||hd>.001f)return 4;}}
        {float n=ee_affinity_attack_speed(&a,1),f=ee_affinity_attack_speed_scaled(&a,1,5),expected=1+5*(n-1);if(ee_affinity_attack_speed_scaled(&a,1,0)!=1||f<expected-.001f||f>expected+.001f)return 5;
         n=ee_affinity_combat_speed(&a,1);f=ee_affinity_combat_speed_scaled(&a,1,5);expected=1+5*(n-1);if(ee_affinity_combat_speed_scaled(&a,1,0)!=1||f<expected-.001f||f>expected+.001f)return 6;}
    }
    ee_affinity_create(&a,42,3);a.xp[0]=47600;if(!ee_affinity_select_secondary(&a,2))return 7;
    if(ee_affinity_attack_speed_scaled(&a,1,5)<1.687f||ee_affinity_attack_speed_scaled(&a,1,5)>1.688f)return 8;
    if(ee_affinity_attack_speed_scaled(&a,1,0.5f)<1.068f||ee_affinity_attack_speed_scaled(&a,1,0.5f)>1.069f)return 10;
    a.xp[0]=0;if(!ee_affinity_award_hit(&a)||!a.xp[0])return 9;
    if(parse_multiplier_tenths("0.5",50)!=5||parse_multiplier_tenths("1",50)!=10||parse_multiplier_tenths("1.5",50)!=15||parse_multiplier_tenths("5",50)!=50||parse_multiplier_tenths("5.5",50)!=50||parse_multiplier_tenths("20.0",200)!=200||parse_multiplier_tenths("0.1",200)!=1)return 11;
    return 0;
}
static int test_mcm_int_seen,test_mcm_toggle_count,test_mcm_bad;
static void __cdecl test_mcm_begin(MCM_Api* a,const char* id,const char* title,const char* path,void (__cdecl *apply)(void)){(void)a;if(!id||!title||!path||!apply)test_mcm_bad=1;}
static void __cdecl test_mcm_section(MCM_Api* a,const char* title){(void)a;if(!title||!*title)test_mcm_bad=1;}
static void __cdecl test_mcm_toggle(MCM_Api* a,const char* section,const char* key,const char* label,const char* tip,int def,unsigned flags){(void)a;(void)section;(void)key;(void)flags;if(!label||!*label||!tip||!*tip||def!=(test_mcm_toggle_count?1:0))test_mcm_bad=1;++test_mcm_toggle_count;}
static void __cdecl test_mcm_number(MCM_Api* a,const char* section,const char* key,const char* label,const char* tip,float def,float min,float max,int decimals,unsigned flags){float expected_min=test_mcm_int_seen==2?0.8f:0.0f;float expected_max=test_mcm_int_seen==0?20.0f:test_mcm_int_seen==1?5.0f:2.0f;(void)a;(void)section;(void)key;(void)flags;if(!label||!*label||!tip||!*tip||def!=1.0f||min!=expected_min||max!=expected_max||decimals!=1)test_mcm_bad=1;++test_mcm_int_seen;}
__declspec(dllexport) int probeTestMcmRegistration(void)
{
    MCM_Api api={1,0,test_mcm_begin,test_mcm_section,test_mcm_toggle,0,test_mcm_number,0};char old=settings_path[0];
    test_mcm_bad=test_mcm_int_seen=test_mcm_toggle_count=0;settings_path[0]='x';MCM_Describe(&api);settings_path[0]=old;
    return test_mcm_bad||test_mcm_int_seen!=3||test_mcm_toggle_count!=3;
}
static Vec* (*test_node_position)(void*,Vec*);
static float (*test_box_height)(void*);
static int test_free_calls;
static void test_free_original(void* skeleton,void* value) {(void)skeleton;(void)value;++test_free_calls;}
static int test_character_calls,test_player_flag;
static void test_character_original(void* c) {(void)c;++test_character_calls;}
static unsigned char test_is_player(void* c) {(void)c;return (unsigned char)test_player_flag;}
static int test_weapon_kind=-1,test_ranged_mode;
static void* test_current_weapon(void* c) {(void)c;return test_weapon_kind<0?0:(void*)11;}
static int test_weapon_category(void* w) {(void)w;return test_weapon_kind;}
static unsigned char test_combat_mode(void* c,unsigned char melee,unsigned char ranged) {(void)c;(void)melee;return (unsigned char)(ranged&&test_ranged_mode);}
static void test_melee_api(void) {character_current_weapon=test_current_weapon;weapon_category=test_weapon_category;character_combat=test_combat_mode;test_weapon_kind=-1;test_ranged_mode=0;}
static unsigned int test_handle_values[8];
static const void* test_handle(void* c) {(void)c;return test_handle_values;}
#ifdef EE_ENABLE_PERSISTENCE
static void* test_stats(void* c) {(void)c;return 0;}
static void* test_hit_stats(void* c) {(void)c;return (void*)123;}
__declspec(dllexport) int probeTestAllyHistoryFilter(void)
{
    void* a=(void*)0x1040;void* b;unsigned int i,bit;
    if(ally_seen_maybe(a))return 1;
    if(!set_ally(a,(void*)0x2000,1,0,0,0,1)||!ally_seen_maybe(a))return 2;
    if(set_ally(a,0,0,0,0,0,0)||!ally_seen_maybe(a))return 3;
    if(!set_ally(a,(void*)0x2000,1,0,0,0,1))return 4;
    bit=ally_seen_bit(a);
    for(i=0x3000;i<0x1000000;i+=16)if(ally_seen_bit((void*)(U64)i)==bit)break;
    if(i>=0x1000000)return 5;b=(void*)(U64)i;
    if(!ally_seen_maybe(b)||set_ally(b,0,0,0,0,0,0))return 6;
    if(!find_ally((void*)0x2000))return 7;
    return 0;
}
static int test_hit_result;static float test_seen_cut;
static int test_hit_original(void* target,int dir,void* dmg,void* attacker,void* tech,int combo){(void)target;(void)dir;(void)attacker;(void)tech;(void)combo;if(dmg)test_seen_cut=((EEDamage*)dmg)->cut;return test_hit_result;}
__declspec(dllexport) int probeTestBossCombatAffinity(void)
{
    unsigned int slot;EEAffinity a;EEDamage d={10,10,0,0,0,0};
    ee_persistence_init(&persistence);character_is_player=test_is_player;character_handle=test_handle;
    test_player_flag=0;test_handle_values[5]=188;test_handle_values[6]=7;
    slot=(188u^(7u*2654435761u))&4095u;
    boss_diag_seen[slot].character=(void*)9;boss_diag_seen[slot].index=188;boss_diag_seen[slot].generation=7;boss_diag_seen[slot].boss_profile=1;
    boss_diag_seen[(slot+1)&4095u].character=(void*)10;boss_diag_seen[(slot+1)&4095u].index=188;boss_diag_seen[(slot+1)&4095u].generation=7;boss_diag_seen[(slot+1)&4095u].boss_profile=0;
    if(!combat_affinity_locked((void*)9,0,&a)||a.selected!=1||a.secondary!=2||ee_affinity_level(&a,0)!=80)return 1;
    if(!ee_affinity_damage(&a,&d)||d.blunt<=10.0f||d.armour<=0.0f)return 2;
    if(combat_affinity_locked((void*)10,0,&a))return 3;
    boss_diag_seen[slot].character=0;boss_diag_seen[(slot+1)&4095u].character=0;
    return 0;
}
__declspec(dllexport) int probeTestHitXP(void)
{
    EECharacterRecord* r;unsigned int old;int i;EEDamage d={10,10,0,0,1,0};
    ee_persistence_init(&persistence);character_is_player=test_is_player;character_handle=test_handle;character_stats=test_hit_stats;original_character_hit=test_hit_original;test_melee_api();
    test_handle_values[5]=42;test_handle_values[6]=7;test_player_flag=1;active=1;
    r=ee_persistence_bind(&persistence,(void*)123,(void*)9,42,7,99,4);if(!r)return 1;
    test_hit_result=0;character_hit_hook((void*)8,0,0,(void*)9,0,0);test_hit_result=5;character_hit_hook((void*)8,0,0,(void*)9,0,0);if(r->affinity.xp[0])return 2;
    test_hit_result=2;for(i=0;i<10;++i)character_hit_hook((void*)8,0,0,(void*)9,0,0);old=r->affinity.xp[0];if(!old)return 3;
    character_hit_hook((void*)8,0,&d,(void*)9,0,0);if(test_seen_cut<=10.0f||d.cut!=10.0f)return 5;old=r->affinity.xp[0];
    test_weapon_kind=6;test_seen_cut=0;character_hit_hook((void*)8,0,&d,(void*)9,0,0);if(test_seen_cut!=10.0f||d.cut!=10.0f||r->affinity.xp[0]!=old)return 6;test_weapon_kind=-1;
    test_player_flag=0;character_hit_hook((void*)8,0,0,(void*)9,0,0);if(r->affinity.xp[0]!=old)return 4;
    active=0;return 0;
}
__declspec(dllexport) int probeTestHookTargets(void) {return !distinct_hook_targets((void*)1,(void*)2)||distinct_hook_targets((void*)1,(void*)1)||distinct_hook_targets(0,(void*)1);}
static float test_wound_seen_cut,test_wound_seen_blunt,test_wound_seen_pierce;
static void* test_crossbow_wound_original(void* a,unsigned char b,int c,void* damage,int* d,void* e,int* f,void* g){EEDamage* x=(EEDamage*)damage;(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;test_wound_seen_cut=x->cut;test_wound_seen_blunt=x->blunt;test_wound_seen_pierce=x->pierce;return (void*)1;}
__declspec(dllexport) int probeTestXPScalingHitHooks(void){EECharacterRecord* r;EEDamage d={10,10,100,0,1,0};unsigned int old;int fail=0;
    ee_persistence_init(&persistence);character_is_player=test_is_player;character_handle=test_handle;character_stats=test_hit_stats;original_character_hit=test_hit_original;original_crossbow_wound=test_crossbow_wound_original;test_melee_api();
    test_handle_values[5]=42;test_handle_values[6]=7;test_player_flag=1;test_hit_result=2;active=1;all_aptitudes=1;
    r=ee_persistence_bind(&persistence,(void*)123,(void*)9,42,7,99,4);if(!r)return 1;
    xp_tenths=0;character_hit_hook((void*)8,0,&d,(void*)9,0,0);if(r->affinity.xp[0]||test_seen_cut<=10.0f)fail=2;
    xp_tenths=20;character_hit_hook((void*)8,0,&d,(void*)9,0,0);if(r->affinity.xp[0]!=8)fail=3;
    old=r->affinity.xp[0];crossbow_wound_hook((void*)8,0,0,&d,0,(void*)9,0,(void*)7);if(r->affinity.xp[0]!=old+8)fail=4;
    xp_tenths=10;all_aptitudes=0;active=0;return fail;}
__declspec(dllexport) int probeTestMultiplierHitHooks(void){
    EECharacterRecord* r;EEDamage d={30,10,0,0,1,0};float one,five;int fail=0;
    ee_persistence_init(&persistence);character_is_player=test_is_player;character_handle=test_handle;character_stats=test_hit_stats;original_character_hit=test_hit_original;original_crossbow_wound=test_crossbow_wound_original;test_melee_api();
    test_handle_values[5]=42;test_handle_values[6]=7;test_player_flag=1;test_hit_result=2;active=1;
    r=ee_persistence_bind(&persistence,(void*)123,(void*)9,42,7,99,4);if(!r)return 1;r->affinity.xp[0]=47600;
    effect_multiplier=0;character_hit_hook((void*)8,0,&d,(void*)9,0,0);if(test_seen_cut!=30)fail=2;
    effect_multiplier=1;character_hit_hook((void*)8,0,&d,(void*)9,0,0);one=test_seen_cut;
    effect_multiplier=5;character_hit_hook((void*)8,0,&d,(void*)9,0,0);five=test_seen_cut;if(one<35.99f||one>36.01f||five<59.99f||five>60.01f)fail=3;
    ee_affinity_select(&r->affinity,1);d=(EEDamage){0,0,100,0,1,0};
    effect_multiplier=0;crossbow_wound_hook((void*)8,0,0,&d,0,(void*)9,0,(void*)7);if(test_wound_seen_blunt!=0)fail=4;
    effect_multiplier=1;crossbow_wound_hook((void*)8,0,0,&d,0,(void*)9,0,(void*)7);one=test_wound_seen_blunt;
    effect_multiplier=5;crossbow_wound_hook((void*)8,0,0,&d,0,(void*)9,0,(void*)7);five=test_wound_seen_blunt;if(one<14.99f||one>15.01f||five<74.99f||five>75.01f)fail=5;
    effect_multiplier=1;active=0;return fail;
}
__declspec(dllexport) int probeTestCrossbowWound(void){
    EECharacterRecord* r;EEDamage d={0,0,100,0,1,0};unsigned int old;
    ee_persistence_init(&persistence);character_is_player=test_is_player;character_handle=test_handle;character_stats=test_hit_stats;original_crossbow_wound=test_crossbow_wound_original;test_handle_values[5]=42;test_handle_values[6]=7;test_player_flag=1;active=1;all_aptitudes=1;
    r=ee_persistence_bind(&persistence,(void*)123,(void*)9,42,7,99,1);if(!r)return 1;r->affinity.xp[0]=0;
    crossbow_wound_hook((void*)8,0,0,&d,0,(void*)9,0,(void*)7);
    if(test_wound_seen_blunt<4.99f||test_wound_seen_blunt>5.01f||test_wound_seen_pierce!=100||d.blunt!=0||r->affinity.xp[0]!=4)return 2;
    old=r->affinity.xp[0];test_player_flag=0;crossbow_wound_hook((void*)8,0,0,&d,0,(void*)9,0,(void*)7);
    if(test_wound_seen_blunt!=0||r->affinity.xp[0]!=old)return 3;
    test_player_flag=1;crossbow_wound_hook((void*)8,0,0,&d,0,(void*)9,0,0);
    if(test_wound_seen_blunt!=0||r->affinity.xp[0]!=old)return 4;
    active=0;all_aptitudes=0;return 0;
}
static void test_cb_trace_original(void* projectile,const Vec* p,const Vec* d){(void)projectile;(void)p;(void)d;}
static void test_cb_update_original(void* ranged,float dt,const void* p,const void* d){(void)ranged;(void)dt;(void)p;(void)d;}
static Vec* test_cb_barrel(void* gun,Vec* out){(void)gun;*out=(Vec){4,5,6};return out;}
__declspec(dllexport) int probeTestCrossbowCapture(void){
    static unsigned char bolt[128],ranged[160];Vec pos={1,2,3},direction={0,1,0};int i;void* actor=(void*)9;
    for(i=0;i<128;++i)((volatile unsigned char*)bolt)[i]=0;for(i=0;i<160;++i)((volatile unsigned char*)ranged)[i]=0;
    for(i=0;i<CB_LIMIT;++i){cb_charges[i]=(CBCharge){0};cb_reset_flight(&cb_flights[i]);}
    for(i=0;i<EE_ALLY_LIMIT;++i)allies[i].character=0;
    test_player_flag=1;character_is_player=test_is_player;character_handle=test_handle;test_handle_values[5]=42;test_handle_values[6]=7;
    original_projectile_trace=test_cb_trace_original;original_ranged_update=test_cb_update_original;cb_barrel=test_cb_barrel;active=1;
    allies[0].character=actor;allies[0].element=1;allies[0].secondary=2;
    *(unsigned*)(bolt+0x34)=42;*(unsigned*)(bolt+0x38)=7;*(void**)(bolt+0x58)=(void*)11;
    cb_trace_hook(bolt,&pos,&direction);if(cb_flights[0].projectile!=bolt||cb_flights[0].element!=1||cb_flights[0].secondary!=2)return 1;
    clear_crossbow_projectile(bolt);if(cb_flights[0].projectile)return 2;
    *(unsigned*)(bolt+0x34)=99;cb_trace_hook(bolt,&pos,&direction);if(cb_flights[0].projectile)return 3;
    *(void**)(ranged+0x68)=actor;*(void**)(ranged+0x28)=(void*)12;*(int*)ranged=2;*(float*)(ranged+0x74)=6;*(float*)(ranged+0x78)=3;
    cb_update_hook(ranged,0,0,0);if(cb_charges[0].character!=actor||cb_charges[0].progress<.499f||cb_charges[0].progress>.501f||cb_charges[0].position.z!=6)return 4;
    clear_crossbow_character(actor);if(cb_charges[0].character)return 5;
    test_player_flag=0;cb_update_hook(ranged,0,0,0);if(cb_charges[0].character)return 6;active=0;return 0;
}
__declspec(dllexport) int probeTestCrossbowCoordinates(void){static CBFlight f;Vec p;cb_reset_flight(&f);f.origin=(Vec){-94584,344,-43728};f.has_origin=1;p=cb_align_position(&f,(Vec){-2140,343,47});if(p.x!=-94584||p.y!=344||p.z!=-43728)return 1;p=cb_align_position(&f,(Vec){-2316,334,-52});if(p.x!=-94760||p.y!=335||p.z!=-43827)return 2;return 0;}
static unsigned char test_cb_shot_bolt[128],test_cb_shot_bolt2[128];static unsigned char* test_cb_active_bolt=test_cb_shot_bolt;
static void* test_cb_create_original(void* manager,const void* mesh,const void* material){(void)manager;(void)mesh;(void)material;return test_cb_active_bolt;}
static void test_cb_shoot_original(void* gun,void* actor,void* target,int stat,const Vec* aim){(void)gun;(void)actor;(void)target;(void)stat;(void)aim;cb_create_hook(0,0,0);*(float*)(test_cb_active_bolt+0x44)=500.0f;}
__declspec(dllexport) int probeTestSetLogPath(const char* path){unsigned i;if(!path)return 0;for(i=0;i<sizeof(log_path)-1&&path[i];++i)log_path[i]=path[i];log_path[i]=0;return path[i]==0;}
__declspec(dllexport) int probeTestLogThrottle(void){static const char msg[]="TEST ERROR\r\n";logline(msg);logline(msg);return 0;}
__declspec(dllexport) int probeTestWindReload(void){EEAffinity a;float rate;
    if(!ee_affinity_create(&a,1,3))return 1;a.xp[0]=47600;rate=ee_affinity_combat_speed(&a,1.0f);
    if(rate<1.099f||rate>1.101f||cb_reload_adjust(3.0f,.1f,rate)>2.991f||cb_reload_adjust(3.0f,.1f,rate)<2.989f)return 2;
    a.selected=1;a.secondary=3;rate=ee_affinity_combat_speed(&a,1.0f);if(rate<1.074f||rate>1.076f)return 3;
    a.secondary=0;if(cb_reload_adjust(3.0f,.1f,ee_affinity_combat_speed(&a,1.0f))!=3.0f)return 4;
    if(cb_reload_adjust(.001f,.1f,1.1f)!=0.0f||cb_reload_adjust(3.0f,-1.0f,1.1f)!=3.0f)return 5;return 0;}
__declspec(dllexport) int probeTestWindReloadIntegration(void){
    static unsigned char ranged[160];EECharacterRecord* r;int i;for(i=0;i<160;++i)((volatile unsigned char*)ranged)[i]=0;
    ee_persistence_init(&persistence);character_is_player=test_is_player;character_handle=test_handle;character_stats=test_hit_stats;
    test_handle_values[5]=42;test_handle_values[6]=7;test_player_flag=1;active=1;
    for(i=0;i<EE_ALLY_LIMIT;++i)allies[i].character=0;
    r=ee_persistence_bind(&persistence,(void*)123,(void*)9,42,7,99,3);if(!r)return 1;r->affinity.xp[0]=47600;
    allies[0].character=(void*)9;allies[0].element=3;*(void**)(ranged+0x68)=(void*)9;*(int*)ranged=2;*(float*)(ranged+0x74)=6.0f;*(float*)(ranged+0x78)=3.0f;
    cb_apply_wind_reload(ranged,.1f);if(*(float*)(ranged+0x78)>2.991f||*(float*)(ranged+0x78)<2.989f)return 2;
    *(float*)(ranged+0x78)=3.0f;r->affinity.selected=1;allies[0].element=1;cb_apply_wind_reload(ranged,.1f);if(*(float*)(ranged+0x78)!=3.0f)return 3;
    allies[0].secondary=3;r->affinity.secondary=3;cb_apply_wind_reload(ranged,.1f);if(*(float*)(ranged+0x78)>2.993f||*(float*)(ranged+0x78)<2.992f)return 4;
    *(float*)(ranged+0x78)=3.0f;*(int*)ranged=0;cb_apply_wind_reload(ranged,.1f);if(*(float*)(ranged+0x78)!=3.0f)return 5;
    active=0;return 0;}
__declspec(dllexport) int probeTestCrossbowSpeed(void){
    int i;EECharacterRecord* r;Vec aim={0};
    for(i=0;i<128;++i){((volatile unsigned char*)test_cb_shot_bolt)[i]=0;((volatile unsigned char*)test_cb_shot_bolt2)[i]=0;}test_cb_active_bolt=test_cb_shot_bolt;
    for(i=0;i<CB_LIMIT;++i)cb_reset_flight(&cb_flights[i]);
    for(i=0;i<EE_ALLY_LIMIT;++i)allies[i].character=0;
    ee_persistence_init(&persistence);character_is_player=test_is_player;character_handle=test_handle;character_stats=test_hit_stats;original_projectile_trace=test_cb_trace_original;original_crossbow_shoot=test_cb_shoot_original;original_harpoon_create=test_cb_create_original;
    test_handle_values[5]=42;test_handle_values[6]=7;test_player_flag=1;active=1;
    r=ee_persistence_bind(&persistence,(void*)123,(void*)9,42,7,99,3);if(!r)return 1;r->affinity.xp[0]=47600;
    allies[0].character=(void*)9;allies[0].element=3;*(unsigned*)(test_cb_shot_bolt+0x34)=42;*(unsigned*)(test_cb_shot_bolt+0x38)=7;*(void**)(test_cb_shot_bolt+0x58)=(void*)11;
    cb_shoot_hook((void*)12,(void*)9,0,0,&aim);if(cb_flights[0].projectile!=test_cb_shot_bolt||cb_flights[0].element!=3)return 6;if(*(float*)(test_cb_shot_bolt+0x44)<549.9f||*(float*)(test_cb_shot_bolt+0x44)>550.1f)return 2;
    clear_crossbow_projectile(test_cb_shot_bolt);test_player_flag=0;cb_shoot_hook((void*)12,(void*)9,0,0,&aim);if(*(float*)(test_cb_shot_bolt+0x44)!=500)return 3;
    clear_crossbow_projectile(test_cb_shot_bolt);test_player_flag=1;r->affinity.selected=1;allies[0].element=1;cb_shoot_hook((void*)12,(void*)9,0,0,&aim);if(*(float*)(test_cb_shot_bolt+0x44)!=500)return 4;
    test_cb_active_bolt=test_cb_shot_bolt2;*(unsigned*)(test_cb_shot_bolt2+0x34)=42;*(unsigned*)(test_cb_shot_bolt2+0x38)=7;*(void**)(test_cb_shot_bolt2+0x58)=(void*)11;r->affinity.selected=3;allies[0].element=3;
    cb_shoot_hook((void*)12,(void*)9,0,0,&aim);if(*(float*)(test_cb_shot_bolt+0x44)!=500||*(float*)(test_cb_shot_bolt2+0x44)<549.9f)return 5;
    clear_crossbow_projectile(test_cb_shot_bolt);clear_crossbow_projectile(test_cb_shot_bolt2);active=0;return 0;
}
typedef struct TestLocale {U64 pad;String id;} TestLocale;
static TestLocale test_locale;
static void* test_locale_manager(void){return (void*)1;}
static void* test_locale_current(void* manager){(void)manager;return &test_locale;}
__declspec(dllexport) int probeTestLocaleDetection(void){
    const char* p;int i;const char* id="ja_JP";test_locale.id.size=5;test_locale.id.capacity=15;for(i=0;i<=5;++i)test_locale.id.text.small[i]=id[i];
    locale_manager_instance=test_locale_manager;locale_current=test_locale_current;ui_locale_ready=0;ee_locale_set("en_GB");select_game_locale();p=ee_locale_get()->id;
    if(p[0]!='j'||p[1]!='a'||!ui_locale_ready)return 1;ee_locale_set("unsupported");p=ee_locale_get()->id;if(p[0]!='e'||p[1]!='n')return 2;ee_locale_set("en_GB");locale_manager_instance=0;locale_current=0;ui_locale_ready=0;return 0;
}
__declspec(dllexport) const char* probeTestWindDetail(const char* id){ee_locale_set(id);return ee_locale_get()->winddetail;}
__declspec(dllexport) const char* probeTestLocaleText(const char* id,int field){const EELocale* l;ee_locale_set(id);l=ee_locale_get();return field==0?l->id:field==1?l->element[1]:field==2?l->armour:field==3?l->blunt:field==4?l->cut:l->nextlevel;}
static void test_set_handle_original(void* character,const void* next) {const unsigned int* h=(const unsigned int*)next;test_handle_values[5]=h[5];test_handle_values[6]=h[6];(void)ee_character_observe(&persistence.characters,character,h[5],h[6],123,5);}
typedef struct EETestGameData {int value[3];int present;} EETestGameData;
typedef struct EETestStats {U64 padding[2];void* character;} EETestStats;
static float test_speed_original(void* stats){(void)stats;return 1.0f;}
static float test_speed_calculation(void* stats,float weight,float skill,unsigned char block){(void)stats;(void)weight;(void)skill;(void)block;return 1.0f;}
__declspec(dllexport) int probeTestWindSpeed(void)
{
    EETestStats stats={{0,0},(void*)9};EECharacterRecord* r;float x;
    ee_persistence_init(&persistence);character_is_player=test_is_player;character_handle=test_handle;original_attack_speed=test_speed_original;original_block_speed=test_speed_original;test_melee_api();
    test_handle_values[5]=42;test_handle_values[6]=7;test_player_flag=1;active=1;
    r=ee_persistence_bind(&persistence,&stats,(void*)9,42,7,123,3);if(!r)return 1;
    x=attack_speed_hook(&stats);if(x<1.019f||x>1.021f)return 2;
    r->affinity.xp[0]=47600;x=block_speed_hook(&stats);if(x<1.099f||x>1.101f)return 3;
    original_calculate_combat_speed=test_speed_calculation;x=calculate_combat_speed_hook(&stats,1.0f,1.0f,1);if(x<1.099f||x>1.101f)return 10;
    r->affinity.selected=1;if(attack_speed_hook(&stats)!=1.0f)return 4;
    r->affinity.selected=2;r->affinity.xp[0]=47600;x=attack_speed_hook(&stats);if(x<1.049f||x>1.051f||block_speed_hook(&stats)!=1.0f)return 6;
    test_weapon_kind=6;if(attack_speed_hook(&stats)!=1.0f)return 7;test_weapon_kind=8;test_ranged_mode=1;if(attack_speed_hook(&stats)<1.049f)return 8;test_weapon_kind=-1;if(attack_speed_hook(&stats)!=1.0f)return 9;test_ranged_mode=0;
    r->affinity.selected=3;test_player_flag=0;if(block_speed_hook(&stats)!=1.0f)return 5;
    active=0;return 0;
}
static int test_stats_calls;
static void test_stats_original(void* stats,void* data) {(void)stats;(void)data;++test_stats_calls;}
static void test_stats_destroy_original(void* stats) {(void)stats;}
static unsigned char test_list_exists(void* data,const EEGameString* list) {(void)list;return (unsigned char)((EETestGameData*)data)->present;}
static int test_list_size(void* data,const EEGameString* list) {(void)list;return ((EETestGameData*)data)->present;}
static const EEGameString* test_list_name(void* data,const EEGameString* list,int index) {static const EEGameString name={{"SB_Data"},7,15,0};(void)data;(void)list;(void)index;return &name;}
static const int* test_list_values(void* data,const EEGameString* list,int index) {(void)list;(void)index;return ((EETestGameData*)data)->value;}
static void* test_find_reference(void* data,const EEGameString* list,const EEGameString* item) {(void)list;(void)item;return ((EETestGameData*)data)->value;}
static void test_add_reference(void* data,const EEGameString* list,const EEGameString* item,int a,int b,int c) {EETestGameData* d=(EETestGameData*)data;(void)list;(void)item;d->value[0]=a;d->value[1]=b;d->value[2]=c;d->present=1;}
__declspec(dllexport) int probeTestPersistenceHooks(void) {
    EETestGameData data={{0,0,0},0};EETestStats stats={{0,0},(void*)9};EECharacterRecord* r;unsigned char rates[5];int i;
    ee_persistence_init(&persistence);character_handle=test_handle;test_handle_values[5]=42;test_handle_values[6]=7;
    gamedata_api.list_exists=test_list_exists;gamedata_api.list_size=test_list_size;gamedata_api.list_name=test_list_name;gamedata_api.list_values=test_list_values;gamedata_api.find_reference=test_find_reference;gamedata_api.add_reference=test_add_reference;
    original_stats_save=test_stats_original;original_stats_load=test_stats_original;original_stats_destroy=test_stats_destroy_original;test_stats_calls=0;active=1;
    r=ee_persistence_bind(&persistence,&stats,(void*)9,42,7,123,4);if(!r)return 1;
    for(i=0;i<5;++i)rates[i]=r->affinity.rates[i];
    stats_save_hook(&stats,&data);if(!data.present||((data.value[0]>>8)&255)!=4||(data.value[0]&7)!=4||test_stats_calls!=1)return 2;
    if(!ee_character_select(&persistence.characters,(void*)9,42,7,2))return 3;
    stats_save_hook(&stats,&data);if((data.value[0]&7)!=2||test_stats_calls!=2)return 4;
    ee_persistence_forget(&persistence,&stats,(void*)9);
    stats_load_hook(&stats,&data);if(test_stats_calls!=3)return 5;
    r=ee_persistence_bind(&persistence,&stats,(void*)10,42,7,999,1);if(!r||r->affinity.selected!=2)return 6;
    for(i=0;i<5;++i)if(r->affinity.rates[i]!=rates[i])return 7;
    {unsigned int next[8]={0};next[5]=84;next[6]=8;original_character_set_handle=test_set_handle_original;character_set_handle_hook((void*)10,next);r=ee_character_observe(&persistence.characters,(void*)10,84,8,7,5);if(!r||r->affinity.selected!=2)return 8;}
    test_handle_values[5]=42;test_handle_values[6]=7;
    stats_destroy_hook(&stats);ee_persistence_forget(&persistence,0,(void*)10);active=0;return 0;
}
#endif
static unsigned char test_no_combat(void* c,unsigned char a,unsigned char b) {(void)c;(void)a;(void)b;return 0;}
static void* test_appearance(void* c) {(void)c;return (void*)2;}
static void* test_body(void* a) {(void)a;return (void*)3;}
__declspec(dllexport) int probeTestIdentity(void) {
    test_melee_api();
    original_character_update=test_character_original;original_character_destroy=test_character_original;
    character_is_player=test_is_player;character_appearance=test_appearance;appearance_body=test_body;character_handle=test_handle;test_handle_values[5]=42;test_handle_values[6]=7;
#ifdef EE_ENABLE_PERSISTENCE
    character_stats=test_stats;ee_persistence_init(&persistence);
#endif
    character_combat=test_no_combat;
    test_character_calls=0;test_player_flag=1;active=1;character_update_hook((void*)1);
    if(!is_ally_body((void*)3)||test_character_calls!=1)return 1;
    test_player_flag=0;character_update_hook((void*)1);
    if(is_ally_body((void*)3)||test_character_calls!=2)return 2;
    test_player_flag=1;character_update_hook((void*)1);character_destroy_hook((void*)1);
    if(is_ally_body((void*)3)||test_character_calls!=4)return 3;
    active=0;return 0;
}
__declspec(dllexport) int probeTestAptitudeOverride(void) {
    EEAffinity a,b;int i,record[3];
    if(!ee_affinity_create(&a,7,1))return 1;
    a.rates[0]=60;a.rates[1]=60;a.rates[2]=80;a.rates[3]=80;a.rates[4]=100;
    for(i=0;i<25;++i)if(!ee_affinity_award_hit_rate(&a,1))return 2;
    if(a.xp[0]!=100||a.rates[0]!=60||!ee_affinity_encode(&a,record)||!ee_affinity_decode(&b,record)||b.rates[0]!=60)return 3;
    for(i=0;i<25;++i)ee_affinity_award_hit_rate(&b,0);
    return b.xp[0]==160?0:4;
}
__declspec(dllexport) int probeTestNewGameAptitude(void) {
    static EECharacters chars;unsigned int counts[5]={0},i,j;EECharacterRecord* r;
    ee_characters_init(&chars);ee_characters_set_seed_provider(fresh_aptitude_seed);
    for(i=0;i<1000;++i){r=ee_character_observe(&chars,(void*)1,10,20,99,0);if(!r||r->affinity.selected!=0)return 1;
        for(j=0;j<5;++j)if(r->affinity.rates[j]==100)++counts[j];ee_character_forget(&chars,(void*)1);}
    ee_characters_set_seed_provider(0);for(i=0;i<5;++i)if(counts[i]<80||counts[i]>330)return 2;return 0;
}
__declspec(dllexport) int probeTestAffinity(void) {unsigned int i,counts[6]={0};for(i=1;i<1000;++i){unsigned int a=affinity_seed(i,7);if(a!=affinity_seed(i,7)||a<1||a>5)return 1;++counts[a];}for(i=1;i<=5;++i)if(counts[i]<120)return 2;return 0;}
__declspec(dllexport) int probeTestFrostSetup(int slot,void* owner,void* blade,void* scene,int bone,void* tag){
    MODULE ogre=GetModuleHandleA("OgreMain_x64.dll");Entry* e;if(!ogre||slot<0||slot>=2||!owner||!blade||!scene)return 0;e=&entries[slot];
    *(void**)(&entity_mesh)=GetProcAddress(ogre,"?getMesh@Entity@Ogre@@QEBAAEBV?$SharedPtr@VMesh@Ogre@@@2@XZ");
    *(void**)(&frost_entity_create)=GetProcAddress(ogre,"?createEntity@SceneManager@Ogre@@UEAAPEAVEntity@2@AEBV?$SharedPtr@VMesh@Ogre@@@2@W4SceneMemoryMgrTypes@2@@Z");
    *(void**)(&frost_entity_material)=GetProcAddress(ogre,"?setMaterialName@Entity@Ogre@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0@Z");
    *(void**)(&frost_get_visible)=GetProcAddress(ogre,"?getVisible@MovableObject@Ogre@@QEBA_NXZ");
    *(void**)(&frost_set_visible)=GetProcAddress(ogre,"?setVisible@MovableObject@Ogre@@QEAAX_N@Z");
    *(void**)(&frost_cast_shadows)=GetProcAddress(ogre,"?setCastShadows@MovableObject@Ogre@@QEAAX_N@Z");
    *(void**)(&parent_node)=GetProcAddress(ogre,"?getParentSceneNode@MovableObject@Ogre@@QEBAPEAVSceneNode@2@XZ");
    *(void**)(&tag_position)=GetProcAddress(ogre,"?_getDerivedPosition@OldNode@Ogre@@UEBAAEBVVector3@2@XZ");
    *(void**)(&tag_rotation)=GetProcAddress(ogre,"?_getDerivedOrientation@OldNode@Ogre@@UEBAAEBVQuaternion@2@XZ");
    *(void**)(&node_create)=GetProcAddress(ogre,"?createSceneNode@SceneManager@Ogre@@UEAAPEAVSceneNode@2@W4SceneMemoryMgrTypes@2@@Z");
    *(void**)(&node_destroy)=GetProcAddress(ogre,"?destroySceneNode@SceneManager@Ogre@@UEAAXPEAVSceneNode@2@@Z");
    *(void**)(&node_add)=GetProcAddress(ogre,"?addChild@Node@Ogre@@QEAAXPEAV12@@Z");
    *(void**)(&node_position)=GetProcAddress(ogre,"?setPosition@Node@Ogre@@QEAAXAEBVVector3@2@@Z");
    *(void**)(&node_rotation)=GetProcAddress(ogre,"?setOrientation@Node@Ogre@@QEAAXMMMM@Z");
    *(void**)(&node_attach)=GetProcAddress(ogre,"?attachObject@SceneNode@Ogre@@QEAAXPEAVMovableObject@2@@Z");
    *(void**)(&original_scene_detach)=GetProcAddress(ogre,"?detachObject@SceneNode@Ogre@@QEAAXPEAVMovableObject@2@@Z");
    *(void**)(&original_attach)=GetProcAddress(ogre,"?attachObjectToBone@Entity@Ogre@@QEAAPEAVTagPoint@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@PEAVMovableObject@2@AEBVQuaternion@2@AEBVVector3@2@@Z");
    *(void**)(&original_detach)=GetProcAddress(ogre,"?detachObjectFromBone@Entity@Ogre@@QEAAXPEAVMovableObject@2@@Z");
    *(void**)(&original_destroy_entity)=GetProcAddress(ogre,"?destroyEntity@SceneManager@Ogre@@UEAAXPEAVEntity@2@@Z");
    if(!entity_mesh||!frost_entity_create||!frost_entity_material||!frost_get_visible||!frost_set_visible||!frost_cast_shadows||!parent_node||!tag_position||!tag_rotation||!node_create||!node_destroy||!node_add||!node_position||!node_rotation||!node_attach||!original_scene_detach||!original_destroy_entity)return 0;
    e->owner=owner;e->blade=blade;e->scene=scene;e->parent=parent_node(owner);e->tag=tag;e->frost_bone=bone;return e->parent&&e->tag;
}
__declspec(dllexport) int probeTestFrostStep(int slot,int wanted){if(slot<0||slot>=2)return -1;frost_update(&entries[slot],wanted);return entries[slot].frost&&entries[slot].frost_attached;}
__declspec(dllexport) int probeTestFrostVisible(int slot){if(slot<0||slot>=2||!entries[slot].frost||!frost_get_visible)return -1;return frost_get_visible(entries[slot].frost);}
__declspec(dllexport) void* probeTestFrostObject(int slot){return slot>=0&&slot<2?entries[slot].frost:0;}
__declspec(dllexport) void* probeTestFrostNode(int slot){return slot>=0&&slot<2?entries[slot].frost_node:0;}
__declspec(dllexport) int probeTestInit(void* owner,void* blade,void* tag) {
    MODULE ogre=GetModuleHandleA("OgreMain_x64.dll"),pu=GetModuleHandleA("Plugin_ParticleUniverse_x64.dll");int ok=1;
    API(parent_node,ogre,"?getParentSceneNode@MovableObject@Ogre@@QEBAPEAVSceneNode@2@XZ");API(node_scene,ogre,"?getCreator@SceneNode@Ogre@@QEBAPEAVSceneManager@2@XZ");
    API(tag_position,ogre,"?_getDerivedPosition@OldNode@Ogre@@UEBAAEBVVector3@2@XZ");API(tag_rotation,ogre,"?_getDerivedOrientation@OldNode@Ogre@@UEBAAEBVQuaternion@2@XZ");
    API(particle_manager,pu,"?getSingletonPtr@ParticleSystemManager@ParticleUniverse@@SAPEAV12@XZ");API(particle_template,pu,"?getParticleSystemTemplate@ParticleSystemManager@ParticleUniverse@@QEAAPEAVParticleSystem@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z");
    API(particle_create,pu,"?createParticleSystem@ParticleSystemManager@ParticleUniverse@@QEAAPEAVParticleSystem@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0PEAVSceneManager@Ogre@@@Z");API(particle_destroy,pu,"?destroyParticleSystem@ParticleSystemManager@ParticleUniverse@@QEAAXPEAVParticleSystem@2@PEAVSceneManager@Ogre@@@Z");
    API(particle_start,pu,"?start@ParticleSystem@ParticleUniverse@@QEAAXXZ");API(particle_update,pu,"?_update@ParticleSystem@ParticleUniverse@@QEAAXM@Z");API(particle_local,pu,"?setKeepLocal@ParticleSystem@ParticleUniverse@@QEAAX_N@Z");API(particle_count,pu,"?getNumberOfEmittedParticles@ParticleSystem@ParticleUniverse@@QEAA_KXZ");API(particle_bounds,pu,"?setBoundsAutoUpdated@ParticleSystem@ParticleUniverse@@QEAAX_NM@Z");
    API(particle_technique,pu,"?getTechnique@ParticleSystem@ParticleUniverse@@QEBAPEAVParticleTechnique@2@_K@Z");API(technique_emitter,pu,"?getEmitter@ParticleTechnique@ParticleUniverse@@QEBAPEAVParticleEmitter@2@_K@Z");API(box_height,pu,"?setHeight@BoxEmitter@ParticleUniverse@@QEAAXM@Z");
    API(set_aabb,ogre,"?setLocalAabb@MovableObject@Ogre@@QEAAXUAabb@2@@Z");API(set_queue,ogre,"?setRenderQueueGroup@MovableObject@Ogre@@UEAAXE@Z");API(node_create,ogre,"?createSceneNode@SceneManager@Ogre@@UEAAPEAVSceneNode@2@W4SceneMemoryMgrTypes@2@@Z");API(node_destroy,ogre,"?destroySceneNode@SceneManager@Ogre@@UEAAXPEAVSceneNode@2@@Z");API(node_add,ogre,"?addChild@Node@Ogre@@QEAAXPEAV12@@Z");API(node_position,ogre,"?setPosition@Node@Ogre@@QEAAXAEBVVector3@2@@Z");API(node_scale,ogre,"?setScale@Node@Ogre@@QEAAXAEBVVector3@2@@Z");API(node_rotation,ogre,"?setOrientation@Node@Ogre@@QEAAXMMMM@Z");API(node_attach,ogre,"?attachObject@SceneNode@Ogre@@QEAAXPEAVMovableObject@2@@Z");
    API(test_node_position,ogre,"?getPosition@Node@Ogre@@QEBA?AVVector3@2@XZ");
    API(test_box_height,pu,"?getHeight@BoxEmitter@ParticleUniverse@@QEBA?BMXZ");
#ifdef EE_ENABLE_TRAIL
    *(void**)(&original_scene_detach)=GetProcAddress(ogre,"?detachObject@SceneNode@Ogre@@QEAAXPEAVMovableObject@2@@Z");
    if(!original_scene_detach||!trail_resolve(ogre))ok=0;
#endif
    *(void**)(&cb_normalise_vec)=GetProcAddress(ogre,"?normalise@Vector3@Ogre@@QEAAMXZ");
    *(void**)(&cb_normalise_quat)=GetProcAddress(ogre,"?normalise@Quaternion@Ogre@@QEAAMXZ");
    *(void**)(&cb_particle_scale)=GetProcAddress(pu,"?setScale@ParticleSystem@ParticleUniverse@@QEAAXAEBVVector3@Ogre@@@Z");
    if(!cb_normalise_vec||!cb_normalise_quat)ok=0;
    if(!ok)return 0;entries[0].owner=owner;entries[0].blade=blade;entries[0].tag=tag;entries[0].born=GetTickCount64();entries[0].length=10.0f;set_ally((void*)1,owner,1,0,0,0,1);allies[0].melee_weapon=1;return 1;
}
__declspec(dllexport) int probeTestCBNativeStart(void* source) {
    if(!source||!allies[0].character||!trail_derive)return 0;
    cb_clear_flight(&cb_flights[0]);cb_clear_charge(&cb_charges[0]);
    cb_reset_flight(&cb_flights[0]);cb_flights[0].projectile=(void*)21;cb_flights[0].source=source;cb_flights[0].born=GetTickCount64();cb_flights[0].element=1;cb_flights[0].secondary=2;
    cb_charges[0]=(CBCharge){0};cb_charges[0].character=allies[0].character;cb_charges[0].position=(Vec){1,2,3};cb_charges[0].progress=.5f;cb_charges[0].sample=GetTickCount64();return 1;
}
__declspec(dllexport) void probeTestCBNativeElement(int element,int secondary){allies[0].element=element;allies[0].secondary=secondary;cb_flights[0].element=element;cb_flights[0].secondary=secondary;}
__declspec(dllexport) void probeTestCBPreviewCharge(float progress,float x,float y,float z){cb_charges[0].character=allies[0].character;cb_charges[0].position=(Vec){x,y,z};cb_charges[0].progress=progress;cb_charges[0].sample=GetTickCount64();}
__declspec(dllexport) void probeTestCBPreviewRotation(float w,float x,float y,float z){cb_charges[0].rotation=(Quat){w,x,y,z};}
__declspec(dllexport) void probeTestCBPreviewStopCharge(void){cb_clear_charge(&cb_charges[0]);}
__declspec(dllexport) void probeTestCBPreviewStopFlight(void){cb_clear_flight(&cb_flights[0]);}
__declspec(dllexport) int probeTestCBNativeTick(float dt) {cb_charges[0].sample=GetTickCount64();update(dt);return cb_flights[0].fx&&cb_charges[0].fx?1:0;}
__declspec(dllexport) int probeTestCBNativeCount(int index) {void* p=index==0?cb_flights[0].fx:index==1?cb_charges[0].fx:index==2?cb_flights[0].fx2:cb_charges[0].fx2;return p?(int)particle_count(p):-1;}
__declspec(dllexport) void probeTestCBNativeStop(void) {cb_clear_flight(&cb_flights[0]);cb_clear_charge(&cb_charges[0]);}
__declspec(dllexport) void probeTestAlly(int enabled) {set_ally((void*)1,enabled?entries[0].owner:0,enabled,0,0,0,1);if(enabled)allies[0].melee_weapon=1;}
__declspec(dllexport) void probeTestMeleeVisual(int enabled) {if(allies[0].character)allies[0].melee_weapon=enabled;}
__declspec(dllexport) int probeTestMeleeContext(int category,int ranged) {test_melee_api();test_weapon_kind=category;test_ranged_mode=ranged;return melee_context((void*)1);}
__declspec(dllexport) void probeTestHands(void* owner,void* left,void* right,int enabled) {set_ally((void*)1,owner,1,left,right,enabled,1);}
__declspec(dllexport) void probeTestElement(int element) {if(allies[0].character)allies[0].element=element;}
__declspec(dllexport) void probeTestVisualSecondary(int element) {if(allies[0].character){allies[0].secondary=element;allies[0].level=100;}}
__declspec(dllexport) int probeTestVisualStages(void) {if(visual_scale(0,0.0f)>.801f||visual_scale(20,0.0f)<.899f||visual_scale(40,0.0f)<.999f||visual_scale(80,0.0f)<1.099f||visual_scale(100,0.0f)<1.149f||visual_scale(40,.1f)<1.299f)return 0;return 1;}
__declspec(dllexport) int probeTestVisualSecondCount(void) {return entries[0].effect2?(int)particle_count(entries[0].effect2):-1;}
__declspec(dllexport) int probeTestVisualSecondHandCount(int side) {return side>=0&&side<2&&hand_effects[side].effect2?(int)particle_count(hand_effects[side].effect2):-1;}
__declspec(dllexport) void probeTestExpirePending(void) {entries[0].born=GetTickCount64()-6000;}
__declspec(dllexport) int probeTestTracked(void) {return entries[0].blade!=0;}
__declspec(dllexport) int probeTestBladeElement(void) {return entries[0].effect?entries[0].element:0;}
__declspec(dllexport) int probeTestHandElement(int side) {return side>=0 && side<2 && hand_effects[side].effect?hand_effects[side].element:0;}
__declspec(dllexport) int probeTestHandCount(int side) {HandEffect* h;if(side<0||side>1)return -1;h=&hand_effects[side];return h->effect?(int)particle_count(h->effect):-1;}
__declspec(dllexport) int probeTestStep(float dt) {update(dt);return entries[0].effect?(int)particle_count(entries[0].effect):-1;}
#ifdef EE_ENABLE_TRAIL
__declspec(dllexport) int probeTestTrailLifecycle(void)
{
    Entry* e=&entries[0];Vec p={1,2,3};Quat q1={1,0,0,0},q2={.9238795f,0,0,.3826834f};int i,element;
    if(!e->scene||!trail_supported)return 1;
    for(element=1;element<=5;++element){EETrailColour* color;trail_cleanup(e);e->element=element;
        trail_tick(e,&p,&q1,.016f);p.x+=10;trail_tick(e,&p,&q1,.016f);if(e->ribbon)return 2;
        trail_tick(e,&p,&q2,.016f);if(!e->ribbon||!e->trail_linked||!e->trail_attached)return 3;
        color=((EETrailColour* (*)(void*,U64))trail_get_colour)(e->ribbon,0);if(!color||color->a<.27f||color->a>.29f)return 4;
        trail_finish(e,.016f);if(!e->ribbon)return 6;for(i=0;i<32;++i)trail_finish(e,.016f);if(e->ribbon)return 7;
        trail_tick(e,&p,&q1,.016f);trail_tick(e,&p,&q2,.016f);if(!e->ribbon)return 8;
        for(i=0;i<40;++i)trail_tick(e,&p,&q2,.016f);if(e->ribbon||e->ribbon_node)return 5;
    }
    e->element=1;trail_cleanup(e);return 0;
}
__declspec(dllexport) int probeTestTrailDual(void)
{
    Entry *a=&entries[0],*b=&entries[1];Vec p={1,2,3};Quat q1={1,0,0,0},q2={.9238795f,0,0,.3826834f};
    if(!a->scene||!b->scene||!trail_supported)return 1;trail_cleanup(a);trail_cleanup(b);
    trail_tick(a,&p,&q1,.016f);trail_tick(b,&p,&q1,.016f);trail_tick(a,&p,&q2,.016f);trail_tick(b,&p,&q2,.016f);
    if(!a->ribbon||!b->ribbon||a->ribbon==b->ribbon)return 2;trail_cleanup(a);if(!b->ribbon)return 3;trail_cleanup(b);return 0;
}
__declspec(dllexport) int probeTestTrailActorSpace(void)
{
    Entry temp;Vec p={1,2,3},offset={80,30,0},world;Quat q1={1,0,0,0},q2={.9238795f,0,0,.3826834f};void* root;float expected_x,expected_y;unsigned z;for(z=0;z<sizeof(temp);++z)((volatile unsigned char*)&temp)[z]=0;
    if(!entries[0].scene||!trail_supported)return 1;temp.scene=entries[0].scene;temp.length=20;temp.element=1;
    root=trail_root(temp.scene,0);temp.parent=node_create(temp.scene,0);if(!root||!temp.parent)return 2;
    node_add(root,temp.parent);node_position(temp.parent,&offset);trail_derive(temp.parent,&world);
    trail_tick(&temp,&p,&q1,.016f);trail_tick(&temp,&p,&q2,.016f);if(!temp.ribbon_node)return 3;
    trail_derive(temp.ribbon_node,&world);
    expected_x=offset.x+p.x+2*(q2.x*q2.y-q2.w*q2.z)*(temp.length*.55f-5.0f);
    expected_y=offset.y+p.y+(1-2*(q2.x*q2.x+q2.z*q2.z))*(temp.length*.55f-5.0f);
    if((world.x-expected_x)*(world.x-expected_x)>.01f||(world.y-expected_y)*(world.y-expected_y)>.01f)return 4;
    node_rotation(temp.parent,.70710678f,0,0,.70710678f);trail_derive(temp.parent,&world);trail_derive(temp.ribbon_node,&world);
    {float x=offset.x-(expected_y-offset.y),y=offset.y+(expected_x-offset.x);if((world.x-x)*(world.x-x)>.01f||(world.y-y)*(world.y-y)>.01f)return 5;}
    trail_cleanup(&temp);node_destroy(temp.scene,temp.parent);return 0;
}
#endif
__declspec(dllexport) int probeTestAddSecond(void* owner,void* blade,void* tag) {if(entries[1].blade)return 0;entries[1].owner=owner;entries[1].blade=blade;entries[1].tag=tag;entries[1].born=GetTickCount64();entries[1].length=20;return 1;}
__declspec(dllexport) int probeTestSecondCount(void) {return entries[1].effect?(int)particle_count(entries[1].effect):-1;}
__declspec(dllexport) int probeTestSecondElement(void) {return entries[1].effect?entries[1].element:0;}
__declspec(dllexport) float probeTestHeight(int i) {void* tech;void* emitter;if(i<0||i>=EE_TRACK_LIMIT||!entries[i].effect)return -1;tech=particle_technique(entries[i].effect,0);emitter=tech?technique_emitter(tech,0):0;return emitter?test_box_height(emitter):-1;}
__declspec(dllexport) void probeTestCleanupSecond(void) {erase(&entries[1]);}
__declspec(dllexport) int probeTestFreeTag(void) {void* tag=entries[0].tag;test_free_calls=0;original_free_tag=test_free_original;free_tag_hook((void*)1,tag);return test_free_calls==1 && !entries[0].blade;}
__declspec(dllexport) int probeTestFreeObject(void) {void* blade=entries[0].blade;test_free_calls=0;original_free_object=test_free_original;free_object_hook((void*)1,blade);return test_free_calls==1 && !entries[0].blade;}
__declspec(dllexport) int probeTestPositionMatches(void) {const Vec* p;const Quat* q;Vec n;float dx,dy,dz,start,off;if(!entries[0].node)return 0;p=tag_position(entries[0].tag);q=tag_rotation(entries[0].tag);start=entries[0].length*(entries[0].pole?.45f:.12f);off=(start+entries[0].length*.98f)*.5f-5;test_node_position(entries[0].node,&n);dx=n.x-(p->x+2*(q->x*q->y-q->w*q->z)*off);dy=n.y-(p->y+(1-2*(q->x*q->x+q->z*q->z))*off);dz=n.z-(p->z+2*(q->y*q->z+q->w*q->x)*off);return dx*dx+dy*dy+dz*dz<.0001f;}
__declspec(dllexport) void probeTestPositions(Vec* actual,Vec* target) {if(entries[0].node){test_node_position(entries[0].node,actual);*target=*tag_position(entries[0].tag);}}
__declspec(dllexport) void probeTestCleanup(void) {erase(&entries[0]);}
#ifdef EE_ENABLE_UI
typedef struct EETestWidget {int visible,selected,font_height;EEMyGUICoord rect;} EETestWidget;
static EETestWidget test_widgets[256];static int test_widget_count,test_canvas_created,test_window_calls;
static unsigned char test_canvas_pixels[256*256*4];static void* test_mouse_focus;
static unsigned char test_window_bytes[0xe0];
static void* test_gui_instance(void){return (void*)1;}
static void* test_ui_create(void* parent,const EEGameString* type,const EEGameString* skin,const EEMyGUICoord* coord,int align,const EEGameString* name) {EETestWidget* w;(void)parent;(void)type;(void)skin;(void)name;if(align!=10||test_widget_count>=256)return 0;w=&test_widgets[test_widget_count++];w->rect=*coord;w->font_height=20;return w;}
static void test_ui_destroy(void* gui,void* widget){(void)gui;(void)widget;}
static void test_ui_visible(void* widget,unsigned char on){((EETestWidget*)widget)->visible=on;}
static void test_ui_caption(void* widget,const EEGameString* text){(void)widget;(void)text;}
static void test_ui_selected(void* widget,unsigned char on){((EETestWidget*)widget)->selected=on;}
static void test_ui_font(void* widget,const EEGameString* name){(void)widget;(void)name;}
static int test_ui_font_height(void* widget){return ((EETestWidget*)widget)->font_height;}
static void test_ui_set_font_height(void* widget,int height){((EETestWidget*)widget)->font_height=height;}
static unsigned char test_ui_canvas_created(void* widget){(void)widget;return (unsigned char)test_canvas_created;}
static void test_ui_canvas_create(void* widget,int w,int h,int mode,int usage,int format){(void)widget;if(w==256&&h==256&&mode==0&&usage==18&&format==4)test_canvas_created=1;}
static int test_ui_canvas_dimension(void* widget){(void)widget;return 256;}
static void* test_ui_canvas_lock(void* widget,int usage){(void)widget;return usage==16?test_canvas_pixels:0;}
static void test_ui_canvas_unlock(void* widget){(void)widget;}
static void test_ui_set_size(void* widget,int width,int height){(void)widget;(void)width;(void)height;}
static void test_ui_set_position(void* widget,int x,int y){((EETestWidget*)widget)->rect.left=x;((EETestWidget*)widget)->rect.top=y;}
static int test_ui_width(void* window){(void)window;return 700;}
static int test_ui_height(void* window){(void)window;return 500;}
static unsigned char test_ui_window_visible(void* window){(void)window;return 1;}
static void* test_ui_focus(void* input){(void)input;return test_mouse_focus;}
static void* test_ui_parent(void* widget){(void)widget;return 0;}
static void test_ui_window_original(void* window){(void)window;++test_window_calls;}
static unsigned char test_ui_mouse_original(void* input,int x,int y,int button){(void)input;(void)x;(void)y;(void)button;return 1;}
__declspec(dllexport) int probeTestUIHooks(void)
{
    unsigned char* window=test_window_bytes;EETestStats stats={{0,0},(void*)9};EETestGameData data={{0,0,0},0};int slot;EEUIView* view;
    *(void**)(window+8)=(void*)7;*(void**)(window+0xd0)=(void*)9;
    ee_persistence_init(&persistence);test_widget_count=0;test_canvas_created=0;test_window_calls=0;ui_failed=0;ui_panel_preference=1;ui_scale_tenths=10;ee_ui_view_set_scale(10);active=1;test_player_flag=1;
    character_is_player=test_is_player;character_handle=test_handle;character_stats=test_stats;test_handle_values[5]=42;test_handle_values[6]=7;
    mygui_api.gui_instance=test_gui_instance;mygui_api.create_widget=test_ui_create;mygui_api.destroy_widget=test_ui_destroy;mygui_api.set_visible=test_ui_visible;mygui_api.set_caption=test_ui_caption;mygui_api.set_selected=test_ui_selected;mygui_api.set_font=test_ui_font;mygui_api.get_font_height=test_ui_font_height;mygui_api.set_font_height=test_ui_set_font_height;
    mygui_api.canvas_created=test_ui_canvas_created;mygui_api.canvas_create=test_ui_canvas_create;mygui_api.canvas_width=test_ui_canvas_dimension;mygui_api.canvas_height=test_ui_canvas_dimension;mygui_api.canvas_lock=test_ui_canvas_lock;mygui_api.canvas_unlock=test_ui_canvas_unlock;mygui_api.set_size=test_ui_set_size;mygui_api.set_position=test_ui_set_position;ee_mygui_backend(&mygui_api,&ui_api);
    stats_window_width=test_ui_width;stats_window_height=test_ui_height;stats_window_visible=test_ui_window_visible;input_focus=test_ui_focus;widget_parent=test_ui_parent;
    original_stats_window_update=test_ui_window_original;original_stats_window_clear=test_ui_window_original;original_stats_window_destroy=test_ui_window_original;original_mouse_press=test_ui_mouse_original;original_mouse_move=test_ui_mouse_original;original_mouse_release=test_ui_mouse_original;
    original_stats_save=test_stats_original;
    gamedata_api.list_exists=test_list_exists;gamedata_api.list_size=test_list_size;gamedata_api.list_name=test_list_name;gamedata_api.list_values=test_list_values;gamedata_api.find_reference=test_find_reference;gamedata_api.add_reference=test_add_reference;
    stats_window_update_hook(window);slot=ui_find(window);if(slot<0||test_window_calls!=1)return 1;view=&stats_views[slot].view;if(!view->model.visible)return 12;
    set_ally((void*)9,(void*)3,1,0,0,0,1);
    {int px=((EETestWidget*)view->panel)->rect.left,py=((EETestWidget*)view->panel)->rect.top;
     test_mouse_focus=view->header;mouse_press_hook((void*)2,100,100,0);mouse_move_hook((void*)2,145,130,0);mouse_release_hook((void*)2,145,130,0);
     if(((EETestWidget*)view->panel)->rect.left!=px+45||((EETestWidget*)view->panel)->rect.top!=py+30)return 22;
     mouse_move_hook((void*)2,180,180,0);if(((EETestWidget*)view->panel)->rect.left!=px+45)return 23;}
    test_mouse_focus=view->opener;if(!mouse_press_hook((void*)2,0,0,0)||view->model.visible)return 2;mouse_press_hook((void*)2,0,0,0);if(!view->model.visible)return 13;
    stats_window_update_hook(window);if(!view->graph_ready||!test_canvas_created)return 3;
    test_mouse_focus=view->buttons[4];mouse_press_hook((void*)2,0,0,0);if(!find_ally((void*)3)||find_ally((void*)3)->element!=4)return 10;stats_window_update_hook(window);if(view->model.affinity.selected!=4)return 4;
    {EECharacterRecord* r=ee_character_observe(&persistence.characters,(void*)9,42,7,0,4);if(!r)return 7;r->affinity.xp[0]=13800;stats_window_update_hook(window);
     test_mouse_focus=view->mode_second;mouse_press_hook((void*)2,0,0,0);if(!view->second_mode)return 8;
     test_mouse_focus=view->buttons[2];mouse_press_hook((void*)2,0,0,0);if(!find_ally((void*)3)||find_ally((void*)3)->secondary!=2)return 11;stats_window_update_hook(window);if(view->model.affinity.secondary!=2)return 9;}
    stats_save_hook(&stats,&data);if(!data.present||(data.value[0]&7)!=4||((data.value[0]>>16)&7)!=2)return 5;
    *(void**)(window+0xd0)=(void*)10;test_handle_values[5]=43;stats_window_update_hook(window);if(!view->model.visible||view->model.character!=(void*)10||view->model.index!=43)return 14;
    if(((EETestWidget*)view->panel)->rect.left!=stats_window_width(window)+12+45||((EETestWidget*)view->panel)->rect.top!=40)return 24;
    set_ally((void*)10,(void*)4,1,0,0,0,1);test_mouse_focus=view->buttons[3];mouse_press_hook((void*)2,0,0,0);if(!find_ally((void*)4)||find_ally((void*)4)->element!=3||!find_ally((void*)3)||find_ally((void*)3)->element!=4)return 19;
    test_mouse_focus=view->close;mouse_press_hook((void*)2,0,0,0);if(view->model.visible||ui_panel_preference)return 15;
    *(void**)(window+0xd0)=(void*)9;test_handle_values[5]=42;stats_window_update_hook(window);if(view->model.visible||view->model.character!=(void*)9)return 16;
    test_mouse_focus=view->opener;mouse_press_hook((void*)2,0,0,0);if(!view->model.visible)return 17;
    ui_scale_tenths=15;ee_ui_view_set_scale(15);stats_window_update_hook(window);if(!view->model.visible||view->model.character!=(void*)9||((EETestWidget*)view->panel)->rect.width!=795||((EETestWidget*)view->effect)->font_height!=30)return 18;ui_scale_tenths=10;ee_ui_view_set_scale(10);
    if(((EETestWidget*)view->panel)->rect.left!=stats_window_width(window)+18+45||((EETestWidget*)view->panel)->rect.top!=45)return 25;
    ui_test_native_x=0.87f;ui_test_native_y=0.97f;stats_window_update_hook(window);if(!view->model.visible||((EETestWidget*)view->panel)->rect.width!=461||((EETestWidget*)view->panel)->rect.height!=435||((EETestWidget*)view->effect)->font_height!=20)return 20;
    ui_test_native_x=1.0f;ui_test_native_y=1.0f;stats_window_update_hook(window);if(((EETestWidget*)view->panel)->rect.width!=530)return 21;
    stats_window_clear_hook(window);if(ui_find(window)>=0)return 6;set_ally((void*)9,0,0,0,0,0,0);set_ally((void*)10,0,0,0,0,0,0);
    active=0;return 0;
}
#endif
#endif
