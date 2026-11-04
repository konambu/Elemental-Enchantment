/* Private visual probe. This is NOT a release implementation: every detected
 * drawn sword gets fire; no character selection, stats, or save state yet. */
typedef unsigned long DWORD;
int _fltused=0;
typedef unsigned long long U64;
typedef void* MODULE;
typedef struct { union { char small[16]; const char* heap; } text; U64 size,capacity,allocator; } String;
typedef struct { float x,y,z; } Vec;
typedef struct { float w,x,y,z; } Quat;
#ifdef SB_ENABLE_PERSISTENCE
#include "spellblade_persistence.h"
#include "spellblade_gamedata.h"
#endif
#ifdef SB_ENABLE_UI
#include "spellblade_mygui.h"
#include "spellblade_localization.h"
#endif
typedef struct { void* owner; void* blade; void* tag; void* scene; void* parent; void* node; void* effect; void* effect2; U64 born; float length;float applied_scale; int pole; int element;int secondary;int level;
#ifdef SB_ENABLE_TRAIL
    void* ribbon;void* ribbon_node;void* ribbon_root;Vec trail_direction;int trail_has_direction;float trail_idle;int trail_element,trail_linked,trail_attached;
#endif
} Entry;
__declspec(dllimport) MODULE __stdcall GetModuleHandleA(const char*);
__declspec(dllimport) void* __stdcall GetProcAddress(MODULE,const char*);
__declspec(dllimport) DWORD __stdcall GetModuleFileNameA(MODULE,char*,DWORD);
__declspec(dllimport) unsigned int __stdcall GetPrivateProfileIntA(const char*,const char*,int,const char*);
__declspec(dllimport) DWORD __stdcall GetCurrentThreadId(void);
__declspec(dllimport) U64 __stdcall GetTickCount64(void);
__declspec(dllimport) void __stdcall AcquireSRWLockExclusive(void*);
__declspec(dllimport) void __stdcall ReleaseSRWLockExclusive(void*);
__declspec(dllimport) void* __stdcall CreateFileA(const char*,DWORD,DWORD,void*,DWORD,DWORD,void*);
__declspec(dllimport) int __stdcall WriteFile(void*,const void*,DWORD,DWORD*,void*);
__declspec(dllimport) int __stdcall CloseHandle(void*);
#define SB_TRACK_LIMIT 512
#define SB_ALLY_LIMIT 512
static Entry entries[SB_TRACK_LIMIT];
typedef struct {void* character;void* body;void* left;void* right;int unarmed;int element;int attacking;int secondary;int level;int melee_weapon;float hit_glow;} Ally;
static Ally allies[SB_ALLY_LIMIT];
typedef struct {void* character;void* body;void* bone;void* scene;void* parent;void* node;void* effect;void* effect2;float applied_scale;int element;int secondary;} HandFire;
static HandFire hand_fires[SB_ALLY_LIMIT*2];
static void* guard; static DWORD internal; static int active,logged_missing,logged_full; static U64 serial;
static char log_path[1024]; static MODULE self;
static char settings_path[1024]; static int all_aptitudes;
#ifdef SB_ENABLE_UI
static void select_game_locale(void);
#endif
typedef struct MCM_Api MCM_Api;
struct MCM_Api {
    int version; void* impl;
    void (__cdecl *beginMod)(MCM_Api*,const char*,const char*,const char*,void (__cdecl *)(void));
    void (__cdecl *section)(MCM_Api*,const char*);
    void (__cdecl *toggle)(MCM_Api*,const char*,const char*,const char*,const char*,int,unsigned);
};
static void __cdecl reload_settings(void) {
    all_aptitudes=settings_path[0] && GetPrivateProfileIntA("General","AllAptitudes100",0,settings_path)!=0;
}
__declspec(dllexport) void __cdecl MCM_Describe(MCM_Api* api) {
    if(!api || api->version<1 || !api->beginMod || !api->section || !api->toggle || !settings_path[0])return;
#ifdef SB_ENABLE_UI
    select_game_locale();
#endif
    api->beginMod(api,"ElementalEnchantment","Elemental Enchantment",settings_path,reload_settings);
#ifdef SB_ENABLE_UI
    api->section(api,sb_locale_get()->mcm_section);
    api->toggle(api,"General","AllAptitudes100",sb_locale_get()->mcm_label,sb_locale_get()->mcm_tip,0,0);
#else
    api->section(api,"Aptitude");
    api->toggle(api,"General","AllAptitudes100","All elemental aptitudes 100%","All characters gain elemental experience at 100% speed.",0,0);
#endif
}

static void clear_crossbow_scene(void*);
static void clear_crossbow_character(void*);
static void clear_crossbow_source(void*);
static void clear_crossbow_projectile(void*);
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
static void (*node_add)(void*,void*); static void (*node_position)(void*,const Vec*);
static void (*node_scale)(void*,const Vec*);
static void (*node_rotation)(void*,float,float,float,float); static void (*node_attach)(void*,void*);
static void* (*original_attach)(void*,const void*,void*,const void*,const void*);
static void (*original_detach)(void*,void*); static void (*original_detach_all)(void*);
static void (*original_clear)(void*); static void (*original_destroy_entity)(void*,void*);
static void (*original_destroy_node)(void*,void*); static void (*original_scene_detach)(void*,void*); static void (*original_gpu)(void*,float);
static void (*original_free_tag)(void*,void*); static void (*original_free_object)(void*,void*);
#ifdef SB_ENABLE_TRAIL
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
#ifdef SB_ENABLE_PERSISTENCE
static SBPersistence persistence;
static SBGameDataAPI gamedata_api;
static void* (*character_stats)(void*);
static void (*original_stats_load)(void*,void*);
static void (*original_stats_save)(void*,void*);
static void (*original_stats_destroy)(void*);
static void (*original_character_set_handle)(void*,const void*);
static int (*original_character_hit)(void*,int,void*,void*,void*,int);
static void* (*original_crossbow_wound)(void*,unsigned char,int,void*,int*,void*,int*,void*);
static float (*original_attack_speed)(void*);
static float (*original_block_speed)(void*);
#endif
/* Caller holds guard. Entries contain identities only; no stored character is dereferenced. */
static Ally* find_ally(void* body) {int i;if(!body)return 0;for(i=0;i<SB_ALLY_LIMIT;++i)if(allies[i].body==body&&allies[i].character)return &allies[i];return 0;}
#ifdef SPELLBLADE_NATIVE_TEST
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
static Ally* set_ally(void* character,void* body,int is_player,void* left,void* right,int unarmed,int element) {int i,slot=-1;if(!character)return 0;for(i=0;i<SB_ALLY_LIMIT;++i){if(allies[i].character==character){slot=i;break;}if(slot<0 && !allies[i].character)slot=i;}if(slot>=0){if(is_player&&body){
    if(allies[slot].character!=character){allies[slot].secondary=0;allies[slot].level=0;allies[slot].hit_glow=0.0f;allies[slot].attacking=0;allies[slot].melee_weapon=0;}
#ifdef SB_ENABLE_PERSISTENCE
    allies[slot].element=element;
#else
    if(allies[slot].character!=character)allies[slot].element=element;
#endif
    allies[slot].character=character;allies[slot].body=body;allies[slot].left=left;allies[slot].right=right;allies[slot].unarmed=unarmed;return &allies[slot];}else if(allies[slot].character==character){allies[slot].character=0;allies[slot].body=0;allies[slot].left=0;allies[slot].right=0;allies[slot].unarmed=0;allies[slot].element=0;}}return 0;}
static void forget_body(void* body) {int i;for(i=0;i<SB_ALLY_LIMIT;++i)if(allies[i].body==body){allies[i].character=0;allies[i].body=0;}}
static void logline(const char* s) { DWORD n=0,w; void* f; while(s[n])++n; f=CreateFileA(log_path,4,3,0,4,0x80,0); if(f!=(void*)(U64)-1) {WriteFile(f,s,n,&w,0);CloseHandle(f);} }
/* Cache profile IDs for loaded characters. Caller holds guard; cached pointer
 * values are only compared, never dereferenced. */
