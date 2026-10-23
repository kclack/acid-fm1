/* SPDX-License-Identifier: MIT
 * Original portable float DSP. Inspired by classic acid instruments and public
 * feature descriptions, not a port or circuit-identical model of Acid V. */
#include "acid.h"
/* Freestanding JieLi builds have no libc headers. These byte helpers also
 * keep the source identical in the native and WebAssembly builds. */
static void *acid_zero(void *dst, int value, uint32_t size)
{
    uint8_t *p=(uint8_t *)dst; while(size--)*p++=(uint8_t)value; return dst;
}
static void *acid_copy(void *dst,const void *src,uint32_t size)
{
    uint8_t *d=(uint8_t *)dst;const uint8_t *s=(const uint8_t *)src;
    while(size--)*d++=*s++;return dst;
}

#define PI 3.14159265358979323846f
#define TAU 6.2831853071795864769f
#define PARAM(n,l,h,d,q) {n,l,h,d,q}
const AcidParamInfo acid_params[P_COUNT] = {
    PARAM("Cutoff",0,1,.42f,0), PARAM("Resonance",0,1,.65f,0),
    PARAM("Envelope",0,1,.65f,0), PARAM("Decay",0,1,.38f,0),
    PARAM("Accent",0,1,.65f,0), PARAM("Slide",0,1,.32f,0),
    PARAM("Waveform",0,1,0,0), PARAM("Pulse width",.1f,.9f,.5f,0),
    PARAM("Tune",-12,12,0,0), PARAM("Sub",0,1,.12f,0),
    PARAM("Drive",0,1,.25f,0), PARAM("Distortion",0,6,0,1),
    PARAM("Distortion mix",0,1,.25f,0), PARAM("Tempo",30,300,130,0),
    PARAM("Swing",0,.6f,0,0), PARAM("Gate",.05f,.98f,.55f,0),
    PARAM("Length",1,64,16,1), PARAM("Direction",0,3,0,1),
    PARAM("Root",24,84,36,1), PARAM("Scale",0,4,1,1),
    PARAM("Delay mix",0,.65f,.14f,0), PARAM("Delay feedback",0,.85f,.32f,0),
    PARAM("Delay division",0,5,2,1), PARAM("Reverb",0,.6f,.08f,0),
    PARAM("Chorus",0,.6f,0,0), PARAM("Volume",0,1,.7f,0),
    PARAM("Bass boost",0,1,.2f,0), PARAM("Circuit noise",0,1,0,0),
    PARAM("Vibrato rate",.1f,12,5,0), PARAM("Vibrato depth",0,1,0,0),
    PARAM("Mode",0,2,0,1), PARAM("Clock",0,1,0,1),
    PARAM("Mod 1 rate",0,8,3,1), PARAM("Mod 1 depth",-1,1,0,0),
    PARAM("Mod 1 target",0,6,0,1), PARAM("Mod 1 shape",0,5,1,1),
    PARAM("Mod 1 length",1,64,16,1),
    PARAM("Mod 2 rate",0,8,4,1), PARAM("Mod 2 depth",-1,1,0,0),
    PARAM("Mod 2 target",0,6,0,1), PARAM("Mod 2 shape",0,5,0,1),
    PARAM("Mod 2 length",1,64,12,1),
    PARAM("Mod 3 rate",0,8,2,1), PARAM("Mod 3 depth",-1,1,0,0),
    PARAM("Mod 3 target",0,6,0,1), PARAM("Mod 3 shape",0,5,5,1),
    PARAM("Mod 3 length",1,64,7,1),
    PARAM("Accent attack",0,1,.05f,0), PARAM("Filter tracking",0,1,.25f,0),
    PARAM("Clipping",0,1,.3f,0), PARAM("Accent length",1,64,16,1),
    PARAM("Slide length",1,64,16,1), PARAM("Mutation",0,1,.25f,0)
};
#undef PARAM

