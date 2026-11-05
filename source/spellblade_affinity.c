#include "spellblade_affinity.h"
#ifdef SB_AFFINITY_STANDALONE
int _fltused=0;
#endif

static unsigned int mix(unsigned int x)
{ x^=x>>16; x*=0x7feb352du; x^=x>>15; x*=0x846ca68bu; x^=x>>16; return x; }

/* Ring: fire -> wind -> ice -> water -> lightning -> fire.
 * Storage order remains fire, lightning, wind, water, ice. */
static void aptitude(SBAffinity* s,unsigned int primary,int all)
{
    static const int ring[5]={0,2,4,3,1};int i,pos=0;
    for(i=0;i<5;++i)if((unsigned int)ring[i]==primary)pos=i;
    for(i=0;i<5;++i)s->rates[i]=(unsigned char)(all?100:60);
    if(!all){s->rates[ring[pos]]=100;s->rates[ring[(pos+1)%5]]=80;s->rates[ring[(pos+4)%5]]=80;}
}

int sb_affinity_create(SBAffinity* s, unsigned int seed, unsigned int trial_element)
{
    int i;if (!s || trial_element>5) return 0;
    s->selected=trial_element;s->secondary=0;s->xp_fraction=0;
    for(i=0;i<5;++i)s->xp[i]=0;
    aptitude(s,mix(seed^0x6d2b79f5u)%5u,mix(seed^0xa511e9b3u)%100u==0);
    return 1;
}
int sb_affinity_select(SBAffinity* s, unsigned int element)
{ if(!s || element>5)return 0;s->selected=element;if(!element||s->secondary==element)s->secondary=0;return 1; }
int sb_affinity_select_secondary(SBAffinity* s,unsigned int element)
{if(!s||element>5||!s->selected||(element&&element==s->selected))return 0;if(element&&sb_affinity_level(s,0)<50)return 0;s->secondary=element;return 1;}
unsigned int sb_affinity_secondary_percent(const SBAffinity* s)
{unsigned int l=sb_affinity_level(s,0);return l<50?0:l<60?25:l<70?35:l<80?45:l<90?55:l<100?65:75;}
unsigned int sb_affinity_level(const SBAffinity* s,unsigned int element)
{unsigned int lo=0,hi=100,total;if(!s||element>=5)return 0;total=s->xp[0];while(lo<hi){unsigned int mid=(lo+hi+1u)/2u;if(total>=4u*(20u*mid+mid*(mid-1u)))lo=mid;else hi=mid-1u;}return lo;}
int sb_affinity_award_hit_rate_scaled(SBAffinity* s,int all_aptitudes,unsigned int tenths)
{unsigned int amount,total;if(!s||!s->selected||s->selected>5)return 0;if(tenths>200)tenths=200;amount=(4u*tenths*(all_aptitudes?100u:(unsigned int)s->rates[s->selected-1]))/10u+s->xp_fraction;total=(unsigned int)s->xp[0]+amount/100u;s->xp_fraction=amount%100u;if(total>=47600u){total=47600u;s->xp_fraction=0;}s->xp[0]=(unsigned short)total;return 1;}
int sb_affinity_award_hit_rate(SBAffinity* s,int all_aptitudes)
{return sb_affinity_award_hit_rate_scaled(s,all_aptitudes,10u);}
int sb_affinity_award_hit(SBAffinity* s) {return sb_affinity_award_hit_rate(s,0);}
static float element_weight(const SBAffinity* s,unsigned int e)
{if(!s||!s->selected)return 0.0f;if(s->selected==e)return 1.0f;if(s->secondary==e)return (float)sb_affinity_secondary_percent(s)/100.0f;return 0.0f;}
float sb_affinity_combat_speed(const SBAffinity* s,float speed)
{float level,weight;if(!s||!(speed>0.0f)||speed>100.0f)return speed;level=(float)sb_affinity_level(s,0);weight=element_weight(s,3);return speed*(1.0f+weight*(.02f+level*.0008f));}
float sb_affinity_attack_speed(const SBAffinity* s,float speed)
{float level,wind,light;if(!s||!(speed>0.0f)||speed>100.0f)return speed;level=(float)sb_affinity_level(s,0);wind=element_weight(s,3);light=element_weight(s,2);return speed*(1.0f+wind*(.02f+level*.0008f)+light*(.01f+level*.0004f));}
int sb_affinity_damage(const SBAffinity* s,SBDamage* d)
{
    float level,scale,base,fire,light,water,ice;if(!s||!d||!s->selected||s->selected>5)return 0;
    level=(float)sb_affinity_level(s,0);scale=0.05f+0.001f*level;
    base=d->cut+d->blunt;if(base<0.0f)base=0.0f;
    fire=element_weight(s,1);light=element_weight(s,2);water=element_weight(s,4);ice=element_weight(s,5);
    if(fire+light+water+ice==0.0f)return 0;
    d->blunt+=base*scale*fire;d->cut+=base*(scale*water+(.02f+.0008f*level)*ice);
    d->armour+=scale*light;d->bleed+=(.1f+.004f*level)*ice;
    return 1;
}