typedef struct { void* character; unsigned int index, generation; int boss_profile; } BossDiagSeen;
typedef struct { const char *id; int primary,secondary,level; } BossProfile;
static const BossProfile boss_profiles[]={
    {"17225-Dialogue.mod",1,2,80}, {"59164-Dialogue.mod",2,1,65},
    {"59165-Dialogue.mod",1,2,65}, {"51751-Dialogue.mod",2,5,60},
    {"43159-Dialogue.mod",4,3,80}, {"59030-Dialogue.mod",3,2,90},
    {"1533858-rebirth.mod",2,5,100}, {"51333-rebirth.mod",5,4,90},
    {"50350-rebirth.mod",3,5,70}, /* Moll */
    {"56727-Dialogue.mod",4,1,70}, /* Flying Bull */
    {"56730-Dialogue.mod",1,3,65}, /* Ghost */
    {"98164-Dialogue.mod",4,3,65}, /* Tora */
    {"62981-Dialogue.mod",5,2,85}, /* Savant */
    {"95756-__december17.mod",1,4,80}, /* Valamon */
    {"62827-rebirth.mod",4,5,80}, /* Crab Queen */
    {"62807-rebirth.mod",2,5,85}, /* Skeleton Bandits' Elder */
    {"96407-Dialogue.mod",4,5,75}, /* King Gurgler */
    {"48447-Dialogue.mod",4,1,65}, /* Big Grim */
    {"56576-Dialogue.mod",2,4,85}, /* Eyegore */
    {"56365-Dialogue.mod",2,5,70}, /* Longen */
    {"65004-Dialogue.mod",3,1,60}, /* Boss Simion */
    {"56824-Dialogue.mod",1,2,65}, /* The Preacher */
    {"56887-Dialogue.mod",3,5,60}, /* Buzan */
    {"56888-Dialogue.mod",5,3,60}, /* Dimak */
    {"56889-Dialogue.mod",1,4,60}, /* Red Sabre Boss */
    {"48718-Dialogue.mod",5,4,60}, /* Big Gray */
    {"48738-Dialogue.mod",4,1,60}, /* Big Al */
    {"63884-Dialogue.mod",3,5,65}, /* Shade */
    {"96409-rebirth.mod",2,1,80}, /* Screamer the False */
    {"96407-rebirth.mod",5,2,80}, /* Ponk */
    {"1533326-__world reactions Shek.mod",4,1,85}, /* Mukai */
    {"50519-rebirth.mod",1,3,75} /* Cannibal Grand Wizard */
};
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
      for(j=0;text && j<sizeof(boss_profiles)/sizeof(boss_profiles[0]);++j) {
        const char* target=boss_profiles[j].id;
        for(i=0;i<sid->size && target[i] && text[i]==target[i];++i){}
        if(i==sid->size && !target[i]){ profile=(int)j+1; break; }
      }
    }
    boss_diag_seen[slot].character=character;boss_diag_seen[slot].index=index;boss_diag_seen[slot].generation=generation;boss_diag_seen[slot].boss_profile=profile;++boss_diag_count;
    return profile;
}
#ifdef SB_ENABLE_TRAIL
#include "spellblade_trail.inl"
#endif
static int eq(const String* s,const char* text) { U64 i; const char* p; if(!s || s->size>1024 || s->capacity<s->size)return 0; p=s->capacity<16?s->text.small:s->text.heap; if(!p)return 0; for(i=0;i<s->size && text[i];++i)if(p[i]!=text[i])return 0; return i==s->size && !text[i]; }
static int has(const String* s,const char* part) { U64 i,j;const char* p; if(!s || !s->size || s->size>1024 || s->capacity<s->size)return 0;p=s->capacity<16?s->text.small:s->text.heap;if(!p)return 0;for(i=0;i<s->size;++i) {for(j=0;part[j] && i+j<s->size;++j) {char x=p[i+j];if(x>='A' && x<='Z')x+='a'-'A';if(x!=part[j])break;}if(!part[j])return 1;}return 0; }
static int blade_name(const String* s) {return has(s,"katana")||has(s,"sword")||has(s,"sabre")||has(s,"blade");}
static void cleanup(Entry* e) {
#ifdef SB_ENABLE_TRAIL
    trail_cleanup(e);
#endif
    if(e->effect2 && e->scene)particle_destroy(particle_manager(),e->effect2,e->scene);if(e->effect && e->scene)particle_destroy(particle_manager(),e->effect,e->scene);if(e->node && e->scene)node_destroy(e->scene,e->node);e->effect2=0;e->secondary=0;e->effect=0;e->node=0;e->scene=0;e->parent=0;e->element=0;e->applied_scale=0.0f;}
static void erase(Entry* e) {cleanup(e);e->owner=0;e->blade=0;e->tag=0;}
static void clear_hand(HandFire* h) {if(h->effect2 && h->scene)particle_destroy(particle_manager(),h->effect2,h->scene);if(h->effect && h->scene)particle_destroy(particle_manager(),h->effect,h->scene);if(h->node && h->scene)node_destroy(h->scene,h->node);h->character=0;h->body=0;h->bone=0;h->scene=0;h->parent=0;h->node=0;h->effect=0;h->effect2=0;h->secondary=0;h->element=0;h->applied_scale=0.0f;}
static void clear_character_hands(void* character) {int i;if(!character)return;for(i=0;i<SB_ALLY_LIMIT*2;++i)if(hand_fires[i].character==character)clear_hand(&hand_fires[i]);}
static void* attach_hook(void* owner,const void* bone,void* object,const void* orientation,const void* offset) {
    void* tag=original_attach(owner,bone,object,orientation,offset);int i,pole;void** mesh;void* box;const Vec* max;float length;
    if(!active||internal==GetCurrentThreadId()||!tag||!object)return tag;
    __try {if(*(void**)object!=entity_vtable)return tag;
        mesh=(void**)entity_mesh(object);if(!mesh || !*mesh)return tag;
        if(!eq((const String*)bone,"Bip01 Prop2") && !eq((const String*)bone,"Bip01 Prop1") &&
           !(eq((const String*)bone,"Bip01 L Hand") && (has(resource_name(*mesh),"_bare") || blade_name(resource_name(*mesh)))))return tag;
        if(has(resource_name(*mesh),"sheath") || has(resource_name(*mesh),"crossbow") || has(resource_name(*mesh),"arcane_focus") || has(resource_name(*mesh),"holster"))return tag;
        box=mesh_bounds(*mesh);max=box?bounds_maximum(box):0;length=max?max->y:10.0f;pole=has(resource_name(*mesh),"polearm")||has(resource_name(*mesh),"spear");
        AcquireSRWLockExclusive(&guard);for(i=0;i<SB_TRACK_LIMIT;++i)if(entries[i].blade==object)break;
        if(i==SB_TRACK_LIMIT){for(i=0;i<SB_TRACK_LIMIT;++i)if(!entries[i].blade){entries[i].owner=owner;entries[i].blade=object;entries[i].tag=tag;entries[i].born=GetTickCount64();entries[i].length=(length>2 && length<80)?length:10.0f;entries[i].pole=pole;break;}if(i==SB_TRACK_LIMIT && !logged_full){logged_full=1;logline("CAPACITY: all weapon tracking slots are in use.\r\n");}}
        ReleaseSRWLockExclusive(&guard);
    } __except(1) {logline("SKIP: blade metadata unavailable.\r\n");} return tag;
}
static void detach_hook(void* owner,void* blade) {int i;if(internal==GetCurrentThreadId()){original_detach(owner,blade);return;}AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {for(i=0;i<SB_TRACK_LIMIT;++i)if(entries[i].owner==owner && entries[i].blade==blade)erase(&entries[i]);} __finally {internal=0;ReleaseSRWLockExclusive(&guard);}original_detach(owner,blade);}
static void detach_all_hook(void* owner) {int i;if(internal==GetCurrentThreadId()){original_detach_all(owner);return;}AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {for(i=0;i<SB_TRACK_LIMIT;++i)if(entries[i].owner==owner)erase(&entries[i]);} __finally {internal=0;ReleaseSRWLockExclusive(&guard);}original_detach_all(owner);}
static void clear_hook(void* scene) {int i;if(internal==GetCurrentThreadId()){original_clear(scene);return;}AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {clear_crossbow_scene(scene);for(i=0;i<SB_TRACK_LIMIT;++i)if(entries[i].blade && (!entries[i].scene || entries[i].scene==scene))erase(&entries[i]);for(i=0;i<SB_ALLY_LIMIT*2;++i)if(hand_fires[i].scene==scene)clear_hand(&hand_fires[i]);for(i=0;i<SB_ALLY_LIMIT;++i){allies[i].character=0;allies[i].body=0;}} __finally {internal=0;ReleaseSRWLockExclusive(&guard);}original_clear(scene);}
static void entity_hook(void* scene,void* object) {int i;if(internal==GetCurrentThreadId()){original_destroy_entity(scene,object);return;}AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {for(i=0;i<SB_TRACK_LIMIT;++i)if(entries[i].owner==object || entries[i].blade==object)erase(&entries[i]);for(i=0;i<SB_ALLY_LIMIT*2;++i)if(hand_fires[i].body==object)clear_hand(&hand_fires[i]);forget_body(object);} __finally {internal=0;ReleaseSRWLockExclusive(&guard);}original_destroy_entity(scene,object);}
static void node_hook(void* scene,void* node) {int i;if(internal==GetCurrentThreadId()){original_destroy_node(scene,node);return;}AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {clear_crossbow_source(node);for(i=0;i<SB_TRACK_LIMIT;++i)if(entries[i].parent==node)erase(&entries[i]);for(i=0;i<SB_ALLY_LIMIT*2;++i)if(hand_fires[i].parent==node)clear_hand(&hand_fires[i]);} __finally {internal=0;ReleaseSRWLockExclusive(&guard);}original_destroy_node(scene,node);}
static void scene_detach_hook(void* node,void* object) {int i;if(internal==GetCurrentThreadId()){original_scene_detach(node,object);return;}AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {for(i=0;i<SB_TRACK_LIMIT;++i)if(entries[i].owner==object)cleanup(&entries[i]);for(i=0;i<SB_ALLY_LIMIT*2;++i)if(hand_fires[i].body==object)clear_hand(&hand_fires[i]);} __finally {internal=0;ReleaseSRWLockExclusive(&guard);}original_scene_detach(node,object);}
static void free_tag_hook(void* skeleton,void* tag) {int i;if(internal==GetCurrentThreadId()){original_free_tag(skeleton,tag);return;}AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {for(i=0;i<SB_TRACK_LIMIT;++i)if(entries[i].tag==tag && tag)erase(&entries[i]);} __finally {internal=0;ReleaseSRWLockExclusive(&guard);}original_free_tag(skeleton,tag);}
static void free_object_hook(void* skeleton,void* object) {int i;if(internal==GetCurrentThreadId()){original_free_object(skeleton,object);return;}AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {for(i=0;i<SB_TRACK_LIMIT;++i)if(entries[i].blade==object && object)erase(&entries[i]);} __finally {internal=0;ReleaseSRWLockExclusive(&guard);}original_free_object(skeleton,object);}
#ifdef SB_ENABLE_PERSISTENCE
static int failed_save_read(void* context,int out[3]) {(void)context;(void)out;return -1;}
/* Caller holds guard. Player saves stay untouched; non-player profiles are
 * transient and never inserted into persistence or awarded experience. */