static float bound(float x,float lo,float hi) { return x < lo ? lo : x > hi ? hi : x; }
static float absf(float x) { return x < 0 ? -x : x; }
static int finitef(float x) { return x == x && x > -3.0e37f && x < 3.0e37f; }
static float frac(float x) { return x - (float)(int)x; }
/* Single precision only, no libm or compiler soft-double dependency. */
static float exp2fast(float x)
{
    int n; float f, p;
    x = bound(x,-20.0f,20.0f); n = (int)x; if (x < (float)n) n--;
    f = x-(float)n;
    p = 1.0f+f*(.69314718f+f*(.24022651f+f*(.05550411f+f*(.00961813f+f*.00133336f))));
    if (n >= 0) return p*(float)(1u << n);
    return p/(float)(1u << -n);
}
static float sinphase(float phase)
{
    float x = phase*2.0f-1.0f, y = 4.0f*x*(1.0f-absf(x));
    return y*(.775f+.225f*absf(y));
}
static float soft(float x)
{
    float q; x=bound(x,-3,3); q=x*x;
    return x*(27.0f+q)/(27.0f+9.0f*q);
}
static uint32_t random32(Acid *a)
{
    uint32_t x=a->rng; x^=x<<13; x^=x>>17; x^=x<<5; a->rng=x ? x : 1u; return x;
}
static float random01(Acid *a) { return (float)(random32(a)>>8)*(1.0f/16777216.0f); }
static float blep(float t,float dt)
{
    if(t<dt) {t/=dt; return t+t-t*t-1.0f;}
    if(t>1.0f-dt) {t=(t-1.0f)/dt;return t*t+t+t+1.0f;}
    return 0;
}

int acid_quantize(int note,int scale)
{
    static const uint16_t masks[5]={0xfffu,0x5adu,0x4a9u,0x6adu,0xab5u};
    int pc,down=0; uint16_t mask=masks[scale<0||scale>4?0:scale];
    pc=note%12;if(pc<0)pc+=12;
    while(!(mask & (1u<<pc)) && down<12){down++;pc=(pc+11)%12;}
    return note-down;
}

void acid_set(Acid *a,int id,float value)
{
    if(id<0||id>=P_COUNT||!finitef(value))return;
    value=bound(value,acid_params[id].lo,acid_params[id].hi);
    if(acid_params[id].discrete)value=(float)(int)(value+.5f);
    a->project.p[id]=value;
    if(id==P_MODE){a->held_count=0;acid_zero(a->held,0,sizeof a->held);}
    if(id==P_MODE || id==P_CLOCK){a->gate=0;a->amp_target=0;a->pending=1;}
    if(id==P_LENGTH && a->position>=(int)value)a->position=0;
}
void acid_step_set(Acid *a,int step,int field,float v)
{
    AcidStep *s;if(step<0||step>=64||!finitef(v))return;s=&a->project.step[step];
    switch(field){
    case 0:s->note=(int8_t)bound(v,-24,36);break;
    case 1:s->gate=(uint8_t)(v!=0);break;
    case 2:s->accent=(uint8_t)(v!=0);break;
    case 3:s->slide=(uint8_t)(v!=0);break;
    case 4:s->probability=(uint8_t)bound(v,0,100);break;
    case 5:s->ratchet=(uint8_t)bound(v,1,4);break;
    case 6:s->cutoff=bound(v,-1,1);break;
    default:break;
    }
}
void acid_curve_set(Acid *a,int m,int point,float v)
{if(m>=0&&m<3&&point>=0&&point<8&&finitef(v))a->project.curve[m][point]=bound(v,-1,1);}

