// wallpaper_all.c - 5合1壁纸生成器
// 编译: gcc -o wallpaper_all wallpaper_all.c -lm   (Linux测试)
//       或直接在自研OS中调用对应函数
// 调用: background_aurora(buf);
//       background_synthwave(buf);
//       background_mandelbrot(buf);
//       background_nebula(buf);
//       background_original(buf);

#ifndef _WALLPAPER_ALL_H
#define _WALLPAPER_ALL_H

#ifndef WIDTH
#define WIDTH  1024
#endif
#ifndef HEIGHT
#define HEIGHT 768
#endif

/* ======================== 基础工具 ======================== */

static float my_sqrt(float x){
    if(x<=0)return 0;float s=x;int i;
    for(i=0;i<10;i++)s=(s+x/s)*0.5f;return s;
}
static float my_sin(float x){
    while(x> 3.14159265f)x-=6.28318530f;
    while(x<-3.14159265f)x+=6.28318530f;
    float x2=x*x;
    return x-x2*x/6.0f+x2*x2*x/120.0f-x2*x2*x2*x/5040.0f;
}
static float my_fabs(float x){return x<0?-x:x;}
static int clampi(float v){int i=(int)v;return i<0?0:i>255?255:i;}

static void wp_set_pixel(char *buf, int x, int y, int r, int g, int b){
    if(r<0)r=0;if(r>255)r=255;
    if(g<0)g=0;if(g>255)g=255;
    if(b<0)b=0;if(b>255)b=255;
    int idx=(y*WIDTH+x)*4;
    buf[idx]  =(char)b;
    buf[idx+1]=(char)g;
    buf[idx+2]=(char)r;
    buf[idx+3]=(char)0xFF;
}

/* ======================== 噪声工具 ======================== */

static unsigned int _hash_u(unsigned int n){
    n=(n^(n>>13))*1274126177u;
    return n^(n>>16);
}
static float hash2d(int ix, int iy){
    return (float)(_hash_u((unsigned)(ix*374761393+iy*668265263))&0xFFFF)/65535.0f;
}
static float smooth_noise(float x, float y, float sc){
    float sx=x/sc, sy=y/sc;
    int ix=(int)sx, iy=(int)sy;
    float fx=sx-ix, fy=sy-iy;
    fx=fx*fx*(3-2*fx); fy=fy*fy*(3-2*fy);
    return (hash2d(ix,iy)*(1-fx)+hash2d(ix+1,iy)*fx)*(1-fy)+
           (hash2d(ix,iy+1)*(1-fx)+hash2d(ix+1,iy+1)*fx)*fy;
}
static float fbm(float x, float y){
    float val=0,amp=0.5f;int i;
    for(i=0;i<5;i++){val+=amp*smooth_noise(x,y,80.0f);x*=2;y*=2;amp*=0.5f;}
    return val;
}

/* ================================================================
   1. 原始版 - 蓝紫渐变 + 发光 + 光带 + 圆环 + 暗角
   ================================================================ */
void background_original(char *buf){
    int x,y;
    for(y=0;y<HEIGHT;y++){
        for(x=0;x<WIDTH;x++){
            float nx=(float)x/WIDTH, ny=(float)y/HEIGHT;
            float cx=(nx-0.5f)*2, cy=(ny-0.5f)*2;
            float dist=my_sqrt(cx*cx+cy*cy);

            int r=(int)(10+20*ny), g=(int)(10+15*ny), b=(int)(40+60*(1-ny));

            {float glow=1-dist*1.2f; if(glow>0){r+=30*glow;g+=20*glow;b+=80*glow;}}
            {float d=my_fabs((cx+cy)*0.5f-0.3f)*4;float s=1-d;
             if(s>0){s*=0.3f;r+=60*s;g+=40*s;b+=120*s;}}
            {float ring=my_fabs(my_sin(dist*12-ny*3))*0.15f;r+=40*ring;g+=50*ring;b+=100*ring;}
            {float v=1-dist*0.6f;if(v<0)v=0;if(v>1)v=1;r*=v;g*=v;b*=v;}

            wp_set_pixel(buf,x,y,r,g,b);
        }
    }
}

