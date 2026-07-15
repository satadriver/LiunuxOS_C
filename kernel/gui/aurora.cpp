// aurora.c - 极光
// 依赖: 无
// 调用: background_aurora((char*)显存地址);
#include "../def.h"
#include "../video.h"
#include "../Utils.h"

static float my_sin(float x) {
    while(x> 3.14159265f) x-=6.28318530f;
    while(x<-3.14159265f) x+=6.28318530f;
    float x2=x*x;
    return x - x2*x/6.0f + x2*x2*x/120.0f - x2*x2*x2*x/5040.0f;
}
static unsigned int hash_u(unsigned int n){n=(n^(n>>13))*1274126177u;return n^(n>>16);}

static float hash2d(int ix,int iy){return(float)(hash_u((unsigned)(ix*374761393+iy*668265263))&0xFFFF)/65535.0f;}

static float smooth_noise(float x,float y,float sc){
    float sx=x/sc,sy=y/sc;int ix=(int)sx,iy=(int)sy;
    float fx=sx-ix,fy=sy-iy;
    fx=fx*fx*(3-2*fx); fy=fy*fy*(3-2*fy);
    return(hash2d(ix,iy)*(1-fx)+hash2d(ix+1,iy)*fx)*(1-fy)+
          (hash2d(ix,iy+1)*(1-fx)+hash2d(ix+1,iy+1)*fx)*fy;
}
static void px(char*buf,int x,int y,int r,int g,int b){
    if(r<0)r=0;if(r>255)r=255;if(g<0)g=0;if(g>255)g=255;if(b<0)b=0;if(b>255)b=255;
    int i=(y* gVideoWidth +x)* gBytesPerPixel;buf[i]=b;buf[i+1]=g;buf[i+2]=r;
    //buf[i+3]=0xFF;
}
static int clampi(float v){int i=(int)v;return i<0?0:i>255?255:i;}

extern "C" __declspec(dllexport) void background_aurora(char*buf){
    int x,y;
    for(y=0;y< gWindowHeight;y++){
        float ny=(float)y/ gWindowHeight;
        for(x=0;x< gVideoWidth;x++){
            float nx=(float)x/ gVideoWidth;
            int r=2,g=2,b=8;

            /* 星星 */
            if(hash2d(x*3/4,y*3/4)>0.997f){
                float sb=hash2d(x*37/10,y*37/10);
                r=clampi(200+55*sb);g=clampi(200+55*sb);b=clampi(220+35*sb);
            } else {
                /* 4层极光帘幕 */
                int layer;
                for(layer=0;layer<4;layer++){
                    float wave_x=my_sin(nx*6.0f+layer*1.5f+ny*2.0f)*0.15f;
                    float wave_y=my_sin(nx*3.0f+layer*0.8f)*0.1f;
                    float cy=0.25f+layer*0.08f+wave_x+wave_y;
                    float dy=ny-cy; if(dy<0)dy=-dy;
                    if(dy<0.25f){
                        float intensity=1.0f-dy/0.25f;
                        intensity*=intensity; /* ^1.5 */
                        intensity*=0.5f+0.5f*smooth_noise((float)x+layer*100,(float)y+layer*50,30.0f);
                        if(layer==0){r+=clampi(30*intensity);g+=clampi(255*intensity);b+=clampi(100*intensity);}
                        else if(layer==1){r+=clampi(20*intensity);g+=clampi(200*intensity);b+=clampi(180*intensity);}
                        else if(layer==2){r+=clampi(80*intensity);g+=clampi(100*intensity);b+=clampi(255*intensity);}
                        else{r+=clampi(50*intensity);g+=clampi(255*intensity);b+=clampi(80*intensity);}
                    }
                }
                /* 山脉剪影 */
                float mt=my_sin(nx*8)*0.03f+my_sin(nx*15+1)*0.015f+my_sin(nx*25+2)*0.008f;
                if(ny>0.88f+mt){
                    float dk=ny-0.88f-mt; if(dk>0.12f)dk=0.12f; dk/=0.12f;
                    r=(int)(r*(1-dk));g=(int)(g*(1-dk));b=(int)(b*(1-dk));
                }
            }
            px(buf,x,y,r,g,b);
        }
    }
}