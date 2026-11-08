#include "elemental_radar.h"
typedef struct Point {int x,y;} Point;
static const int ux[5]={0,951,588,-588,-951};
static const int uy[5]={-1000,-309,809,809,-309};
static const int ring_order[5]={0,2,4,3,1};
static void dot(unsigned char* p,int w,int h,int x,int y,unsigned int color)
{unsigned char* q;if(x<0||y<0||x>=w||y>=h)return;q=p+4*(y*w+x);q[0]=(unsigned char)(color>>24);q[1]=(unsigned char)(color>>16);q[2]=(unsigned char)(color>>8);q[3]=(unsigned char)color;}
static void line(unsigned char* p,int w,int h,Point a,Point b,unsigned int color)
{
    int dx=b.x>a.x?b.x-a.x:a.x-b.x,sx=a.x<b.x?1:-1;
    int dy=-(b.y>a.y?b.y-a.y:a.y-b.y),sy=a.y<b.y?1:-1,err=dx+dy;
    for(;;){int e;dot(p,w,h,a.x,a.y,color);if(a.x==b.x&&a.y==b.y)break;e=err*2;if(e>=dy){err+=dy;a.x+=sx;}if(e<=dx){err+=dx;a.y+=sy;}}
}
static int edge(Point a,Point b,int x,int y){return (b.x-a.x)*(y-a.y)-(b.y-a.y)*(x-a.x);}
static void triangle(unsigned char* p,int w,int h,Point a,Point b,Point c,unsigned int color)
{
    int x,y,minx=a.x,maxx=a.x,miny=a.y,maxy=a.y,area=edge(a,b,c.x,c.y);
    if(!area)return;
    if(b.x<minx)minx=b.x;if(c.x<minx)minx=c.x;if(b.x>maxx)maxx=b.x;if(c.x>maxx)maxx=c.x;
    if(b.y<miny)miny=b.y;if(c.y<miny)miny=c.y;if(b.y>maxy)maxy=b.y;if(c.y>maxy)maxy=c.y;
    if(minx<0)minx=0;if(miny<0)miny=0;if(maxx>=w)maxx=w-1;if(maxy>=h)maxy=h-1;
    for(y=miny;y<=maxy;++y)for(x=minx;x<=maxx;++x){int e1=edge(a,b,x,y),e2=edge(b,c,x,y),e3=edge(c,a,x,y);
        if((e1>=0&&e2>=0&&e3>=0)||(e1<=0&&e2<=0&&e3<=0))dot(p,w,h,x,y,color);}
}
int ee_radar_draw(unsigned char* pixels,int w,int h,const EEAffinity* state)
{
    Point center,axis[5],polygon[5],ring[5];int i,j,x,y,radius;
    if(!pixels||!state||w<32||h<32||w>512||h>512||state->selected>5)return 0;
    for(i=0;i<5;++i)if(state->rates[i]>100)return 0;
    center.x=w/2;center.y=h/2;radius=(w<h?w:h)*38/100;
    /* MyGUI's D3D canvas interprets these bytes as BGRA. Match the warm
     * paper center of Kenshi_GenericTextBoxSkin rather than a blue panel. */
    for(y=0;y<h;++y)for(x=0;x<w;++x)dot(pixels,w,h,x,y,0x65707dffu);
    for(i=0;i<5;++i){axis[i].x=center.x+ux[i]*radius/1000;axis[i].y=center.y+uy[i]*radius/1000;
        polygon[i].x=center.x+ux[i]*radius*state->rates[ring_order[i]]/100000;polygon[i].y=center.y+uy[i]*radius*state->rates[ring_order[i]]/100000;}
    for(i=0;i<5;++i)triangle(pixels,w,h,center,polygon[i],polygon[(i+1)%5],0x7d9b83ffu);
    for(j=1;j<=4;++j){for(i=0;i<5;++i){ring[i].x=center.x+(axis[i].x-center.x)*j/4;ring[i].y=center.y+(axis[i].y-center.y)*j/4;}
        for(i=0;i<5;++i)line(pixels,w,h,ring[i],ring[(i+1)%5],0x665440ffu);}
    for(i=0;i<5;++i){line(pixels,w,h,center,axis[i],0x665440ffu);line(pixels,w,h,polygon[i],polygon[(i+1)%5],0x633722ffu);
        for(y=-2;y<=2;++y)for(x=-2;x<=2;++x)if(x*x+y*y<=5)dot(pixels,w,h,polygon[i].x+x,polygon[i].y+y,(state->selected==(unsigned int)(ring_order[i]+1))?0x9a351fffU:0x493322ffu);}
    return 1;
}