/* ================================================================
   2. 赛博朋克 - Synthwave Neon Grid
   ================================================================ */
void background_synthwave(char *buf){
    int x,y;
    int horizon=HEIGHT*45/100;
    for(y=0;y<HEIGHT;y++){
        for(x=0;x<WIDTH;x++){
            float nx=(float)x/WIDTH;
            int r,g,b;

            if(y<horizon){
                float t=(float)y/horizon;
                r=clampi(5+80*(1-t)*(1-t));
                g=clampi(10*(1-t));
                b=clampi(30+60*(1-t));
                /* 太阳 */
                float dx=(nx-0.5f)/0.18f;
                float sy=(float)(y-horizon*55/100)/(HEIGHT*0.22f);
                float sd=dx*dx+sy*sy;
                if(sd<1.0f){
                    float sun=1.0f-sd;
                    float sl=sy;if(sl<0)sl=-sl;
                    if(sl>0.02f&&sl<0.08f) sun=0;
                    r=clampi(r+255*sun*0.9f);
                    g=clampi(g+80*sun*0.5f);
                    b=clampi(b+120*sun*0.6f);
                }
            }else{
                float depth=(float)(y-horizon)/(HEIGHT-horizon);
                float vx=(nx-0.5f)/(depth+0.001f);
                float vm=vx/60;vm=vm-(int)vm;if(vm<0)vm=-vm;vm=vm*2-1;if(vm<0)vm=-vm;
                vm=1-vm*8;if(vm<0)vm=0;
                float hy=1/depth,hm=hy/40;hm=hm-(int)hm;if(hm<0)hm=-hm;hm=hm*2-1;if(hm<0)hm=-hm;
                hm=1-hm*8;if(hm<0)hm=0;
                float line=vm>hm?vm:hm;
                float fade=1-depth*0.5f;
                r=clampi(10+180*line*fade);
                g=clampi(30*line*fade);
                b=clampi(30+200*line*fade);
                float fog=depth*0.5f;
                r=clampi(r*(1-fog)+5*fog);
                g=clampi(g*(1-fog));
                b=clampi(b*(1-fog)+30*fog);
            }
            wp_set_pixel(buf,x,y,r,g,b);
        }
    }
}

/* ================================================================
   3. 分形 - Mandelbrot
   ================================================================ */
void background_mandelbrot(char *buf){
    int x,y;
    float x0m=-2.2f,x0x=0.8f,y0m=-1.2f,y0x=1.2f;
    float dx=(x0x-x0m)/WIDTH, dy=(y0x-y0m)/HEIGHT;
    for(y=0;y<HEIGHT;y++){
        for(x=0;x<WIDTH;x++){
            float cx=x0m+x*dx, cy=y0m+y*dy;
            float zx=0,zy=0;int i=0;
            while(zx*zx+zy*zy<4&&i<80){
                float t=zx*zx-zy*zy+cx;zy=2*zx*zy+cy;zx=t;i++;
            }
            int r=0,g=0,b=0;
            if(i<80){
                float t=(float)i/80,t1=1-t;
                r=(int)(9*t1*t*t*t*255);
                g=(int)(15*t1*t1*t*t*255);
                b=(int)(8.5*t1*t1*t1*t*255);
                if(i>56){float gl=(i-56)/24.0f;r+=20*gl;g+=40*gl;b+=100*gl;}
            }
            wp_set_pixel(buf,x,y,r,g,b);
        }
    }
}

/* ================================================================
   4. 极光 - Aurora Borealis
   ================================================================ */