static int combat_affinity_locked(void* character,void* stats,SBAffinity* out)
{
    const unsigned char* h; unsigned int index,serial_number; int boss;
    if(!character||!out)return 0;
    if(character_is_player(character)) {
        SBCharacterRecord* r;
        h=(const unsigned char*)character_handle(character);if(!stats||!h)return 0;
        index=*(const unsigned int*)(h+0x14);serial_number=*(const unsigned int*)(h+0x18);
        r=sb_persistence_bind(&persistence,stats,character,index,serial_number,index^(serial_number*0x9e3779b9u),(unsigned int)element_from_character(character));
        if(!r||r->read_only||!r->affinity.selected)return 0;
        *out=r->affinity;return 1;
    }
    boss=boss_diag_character(character);if(!boss)return 0;
    { const BossProfile* p=&boss_profiles[boss-1];unsigned int l=(unsigned int)p->level;
      if(!sb_affinity_create(out,0,(unsigned int)p->primary))return 0;
      out->xp[0]=(unsigned short)(4u*(20u*l+l*(l-1u)));
      out->secondary=(unsigned int)p->secondary;
    }
    return out->selected!=0;
}
static float element_speed(void* stats,float base,int attacking)
{
    float result=base;if(!active||!stats||internal==GetCurrentThreadId())return base;
    __try {void* character=*(void**)((unsigned char*)stats+0x10);
        if(character&&melee_context(character)){SBAffinity a;AcquireSRWLockExclusive(&guard);
            __try {if(combat_affinity_locked(character,stats,&a))result=attacking?sb_affinity_attack_speed(&a,base):sb_affinity_combat_speed(&a,base);}
            __finally {ReleaseSRWLockExclusive(&guard);}
        }
    } __except(1) {logline("WIND: speed bonus skipped after native error.\r\n");}
    return result;
}
static float attack_speed_hook(void* stats){float base=original_attack_speed(stats);return element_speed(stats,base,1);}
static float block_speed_hook(void* stats){float base=original_block_speed(stats);return element_speed(stats,base,0);}
static int character_hit_hook(void* target,int direction,void* damage,void* attacker,void* technique,int combo)
{
    int result,has_bonus=0;SBDamage original,modified,bonus;
    if(active&&damage&&attacker&&target&&attacker!=target&&internal!=GetCurrentThreadId())__try {
        if(melee_context(attacker)){void* stats=character_stats(attacker);SBAffinity a;AcquireSRWLockExclusive(&guard);
            __try {if(combat_affinity_locked(attacker,stats,&a)){original=*(SBDamage*)damage;modified=original;has_bonus=sb_affinity_damage(&a,&modified);}}
            __finally {ReleaseSRWLockExclusive(&guard);}
        }
    } __except(1) {has_bonus=0;logline("EFFECT: hit bonus skipped after native error.\r\n");}
    if(has_bonus){bonus.cut=modified.cut-original.cut;bonus.blunt=modified.blunt-original.blunt;bonus.pierce=modified.pierce-original.pierce;bonus.stun=modified.stun-original.stun;bonus.bleed=modified.bleed-original.bleed;bonus.armour=modified.armour-original.armour;
        result=original_character_hit(target,direction,&modified,attacker,technique,combo);
        __try {SBDamage* d=(SBDamage*)damage;d->cut=modified.cut-bonus.cut;d->blunt=modified.blunt-bonus.blunt;d->pierce=modified.pierce-bonus.pierce;d->stun=modified.stun-bonus.stun;d->bleed=modified.bleed-bonus.bleed;d->armour=modified.armour-bonus.armour;} __except(1) {logline("EFFECT: output copy skipped after native error.\r\n");}
    }else result=original_character_hit(target,direction,damage,attacker,technique,combo);
    if(active&&attacker&&target&&attacker!=target&&(result==1||result==2||result==6)&&internal!=GetCurrentThreadId())__try {
        if(character_is_player(attacker)&&melee_context(attacker)){void* stats=character_stats(attacker);const unsigned char* h=(const unsigned char*)character_handle(attacker);
            if(stats&&h){unsigned int index=*(const unsigned int*)(h+0x14),serial=*(const unsigned int*)(h+0x18);AcquireSRWLockExclusive(&guard);
                __try {SBCharacterRecord* r=sb_persistence_bind(&persistence,stats,attacker,index,serial,index^(serial*0x9e3779b9u),(unsigned int)element_from_character(attacker));if(r&&!r->read_only&&sb_affinity_award_hit_rate(&r->affinity,all_aptitudes)){int k;if(sb_affinity_level(&r->affinity,0)>=40)for(k=0;k<SB_ALLY_LIMIT;++k)if(allies[k].character==attacker){allies[k].hit_glow=.18f;break;}}}
                __finally {ReleaseSRWLockExclusive(&guard);}
            }
        }
    } __except(1) {logline("XP: hit ignored after native error.\r\n");}
    return result;
}
static void* crossbow_wound_hook(void* medical,unsigned char low,int direction,void* damage,int* material,void* attacker,int* attackdir,void* bolt)
{
    int applied=0;SBDamage modified,original,bonus;void* result;void* stats=0;unsigned int index=0,serial_number=0;
    if(active&&bolt&&damage&&attacker&&internal!=GetCurrentThreadId())__try {
        {int player=character_is_player(attacker);const unsigned char* h=player?(const unsigned char*)character_handle(attacker):0;void* character_stat=character_stats(attacker);SBAffinity a;
            if(player&&character_stat&&h){stats=character_stat;index=*(const unsigned int*)(h+0x14);serial_number=*(const unsigned int*)(h+0x18);}
            AcquireSRWLockExclusive(&guard);
            __try {if(combat_affinity_locked(attacker,character_stat,&a)){float pierce;original=*(SBDamage*)damage;modified=original;pierce=modified.pierce>0.0f?modified.pierce:0.0f;modified.cut+=pierce;applied=sb_affinity_damage(&a,&modified);modified.cut-=pierce;}}
            __finally {ReleaseSRWLockExclusive(&guard);}
        }
    } __except(1){applied=0;stats=0;logline("CROSSBOW: damage skipped after error.\r\n");}
    if(applied){bonus.cut=modified.cut-original.cut;bonus.blunt=modified.blunt-original.blunt;bonus.pierce=modified.pierce-original.pierce;bonus.stun=modified.stun-original.stun;bonus.bleed=modified.bleed-original.bleed;bonus.armour=modified.armour-original.armour;
        result=original_crossbow_wound(medical,low,direction,&modified,material,attacker,attackdir,bolt);
        __try {SBDamage* d=(SBDamage*)damage;d->cut=modified.cut-bonus.cut;d->blunt=modified.blunt-bonus.blunt;d->pierce=modified.pierce-bonus.pierce;d->stun=modified.stun-bonus.stun;d->bleed=modified.bleed-bonus.bleed;d->armour=modified.armour-bonus.armour;}__except(1){}
    }else result=original_crossbow_wound(medical,low,direction,damage,material,attacker,attackdir,bolt);
    if(bolt&&active&&internal!=GetCurrentThreadId()){AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try{clear_crossbow_projectile(bolt);}__finally{internal=0;ReleaseSRWLockExclusive(&guard);}}
    if(result&&stats&&attacker&&bolt&&active)__try {AcquireSRWLockExclusive(&guard);
        __try {SBCharacterRecord* r=sb_persistence_bind(&persistence,stats,attacker,index,serial_number,index^(serial_number*0x9e3779b9u),(unsigned int)element_from_character(attacker));if(r&&!r->read_only)sb_affinity_award_hit_rate(&r->affinity,all_aptitudes);}
        __finally {ReleaseSRWLockExclusive(&guard);}
    }__except(1){logline("CROSSBOW: hit XP skipped after error.\r\n");}
    return result;
}
static void stats_load_hook(void* stats,void* data) {
    if(active&&stats&&data&&internal!=GetCurrentThreadId())__try {SBGameDataContext c={data,&gamedata_api};SBSaveBackend b={&c,sb_gamedata_read,sb_gamedata_write};int status;
        AcquireSRWLockExclusive(&guard);__try {status=sb_persistence_stage(&persistence,stats,&b);} __finally {ReleaseSRWLockExclusive(&guard);}
        if(status==SB_SAVE_UNSUPPORTED)logline("SAVE: unknown affinity version retained without changes.\r\n");
    } __except(1) {
        /* A fault while reading must not turn unknown saved data into a new,
         * writable default. Stage a read-only sentinel without touching data. */
        SBSaveBackend failed={0,failed_save_read,0};
        AcquireSRWLockExclusive(&guard);
        __try {sb_persistence_stage(&persistence,stats,&failed);} __finally {ReleaseSRWLockExclusive(&guard);}
        logline("SAVE: affinity read failed; preserving data as read-only.\r\n");
    }
    original_stats_load(stats,data);
}
static void stats_save_hook(void* stats,void* data) {
    original_stats_save(stats,data);
    if(active&&stats&&data&&internal!=GetCurrentThreadId())__try {void* character=*(void**)((unsigned char*)stats+0x10);const unsigned char* h=character?(const unsigned char*)character_handle(character):0;
        if(h){SBGameDataContext c={data,&gamedata_api};SBSaveBackend b={&c,sb_gamedata_read,sb_gamedata_write};int status;
            AcquireSRWLockExclusive(&guard);__try {status=sb_persistence_save(&persistence,character,*(const unsigned int*)(h+0x14),*(const unsigned int*)(h+0x18),&b);} __finally {ReleaseSRWLockExclusive(&guard);}
            if(status==SB_SAVE_ERROR||status==SB_SAVE_UNSUPPORTED)logline("SAVE: affinity was not written.\r\n");
        }
    } __except(1) {logline("SAVE: skipped affinity write after error.\r\n");}
}
static void stats_destroy_hook(void* stats) {if(internal!=GetCurrentThreadId()){AcquireSRWLockExclusive(&guard);sb_persistence_forget(&persistence,stats,0);ReleaseSRWLockExclusive(&guard);}original_stats_destroy(stats);}
static void character_set_handle_hook(void* character,const void* next) {
    int started=0;unsigned int old_index=0,old_serial=0;const unsigned char* h;
    if(active&&character&&internal!=GetCurrentThreadId())__try {h=(const unsigned char*)character_handle(character);if(h){old_index=*(const unsigned int*)(h+0x14);old_serial=*(const unsigned int*)(h+0x18);AcquireSRWLockExclusive(&guard);started=sb_character_handle_begin(&persistence.characters,character,old_index,old_serial);ReleaseSRWLockExclusive(&guard);}} __except(1) {started=0;}
    original_character_set_handle(character,next);
    if(started){unsigned int index=old_index,serial_number=old_serial;__try {h=(const unsigned char*)character_handle(character);if(h){index=*(const unsigned int*)(h+0x14);serial_number=*(const unsigned int*)(h+0x18);}} __except(1) {logline("SAVE: handle continuity check failed.\r\n");}AcquireSRWLockExclusive(&guard);sb_character_handle_end(&persistence.characters,character,index,serial_number);ReleaseSRWLockExclusive(&guard);}
}
#endif
static void character_update_hook(void* character) {
    void* app=0;void* body=0;void* skeleton=0;void* left=0;void* right=0;int player=0,unarmed=0,element=1;
    int secondary=0,level=0,melee=0,boss=0;
#ifdef SB_ENABLE_TRAIL
    int attacking=0;
#endif
#ifdef SB_ENABLE_PERSISTENCE
    void* stats=0;unsigned int index=0,serial_number=0;int has_handle=0;
#endif
    String hands={{"hands"},5,15,0},lb={{"Bip01 L Hand"},12,15,0},rb={{"Bip01 R Hand"},12,15,0};
    if(active && internal!=GetCurrentThreadId())__try {
        player=character_is_player(character);
        if(!player){AcquireSRWLockExclusive(&guard);__try {__try {boss=boss_diag_character(character);} __except(1) {boss=0;}} __finally {ReleaseSRWLockExclusive(&guard);}}
        if(player||boss){element=player?element_from_character(character):boss_profiles[boss-1].primary;melee=melee_context(character);
#ifdef SB_ENABLE_PERSISTENCE
            {const unsigned char* h=(const unsigned char*)character_handle(character);if(h){index=*(const unsigned int*)(h+0x14);serial_number=*(const unsigned int*)(h+0x18);has_handle=1;}stats=character_stats(character);}
#endif
            app=character_appearance(character);if(app){body=appearance_body(app);unarmed=melee==1 && character_combat(character,1,0) && !appearance_attached(app,&hands);if(unarmed){skeleton=appearance_skeleton(app);if(skeleton){left=skeleton_bone(skeleton,&lb);right=skeleton_bone(skeleton,&rb);}}
            }
#ifdef SB_ENABLE_TRAIL
            if(trail_character_combat&&trail_combat_state&&character_combat(character,1,0)){void* combat=trail_character_combat(character);if(combat)attacking=trail_combat_state(combat)==0;}
#endif
        }
        AcquireSRWLockExclusive(&guard);
        __try {
            if(boss){secondary=boss_profiles[boss-1].secondary;level=boss_profiles[boss-1].level;}
#ifdef SB_ENABLE_PERSISTENCE
            if(player&&body&&has_handle){SBCharacterRecord* r=sb_persistence_bind(&persistence,stats,character,index,serial_number,index^(serial_number*0x9e3779b9u),(unsigned int)element);if(r){element=(int)r->affinity.selected;if(!r->read_only){secondary=(int)r->affinity.secondary;level=(int)sb_affinity_level(&r->affinity,0);}}}
#endif
            {Ally* ally=set_ally(character,body,player||boss,left,right,unarmed,element);
            if(ally){ally->secondary=secondary;ally->level=level;ally->melee_weapon=melee==2;
#ifdef SB_ENABLE_TRAIL
                ally->attacking=attacking;
#endif
            }}
        } __finally {ReleaseSRWLockExclusive(&guard);}
    } __except(1) {logline("SKIP: player identity or hand bones unavailable.\r\n");}original_character_update(character);
}
static void character_destroy_hook(void* character) {if(internal!=GetCurrentThreadId()){AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();__try {clear_crossbow_character(character);clear_character_hands(character);set_ally(character,0,0,0,0,0,0);
#ifdef SB_ENABLE_PERSISTENCE
    sb_persistence_forget(&persistence,0,character);
#endif
    } __finally {internal=0;ReleaseSRWLockExclusive(&guard);}}original_character_destroy(character);}
