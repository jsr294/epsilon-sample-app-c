#include "game_core.h"
#include <string.h>
/* Maths intégrées (pas de libm : indispensable sur NumWorks). Précision suffisante pour le jeu. */
static float msqrt(float x){if(x<=0)return 0;float h=.5f*x,y;unsigned i;memcpy(&i,&x,4);i=0x5f3759dfu-(i>>1);memcpy(&y,&i,4);
  y=y*(1.5f-h*y*y);y=y*(1.5f-h*y*y);y=y*(1.5f-h*y*y);return x*y;}
static float msin(float x){while(x>3.14159265f)x-=6.2831853f;while(x<-3.14159265f)x+=6.2831853f;
  float y=1.27323954f*x-.405284735f*x*(x<0?-x:x);return .225f*(y*(y<0?-y:y)-y)+y;}
static float mcos(float x){return msin(x+1.57079633f);}
static float matan2(float y,float x){float ax=x<0?-x:x,ay=y<0?-y:y;if(ax==0&&ay==0)return 0;
  float a=(ax<ay?ax:ay)/(ax>ay?ax:ay),q=a*a,r=((-.0464964749f*q+.15931422f)*q-.327622764f)*q*a+a;
  if(ay>ax)r=1.57079633f-r;if(x<0)r=3.14159265f-r;return y<0?-r:r;}
Game G;
const Weapon WP[4]={{12,14,1,30,.9f,16,.02f},{8,6,1,40,1.1f,18,.07f},{9,30,5,8,.8f,9,.22f},{60,55,1,5,2.2f,40,0}};
static unsigned rs;
static unsigned rnd(void){rs=rs*1664525u+1013904223u;return rs>>8;}
static float rf(void){return (rnd()&0xFFFF)/65536.0f;}
static float d2(float a,float b,float c,float d){return (a-c)*(a-c)+(b-d)*(b-d);}
static float cl(float v,float a,float b){return v<a?a:v>b?b:v;}
static int blocked(float x,float y,float r){
  for(int i=0;i<G.no;i++){const Obst*o=&G.o[i];if(x>o->x-r&&x<o->x+o->w+r&&y>o->y-r&&y<o->y+o->h+r)return 1;}
  return 0;}
static void push(float*x,float*y,float r){
  for(int i=0;i<G.no;i++){const Obst*o=&G.o[i];
    float dx=*x-cl(*x,o->x,o->x+o->w),dy=*y-cl(*y,o->y,o->y+o->h),d=msqrt(dx*dx+dy*dy);
    if(d<r&&d>1e-4f){*x+=dx/d*(r-d);*y+=dy/d*(r-d);}}
  float dx=*x-CX,dy=*y-CY,d=msqrt(dx*dx+dy*dy);
  if(d>IR-r){*x=CX+dx/d*(IR-r);*y=CY+dy/d*(IR-r);}}
static void move(Fighter*f,float dx,float dy){f->x+=dx;f->y+=dy;push(&f->x,&f->y,.35f);f->mv=(dx*dx+dy*dy>1e-6f);}
static void hurt(int t,int dmg,int own){
  Fighter*f=&G.f[t];if(!f->alive)return;f->hp-=dmg;
  if(f->hp<=0){f->hp=0;f->alive=0;G.alive--;if(own==0)G.kills++;}}
static void loot(Fighter*f,Chest*c){
  c->open=1;
  if(c->kind==0){int w=1+rnd()%3;f->has|=1<<w;f->ammo[w]+=WP[w].mag;f->w=w;}
  else if(c->kind==1){for(int w=0;w<4;w++)if(f->has>>w&1)f->ammo[w]+=WP[w].mag/2;}
  else f->hp=f->hp>55?100:f->hp+45;}
static void near_chest(int i){
  Fighter*f=&G.f[i];
  for(int k=0;k<NC;k++){Chest*c=&G.c[k];if(!c->open&&d2(f->x,f->y,c->x,c->y)<2.56f){loot(f,c);return;}}}
static void fire(int i){
  Fighter*f=&G.f[i];const Weapon*w=&WP[f->w];
  if(f->cd>0||f->ammo[f->w]<=0)return;
  f->cd=w->cd;f->ammo[f->w]--;
  for(int p=0;p<w->n;p++){
    int k=0;while(k<NB&&G.b[k].on)k++;if(k==NB)return;
    float a=f->a+(rf()-.5f)*2*w->spread;Bullet*b=&G.b[k];
    b->x=f->x+mcos(f->a)*.5f;b->y=f->y+msin(f->a)*.5f;b->vx=mcos(a)*w->spd;b->vy=msin(a)*w->spd;
    b->life=(int)(w->range/w->spd);b->dmg=w->dmg;b->own=i;b->on=1;}}