void background_aurora(char *buf){
    int x,y;
    for(y=0;y<HEIGHT;y++){
        float ny=(float)y/HEIGHT;
        for(x=0;x<WIDTH;x++){
            float nx=(float)x/WIDTH;
            int r=2,g=2,b=8;

            if(hash2d(x*3/4,y*3/4)>0.997f){
                float sb=hash2d(x*37/10,y*37/10);
                r=clampi(200+55*sb);g=clampi(200+55*sb);b=clampi(220+35*sb);
            }else{
                int layer;
                for(layer=0;layer<4;layer++){
                    float wx=my_sin(nx*6+layer*1.5f+ny*2)*0.15f;
                    float wy=my_sin(nx*3+layer*0.8f)*0.1f;
                    float cy2=0.25f+layer*0.08f+wx+wy;
                    float dy=ny-cy2;if(dy<0)dy=-dy;
                    if(dy<0.25f){
                        float it=1-dy/0.25f; it*=it;
                        it*=0.5f+0.5f*smooth_noise(x+layer*100,y+layer*50,30);
                        if(layer==0){r+=clampi(30*it);g+=clampi(255*it);b+=clampi(100*it);}
                        else if(layer==1){r+=clampi(20*it);g+=clampi(200*it);b+=clampi(180*it);}
                        else if(layer==2){r+=clampi(80*it);g+=clampi(100*it);b+=clampi(255*it);}
                        else{r+=clampi(50*it);g+=clampi(255*it);b+=clampi(80*it);}
                    }
                }
                float mt=my_sin(nx*8)*0.03f+my_sin(nx*15+1)*0.015f+my_sin(nx*25+2)*0.008f;
                if(ny>0.88f+mt){
                    float dk=ny-0.88f-mt;if(dk>0.12f)dk=0.12f;dk/=0.12f;
                    r=(int)(r*(1-dk));g=(int)(g*(1-dk));b=(int)(b*(1-dk));
                }
            }
            wp_set_pixel(buf,x,y,r,g,b);
        }
    }
}

/* ================================================================
   5. 星云 - Nebula / Galaxy
   ================================================================ */
void background_nebula(char *buf){
    int x,y;
    for(y=0;y<HEIGHT;y++){
        for(x=0;x<WIDTH;x++){
            float nx=(float)x/WIDTH,ny=(float)y/HEIGHT;
            float cx=(nx-0.5f)*2,cy=(ny-0.5f)*2;
            float dist=my_sqrt(cx*cx+cy*cy);
            /* 近似 atan2 */
            float ax=my_fabs(cx),ay=my_fabs(cy),angle;
            if(ax>ay) angle=ay/(ax+1e-6f); else angle=1-ax/(ay+1e-6f);
            angle*=0.785398f;
            if(cy<0) angle=-angle;
            if(cx<0) angle=3.14159f-angle;

            int r=1,g=1,b=5;
            float a1=my_sin(angle-dist*8)*0.5f+0.5f;
            float a2=my_sin(angle-dist*8+3.14159f)*0.5f+0.5f;
            float arm=a1>a2?a1:a2;
            {float f=1-dist*0.8f;if(f<0)f=0;arm*=f;}
            float n1=fbm((float)x,(float)y);
            float n2=fbm((float)x+500,(float)y+500);
            float core=1-dist*2.5f;if(core<0)core=0;core*=core;

            if(hash2d((int)(x*0.7f),(int)(y*0.7f))>0.998f){
                float sb=hash2d((int)(x*3.1f),(int)(y*3.1f));
                r=180+(int)(75*sb);g=180+(int)(75*sb);b=200+(int)(55*sb);
            }else{
                r+=(int)(180*core+120*arm*n1);
                g+=(int)(60*core+30*arm*n1+60*arm*n2);
                b+=(int)(100*core+200*arm*n2+40*arm*n1);
            }
            wp_set_pixel(buf,x,y,r,g,b);
        }
    }
}

#endif /* _WALLPAPER_ALL_H */