void acid_panic(Acid *a)
{
    a->gate=0;a->amp=0;a->amp_target=0;a->env=0;a->accent_env=0;
    a->held_count=0;acid_zero(a->held,0,sizeof a->held);
    acid_zero(a->z,0,sizeof a->z);a->dc_x=a->dc_y=0;
    acid_zero(a->delay_buf,0,sizeof a->delay_buf);
    acid_zero(a->chorus_buf,0,sizeof a->chorus_buf);
    acid_zero(a->reverb_buf,0,sizeof a->reverb_buf);
    acid_zero(a->rev_damp,0,sizeof a->rev_damp);a->peak=0;
}
void acid_start(Acid *a)
{
    a->playing=1;a->position=-1;a->direction=1;a->step_serial=0;
    a->remaining=0;a->elapsed=0;a->pending=1;a->ticks=0;
    a->clock_age=0;a->clock_elapsed=0;a->external_lost=0;a->arp_index=0;
    a->rng=a->project.seed?a->project.seed:1;
    a->gate=0;a->amp_target=0;
}
void acid_stop(Acid *a){a->playing=0;a->gate=0;a->amp_target=0;a->pending=0;}

static void trigger(Acid *a,int note,int accent,int slide)
{
    a->target_pitch=bound((float)note,0,127);a->note=note;
    a->accented=accent;a->accent_target=accent?a->project.p[P_ACCENT]:0;
    if(!slide||!a->gate){a->pitch=a->target_pitch;a->env=1.0f;a->accent_env=0;}
    a->gate=1;a->amp_target=accent ? 1.0f : .72f;
}
void acid_note_on(Acid *a,int note,int velocity)
{
    if(note<0||note>127)return;
    if(!a->held[note])a->held_count++;
    a->held[note]=(uint8_t)bound((float)velocity,1,127);a->order[note]=++a->serial;
    if((int)a->project.p[P_MODE]==1)trigger(a,note,velocity>=100,a->gate);
    else if((int)a->project.p[P_MODE]==0)acid_set(a,P_ROOT,(float)note);
}
void acid_note_off(Acid *a,int note)
{
    uint32_t newest=0;int i,last=-1;
    if(note<0||note>127)return;
    if(a->held[note])a->held_count--;a->held[note]=0;
    if((int)a->project.p[P_MODE]!=1||a->note!=note)return;
    for(i=0;i<128;i++)if(a->held[i]&&a->order[i]>newest){newest=a->order[i];last=i;}
    if(last>=0)trigger(a,last,a->held[last]>=100,1);
    else {a->gate=0;a->amp_target=0;}
}

static int next_position(Acid *a)
{
    int len=(int)a->project.p[P_LENGTH],dir=(int)a->project.p[P_DIRECTION],pos=a->position;
    if(pos<0)return dir==1?len-1:0;
    if(dir==3)return (int)(random32(a)%(uint32_t)len);
    if(dir==1)return(pos+len-1)%len;
    if(dir==2){if(len==1)return 0;pos+=a->direction;if(pos>=len){a->direction=-1;pos=len-2;}if(pos<0){a->direction=1;pos=1;}return pos;}
    return(pos+1)%len;
}
static void sequence_step(Acid *a)
{
    float *p=a->project.p;int prev=a->position,legato=0,accent,slide,mode=(int)p[P_MODE];
    AcidStep *s;int i,note;
    if(prev>=0)legato=a->project.step[(a->step_serial-1)%(int)p[P_SLIDE_LENGTH]].slide && a->gate;
    a->position=next_position(a);s=&a->project.step[a->position];
    accent=a->project.step[a->step_serial%(int)p[P_ACC_LENGTH]].accent;
    slide=a->project.step[a->step_serial%(int)p[P_SLIDE_LENGTH]].slide;
    (void)slide;
    a->duration=a->fs*15.0f/p[P_TEMPO]*(1.0f+((a->step_serial&1)?-p[P_SWING]:p[P_SWING]));
    if(p[P_CLOCK]>.5f)a->duration=a->estimated_step;
    a->remaining+=a->duration;a->elapsed=0;a->ratchet_index=0;a->pending=0;
    a->step_serial++;
    a->cutoff_lock=s->cutoff;
    a->step_active=0;
    if(mode==2){
        int notes[16],count=0,index;
        for(i=0;i<128&&count<16;i++)if(a->held[i])notes[count++]=i;
        if(!count){a->gate=0;a->amp_target=0;return;}
        index=(a->arp_index++)%count;if((int)p[P_DIRECTION]==1)index=count-1-index;
        if((int)p[P_DIRECTION]==3)index=(int)(random32(a)%(uint32_t)count);
        a->step_active=1;trigger(a,notes[index],accent,legato);return;
    }
    if(!s->gate||random01(a)*100.0f>=(float)s->probability){a->gate=0;a->amp_target=0;return;}
    note=(int)p[P_ROOT]+acid_quantize(s->note,(int)p[P_SCALE]);
    a->step_active=1;trigger(a,note,accent,legato);
}

