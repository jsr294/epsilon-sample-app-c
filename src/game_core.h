/* NUMNLANDS — noyau de jeu portable (C99, sans malloc, sans I/O).
   Partagé entre le rendu HTML (WebAssembly) et la version NumWorks. */
#ifndef GAME_CORE_H
#define GAME_CORE_H
#define NF 12            /* combattants : joueur (0) + 11 bots */
#define NB 40            /* balles simultanées */
#define NC 16            /* coffres */
#define NO 64            /* obstacles (maisons, arbres, rochers) */
#define CX 32.0f         /* centre de l'île */
#define CY 32.0f
#define IR 29.0f         /* rayon de l'île (tuiles) */
#define VIEW_HDR 8
#define VIEW_F 10
#define VIEW_B 4
#define VIEW_C 4
#define VIEW_LEN (VIEW_HDR+NF*VIEW_F+NB*VIEW_B+NC*VIEW_C)
typedef struct{float x,y,a,wx,wy,px,py;int hp,w,has,ammo[4],cd,alive,st,mv,det;}Fighter;
typedef struct{float x,y,vx,vy;int life,dmg,own,on;}Bullet;
typedef struct{float x,y;int open,kind;}Chest;      /* kind 0 arme, 1 munitions, 2 soin */
typedef struct{float x,y,w,h;int kind;}Obst;        /* kind 0 arbre, 1 rocher, 2 maison */
typedef struct{int dmg,cd,n,mag;float spd,range,spread;}Weapon;
typedef struct{
  Fighter f[NF];Bullet b[NB];Chest c[NC];Obst o[NO];
  int no,tick,phase,alive,kills;           /* phase : 0 en cours, 1 victoire, 2 défaite */
  float sx,sy,sr;                          /* tempête : centre et rayon */
  float in_mx,in_my;int in_fire,in_pick,in_swap;
}Game;
extern Game G;
extern const Weapon WP[4];                 /* pistolet, fusil, shotgun, sniper */
void gc_init(unsigned seed);
/* mx,my : direction MONDE (|v|<=1). fire : maintenu. pick/swap : impulsions. */
void gc_input(float mx,float my,int fire,int pick,int swap);
void gc_step(void);                        /* un tick = 1/30 s */
const float* gc_view(void);                /* tampon d'état pour le rendu (VIEW_LEN floats) */
const float* gc_map(void);                 /* [n, (x,y,w,h,kind)*n] */
#endif