#ifdef SB_ENABLE_UI
#include "spellblade_ui_runtime.inl"
#endif
static void* create_secondary_effect(void* manager,void* scene,void* node,const String* tmpl,float height)
{
    String name={{"SB_S00000000"},12,15,0};unsigned int n=(unsigned int)++serial,j;void* effect;void* tech;void* emitter;
    if(!scene||!node||!particle_template(manager,tmpl))return 0;
    for(j=0;j<8;++j)name.text.small[4+j]="0123456789abcdef"[(n>>(28-j*4))&15];
    effect=particle_create(manager,&name,tmpl,scene);if(!effect)return 0;
    if(height>0.0f){tech=particle_technique(effect,0);emitter=tech?technique_emitter(tech,0):0;if(!emitter){particle_destroy(manager,effect,scene);return 0;}box_height(emitter,height);if(tmpl->size==27||tmpl->size==23){tech=particle_technique(effect,1);emitter=tech?technique_emitter(tech,0):0;if(emitter)box_height(emitter,height);}}
    node_attach(node,effect);particle_local(effect,1);set_queue(effect,84);particle_start(effect);particle_bounds(effect,0,0);return effect;
}
#include "crossbow_visual.inl"
static float visual_scale(int level,float glow)
{float scale=level>=100?1.15f:(level>=80?1.1f:(level>=40?1.0f:(level>=20?.9f:.8f)));return glow>0.0f?scale*1.3f:scale;}
static void update_hands(void* manager,float dt) {
    int i,k;const float bounds[6]={0,0,0,20,20,20};String fire={{0},21,21,0},light={{0},26,26,0},wind={{0},21,21,0},water={{0},22,22,0},ice={{0},20,20,0};fire.text.heap="SB_Prototype_HandFire";light.text.heap="SB_Prototype_LightningHand";wind.text.heap="SB_Prototype_WindHand";water.text.heap="SB_Prototype_WaterHand";ice.text.heap="SB_Prototype_IceHand";
    for(i=0;i<SB_ALLY_LIMIT;++i)for(k=0;k<2;++k){Ally* a=&allies[i];HandFire* h=&hand_fires[i*2+k];void* bone=k?a->right:a->left;void* parent;const Vec* p;const Quat* q;
        if(!a->character||!a->body||!a->unarmed||!a->element||!bone){if(h->effect)clear_hand(h);continue;}
        parent=parent_node(a->body);if(!parent){if(h->effect)clear_hand(h);continue;}
        if(h->effect && (h->character!=a->character||h->body!=a->body||h->bone!=bone||h->parent!=parent||h->element!=a->element))clear_hand(h);
        if(!h->effect){String name={{"SB_H00000000"},12,15,0};unsigned int n=(unsigned int)++serial,j;h->character=a->character;h->body=a->body;h->bone=bone;h->parent=parent;h->scene=node_scene(parent);if(!h->scene){clear_hand(h);continue;}
            const String* tmpl=a->element==5?&ice:(a->element==4?&water:(a->element==3?&wind:(a->element==2?&light:&fire)));if(!particle_template(manager,tmpl)){clear_hand(h);continue;}h->element=a->element;for(j=0;j<8;++j)name.text.small[4+j]="0123456789abcdef"[(n>>(28-j*4))&15];h->effect=particle_create(manager,&name,tmpl,h->scene);h->node=node_create(h->scene,0);if(!h->effect||!h->node){clear_hand(h);continue;}
            node_add(parent,h->node);node_attach(h->node,h->effect);particle_local(h->effect,1);set_queue(h->effect,84);particle_start(h->effect);particle_bounds(h->effect,0,0);
        }
        if(h->effect2&&h->secondary!=a->secondary){particle_destroy(manager,h->effect2,h->scene);h->effect2=0;h->secondary=0;}
        if(a->secondary&&!h->effect2){const String* second=a->secondary==5?&ice:(a->secondary==4?&water:(a->secondary==3?&wind:(a->secondary==2?&light:&fire)));h->effect2=create_secondary_effect(manager,h->scene,h->node,second,0);if(h->effect2)h->secondary=a->secondary;}
        {float scale=visual_scale(a->level,a->hit_glow);if(scale!=h->applied_scale){Vec size={scale,scale,scale};node_scale(h->node,&size);h->applied_scale=scale;}}
        p=tag_position(bone);q=tag_rotation(bone);if(p&&q){node_position(h->node,p);node_rotation(h->node,q->w,q->x,q->y,q->z);}particle_update(h->effect,dt);set_aabb(h->effect,bounds);
        if(h->effect2){particle_update(h->effect2,dt);set_aabb(h->effect2,bounds);}
    }
}
static void update(float dt) {
    int i; void* manager; const float bounds[6]={0,0,0,60,60,60};
    String fire={{0},17,17,0},light={{0},27,27,0},wind={{0},22,22,0},water={{0},23,23,0},ice={{0},21,21,0};fire.text.heap="SB_Prototype_Fire";light.text.heap="SB_Prototype_LightningBlade";wind.text.heap="SB_Prototype_WindBlade";water.text.heap="SB_Prototype_WaterBlade";ice.text.heap="SB_Prototype_IceBlade";
    if(dt<0 || dt>.1f)dt=.016f;manager=particle_manager();if(!manager || !particle_template(manager,&fire)){if(!logged_missing){logged_missing=1;logline("WAIT: fire template not loaded yet.\r\n");}return;}
    AcquireSRWLockExclusive(&guard);internal=GetCurrentThreadId();
    __try {for(i=0;i<SB_TRACK_LIMIT;++i)if(entries[i].blade) {
        Entry* e=&entries[i];Ally* ally=find_ally(e->owner);void* parent;const Vec* p;const Quat* q;int element=ally?ally->element:0,secondary=ally?ally->secondary:0;const String* tmpl=element==5?&ice:(element==4?&water:(element==3?&wind:(element==2?&light:&fire)));e->level=ally?ally->level:0;
        if(!element||!ally->melee_weapon){if(e->effect)cleanup(e);if(!ally&&GetTickCount64()-e->born>5000)erase(e);continue;}if(e->effect && e->element!=element)cleanup(e);
        parent=parent_node(e->owner);
        if(!parent) {cleanup(e);continue;}if(e->parent && e->parent!=parent)cleanup(e);
        if(!e->effect) {String name={{"SB_F00000000"},12,15,0};unsigned int n=(unsigned int)++serial,j;
            e->scene=node_scene(parent);if(!e->scene)continue;e->parent=parent;
            for(j=0;j<8;++j)name.text.small[4+j]="0123456789abcdef"[(n>>(28-j*4))&15];
            if(!particle_template(manager,tmpl)){cleanup(e);continue;}e->element=element;e->effect=particle_create(manager,&name,tmpl,e->scene);e->node=node_create(e->scene,0);
            if(!e->effect || !e->node){cleanup(e);continue;}
            {float start=e->length*(e->pole?.45f:.12f),end=e->length*.98f;void* tech=particle_technique(e->effect,0);void* emitter=tech?technique_emitter(tech,0):0;if(!emitter){cleanup(e);continue;}box_height(emitter,end-start);if(element==2||element==4){tech=particle_technique(e->effect,1);emitter=tech?technique_emitter(tech,0):0;if(emitter)box_height(emitter,end-start);}node_add(parent,e->node);node_attach(e->node,e->effect);particle_local(e->effect,1);}
            set_queue(e->effect,84);particle_start(e->effect);particle_bounds(e->effect,0,0);
        }
        if(e->effect2&&e->secondary!=secondary){particle_destroy(manager,e->effect2,e->scene);e->effect2=0;e->secondary=0;}
        if(secondary&&!e->effect2){float start=e->length*(e->pole?.45f:.12f),end=e->length*.98f;const String* second=secondary==5?&ice:(secondary==4?&water:(secondary==3?&wind:(secondary==2?&light:&fire)));e->effect2=create_secondary_effect(manager,e->scene,e->node,second,end-start);if(e->effect2)e->secondary=secondary;}
        {float scale=visual_scale(e->level,ally->hit_glow);if(scale!=e->applied_scale){Vec size={scale,scale,scale};node_scale(e->node,&size);e->applied_scale=scale;}}
        p=tag_position(e->tag);q=tag_rotation(e->tag);if(p && q){float start=e->length*(e->pole?.45f:.12f),end=e->length*.98f,off=(start+end)*.5f-5.0f;Vec adjusted={p->x+2*(q->x*q->y-q->w*q->z)*off,p->y+(1-2*(q->x*q->x+q->z*q->z))*off,p->z+2*(q->y*q->z+q->w*q->x)*off};node_position(e->node,&adjusted);node_rotation(e->node,q->w,q->x,q->y,q->z);
#ifdef SB_ENABLE_TRAIL
            if(e->level>=20&&ally->attacking)trail_tick(e,p,q,dt);else trail_finish(e,dt);
#endif
        }
        particle_update(e->effect,dt);set_aabb(e->effect,bounds);
        if(e->effect2){particle_update(e->effect2,dt);set_aabb(e->effect2,bounds);}
    }update_hands(manager,dt);cb_update_visuals(manager,dt);for(i=0;i<SB_ALLY_LIMIT;++i)if(allies[i].hit_glow>0.0f){allies[i].hit_glow-=dt;if(allies[i].hit_glow<0.0f)allies[i].hit_glow=0.0f;}} __finally {internal=0;ReleaseSRWLockExclusive(&guard);}
}
static void gpu_hook(void* world,float dt) {original_gpu(world,dt);if(active)__try {update(dt);} __except(1) {active=0;logline("DISABLED: sword-fire update failed. Restart without probe.\r\n");}}
static int distinct_hook_targets(void* a,void* b){return a&&b&&a!=b;}
static int module_matches(MODULE m,unsigned int stamp,unsigned int size) {unsigned char* b=(unsigned char*)m;unsigned int p;if(!b || *(unsigned short*)b!=0x5a4d)return 0;p=*(unsigned int*)(b+0x3c);return *(unsigned int*)(b+p)==0x4550 && *(unsigned int*)(b+p+8)==stamp && *(unsigned int*)(b+p+80)==size;}
__declspec(dllexport) void startPlugin(void) {
    MODULE ogre=GetModuleHandleA("OgreMain_x64.dll"),pu=GetModuleHandleA("Plugin_ParticleUniverse_x64.dll"),lib=GetModuleHandleA("KenshiLib.dll"),game=GetModuleHandleA("kenshi_x64.exe");
    int ok=1;DWORD i,last=0;void* stub;void* gpu_stub;void* gpu_target;void* target;void* char_update;void* char_destroy;int (*hook)(void*,void*,void**);long long (*address)(void*);
#ifdef SB_ENABLE_PERSISTENCE
    void* stats_load;void* stats_save;void* stats_destroy;void* char_set_handle;void* char_hit;void* attack_speed;void* block_speed;void* crossbow_wound;
#endif
#ifdef SB_ENABLE_UI
    MODULE mygui=GetModuleHandleA("MyGUIEngine_x64.dll");void* ui_update;void* ui_clear;void* ui_destroy;
#endif
    if(!self || !GetModuleFileNameA(self,log_path,sizeof(log_path)))return;for(i=0;log_path[i];++i)if(log_path[i]=='\\'||log_path[i]=='/')last=i;
    for(i=0;i<=last;++i)settings_path[i]=log_path[i];
    {const char* name="ElementalEnchantment.ini";for(i=0;name[i] && last+1+i<sizeof(settings_path)-1;++i)settings_path[last+1+i]=name[i];settings_path[last+1+i]=0;}
    reload_settings();
    {const char* name="SpellbladeFireProbe.log";for(i=0;name[i];++i)log_path[last+1+i]=name[i];log_path[last+1+i]=0;}
    if(!lib || !module_matches(ogre,0x5ca5f929,0x9c9000) || !module_matches(pu,0x5ca5faa4,0x718000) || !(module_matches(game,0x6602d59d,0x232d000)||module_matches(game,0x65d604d7,0x232c000))) {logline("INACTIVE: unreviewed game or engine build.\r\n");return;}
#ifdef SB_ENABLE_UI
    if(!mygui){logline("INACTIVE: MyGUI module missing.\r\n");return;}
#endif
#define API(f,m,s) *(void**)(&f)=GetProcAddress(m,s);if(!f)ok=0
    API(hook,lib,"?AddHook@KenshiLib@@YA?AW4HookStatus@1@PEAX0PEAPEAX@Z");API(address,lib,"?GetRealAddress@KenshiLib@@YA_JPEAX@Z");
    if(!ok)return;
#ifdef SB_ENABLE_UI
    stub=GetProcAddress(lib,"?getInstance@LocaleManager@@SAPEAV1@XZ");locale_manager_instance=stub?(void*)(U64)address(stub):0;
    stub=GetProcAddress(lib,"?getCurrentLocale@LocaleManager@@QEBAPEAVLocaleInfo@@XZ");locale_current=stub?(void*)(U64)address(stub):0;
#endif
#define GAMEAPI(f,s) stub=GetProcAddress(lib,s);*(void**)(&f)=stub?(void*)(U64)address(stub):0;if(!f)ok=0
    GAMEAPI(character_is_player,"?isPlayerCharacter@Character@@QEBA_NXZ");GAMEAPI(character_appearance,"?getAppearance@Character@@QEAAPEAVAppearanceBase@@XZ");GAMEAPI(appearance_body,"?getBody@AppearanceBase@@QEBAPEAVEntity@Ogre@@XZ");
    GAMEAPI(character_handle,"?getHandle@RootObjectBase@@QEBAAEBVhand@@XZ");
#ifdef SB_ENABLE_TRAIL
    stub=GetProcAddress(lib,"?getCombatClass@Character@@QEBAPEAVCombatClass@@XZ");*(void**)(&trail_character_combat)=stub?(void*)(U64)address(stub):0;
    stub=GetProcAddress(lib,"?getCombatState@CombatClass@@QEBA?AW4swordStateEnum@@XZ");*(void**)(&trail_combat_state)=stub?(void*)(U64)address(stub):0;
    if(!trail_character_combat||!trail_combat_state)logline("TRAIL: combat state unavailable; trails disabled.\r\n");
#endif
#ifdef SB_ENABLE_PERSISTENCE
    sb_persistence_init(&persistence);
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
    stub=GetProcAddress(lib,"?getAttackSpeed@CharStats@@QEAAMXZ");attack_speed=stub?(void*)(U64)address(stub):0;
    stub=GetProcAddress(lib,"?getBlockSpeed@CharStats@@QEAAMXZ");block_speed=stub?(void*)(U64)address(stub):0;
    if(!stats_load||!stats_save||!stats_destroy||!char_set_handle||!char_hit||!attack_speed||!block_speed)ok=0;
#endif
#ifdef SB_ENABLE_UI
    GAMEAPI(stats_window_width,"?getWidth@GUIWindow@@UEBAHXZ");GAMEAPI(stats_window_height,"?getHeight@GUIWindow@@UEBAHXZ");GAMEAPI(stats_window_visible,"?_NV_isVisible@CharacterStatsWindow@@QEBA_NXZ");
    stub=GetProcAddress(lib,"?_NV_update@CharacterStatsWindow@@QEAAXXZ");ui_update=stub?(void*)(U64)address(stub):0;
    stub=GetProcAddress(lib,"?_NV_clear@CharacterStatsWindow@@QEAAXXZ");ui_clear=stub?(void*)(U64)address(stub):0;
    stub=GetProcAddress(lib,"?_DESTRUCTOR@CharacterStatsWindow@@QEAAXXZ");ui_destroy=stub?(void*)(U64)address(stub):0;
    if(!ui_update||!ui_clear||!ui_destroy)ok=0;
    API(mygui_api.gui_instance,mygui,"?getInstancePtr@?$Singleton@VGui@MyGUI@@@MyGUI@@SAPEAVGui@2@XZ");
    API(mygui_api.create_widget,mygui,"?createWidgetT@Widget@MyGUI@@QEAAPEAV12@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0AEBU?$TCoord@H@types@2@UAlign@2@0@Z");
    API(mygui_api.set_font,mygui,"?setFontName@TextBox@MyGUI@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z");
    API(mygui_api.set_style,mygui,"?setWidgetStyle@Widget@MyGUI@@QEAAXUWidgetStyle@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z");
    API(mygui_api.set_size,mygui,"?setSize@Widget@MyGUI@@QEAAXHH@Z");
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
    sb_mygui_backend(&mygui_api,&ui_api);
#endif
    GAMEAPI(character_combat,"?isInCombatMode@Character@@QEBA_N_N0@Z");GAMEAPI(appearance_attached,"?getAttachedEntity@AppearanceBase@@QEBAPEAVAttachedEntity@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z");GAMEAPI(appearance_skeleton,"?getSkeleton@AppearanceBase@@QEBAPEAVOldSkeletonInstance@Ogre@@XZ");
    GAMEAPI(weapon_category,"?getCategory@Weapon@@QEBA?AW4WeaponCategory@@XZ");character_current_weapon=current_weapon_virtual;
    API(skeleton_bone,ogre,"?getBone@Skeleton@Ogre@@UEBAPEAVOldBone@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z");
    stub=GetProcAddress(lib,"?_NV_update@Character@@QEAAXXZ");char_update=stub?(void*)(U64)address(stub):0;
    stub=GetProcAddress(lib,"?_DESTRUCTOR@Character@@QEAAXXZ");char_destroy=stub?(void*)(U64)address(stub):0;
    if(!char_update||!char_destroy)ok=0;
    entity_vtable=GetProcAddress(ogre,"??_7Entity@Ogre@@6BMovableObject@1@@");if(!entity_vtable)ok=0;
    API(entity_mesh,ogre,"?getMesh@Entity@Ogre@@QEBAAEBV?$SharedPtr@VMesh@Ogre@@@2@XZ");
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
#ifdef SB_ENABLE_TRAIL
    if(!trail_resolve(ogre))logline("TRAIL: optional ribbon API unavailable.\r\n");
#endif
    gpu_stub=GetProcAddress(lib,"?_NV_mainLoop_GPUSensitiveStuff@GameWorld@@QEAAXM@Z");if(!ok||!gpu_stub){logline("INACTIVE: required native API missing.\r\n");return;}
    *(void**)(&cb_normalise_vec)=GetProcAddress(ogre,"?normalise@Vector3@Ogre@@QEAAMXZ");
    *(void**)(&cb_normalise_quat)=GetProcAddress(ogre,"?normalise@Quaternion@Ogre@@QEAAMXZ");
    *(void**)(&cb_particle_scale)=GetProcAddress(pu,"?setScale@ParticleSystem@ParticleUniverse@@QEAAXAEBVVector3@Ogre@@@Z");
    stub=GetProcAddress(lib,"?_NV_getBarrelPos@GunClass@@QEAA?AVVector3@Ogre@@XZ");cb_barrel=stub?(void*)(U64)address(stub):0;
    gpu_target=(void*)(U64)address(gpu_stub);
#ifdef SB_ENABLE_PERSISTENCE
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
            if(ok)logline("CROSSBOW: reload, shoot and projectile hooks installed.\r\n");
        }else logline("CROSSBOW: unverified flight entry points; visuals disabled.\r\n");
    }else logline("CROSSBOW: flight unsupported on this executable.\r\n");