int sb_affinity_damage_scaled(const SBAffinity* s,SBDamage* d,float multiplier)
{
    SBDamage original;float m;
    if(!d||!(multiplier>0.0f))return 0;if(multiplier>5.0f)multiplier=5.0f;
    original=*d;if(!sb_affinity_damage(s,d))return 0;m=multiplier;
    d->cut=original.cut+(d->cut-original.cut)*m;
    d->blunt=original.blunt+(d->blunt-original.blunt)*m;
    d->pierce=original.pierce+(d->pierce-original.pierce)*m;
    d->stun=original.stun+(d->stun-original.stun)*m;
    d->bleed=original.bleed+(d->bleed-original.bleed)*m;
    d->armour=original.armour+(d->armour-original.armour)*m;
    return 1;
}
float sb_affinity_combat_speed_scaled(const SBAffinity* s,float speed,float multiplier)
{if(!(multiplier>0.0f))multiplier=0.0f;if(multiplier>5.0f)multiplier=5.0f;return speed+(sb_affinity_combat_speed(s,speed)-speed)*multiplier;}
float sb_affinity_attack_speed_scaled(const SBAffinity* s,float speed,float multiplier)
{if(!(multiplier>0.0f))multiplier=0.0f;if(multiplier>5.0f)multiplier=5.0f;return speed+(sb_affinity_attack_speed(s,speed)-speed)*multiplier;}

int sb_affinity_encode(const SBAffinity* s,int out[3])
{
    unsigned int code=5;int i,j,all=1;
    if(!s||!out||s->selected>5||s->secondary>5||s->xp[0]>47600u||s->xp_fraction>=100u||s->secondary==s->selected&&s->secondary||s->secondary&&(!s->selected||sb_affinity_level(s,0)<50))return 0;
    for(i=0;i<5;++i){if(s->rates[i]!=100)all=0;}
    if(!all){SBAffinity check;for(i=0;i<5;++i)if(s->rates[i]==100){aptitude(&check,(unsigned int)i,0);for(j=0;j<5&&check.rates[j]==s->rates[j];++j);if(j==5){code=(unsigned int)i;break;}}if(code==5)return 0;}
    out[0]=(int)((4u<<8)|(code<<3)|s->selected|(s->secondary<<16));
    out[1]=(int)s->xp[0];out[2]=(int)s->xp_fraction;return 1;
}
int sb_affinity_decode(SBAffinity* s,const int in[3])
{
    SBAffinity tmp;unsigned int meta,packed,version;int i;
    if(!s||!in)return 0;
    meta=(unsigned int)in[0];packed=(unsigned int)in[1];
    version=(meta>>8)&255u;
    if(version==4){unsigned int code=(meta&255u)>>3;tmp.selected=meta&7u;tmp.secondary=(meta>>16)&7u;tmp.xp_fraction=(unsigned int)in[2];if((meta&0xfff80000u)||code>5||tmp.selected>5||tmp.secondary>5||packed>47600u||tmp.xp_fraction>=100u)return 0;aptitude(&tmp,code==5?0:code,code==5);tmp.xp[0]=(unsigned short)packed;for(i=1;i<5;++i)tmp.xp[i]=0;if(tmp.secondary&&(!tmp.selected||tmp.selected==tmp.secondary||sb_affinity_level(&tmp,0)<50))return 0;*s=tmp;return 1;}
    if(version==3){unsigned int code=(meta&255u)>>3,highest=0;tmp.selected=meta&7u;if(code>5||tmp.selected>5)return 0;aptitude(&tmp,code==5?0:code,code==5);tmp.xp[0]=(unsigned short)(meta>>16);tmp.xp[1]=(unsigned short)packed;tmp.xp[2]=(unsigned short)(packed>>16);tmp.xp[3]=(unsigned short)(unsigned int)in[2];tmp.xp[4]=(unsigned short)((unsigned int)in[2]>>16);for(i=0;i<5;++i){if(tmp.xp[i]>47600u)return 0;if(tmp.xp[i]>highest)highest=tmp.xp[i];}tmp.xp[0]=(unsigned short)highest;for(i=1;i<5;++i)tmp.xp[i]=0;tmp.secondary=0;tmp.xp_fraction=0;*s=tmp;return 1;}
    if((version!=1&&version!=2) || (meta>>16)!=0 || (meta&255u)>5 || (packed>>28)!=0 || in[2]<0 || in[2]>100)return 0;
    tmp.selected=meta&255u;
    for(i=0;i<4;++i){unsigned int rate=(packed>>(7*i))&127u;if(rate>100)return 0;tmp.rates[i]=(unsigned char)rate;}
    tmp.rates[4]=(unsigned char)in[2];
    for(i=0;i<5;++i)tmp.xp[i]=0;tmp.secondary=0;tmp.xp_fraction=0;
    {unsigned int best=0;int all=1;for(i=0;i<5;++i){if(tmp.rates[i]!=100)all=0;if(tmp.rates[i]>tmp.rates[best])best=(unsigned int)i;}aptitude(&tmp,best,all);}
    *s=tmp;return 1;
}
