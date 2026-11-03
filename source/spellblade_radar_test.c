#include "spellblade_radar.h"
static unsigned char pixels[128*128*4];
__declspec(dllexport) int sb_radar_sample(unsigned char* target,int w,int h) {SBAffinity sample={2,{72,96,38,55,82}};return sb_radar_draw(target,w,h,&sample);}
__declspec(dllexport) int sb_run_radar_tests(void)
{
    SBAffinity a={2,{100,80,60,60,80}};int i,colored=0;
    if(!sb_radar_draw(pixels,128,128,&a))return 1;
    if(pixels[0]!=0x65||pixels[1]!=0x70||pixels[2]!=0x7d||pixels[3]!=0xff)return 2;
    for(i=0;i<128*128;++i)if(pixels[4*i]==0x7d&&pixels[4*i+1]==0x9b)++colored;
    if(colored<100)return 3;
    a.rates[0]=101;if(sb_radar_draw(pixels,128,128,&a))return 4;
    a.rates[0]=100;a.selected=6;if(sb_radar_draw(pixels,128,128,&a))return 5;
    if(sb_radar_draw(pixels,10,10,&a))return 6;
    return 0;
}
