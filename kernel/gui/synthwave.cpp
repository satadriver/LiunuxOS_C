// synthwave.c - 赛博朋克网格
// 依赖: 无
// 调用: background_synthwave((char*)显存地址);
#define WIDTH  1024
#define HEIGHT 768

static float my_sin(float x){
    while(x> 3.14159265f)x-=6.28318530f;
    while(x<-3.14159265f)x+=6.28318530f;
    float x2=x*x;return x-x2*x/6+x2*x2*x/120-x2*x2*x2*x/5040;
}
static float my_fabs(float x){return x<0?-x:x;}
static void px(char*buf,int x,int y,int r,int g,int b){
    if(r<0)r=0;if(r>255)r=255;if(g<0)g=0;if(g>255)g=255;if(b<0)b=0;if(b>255)b=255;
    int i=(y*WIDTH+x)*4;buf[i]=b;buf[i+1]=g;buf[i+2]=r;buf[i+3]=0xFF;
}
static int clampi(float v){int i=(int)v;return i<0?0:i>255?255:i;}

void background_synthwave(char*buf){
    int x,y;
    int horizon=HEIGHT*45/100; /* 45% */
    for(y=0;y<HEIGHT;y++){
        for(x=0;x<WIDTH;x++){
            float nx=(float)x/WIDTH;
            int r,g,b;

            if(y<horizon){
                /* 天空: 深蓝到粉橙渐变 */
                float t=(float)y/horizon;
                r=clampi(5+80*(1-t)*(1-t));
                g=clampi(10*(1-t));
                b=clampi(30+60*(1-t));
                /* 太阳 */
                float dx=(nx-0.5f)/(0.18f);
                float sy=(float)(y-horizon*55/100)/(HEIGHT*0.22f);
                float sd=dx*dx+sy*sy;
                if(sd<1.0f){
                    float sun=1.0f-sd;
                    /* 太阳横切线 */
                    float sl=sy;if(sl<0)sl=-sl;
                    if(sl>0.02f&&sl<0.08f) sun=0.0f;
                    r=clampi(r+255*sun*0.9f);
                    g=clampi(g+80*sun*0.5f);
                    b=clampi(b+120*sun*0.6f);
                }
            } else {
                /* 地面: 透视网格 */
                float depth=(float)(y-horizon)/(HEIGHT-horizon);
                /* 纵向线 */
                float vx=(nx-0.5f)/depth; if(depth<0.01f)vx=0;
                float vm=vx/60.0f; vm=vm-(int)vm; if(vm<0)vm=-vm; vm=vm*2-1; if(vm<0)vm=-vm;
                vm=1.0f-vm*8.0f; if(vm<0)vm=0;
                /* 横向线 */
                float hy=1.0f/depth;
                float hm=hy/40.0f; hm=hm-(int)hm; if(hm<0)hm=-hm; hm=hm*2-1; if(hm<0)hm=-hm;
                hm=1.0f-hm*8.0f; if(hm<0)hm=0;
                float line=vm>hm?vm:hm;
                float fade=1.0f-depth*0.5f;
                r=clampi(10+180*line*fade);
                g=clampi(30*line*fade);
                b=clampi(30+200*line*fade);
                /* 远处雾化 */
                float fog=depth*0.5f;
                r=clampi(r*(1-fog)+5*fog);
                g=clampi(g*(1-fog));
                b=clampi(b*(1-fog)+30*fog);
            }
            px(buf,x,y,r,g,b);
        }
    }
}