void acid_midi(Acid *a,uint8_t status,uint8_t d1,uint8_t d2)
{
    uint8_t type=status&0xf0;float *p=a->project.p;
    if(status==0xf8 && p[P_CLOCK]>.5f){
        a->clock_age=0;a->external_lost=0;
        if(a->playing){
            if(a->ticks && a->clock_elapsed>0){float estimate=a->clock_elapsed*6.0f;a->estimated_step+=(estimate-a->estimated_step)*.12f;}
            a->clock_elapsed=0;
            if(a->ticks%6u==0)sequence_step(a);
            a->ticks++;
        }return;
    }
    if(status==0xfa){acid_start(a);return;}
    if(status==0xfb){a->playing=1;a->clock_age=0;a->external_lost=0;return;}
    if(status==0xfc){acid_stop(a);return;}
    if(type==0x90 && d2)acid_note_on(a,d1,d2);
    else if(type==0x80||type==0x90)acid_note_off(a,d1);
    else if(type==0xb0){
        switch(d1){
        case 1:acid_set(a,P_VIB_DEPTH,(float)d2/127.0f);break;
        case 7:acid_set(a,P_VOLUME,(float)d2/127.0f);break;
        case 71:acid_set(a,P_RESONANCE,(float)d2/127.0f);break;
        case 74:acid_set(a,P_CUTOFF,(float)d2/127.0f);break;
        case 75:acid_set(a,P_DECAY,(float)d2/127.0f);break;
        case 76:acid_set(a,P_ENV,(float)d2/127.0f);break;
        case 77:acid_set(a,P_ACCENT,(float)d2/127.0f);break;
        case 78:acid_set(a,P_DRIVE,(float)d2/127.0f);break;
        case 91:acid_set(a,P_REVERB,(float)d2/127.0f*.6f);break;
        case 93:acid_set(a,P_DELAY,(float)d2/127.0f*.65f);break;
        case 120:acid_stop(a);acid_panic(a);break;
        case 123:a->gate=0;a->amp_target=0;a->held_count=0;acid_zero(a->held,0,sizeof a->held);break;
        default:break;
        }
    }else if(type==0xe0)acid_set(a,P_TUNE,((float)(d1+128u*d2)-8192.0f)*(2.0f/8192.0f));
}