#ifdef SB_ENABLE_PERSISTENCE
    if(!stats_load||hook(stats_load,stats_load_hook,(void**)&original_stats_load)!=0||!original_stats_load)ok=0;
    if(!stats_save||hook(stats_save,stats_save_hook,(void**)&original_stats_save)!=0||!original_stats_save)ok=0;
    if(!stats_destroy||hook(stats_destroy,stats_destroy_hook,(void**)&original_stats_destroy)!=0||!original_stats_destroy)ok=0;
    if(!char_set_handle||hook(char_set_handle,character_set_handle_hook,(void**)&original_character_set_handle)!=0||!original_character_set_handle)ok=0;
    if(!char_hit||hook(char_hit,character_hit_hook,(void**)&original_character_hit)!=0||!original_character_hit)ok=0;
    if(!crossbow_wound||hook(crossbow_wound,crossbow_wound_hook,(void**)&original_crossbow_wound)!=0||!original_crossbow_wound)ok=0;
    if(!attack_speed||hook(attack_speed,attack_speed_hook,(void**)&original_attack_speed)!=0||!original_attack_speed)ok=0;
    if(!block_speed||hook(block_speed,block_speed_hook,(void**)&original_block_speed)!=0||!original_block_speed)ok=0;
#endif
#ifdef SB_ENABLE_UI
    if(!ui_update||hook(ui_update,stats_window_update_hook,(void**)&original_stats_window_update)!=0||!original_stats_window_update)ok=0;
    if(!ui_clear||hook(ui_clear,stats_window_clear_hook,(void**)&original_stats_window_clear)!=0||!original_stats_window_clear)ok=0;
    if(!ui_destroy||hook(ui_destroy,stats_window_destroy_hook,(void**)&original_stats_window_destroy)!=0||!original_stats_window_destroy)ok=0;
    HOOK(mygui,"?injectMousePress@InputManager@MyGUI@@QEAA_NHHUMouseButton@2@@Z",mouse_press_hook,original_mouse_press);
