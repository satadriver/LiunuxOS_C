// nebula.c - 星云/银河
// 依赖: 无
// 调用: background_nebula((char*)显存地址);


#include "../def.h"
#include "../video.h"

static float my_sqrt(float x){
    if(x<=0)return 0;float s=x;int i;
    for(i=0;i<10;i++)s=(s+x/s)*0.5f;return s;
}
static float my_sin(float x){
    while(x> 3.14159265f)x-=6.28318530f;
    while(x<-3.14159265f)x+=6.28318530f;
    float x2=x*x;return x-x2*x/6.0f+x2*x2*x/120.0f-x2*x2*x2*x/5040.0f;
}
static float my_fabs(float x){return x<0?-x:x;}
static unsigned int hash_u(unsigned int n){n=(n^(n>>13))*1274126177u;return n^(n>>16);}
static float hash2d(int ix,int iy){
    return(float)(hash_u((unsigned)(ix*374761393+iy*668265263))&0xFFFF)/65535.0f;
}
static float value_noise(float x,float y,float sc){
    float sx=x/sc,sy=y/sc;int ix=(int)sx,iy=(int)sy;
    float fx=sx-ix,fy=sy-iy;
    fx=fx*fx*(3-2*fx);fy=fy*fy*(3-2*fy);
    return(hash2d(ix,iy)*(1-fx)+hash2d(ix+1,iy)*fx)*(1-fy)+
          (hash2d(ix,iy+1)*(1-fx)+hash2d(ix+1,iy+1)*fx)*fy;
}
static float fbm(float x,float y){
    float val=0,amp=0.5f;int i;
    for(i=0;i<5;i++){val+=amp*value_noise(x,y,80.0f);x*=2;y*=2;amp*=0.5f;}
    return val;
}
static void px(char*buf,int x,int y,int r,int g,int b){
    if(r<0)
        r=0;
    if(r>255)
        r=255;
    if(g<0)
        g=0;
    if(g>255)g=255;
    if(b<0)b=0;
    if(b>255)b=255;
    int i=(y* gVideoWidth +x)* gBytesPerPixel;buf[i]=b;buf[i+1]=g;buf[i+2]=r;
    
    //buf[i+3]=0xFF;
}

void background_nebula(char*buf){
    int x,y;
    for(y=0;y< gVideoHeight;y++){
        for(x=0;x< gVideoWidth;x++){
            float nx=(float)x/ gVideoWidth, ny=(float)y/ gVideoHeight;
            float cx=(nx-0.5f)*2, cy=(ny-0.5f)*2;
            float dist=my_sqrt(cx*cx+cy*cy);
            /* 用 atan2 近似: atan2(y,x) ≈ π/4 * y/(|x|+|y|) + corrections */
            /* 但更简单: 用极坐标距离+角度的近似 */
            /* 角度近似（足够用于视觉效果） */
            float angle;
            {
                float ax=my_fabs(cx), ay=my_fabs(cy);
                float a;
                if(ax>ay) a=ay/(ax+0.0001f);
                else      a=1.0f-ax/(ay+0.0001f);
                a*=0.785398f; /* π/4 */
                if(cy<0) a=-a;
                if(cx<0) a=3.14159f-a;
                angle=a;
            }

            int r=1, g=1, b=5;

            /* 螺旋星臂 */
            float a1=my_sin(angle-dist*8)*0.5f+0.5f;
            float a2=my_sin(angle-dist*8+3.14159f)*0.5f+0.5f;
            float arm=a1>a2?a1:a2;
            {float f=1-dist*0.8f;if(f<0)f=0;arm*=f;}

            float n1=fbm((float)x,(float)y);
            float n2=fbm((float)x+500,(float)y+500);

            float core=1-dist*2.5f;if(core<0)core=0;core*=core;

            /* 繁星 */
            if(hash2d((int)(x*0.7f),(int)(y*0.7f))>0.998f){
                float sb=hash2d((int)(x*3.1f),(int)(y*3.1f));
                r=180+(int)(75*sb);g=180+(int)(75*sb);b=200+(int)(55*sb);
            }else{
                r+=(int)(180*core+120*arm*n1);
                g+=(int)(60*core+30*arm*n1+60*arm*n2);
                b+=(int)(100*core+200*arm*n2+40*arm*n1);
            }
            px(buf,x,y,r,g,b);
        }
    }
}