static void control(Acid *a)
{
    float *p=a->project.p,dt=32.0f/a->fs,m[7]={0,0,0,0,0,0,0},base,fc,g,angle;
    int i;
    for(i=0;i<3;i++){
        int off=P_MOD1_RATE+i*5,shape=(int)p[off+3],j;float ph,v,rate;
        /* Each lane has an independent cycle length in sixteenth notes. */
        rate=p[P_TEMPO]/15.0f/p[off+4]*exp2fast((p[off]-3.0f)*.5f);
        a->mod_phase[i]+=rate*dt;
        if(a->mod_phase[i]>=1){a->mod_phase[i]=frac(a->mod_phase[i]);a->mod_random[i]=random01(a)*2-1;}
        ph=a->mod_phase[i];
        switch(shape){
        case 0:v=sinphase(ph);break;case 1:v=1.0f-4.0f*absf(ph-.5f);break;
        case 2:v=2*ph-1;break;case 3:v=ph<.5f?1:-1;break;case 4:v=a->mod_random[i];break;
        default:{float at=ph*8.0f,f; j=(int)at;f=at-(float)j;v=a->project.curve[i][j%8]*(1-f)+a->project.curve[i][(j+1)%8]*f;break;}
        }
        a->mod_value[i]=v*p[off+1];m[(int)p[off+2]]+=a->mod_value[i];
    }
    a->vibrato_phase=frac(a->vibrato_phase+p[P_VIB_RATE]*dt);
    a->mod_pitch=m[5]*12.0f+sinphase(a->vibrato_phase)*p[P_VIB_DEPTH]*.8f;
    a->glide_coeff=32.0f/(a->fs*(.002f+p[P_SLIDE]*p[P_SLIDE]*.35f)+32.0f);
    a->pitch+=(a->target_pitch-a->pitch)*a->glide_coeff;
    a->frequency=440.0f*exp2fast((a->pitch+p[P_TUNE]+a->mod_pitch-69.0f)/12.0f);
    a->frequency=bound(a->frequency,8,9000);
    base=a->cutoff_lock>=0?a->cutoff_lock:p[P_CUTOFF];
    base+=m[1]*.5f+p[P_ENV]*a->env*.65f+a->accent_env*.3f+(a->pitch-36)*p[P_TRACKING]/96.0f;
    fc=35.0f*exp2fast(bound(base,0,1.4f)*8.2f);
    fc=bound(fc,25,a->fs*.32f);
    angle=PI*fc/(a->fs*2.0f);g=angle*(1.0f+angle*angle*(.33333333f+angle*angle*.13333333f));
    a->coeff=g/(1+g);
    a->resonance=bound(p[P_RESONANCE]+m[2]*.3f,0,1)*3.85f;
    a->drive=bound(p[P_DRIVE]+m[4]*.4f,0,1);
    a->pulse=bound(p[P_PULSE]+m[3]*.35f,.08f,.92f);
    a->mod_volume=bound(1+m[6]*.8f,0,1.8f);
    a->envelope_coeff=1.0f/(1.0f+1.0f/(a->fs*(.025f+p[P_DECAY]*p[P_DECAY]*2.4f)));
    a->amp_coeff=1.0f/(a->fs*(a->gate?.002f:.018f)+1);
    a->sub=p[P_SUB];a->noise=p[P_NOISE]*.007f;
}

static float distortion(float x,int type,float drive)
{
    float y=x*(1+drive*14),z;
    switch(type){
    case 0:return soft(y);
    case 1:return bound(y,-.85f,.85f);
    case 2:z=bound(y,-8,8);while(z>1||z<-1){if(z>1)z=2-z;if(z<-1)z=-2-z;}return z;
    case 3:z=16.0f+240.0f*(1-drive);return (float)(int)(bound(y,-1,1)*z)/z;
    case 4:return soft(y+.4f)-soft(.4f);
    case 5:return absf(soft(y))*2-.5f;
    default:z=y*.125f+.5f;z-= (float)(int)z;if(z<0)z+=1;return sinphase(z);
    }
}
static float delay_read(Acid *a,float back)
{
    float at=(float)a->delay_pos-back;int pos;float f;
    while(at<0)at+=(float)ACID_DELAY;
    pos=(int)at;f=at-(float)pos;
    return ((float)a->delay_buf[pos&(ACID_DELAY-1)]*(1-f)+(float)a->delay_buf[(pos+1)&(ACID_DELAY-1)]*f)/32768.0f;
}

