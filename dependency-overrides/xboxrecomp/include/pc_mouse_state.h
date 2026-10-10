#ifndef UCRR_PC_MOUSE_STATE_H
#define UCRR_PC_MOUSE_STATE_H
/* One raw-motion snapshot per original controller publication; queries never
 * consume it. Console turn-rate limits still apply. No stale-motion carryover. */
typedef struct { long dx,dy; float x,y; unsigned long long last; int active; } UCRR_PC_MOUSE;
/* Kept outside motion reset so opening settings does not discard the choice. */
static unsigned ucrr_mouse_sensitivity_percent=400;
static void ucrr_mouse_reset(UCRR_PC_MOUSE*m,int active){memset(m,0,sizeof(*m));m->active=active;}
static long ucrr_mouse_add(long a,long b){double s=(double)a+b;return s>4096?4096:s< -4096?-4096:(long)s;}
static float ucrr_mouse_strength(long delta,double dt){double v=delta*0.003*(ucrr_mouse_sensitivity_percent/100.0)*(1.0/60.0)/dt;return (float)(v>1?1:v< -1?-1:v);}
static void ucrr_mouse_publish(UCRR_PC_MOUSE*m,unsigned long long now){
 double dt=m->last&&now>m->last?(now-m->last)/1000.0:1.0/60.0;
 if(dt<0.004)dt=0.004;if(dt>0.1)dt=0.1;
 m->x=m->active?ucrr_mouse_strength(m->dx,dt):0;
 m->y=m->active?ucrr_mouse_strength(-m->dy,dt):0;
 m->dx=m->dy=0;m->last=now;
}
#endif
