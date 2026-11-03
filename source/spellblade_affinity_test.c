#include "spellblade_affinity.h"
__declspec(dllexport) int sb_run_affinity_tests(void)
{
    SBAffinity a,b,c;int record[3],bad[3],i;
    if(!sb_affinity_create(&a,0x12345678u,2)||!sb_affinity_create(&b,0x12345678u,2))return 1;
    for(i=0;i<5;++i)if(a.rates[i]!=b.rates[i]||a.rates[i]<20||a.rates[i]>100)return 2;
    if(a.selected!=2||!sb_affinity_encode(&a,record)||!sb_affinity_decode(&c,record))return 3;
    for(i=0;i<5;++i)if(a.rates[i]!=c.rates[i])return 4;
    if(c.selected!=2||!sb_affinity_select(&c,5)||!sb_affinity_encode(&c,record)||!sb_affinity_decode(&b,record)||b.selected!=5)return 5;
    if(sb_affinity_select(&b,6)||b.selected!=5)return 6;
    bad[0]=0x501;bad[1]=record[1];bad[2]=record[2];if(sb_affinity_decode(&b,bad)||b.selected!=5)return 7;
    bad[0]=record[0];bad[1]=65535;bad[2]=record[2];if(sb_affinity_decode(&b,bad)||b.selected!=5)return 8;
    bad[0]=record[0];bad[1]=record[1];bad[2]=65535;if(sb_affinity_decode(&b,bad))return 9;
    if(!sb_affinity_select(&b,0)||!sb_affinity_encode(&b,record)||!sb_affinity_decode(&c,record)||c.selected!=0)return 10;
    {int j,rare=0;static const int ring[5]={0,2,4,3,1};for(i=0;i<10000;++i){int max=0,pos=-1;if(!sb_affinity_create(&a,(unsigned int)i,2))return 11;for(j=0;j<5;++j)if(a.rates[j]==100){++max;pos=j;}if(max==5){++rare;continue;}if(max!=1)return 12;for(j=0;j<5;++j)if(ring[j]==pos){if(a.rates[ring[(j+1)%5]]!=80||a.rates[ring[(j+4)%5]]!=80||a.rates[ring[(j+2)%5]]!=60||a.rates[ring[(j+3)%5]]!=60)return 13;}}if(rare<50||rare>150)return 14;}
    {int legacy[3]={0x104,10|(20<<7)|(95<<14)|(30<<21),40};if(!sb_affinity_decode(&a,legacy)||a.selected!=4||a.rates[2]!=100)return 15;if(!sb_affinity_encode(&a,record)||((record[0]>>8)&255)!=4||(record[0]&7)!=4)return 16;}
    {int j;if(!sb_affinity_create(&a,23,1))return 17;for(j=0;j<100;++j)sb_affinity_award_hit(&a);if(!a.xp[0]||!sb_affinity_encode(&a,record)||!sb_affinity_decode(&b,record)||b.xp[0]!=a.xp[0])return 18;a.xp[0]=47600;if(sb_affinity_level(&a,0)!=100)return 19;}
    {int j;for(i=0;i<10000;++i){sb_affinity_create(&a,(unsigned int)i,1);if(a.rates[0]==100&&a.rates[1]==80)break;}if(i==10000)return 20;for(j=0;j<20;++j)sb_affinity_award_hit(&a);if(sb_affinity_level(&a,0)!=1)return 21;sb_affinity_select(&a,2);for(j=0;j<20;++j)sb_affinity_award_hit(&a);if(a.xp[0]!=144||sb_affinity_level(&a,1)!=1)return 22;sb_affinity_select(&a,0);if(sb_affinity_award_hit(&a))return 23;}
    {SBDamage d={10,10,0,0,1,0};sb_affinity_create(&a,1,1);if(!sb_affinity_damage(&a,&d)||d.blunt<10.99f||d.blunt>11.01f||d.cut!=10)return 24;sb_affinity_select(&a,4);if(!sb_affinity_damage(&a,&d)||d.cut<=11.0f)return 25;sb_affinity_select(&a,5);if(!sb_affinity_damage(&a,&d)||d.bleed<1.09f)return 26;sb_affinity_select(&a,2);if(!sb_affinity_damage(&a,&d)||d.armour<0.049f)return 27;sb_affinity_select(&a,3);if(sb_affinity_damage(&a,&d))return 28;}
    {float v;sb_affinity_create(&a,1,3);v=sb_affinity_combat_speed(&a,1.0f);if(v<1.019f||v>1.021f)return 29;a.xp[0]=47600;v=sb_affinity_combat_speed(&a,1.0f);if(v<1.099f||v>1.101f)return 30;sb_affinity_select(&a,1);if(sb_affinity_combat_speed(&a,1.0f)!=1.0f)return 31;}
    {SBDamage d;int j;sb_affinity_create(&a,1,1);for(j=0;j<5;++j)a.xp[j]=47600;
     d=(SBDamage){30,10,0,0,1,0};sb_affinity_damage(&a,&d);if(d.blunt<15.99f||d.blunt>16.01f||d.cut!=30)return 32;
     sb_affinity_select(&a,4);d=(SBDamage){30,10,0,0,1,0};sb_affinity_damage(&a,&d);if(d.cut<35.99f||d.cut>36.01f||d.blunt!=10)return 33;
     sb_affinity_select(&a,2);d=(SBDamage){30,10,0,0,1,0};sb_affinity_damage(&a,&d);if(d.cut!=30||d.armour<.149f||d.armour>.151f||sb_affinity_attack_speed(&a,1.0f)<1.049f||sb_affinity_attack_speed(&a,1.0f)>1.051f||sb_affinity_combat_speed(&a,1.0f)!=1.0f)return 34;
     sb_affinity_select(&a,5);d=(SBDamage){30,10,0,0,1,0};sb_affinity_damage(&a,&d);if(d.cut<33.99f||d.cut>34.01f||d.bleed<1.499f||d.bleed>1.501f)return 35;}
    {int legacy[3]={(int)((400u<<16)|(3u<<8)|(0u<<3)|1u),(int)(100u|(20000u<<16)),(int)(500u|(900u<<16))};if(!sb_affinity_decode(&a,legacy)||a.xp[0]!=20000u||a.secondary)return 36;if(!sb_affinity_encode(&a,record)||!sb_affinity_decode(&b,record)||b.xp[0]!=20000u)return 37;}
    {SBDamage d={30,10,0,0,1,0};sb_affinity_create(&a,1,1);if(sb_affinity_select_secondary(&a,2))return 38;a.xp[0]=4u*(20u*50u+50u*49u);if(!sb_affinity_select_secondary(&a,2)||sb_affinity_secondary_percent(&a)!=25)return 39;if(!sb_affinity_damage(&a,&d)||d.blunt<13.99f||d.blunt>14.01f||d.armour<.0249f||d.armour>.0251f)return 40;if(!sb_affinity_encode(&a,record)||!sb_affinity_decode(&b,record)||b.secondary!=2)return 41;if(sb_affinity_select_secondary(&b,1))return 42;}
    {static const unsigned int levels[7]={49,50,60,70,80,90,100},pct[7]={0,25,35,45,55,65,75};int j;sb_affinity_create(&a,7,3);for(j=0;j<7;++j){unsigned int l=levels[j];a.xp[0]=(unsigned short)(4u*(20u*l+l*(l-1u)));if(sb_affinity_secondary_percent(&a)!=pct[j])return 43;}}
    {int j;sb_affinity_create(&a,7,1);a.rates[0]=60;for(j=0;j<25;++j)sb_affinity_award_hit(&a);if(a.xp[0]!=60||a.xp_fraction!=0)return 44;sb_affinity_select(&a,2);a.rates[1]=80;for(j=0;j<25;++j)sb_affinity_award_hit(&a);if(a.xp[0]!=140||a.xp_fraction!=0)return 45;}
    {float v;sb_affinity_create(&a,7,3);a.xp[0]=47600;if(!sb_affinity_select_secondary(&a,2))return 46;v=sb_affinity_attack_speed(&a,1.0f);if(v<1.1374f||v>1.1376f)return 47;v=sb_affinity_combat_speed(&a,1.0f);if(v<1.0999f||v>1.1001f)return 48;}
    return 0;
}