void acid_process(Acid *a,float *out,uint32_t n)
{
    uint32_t i;float *p=a->project.p;
    static const float divisions[6]={.25f,.33333333f,.5f,.75f,1.0f,1.5f};
    static const uint16_t rlen[4]={1499,1601,1747,1999};
    for(i=0;i<n;i++){
        float sample=0,l,r,wet,dt,ph,osc,sub,G,feed,S,y,delay,rv=0,chor;int over,k;
        if(a->playing&&(int)p[P_MODE]!=1){
            if(p[P_CLOCK]<.5f && (a->pending||a->remaining<=0))sequence_step(a);
            if(p[P_CLOCK]>.5f&&a->clock_age>=(uint32_t)(a->fs*2)){a->external_lost=1;a->gate=0;a->amp_target=0;}
            if(a->position>=0&&!a->pending){
                AcidStep *s=&a->project.step[a->position];
                int slide=a->project.step[(a->step_serial-1)%(int)p[P_SLIDE_LENGTH]].slide;
                float subdur=a->duration/(float)s->ratchet;
                int rat=(int)(a->elapsed/subdur);
                if(rat>a->ratchet_index&&rat<s->ratchet&&a->step_active){
                    a->ratchet_index=rat;trigger(a,a->note,a->accented,0);
                }
                if(!slide && a->elapsed-(float)a->ratchet_index*subdur>subdur*p[P_GATE]){a->gate=0;a->amp_target=0;}
                a->elapsed+=1;a->remaining-=1;
            }
        }
        if((a->control++&31u)==0)control(a);
        a->env*=a->envelope_coeff;
        a->accent_env+=((a->gate?a->accent_target:0)-a->accent_env)/(1+a->fs*(.001f+p[P_ACC_ATTACK]*.08f));
        a->amp+=(a->amp_target-a->amp)*a->amp_coeff;
        G=a->coeff;dt=a->frequency/(a->fs*2);
        for(over=0;over<2;over++){
            a->phase+=dt;if(a->phase>=1)a->phase-=1;ph=a->phase;
            osc=2*ph-1-blep(ph,dt);
            y=ph<a->pulse?1:-1;y+=blep(ph,dt);
            feed=ph-a->pulse;if(feed<0)feed+=1;y-=blep(feed,dt);
            osc+=(y-osc)*p[P_WAVE];
            a->sub_phase+=dt*.5f;if(a->sub_phase>=1)a->sub_phase-=1;
            sub=sinphase(a->sub_phase)*a->sub;
            osc=(osc+sub+random01(a)*a->noise)*(1+a->drive*2.0f)*.65f;
            /* Zero-delay linear feedback solution around a saturated four-pole
             * TPT cascade. Pole states stay bounded under extreme resonance. */
            S=(1-G)*(G*G*G*a->z[0]+G*G*a->z[1]+G*a->z[2]+a->z[3]);
            feed=soft((osc-a->resonance*S)/(1+a->resonance*G*G*G*G));
            for(k=0;k<4;k++){y=G*feed+(1-G)*a->z[k];a->z[k]=bound(2*y-a->z[k],-8,8);feed=y;}
            sample+=feed*.5f;
        }
        a->low+=(sample-a->low)*.008f;
        sample=(sample+a->low*p[P_BASS]*.6f)*a->amp*(.65f+a->accent_env*.4f);
        sample+=(distortion(sample,(int)p[P_DIST],a->drive)-sample)*p[P_DIST_MIX];
        sample=soft(sample*(1+p[P_CLIP]*2))*.65f;
        y=sample-a->dc_x+.995f*a->dc_y;a->dc_x=sample;a->dc_y=y;sample=y;
        dt=a->fs*60.0f/p[P_TEMPO]*divisions[(int)p[P_DELAY_TIME]];
        dt=bound(dt,32,(float)ACID_DELAY-4);
        a->delay_smooth+=(dt-a->delay_smooth)*.0003f;
        delay=delay_read(a,a->delay_smooth);
        a->delay_buf[a->delay_pos]=(int16_t)(bound(soft(sample+delay*p[P_FEEDBACK]),-.99f,.99f)*32767);
        a->delay_pos=(a->delay_pos+1)&(ACID_DELAY-1);
        l=sample+delay*p[P_DELAY];r=sample+delay_read(a,a->delay_smooth*.75f)*p[P_DELAY];
        a->chorus_phase=frac(a->chorus_phase+.23f/a->fs);
        a->chorus_buf[a->chorus_pos]=sample;
        k=((int)a->chorus_pos-(int)(.012f*a->fs+sinphase(a->chorus_phase)*.003f*a->fs)+2048)&2047;
        chor=a->chorus_buf[k]*p[P_CHORUS];a->chorus_pos=(a->chorus_pos+1)&2047;
        l+=chor;r-=chor*.75f;
        for(k=0;k<4;k++){
            uint32_t pos=a->rev_pos[k];wet=a->reverb_buf[k][pos];
            a->rev_damp[k]+=(wet-a->rev_damp[k])*.23f;
            a->reverb_buf[k][pos]=soft(sample*.18f+a->rev_damp[k]*.72f);
            a->rev_pos[k]=(pos+1)%rlen[k];rv+=wet*.25f;
        }
        l+=rv*p[P_REVERB];r+=(rv*.7f+a->rev_damp[1]*.3f)*p[P_REVERB];
        a->level+=(p[P_VOLUME]*a->mod_volume-a->level)*.002f;
        l=soft(l*a->level);r=soft(r*a->level);
        if(!finitef(l)||!finitef(r)){l=r=0;acid_zero(a->z,0,sizeof a->z);}
        out[2*i]=l;out[2*i+1]=r;
        y=absf(l)>absf(r)?absf(l):absf(r);a->peak=y>a->peak?y:a->peak*.9997f;
        a->samples++;if(a->clock_age<0xfffffffeu)a->clock_age++;a->clock_elapsed+=1;
    }
}