static void bullets(void){
  for(int k=0;k<NB;k++){Bullet*b=&G.b[k];if(!b->on)continue;
    int n=(int)(msqrt(b->vx*b->vx+b->vy*b->vy)/.5f)+1;
    for(int s=0;s<n&&b->on;s++){
      b->x+=b->vx/n;b->y+=b->vy/n;
      if(blocked(b->x,b->y,0)){b->on=0;break;}
      for(int j=0;j<NF;j++)if(j!=b->own&&G.f[j].alive&&d2(b->x,b->y,G.f[j].x,G.f[j].y)<.2f){hurt(j,b->dmg,b->own);b->on=0;break;}}
    if(--b->life<=0)b->on=0;}}
/* IA : st 0 patrouille/loot, 1 poursuite, 2 attaque. Profil = i%3 (0 agressif, 1 campeur, 2 rapide). */
static void ai(int i){
  Fighter*f=&G.f[i];int t=-1,typ=i%3;float bd=1e9f;
  for(int j=0;j<NF;j++)if(j!=i&&G.f[j].alive){float d=d2(f->x,f->y,G.f[j].x,G.f[j].y);if(d<bd){bd=d;t=j;}}
  float R=WP[f->w].range,tx=f->wx,ty=f->wy,mx=0,my=0,sp=.07f+.008f*typ;
  f->st=0;
  if(t>=0&&bd<R*R&&f->ammo[f->w]>0){
    f->st=2;float a=matan2(G.f[t].y-f->y,G.f[t].x-f->x);f->a=a+(rf()-.5f)*.4f;
    float s=((G.tick/45+i)&1)?.6f:-.6f;mx=-msin(a)*s;my=mcos(a)*s;
    if(bd>R*R*.36f){mx+=mcos(a);my+=msin(a);}
    if(G.tick>240&&rnd()%4==0)fire(i);   /* 8 s de grâce : le temps de fouiller les coffres */
  }else{
    if(d2(f->x,f->y,G.sx,G.sy)>(G.sr-3)*(G.sr-3)){tx=G.sx;ty=G.sy;}
    else if(t>=0&&bd<289&&typ!=1){f->st=1;tx=G.f[t].x;ty=G.f[t].y;}
    else{
      float bc=196;int ck=-1;
      for(int k=0;k<NC;k++)if(!G.c[k].open){float d=d2(f->x,f->y,G.c[k].x,G.c[k].y);if(d<bc){bc=d;ck=k;}}
      if(ck>=0){tx=G.c[ck].x;ty=G.c[ck].y;}
      else if(d2(f->x,f->y,f->wx,f->wy)<4||(G.tick+i*17)%300==0){
        float a=rf()*6.2832f,r=msqrt(rf())*(G.sr-4);if(r>IR-4)r=IR-4;f->wx=G.sx+mcos(a)*r;f->wy=G.sy+msin(a)*r;}}
    float dx=tx-f->x,dy=ty-f->y,d=msqrt(dx*dx+dy*dy)+1e-4f;mx=dx/d;my=dy/d;f->a=matan2(my,mx);}
  if(f->det){float s=f->det>0?1:-1,ox=mx;f->det-=(int)s;mx=-my*s+mx*.3f;my=ox*s+my*.3f;}   /* contournement */
  else if(!(G.tick%30)){if(d2(f->x,f->y,f->px,f->py)<.2f&&f->st!=2)f->det=(rnd()&1?1:-1)*(25+(int)(rnd()%25));f->px=f->x;f->py=f->y;}
  float l=msqrt(mx*mx+my*my);if(l>1){mx/=l;my/=l;}
  move(f,mx*sp,my*sp);}
static void player(void){
  Fighter*p=&G.f[0];float mx=G.in_mx,my=G.in_my,l=msqrt(mx*mx+my*my);
  if(l>1){mx/=l;my/=l;l=1;}
  move(p,mx*.11f,my*.11f);if(l>.1f)p->a=matan2(my,mx);
  if(G.in_fire){                               /* visée automatique sur l'ennemi le plus proche */
    int t=-1;float bd=WP[p->w].range*WP[p->w].range;
    for(int j=1;j<NF;j++)if(G.f[j].alive){float d=d2(p->x,p->y,G.f[j].x,G.f[j].y);if(d<bd){bd=d;t=j;}}
    if(t>=0)p->a=matan2(G.f[t].y-p->y,G.f[t].x-p->x);
    fire(0);}
  if(G.in_pick)near_chest(0);
  if(G.in_swap)for(int k=1;k<4;k++){int w=(p->w+k)&3;if(p->has>>w&1){p->w=w;break;}}
  G.in_pick=G.in_swap=0;}
