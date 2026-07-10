// wallpaper_original.c - 蓝紫渐变（原始版）
// 依赖: 无
// 调用: background_original((char*)显存地址);
#include "../def.h"
#include "../video.h"

static float my_sqrt(float x){
    if(x<=0)return 0;float s=x;int i;
    for(i=0;i<8;i++)s=(s+x/s)*0.5f;return s;
}
static float my_sin(float x){
    while(x> 3.14159265f)x-=6.28318530f;
    while(x<-3.14159265f)x+=6.28318530f;
    float x2=x*x;
    return x-x2*x/6.0f+x2*x2*x/120.0f;
}
static float my_fabs(float x){return x<0?-x:x;}
static void px(char*buf,int x,int y,int r,int g,int b){
    if(r<0)r=0;if(r>255)r=255;if(g<0)g=0;if(g>255)g=255;if(b<0)b=0;if(b>255)b=255;
    int i=(y* gVideoWidth +x)* gBytesPerPixel;buf[i]=b;buf[i+1]=g;buf[i+2]=r;
    //buf[i+3]=0xFF;
}

void background_original(char*buf){
    int x,y;
    for(y=0;y< gVideoHeight;y++){
        for(x=0;x< gVideoWidth;x++){
            float nx=(float)x/ gVideoWidth, ny=(float)y/ gVideoHeight;
            float cx=(nx-0.5f)*2.0f, cy=(ny-0.5f)*2.0f;
            float dist=my_sqrt(cx*cx+cy*cy);

            int r=(int)(10+20*ny);
            int g=(int)(10+15*ny);
            int b=(int)(40+60*(1-ny));

            /* 中心发光 */
            float glow=1-dist*1.2f;
            if(glow>0){r+=30*glow;g+=20*glow;b+=80*glow;}

            /* 对角光带 */
            float diag=(cx+cy)*0.5f;
            float d=my_fabs(diag-0.3f)*4.0f;
            float streak=1-d; if(streak>0){streak*=0.3f;r+=60*streak;g+=40*streak;b+=120*streak;}

            /* 同心环 */
            float ring=my_fabs(my_sin(dist*12-ny*3))*0.15f;
            r+=40*ring;g+=50*ring;b+=100*ring;

            /* 暗角 */
            float vig=1-dist*0.6f; if(vig<0)vig=0; if(vig>1)vig=1;
            r*=vig; g*=vig; b*=vig;

            px(buf,x,y,r,g,b);
        }
    }
}