int acid_project_valid(const AcidProject *q)
{
    int i,j;if(q->magic!=ACID_MAGIC||q->version!=ACID_VERSION)return 0;
    for(i=0;i<P_COUNT;i++)if(!finitef(q->p[i])||q->p[i]<acid_params[i].lo||q->p[i]>acid_params[i].hi||
       (acid_params[i].discrete&&q->p[i]!=(float)(int)q->p[i]))return 0;
    for(i=0;i<64;i++){
        const AcidStep *s=&q->step[i];
        if(s->note<-24||s->note>36||s->gate>1||s->accent>1||s->slide>1||s->probability>100||s->ratchet<1||s->ratchet>4||!finitef(s->cutoff)||s->cutoff<-1||s->cutoff>1)return 0;
    }
    for(i=0;i<3;i++)for(j=0;j<8;j++)if(!finitef(q->curve[i][j])||absf(q->curve[i][j])>1)return 0;
    return 1;
}
int acid_load(Acid *a,const AcidProject *q)
{
    if(!acid_project_valid(q))return 0;
    acid_stop(a);acid_copy(&a->project,q,sizeof *q);a->rng=q->seed?q->seed:1;
    a->position=-1;a->cutoff_lock=-1;return 1;
}
void acid_generate(Acid *a,uint32_t seed,float density)
{
    int i;a->rng=seed?seed:1;a->project.seed=a->rng;density=bound(density,.1f,1);
    for(i=0;i<64;i++){
        AcidStep *s=&a->project.step[i];acid_zero(s,0,sizeof *s);
        s->note=(int8_t)acid_quantize((int)(random32(a)%25u),(int)a->project.p[P_SCALE]);
        s->gate=(uint8_t)(i%4==0||random01(a)<density);
        s->accent=(uint8_t)(random01(a)<.28f);s->slide=(uint8_t)(random01(a)<.24f);
        s->probability=100;s->ratchet=1;s->cutoff=-1;
    }
    a->project.step[0].note=0;
}
void acid_mutate(Acid *a,uint32_t seed,float amount)
{
    int i;amount=bound(amount,0,1);a->rng=seed?seed:1;
    for(i=0;i<(int)a->project.p[P_LENGTH];i++)if(random01(a)<amount){
        AcidStep *s=&a->project.step[i];int op=(int)(random32(a)%4u);
        if(op==0)s->note=(int8_t)acid_quantize((int)(random32(a)%25u),(int)a->project.p[P_SCALE]);
        if(op==1)s->accent^=1;if(op==2)s->slide^=1;if(op==3&&i)s->gate^=1;
    }
}
const char *acid_preset_name(int n)
{
    static const char *const names[8]={"FOUNDATION","WAREHOUSE","LIQUID CHROME","BROKEN CIRCUIT","DUB CORROSION","POLYMETRIC","SLOW BURN","INIT LIVE"};
    return names[(n<0?0:n)%8];
}
void acid_preset(Acid *a,int n)
{
    int i,j;static const int8_t notes[16]={0,0,12,7,0,3,0,10,12,0,7,15,0,10,7,3};
    acid_stop(a);a->held_count=0;acid_zero(a->held,0,sizeof a->held);
    n=(n<0?0:n)%8;acid_zero(&a->project,0,sizeof a->project);
    a->project.magic=ACID_MAGIC;a->project.version=ACID_VERSION;
    for(i=0;i<P_COUNT;i++)a->project.p[i]=acid_params[i].initial;
    for(i=0;i<3;i++)for(j=0;j<8;j++)a->project.curve[i][j]=(float)((j*5+i*3)%8)/3.5f-1;
    acid_generate(a,0xac1d303u+(uint32_t)n*491u,.72f);
    for(i=0;i<16;i++){
        a->project.step[i].note=notes[(i+n*2)%16];a->project.step[i].gate=(uint8_t)(i%7!=6);
        a->project.step[i].accent=(uint8_t)(i==0||i==5||i==11);
        a->project.step[i].slide=(uint8_t)(i==2||i==7||i==14);
    }
    if(n==1){acid_set(a,P_TEMPO,142);acid_set(a,P_DRIVE,.72f);acid_set(a,P_DIST_MIX,.6f);acid_set(a,P_CUTOFF,.25f);acid_set(a,P_RESONANCE,.84f);}
    if(n==2){acid_set(a,P_WAVE,1);acid_set(a,P_CHORUS,.3f);acid_set(a,P_MOD1_DEPTH,.3f);acid_set(a,P_MOD1_TARGET,3);acid_set(a,P_DELAY,.28f);}
    if(n==3){acid_set(a,P_DIST,3);acid_set(a,P_DIST_MIX,.6f);acid_set(a,P_DRIVE,.65f);acid_set(a,P_SWING,.25f);for(i=0;i<16;i+=5)a->project.step[i].ratchet=2;}
    if(n==4){acid_set(a,P_TEMPO,112);acid_set(a,P_DELAY,.38f);acid_set(a,P_FEEDBACK,.65f);acid_set(a,P_REVERB,.4f);acid_set(a,P_DECAY,.65f);}
    if(n==5){acid_set(a,P_ACC_LENGTH,11);acid_set(a,P_SLIDE_LENGTH,7);acid_set(a,P_MOD1_TARGET,1);acid_set(a,P_MOD1_DEPTH,.25f);acid_set(a,P_MOD2_TARGET,2);acid_set(a,P_MOD2_DEPTH,.15f);acid_set(a,P_MOD3_TARGET,3);acid_set(a,P_MOD3_DEPTH,.2f);}
    if(n==6){acid_set(a,P_TEMPO,86);acid_set(a,P_DECAY,.8f);acid_set(a,P_REVERB,.35f);acid_set(a,P_SUB,.6f);acid_set(a,P_CUTOFF,.2f);}
    if(n==7){acid_set(a,P_MODE,1);acid_set(a,P_ENV,.4f);acid_set(a,P_DIST_MIX,0);acid_set(a,P_DELAY,0);acid_set(a,P_REVERB,0);}
    a->cutoff_lock=-1;a->position=-1;
}
void acid_init(Acid *a,float fs)
{
    acid_zero(a,0,sizeof *a);a->fs=bound(fs,32000,96000);a->rng=0x303u;
    a->estimated_step=a->fs*15/130;a->delay_smooth=a->fs*.23f;
    a->note=36;a->pitch=a->target_pitch=36;a->cutoff_lock=-1;
    acid_preset(a,0);
}