void gc_init(unsigned seed){
  memset(&G,0,sizeof G);rs=seed*2654435761u+12345u;
  G.alive=NF;G.sx=CX+(rf()-.5f)*10;G.sy=CY+(rf()-.5f)*10;G.sr=IR+6;
  float a0=rf()*6.2832f;
  for(int i=0;i<NF;i++){Fighter*f=&G.f[i];float a=a0+i*6.2832f/NF;
    f->x=CX+mcos(a)*(IR-5);f->y=CY+msin(a)*(IR-5);f->a=a+3.14159f;f->hp=100;f->has=1;f->ammo[0]=30;f->alive=1;f->wx=CX;f->wy=CY;}
  for(int k=0;G.no<5&&k<300;k++){
    float x=CX+(rf()-.5f)*40,y=CY+(rf()-.5f)*40;if(d2(x,y,CX,CY)>400)continue;
    int ok=1;for(int j=0;j<G.no;j++)if(d2(x,y,G.o[j].x,G.o[j].y)<64)ok=0;
    if(ok){Obst o={x,y,3,3,2};G.o[G.no++]=o;}}
  for(int k=0;G.no<NO&&k<800;k++){
    float x=CX+(rf()-.5f)*56,y=CY+(rf()-.5f)*56;
    if(d2(x,y,CX,CY)>676||blocked(x,y,1.5f))continue;
    int ok=1;for(int i=0;i<NF;i++)if(d2(x,y,G.f[i].x,G.f[i].y)<9)ok=0;
    if(!ok)continue;
    Obst o={x-.4f,y-.4f,.8f,.8f,(int)(rnd()%4==0)};G.o[G.no++]=o;}
  for(int k=0;k<NC;k++){
    G.c[k].open=1;                                   /* « ouvert » = absent tant qu'aucune place n'est trouvée */
    for(int t=0;t<80;t++){
      float x=CX+(rf()-.5f)*52,y=CY+(rf()-.5f)*52;
      if(d2(x,y,CX,CY)>625||blocked(x,y,1.2f))continue;
      G.c[k].x=x;G.c[k].y=y;G.c[k].open=0;G.c[k].kind=rnd()%3;break;}}}
void gc_input(float mx,float my,int fire,int pick,int swap){
  G.in_mx=mx;G.in_my=my;G.in_fire=fire;G.in_pick|=pick;G.in_swap|=swap;}
void gc_step(void){
  if(G.phase)return;
  G.tick++;
  for(int i=0;i<NF;i++){Fighter*f=&G.f[i];if(!f->alive)continue;
    if(f->cd>0)f->cd--;
    if(f->ammo[f->w]<=0)for(int w=0;w<4;w++)if((f->has>>w&1)&&f->ammo[w]>0){f->w=w;break;}
    if(i){ai(i);if(f->alive)near_chest(i);}else player();
    if(d2(f->x,f->y,G.sx,G.sy)>G.sr*G.sr&&G.tick%12==0)hurt(i,2,-1);}
  bullets();
  if(G.tick>600&&G.sr>4)G.sr-=.0069f;           /* la tempête démarre après 20 s */
  if(!G.f[0].alive)G.phase=2;else if(G.alive<=1)G.phase=1;}
const float* gc_view(void){
  static float v[VIEW_LEN];int n=0;
  v[n++]=G.phase;v[n++]=G.alive;v[n++]=G.kills;v[n++]=G.sx;v[n++]=G.sy;v[n++]=G.sr;v[n++]=G.tick;v[n++]=0;
  for(int i=0;i<NF;i++){const Fighter*f=&G.f[i];
    v[n++]=f->x;v[n++]=f->y;v[n++]=f->a;v[n++]=f->hp;v[n++]=f->w;v[n++]=f->ammo[f->w];v[n++]=f->alive;v[n++]=f->cd;v[n++]=f->mv;v[n++]=f->st;}
  for(int i=0;i<NB;i++){const Bullet*b=&G.b[i];v[n++]=b->x;v[n++]=b->y;v[n++]=matan2(b->vy,b->vx);v[n++]=b->on;}
  for(int i=0;i<NC;i++){const Chest*c=&G.c[i];v[n++]=c->x;v[n++]=c->y;v[n++]=c->open;v[n++]=c->kind;}
  return v;}
const float* gc_map(void){
  static float m[1+NO*5];m[0]=G.no;
  for(int i=0;i<G.no;i++){const Obst*o=&G.o[i];float*p=&m[1+i*5];p[0]=o->x;p[1]=o->y;p[2]=o->w;p[3]=o->h;p[4]=o->kind;}
  return m;}