#endif
    target=gpu_target;if(!ok||!target||hook(target,gpu_hook,(void**)&original_gpu)!=0||!original_gpu){logline("INACTIVE: native hooks unavailable.\r\n");return;}
    active=1;logline("ACTIVE: elemental enchantment enabled.\r\n");
}
int __stdcall DllMain(MODULE module,DWORD reason,void* unused) {(void)unused;if(reason==1)self=module;return 1;}
#ifdef SPELLBLADE_NATIVE_TEST
/* Only exported in the isolated engine test build; never in the game package. */
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
#ifdef SB_ENABLE_PERSISTENCE
static void* test_stats(void* c) {(void)c;return 0;}
static void* test_hit_stats(void* c) {(void)c;return (void*)123;}
static int test_hit_result;static float test_seen_cut;
static int test_hit_original(void* target,int dir,void* dmg,void* attacker,void* tech,int combo){(void)target;(void)dir;(void)attacker;(void)tech;(void)combo;if(dmg)test_seen_cut=((SBDamage*)dmg)->cut;return test_hit_result;}
__declspec(dllexport) int probeTestBossCombatAffinity(void)
{
    unsigned int slot;SBAffinity a;SBDamage d={10,10,0,0,0,0};
    sb_persistence_init(&persistence);character_is_player=test_is_player;character_handle=test_handle;
    test_player_flag=0;test_handle_values[5]=188;test_handle_values[6]=7;
    slot=(188u^(7u*2654435761u))&4095u;
    boss_diag_seen[slot].character=(void*)9;boss_diag_seen[slot].index=188;boss_diag_seen[slot].generation=7;boss_diag_seen[slot].boss_profile=1;
    boss_diag_seen[(slot+1)&4095u].character=(void*)10;boss_diag_seen[(slot+1)&4095u].index=188;boss_diag_seen[(slot+1)&4095u].generation=7;boss_diag_seen[(slot+1)&4095u].boss_profile=0;
    if(!combat_affinity_locked((void*)9,0,&a)||a.selected!=1||a.secondary!=2||sb_affinity_level(&a,0)!=80)return 1;
    if(!sb_affinity_damage(&a,&d)||d.blunt<=10.0f||d.armour<=0.0f)return 2;
    if(combat_affinity_locked((void*)10,0,&a))return 3;
    boss_diag_seen[slot].character=0;boss_diag_seen[(slot+1)&4095u].character=0;
    return 0;
}
__declspec(dllexport) int probeTestHitXP(void)
{
    SBCharacterRecord* r;unsigned int old;int i;SBDamage d={10,10,0,0,1,0};
    sb_persistence_init(&persistence);character_is_player=test_is_player;character_handle=test_handle;character_stats=test_hit_stats;original_character_hit=test_hit_original;test_melee_api();
    test_handle_values[5]=42;test_handle_values[6]=7;test_player_flag=1;active=1;
    r=sb_persistence_bind(&persistence,(void*)123,(void*)9,42,7,99,4);if(!r)return 1;
    test_hit_result=0;character_hit_hook((void*)8,0,0,(void*)9,0,0);test_hit_result=5;character_hit_hook((void*)8,0,0,(void*)9,0,0);if(r->affinity.xp[0])return 2;
    test_hit_result=2;for(i=0;i<10;++i)character_hit_hook((void*)8,0,0,(void*)9,0,0);old=r->affinity.xp[0];if(!old)return 3;
    character_hit_hook((void*)8,0,&d,(void*)9,0,0);if(test_seen_cut<=10.0f||d.cut!=10.0f)return 5;old=r->affinity.xp[0];
    test_weapon_kind=6;test_seen_cut=0;character_hit_hook((void*)8,0,&d,(void*)9,0,0);if(test_seen_cut!=10.0f||d.cut!=10.0f||r->affinity.xp[0]!=old)return 6;test_weapon_kind=-1;
    test_player_flag=0;character_hit_hook((void*)8,0,0,(void*)9,0,0);if(r->affinity.xp[0]!=old)return 4;
    active=0;return 0;
}
__declspec(dllexport) int probeTestHookTargets(void) {return !distinct_hook_targets((void*)1,(void*)2)||distinct_hook_targets((void*)1,(void*)1)||distinct_hook_targets(0,(void*)1);}
static float test_wound_seen_cut,test_wound_seen_blunt,test_wound_seen_pierce;
static void* test_crossbow_wound_original(void* a,unsigned char b,int c,void* damage,int* d,void* e,int* f,void* g){SBDamage* x=(SBDamage*)damage;(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;test_wound_seen_cut=x->cut;test_wound_seen_blunt=x->blunt;test_wound_seen_pierce=x->pierce;return (void*)1;}
__declspec(dllexport) int probeTestCrossbowWound(void){
    SBCharacterRecord* r;SBDamage d={0,0,100,0,1,0};unsigned int old;
    sb_persistence_init(&persistence);character_is_player=test_is_player;character_handle=test_handle;character_stats=test_hit_stats;original_crossbow_wound=test_crossbow_wound_original;test_handle_values[5]=42;test_handle_values[6]=7;test_player_flag=1;active=1;all_aptitudes=1;
    r=sb_persistence_bind(&persistence,(void*)123,(void*)9,42,7,99,1);if(!r)return 1;r->affinity.xp[0]=0;
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
    for(i=0;i<SB_ALLY_LIMIT;++i)allies[i].character=0;
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
__declspec(dllexport) int probeTestCrossbowSpeed(void){
    int i;SBCharacterRecord* r;Vec aim={0};
    for(i=0;i<128;++i){((volatile unsigned char*)test_cb_shot_bolt)[i]=0;((volatile unsigned char*)test_cb_shot_bolt2)[i]=0;}test_cb_active_bolt=test_cb_shot_bolt;
    for(i=0;i<CB_LIMIT;++i)cb_reset_flight(&cb_flights[i]);
    for(i=0;i<SB_ALLY_LIMIT;++i)allies[i].character=0;
    sb_persistence_init(&persistence);character_is_player=test_is_player;character_handle=test_handle;character_stats=test_hit_stats;original_projectile_trace=test_cb_trace_original;original_crossbow_shoot=test_cb_shoot_original;original_harpoon_create=test_cb_create_original;
    test_handle_values[5]=42;test_handle_values[6]=7;test_player_flag=1;active=1;
    r=sb_persistence_bind(&persistence,(void*)123,(void*)9,42,7,99,3);if(!r)return 1;r->affinity.xp[0]=47600;
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
    locale_manager_instance=test_locale_manager;locale_current=test_locale_current;ui_locale_ready=0;sb_locale_set("en_GB");select_game_locale();p=sb_locale_get()->id;
    if(p[0]!='j'||p[1]!='a'||!ui_locale_ready)return 1;sb_locale_set("unsupported");p=sb_locale_get()->id;if(p[0]!='e'||p[1]!='n')return 2;sb_locale_set("en_GB");locale_manager_instance=0;locale_current=0;ui_locale_ready=0;return 0;
}
__declspec(dllexport) const char* probeTestLocaleText(const char* id,int field){const SBLocale* l;sb_locale_set(id);l=sb_locale_get();return field==0?l->id:field==1?l->element[1]:field==2?l->armour:field==3?l->blunt:field==4?l->cut:l->nextlevel;}
static void test_set_handle_original(void* character,const void* next) {const unsigned int* h=(const unsigned int*)next;test_handle_values[5]=h[5];test_handle_values[6]=h[6];(void)sb_character_observe(&persistence.characters,character,h[5],h[6],123,5);}
typedef struct SBTestGameData {int value[3];int present;} SBTestGameData;
typedef struct SBTestStats {U64 padding[2];void* character;} SBTestStats;
static float test_speed_original(void* stats){(void)stats;return 1.0f;}
__declspec(dllexport) int probeTestWindSpeed(void)
{
    SBTestStats stats={{0,0},(void*)9};SBCharacterRecord* r;float x;
    sb_persistence_init(&persistence);character_is_player=test_is_player;character_handle=test_handle;original_attack_speed=test_speed_original;original_block_speed=test_speed_original;test_melee_api();
    test_handle_values[5]=42;test_handle_values[6]=7;test_player_flag=1;active=1;
    r=sb_persistence_bind(&persistence,&stats,(void*)9,42,7,123,3);if(!r)return 1;
    x=attack_speed_hook(&stats);if(x<1.019f||x>1.021f)return 2;
    r->affinity.xp[0]=47600;x=block_speed_hook(&stats);if(x<1.099f||x>1.101f)return 3;
    r->affinity.selected=1;if(attack_speed_hook(&stats)!=1.0f)return 4;
    r->affinity.selected=2;r->affinity.xp[0]=47600;x=attack_speed_hook(&stats);if(x<1.049f||x>1.051f||block_speed_hook(&stats)!=1.0f)return 6;
    test_weapon_kind=6;if(attack_speed_hook(&stats)!=1.0f)return 7;test_weapon_kind=8;test_ranged_mode=1;if(attack_speed_hook(&stats)<1.049f)return 8;test_weapon_kind=-1;if(attack_speed_hook(&stats)!=1.0f)return 9;test_ranged_mode=0;
    r->affinity.selected=3;test_player_flag=0;if(block_speed_hook(&stats)!=1.0f)return 5;
    active=0;return 0;
}
static int test_stats_calls;
static void test_stats_original(void* stats,void* data) {(void)stats;(void)data;++test_stats_calls;}
static void test_stats_destroy_original(void* stats) {(void)stats;}
static unsigned char test_list_exists(void* data,const SBGameString* list) {(void)list;return (unsigned char)((SBTestGameData*)data)->present;}
static int test_list_size(void* data,const SBGameString* list) {(void)list;return ((SBTestGameData*)data)->present;}
static const SBGameString* test_list_name(void* data,const SBGameString* list,int index) {static const SBGameString name={{"SB_Data"},7,15,0};(void)data;(void)list;(void)index;return &name;}
static const int* test_list_values(void* data,const SBGameString* list,int index) {(void)list;(void)index;return ((SBTestGameData*)data)->value;}
static void* test_find_reference(void* data,const SBGameString* list,const SBGameString* item) {(void)list;(void)item;return ((SBTestGameData*)data)->value;}
static void test_add_reference(void* data,const SBGameString* list,const SBGameString* item,int a,int b,int c) {SBTestGameData* d=(SBTestGameData*)data;(void)list;(void)item;d->value[0]=a;d->value[1]=b;d->value[2]=c;d->present=1;}
__declspec(dllexport) int probeTestPersistenceHooks(void) {
    SBTestGameData data={{0,0,0},0};SBTestStats stats={{0,0},(void*)9};SBCharacterRecord* r;unsigned char rates[5];int i;
    sb_persistence_init(&persistence);character_handle=test_handle;test_handle_values[5]=42;test_handle_values[6]=7;
    gamedata_api.list_exists=test_list_exists;gamedata_api.list_size=test_list_size;gamedata_api.list_name=test_list_name;gamedata_api.list_values=test_list_values;gamedata_api.find_reference=test_find_reference;gamedata_api.add_reference=test_add_reference;
    original_stats_save=test_stats_original;original_stats_load=test_stats_original;original_stats_destroy=test_stats_destroy_original;test_stats_calls=0;active=1;
    r=sb_persistence_bind(&persistence,&stats,(void*)9,42,7,123,4);if(!r)return 1;
    for(i=0;i<5;++i)rates[i]=r->affinity.rates[i];
    stats_save_hook(&stats,&data);if(!data.present||((data.value[0]>>8)&255)!=4||(data.value[0]&7)!=4||test_stats_calls!=1)return 2;
    if(!sb_character_select(&persistence.characters,(void*)9,42,7,2))return 3;
    stats_save_hook(&stats,&data);if((data.value[0]&7)!=2||test_stats_calls!=2)return 4;
    sb_persistence_forget(&persistence,&stats,(void*)9);
    stats_load_hook(&stats,&data);if(test_stats_calls!=3)return 5;
    r=sb_persistence_bind(&persistence,&stats,(void*)10,42,7,999,1);if(!r||r->affinity.selected!=2)return 6;
    for(i=0;i<5;++i)if(r->affinity.rates[i]!=rates[i])return 7;
    {unsigned int next[8]={0};next[5]=84;next[6]=8;original_character_set_handle=test_set_handle_original;character_set_handle_hook((void*)10,next);r=sb_character_observe(&persistence.characters,(void*)10,84,8,7,5);if(!r||r->affinity.selected!=2)return 8;}
    test_handle_values[5]=42;test_handle_values[6]=7;
    stats_destroy_hook(&stats);sb_persistence_forget(&persistence,0,(void*)10);active=0;return 0;
}
#endif
static unsigned char test_no_combat(void* c,unsigned char a,unsigned char b) {(void)c;(void)a;(void)b;return 0;}
static void* test_appearance(void* c) {(void)c;return (void*)2;}
static void* test_body(void* a) {(void)a;return (void*)3;}
__declspec(dllexport) int probeTestIdentity(void) {
    test_melee_api();
    original_character_update=test_character_original;original_character_destroy=test_character_original;
    character_is_player=test_is_player;character_appearance=test_appearance;appearance_body=test_body;character_handle=test_handle;test_handle_values[5]=42;test_handle_values[6]=7;
#ifdef SB_ENABLE_PERSISTENCE
    character_stats=test_stats;sb_persistence_init(&persistence);
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
    SBAffinity a,b;int i,record[3];
    if(!sb_affinity_create(&a,7,1))return 1;
    a.rates[0]=60;a.rates[1]=60;a.rates[2]=80;a.rates[3]=80;a.rates[4]=100;
    for(i=0;i<25;++i)if(!sb_affinity_award_hit_rate(&a,1))return 2;
    if(a.xp[0]!=100||a.rates[0]!=60||!sb_affinity_encode(&a,record)||!sb_affinity_decode(&b,record)||b.rates[0]!=60)return 3;
    for(i=0;i<25;++i)sb_affinity_award_hit_rate(&b,0);
    return b.xp[0]==160?0:4;
}
__declspec(dllexport) int probeTestAffinity(void) {unsigned int i,counts[6]={0};for(i=1;i<1000;++i){unsigned int a=affinity_seed(i,7);if(a!=affinity_seed(i,7)||a<1||a>5)return 1;++counts[a];}for(i=1;i<=5;++i)if(counts[i]<120)return 2;return 0;}
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
#ifdef SB_ENABLE_TRAIL
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
__declspec(dllexport) int probeTestVisualSecondHandCount(int side) {return side>=0&&side<2&&hand_fires[side].effect2?(int)particle_count(hand_fires[side].effect2):-1;}
__declspec(dllexport) void probeTestExpirePending(void) {entries[0].born=GetTickCount64()-6000;}
__declspec(dllexport) int probeTestTracked(void) {return entries[0].blade!=0;}
__declspec(dllexport) int probeTestBladeElement(void) {return entries[0].effect?entries[0].element:0;}
__declspec(dllexport) int probeTestHandElement(int side) {return side>=0 && side<2 && hand_fires[side].effect?hand_fires[side].element:0;}
__declspec(dllexport) int probeTestHandCount(int side) {HandFire* h;if(side<0||side>1)return -1;h=&hand_fires[side];return h->effect?(int)particle_count(h->effect):-1;}
__declspec(dllexport) int probeTestStep(float dt) {update(dt);return entries[0].effect?(int)particle_count(entries[0].effect):-1;}
#ifdef SB_ENABLE_TRAIL
__declspec(dllexport) int probeTestTrailLifecycle(void)
{
    Entry* e=&entries[0];Vec p={1,2,3};Quat q1={1,0,0,0},q2={.9238795f,0,0,.3826834f};int i,element;
    if(!e->scene||!trail_supported)return 1;
    for(element=1;element<=5;++element){SBTrailColour* color;trail_cleanup(e);e->element=element;
        trail_tick(e,&p,&q1,.016f);p.x+=10;trail_tick(e,&p,&q1,.016f);if(e->ribbon)return 2;
        trail_tick(e,&p,&q2,.016f);if(!e->ribbon||!e->trail_linked||!e->trail_attached)return 3;
        color=((SBTrailColour* (*)(void*,U64))trail_get_colour)(e->ribbon,0);if(!color||color->a<.27f||color->a>.29f)return 4;
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
    Entry temp={0};Vec p={1,2,3},offset={80,30,0},world;Quat q1={1,0,0,0},q2={.9238795f,0,0,.3826834f};void* root;float expected_x,expected_y;
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
__declspec(dllexport) float probeTestHeight(int i) {void* tech;void* emitter;if(i<0||i>=SB_TRACK_LIMIT||!entries[i].effect)return -1;tech=particle_technique(entries[i].effect,0);emitter=tech?technique_emitter(tech,0):0;return emitter?test_box_height(emitter):-1;}
__declspec(dllexport) void probeTestCleanupSecond(void) {erase(&entries[1]);}
__declspec(dllexport) int probeTestFreeTag(void) {void* tag=entries[0].tag;test_free_calls=0;original_free_tag=test_free_original;free_tag_hook((void*)1,tag);return test_free_calls==1 && !entries[0].blade;}
__declspec(dllexport) int probeTestFreeObject(void) {void* blade=entries[0].blade;test_free_calls=0;original_free_object=test_free_original;free_object_hook((void*)1,blade);return test_free_calls==1 && !entries[0].blade;}
__declspec(dllexport) int probeTestPositionMatches(void) {const Vec* p;const Quat* q;Vec n;float dx,dy,dz,start,off;if(!entries[0].node)return 0;p=tag_position(entries[0].tag);q=tag_rotation(entries[0].tag);start=entries[0].length*(entries[0].pole?.45f:.12f);off=(start+entries[0].length*.98f)*.5f-5;test_node_position(entries[0].node,&n);dx=n.x-(p->x+2*(q->x*q->y-q->w*q->z)*off);dy=n.y-(p->y+(1-2*(q->x*q->x+q->z*q->z))*off);dz=n.z-(p->z+2*(q->y*q->z+q->w*q->x)*off);return dx*dx+dy*dy+dz*dz<.0001f;}
__declspec(dllexport) void probeTestPositions(Vec* actual,Vec* target) {if(entries[0].node){test_node_position(entries[0].node,actual);*target=*tag_position(entries[0].tag);}}
__declspec(dllexport) void probeTestCleanup(void) {erase(&entries[0]);}
#ifdef SB_ENABLE_UI
typedef struct SBTestWidget {int visible,selected;} SBTestWidget;
static SBTestWidget test_widgets[40];static int test_widget_count,test_canvas_created,test_window_calls;
static unsigned char test_canvas_pixels[256*256*4];static void* test_mouse_focus;
static unsigned char test_window_bytes[0xe0];
static void* test_gui_instance(void){return (void*)1;}
static void* test_ui_create(void* parent,const SBGameString* type,const SBGameString* skin,const SBMyGUICoord* coord,int align,const SBGameString* name) {(void)parent;(void)type;(void)skin;(void)coord;(void)name;if(align!=10||test_widget_count>=40)return 0;return &test_widgets[test_widget_count++];}
static void test_ui_destroy(void* gui,void* widget){(void)gui;(void)widget;}
static void test_ui_visible(void* widget,unsigned char on){((SBTestWidget*)widget)->visible=on;}
static void test_ui_caption(void* widget,const SBGameString* text){(void)widget;(void)text;}
static void test_ui_selected(void* widget,unsigned char on){((SBTestWidget*)widget)->selected=on;}
static unsigned char test_ui_canvas_created(void* widget){(void)widget;return (unsigned char)test_canvas_created;}
static void test_ui_canvas_create(void* widget,int w,int h,int mode,int usage,int format){(void)widget;if(w==256&&h==256&&mode==0&&usage==18&&format==4)test_canvas_created=1;}
static int test_ui_canvas_dimension(void* widget){(void)widget;return 256;}
static void* test_ui_canvas_lock(void* widget,int usage){(void)widget;return usage==16?test_canvas_pixels:0;}
static void test_ui_canvas_unlock(void* widget){(void)widget;}
static void test_ui_set_size(void* widget,int width,int height){(void)widget;(void)width;(void)height;}
static int test_ui_width(void* window){(void)window;return 700;}
static int test_ui_height(void* window){(void)window;return 500;}
static unsigned char test_ui_window_visible(void* window){(void)window;return 1;}
static void* test_ui_focus(void* input){(void)input;return test_mouse_focus;}
static void* test_ui_parent(void* widget){(void)widget;return 0;}
static void test_ui_window_original(void* window){(void)window;++test_window_calls;}
static unsigned char test_ui_mouse_original(void* input,int x,int y,int button){(void)input;(void)x;(void)y;(void)button;return 1;}
__declspec(dllexport) int probeTestUIHooks(void)
{
    unsigned char* window=test_window_bytes;SBTestStats stats={{0,0},(void*)9};SBTestGameData data={{0,0,0},0};int slot;SBUIView* view;
    *(void**)(window+8)=(void*)7;*(void**)(window+0xd0)=(void*)9;
    sb_persistence_init(&persistence);test_widget_count=0;test_canvas_created=0;test_window_calls=0;ui_failed=0;active=1;test_player_flag=1;
    character_is_player=test_is_player;character_handle=test_handle;character_stats=test_stats;test_handle_values[5]=42;test_handle_values[6]=7;
    mygui_api.gui_instance=test_gui_instance;mygui_api.create_widget=test_ui_create;mygui_api.destroy_widget=test_ui_destroy;mygui_api.set_visible=test_ui_visible;mygui_api.set_caption=test_ui_caption;mygui_api.set_selected=test_ui_selected;
    mygui_api.canvas_created=test_ui_canvas_created;mygui_api.canvas_create=test_ui_canvas_create;mygui_api.canvas_width=test_ui_canvas_dimension;mygui_api.canvas_height=test_ui_canvas_dimension;mygui_api.canvas_lock=test_ui_canvas_lock;mygui_api.canvas_unlock=test_ui_canvas_unlock;mygui_api.set_size=test_ui_set_size;sb_mygui_backend(&mygui_api,&ui_api);
    stats_window_width=test_ui_width;stats_window_height=test_ui_height;stats_window_visible=test_ui_window_visible;input_focus=test_ui_focus;widget_parent=test_ui_parent;
    original_stats_window_update=test_ui_window_original;original_stats_window_clear=test_ui_window_original;original_stats_window_destroy=test_ui_window_original;original_mouse_press=test_ui_mouse_original;
    original_stats_save=test_stats_original;
    gamedata_api.list_exists=test_list_exists;gamedata_api.list_size=test_list_size;gamedata_api.list_name=test_list_name;gamedata_api.list_values=test_list_values;gamedata_api.find_reference=test_find_reference;gamedata_api.add_reference=test_add_reference;
    stats_window_update_hook(window);slot=ui_find(window);if(slot<0||test_window_calls!=1)return 1;view=&stats_views[slot].view;
    set_ally((void*)9,(void*)3,1,0,0,0,1);
    test_mouse_focus=view->opener;if(!mouse_press_hook((void*)2,0,0,0)||!view->model.visible)return 2;
    stats_window_update_hook(window);if(!view->graph_ready||!test_canvas_created)return 3;
    test_mouse_focus=view->buttons[4];mouse_press_hook((void*)2,0,0,0);if(!find_ally((void*)3)||find_ally((void*)3)->element!=4)return 10;stats_window_update_hook(window);if(view->model.affinity.selected!=4)return 4;
    {SBCharacterRecord* r=sb_character_observe(&persistence.characters,(void*)9,42,7,0,4);if(!r)return 7;r->affinity.xp[0]=13800;stats_window_update_hook(window);
     test_mouse_focus=view->mode_second;mouse_press_hook((void*)2,0,0,0);if(!view->second_mode)return 8;
     test_mouse_focus=view->buttons[2];mouse_press_hook((void*)2,0,0,0);if(!find_ally((void*)3)||find_ally((void*)3)->secondary!=2)return 11;stats_window_update_hook(window);if(view->model.affinity.secondary!=2)return 9;}
    stats_save_hook(&stats,&data);if(!data.present||(data.value[0]&7)!=4||((data.value[0]>>16)&7)!=2)return 5;
    stats_window_clear_hook(window);if(ui_find(window)>=0)return 6;set_ally((void*)9,0,0,0,0,0,0);
    active=0;return 0;
}
#endif
#endif
