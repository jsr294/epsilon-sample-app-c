/* NUMNLANDS — version NumWorks (Epsilon, API EADK). Même noyau que la version HTML : game_core.c.
   Rendu allégé : vue de dessus 6 px/tuile, rectangles uniformes uniquement. NON TESTÉ sur calculatrice. */
#include "eadk.h"
#include "game_core.h"
#include <stdbool.h>
const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "NumNLands";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;
#define PX 6
#define RGB(r,g,b) ((eadk_color_t)((((r)>>3)<<11)|(((g)>>2)<<5)|((b)>>3)))
static float msqrt(float x){            /* racine carrée sans libm (Newton) */
  if(x<=0)return 0;float y=x>2?x*.5f:1;for(int i=0;i<12;i++)y=.5f*(y+x/y);return y;}
static float camx,camy;
static void rect(int x,int y,int w,int h,eadk_color_t c){
  if(x<0){w+=x;x=0;}if(y<0){h+=y;y=0;}if(x+w>320)w=320-x;if(y+h>240)h=240-y;
  if(w>0&&h>0){eadk_rect_t r={x,y,w,h};eadk_display_push_rect_uniform(r,c);}}
static int sx(float wx){return (int)((wx-camx)*PX+160);}
static int sy(float wy){return (int)((wy-camy)*PX+120);}
static void span(int y,float a,float b,eadk_color_t c){int x0=sx(a),x1=sx(b);if(x1>x0)rect(x0,y,x1-x0,PX,c);}
static void render(void){
  const eadk_color_t WATER=RGB(40,150,220),GRASS=RGB(110,200,80),STORM=RGB(130,60,200);
  camx=G.f[0].x;camy=G.f[0].y;
  for(int r=0;r<40;r++){
    int y=r*PX;float wy=camy+(y+PX/2-120)/(float)PX,dy=wy-CY;
    rect(0,y,320,PX,WATER);
    if(dy*dy>=IR*IR)continue;
    float h=msqrt(IR*IR-dy*dy),a=CX-h,b=CX+h,ds=wy-G.sy;
    if(ds*ds<G.sr*G.sr){
      float hs=msqrt(G.sr*G.sr-ds*ds),lo=G.sx-hs>a?G.sx-hs:a,hi=G.sx+hs<b?G.sx+hs:b;
      if(lo>=hi)span(y,a,b,STORM);else{span(y,a,lo,STORM);span(y,lo,hi,GRASS);span(y,hi,b,STORM);}
    }else span(y,a,b,STORM);}
  for(int i=0;i<G.no;i++){const Obst*o=&G.o[i];
    eadk_color_t c=o->kind==2?RGB(220,90,80):o->kind==1?RGB(150,155,165):RGB(30,120,50);
    rect(sx(o->x),sy(o->y),(int)(o->w*PX)+1,(int)(o->h*PX)+1,c);}
  for(int i=0;i<NC;i++)if(!G.c[i].open)rect(sx(G.c[i].x)-2,sy(G.c[i].y)-2,5,5,RGB(255,210,60));
  for(int i=0;i<NB;i++)if(G.b[i].on)rect(sx(G.b[i].x)-1,sy(G.b[i].y)-1,3,3,RGB(255,255,150));
  for(int i=NF-1;i>=0;i--)if(G.f[i].alive)rect(sx(G.f[i].x)-3,sy(G.f[i].y)-3,6,6,i?RGB(230,60,70):RGB(255,255,255));
  rect(4,4,62,8,RGB(40,30,60));rect(5,5,G.f[0].hp*60/100,6,RGB(60,220,130));
  char s[16];int n=G.f[0].ammo[G.f[0].w],k=0;char t[8];int m=0;if(!n)t[m++]='0';while(n){t[m++]='0'+n%10;n/=10;}
  while(m)s[k++]=t[--m];s[k]=0;
  eadk_point_t p={72,2};eadk_display_draw_string(s,p,false,RGB(255,210,60),WATER);}
int main(int argc,char*argv[]){
  unsigned seed=1;bool pe=false,ps=false;
  gc_init(seed);
  for(;;){
    eadk_keyboard_state_t k=eadk_keyboard_scan();
    float mx=(eadk_keyboard_key_down(k,eadk_key_right)?1:0)-(eadk_keyboard_key_down(k,eadk_key_left)?1:0);
    float my=(eadk_keyboard_key_down(k,eadk_key_down)?1:0)-(eadk_keyboard_key_down(k,eadk_key_up)?1:0);
    bool e=eadk_keyboard_key_down(k,eadk_key_exe),s=eadk_keyboard_key_down(k,eadk_key_shift);
    gc_input(mx,my,eadk_keyboard_key_down(k,eadk_key_ok),e&&!pe,s&&!ps);pe=e;ps=s;seed++;
    gc_step();render();
    if(G.phase){
      eadk_point_t p={104,110};
      eadk_display_draw_string(G.phase==1?"VICTOIRE !":"ELIMINE",p,true,RGB(255,255,255),RGB(40,30,60));
      eadk_display_draw_string("OK : rejouer",(eadk_point_t){110,135},false,RGB(255,255,255),RGB(40,30,60));
      do{eadk_timing_msleep(50);seed++;}while(!eadk_keyboard_key_down(eadk_keyboard_scan(),eadk_key_ok));
      gc_init(seed);}
    eadk_display_wait_for_vblank();}
  return 0;}
