/* SPDX-License-Identifier: AGPL-3.0-only */
#include "acid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
static Acid a,b;
static float audio[2048],other[2048];
static int checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
static void render(Acid *s,int frames){while(frames){int n=frames>1024?1024:frames;acid_process(s,audio,(uint32_t)n);frames-=n;}}
static void safety(void){int i;for(i=0;i<2048;i++)CHECK(isfinite(audio[i])&&fabsf(audio[i])<=1.0f);}
static void nofx(Acid *s){acid_set(s,P_DELAY,0);acid_set(s,P_REVERB,0);acid_set(s,P_CHORUS,0);acid_set(s,P_DIST_MIX,0);}
int main(void)
{
    int i,j,n;float v;AcidProject saved;
    CHECK(sizeof(AcidProject)<=3840);CHECK(sizeof(Acid)<200000);
    acid_init(&a,44100);CHECK(acid_project_valid(&a.project));
    acid_process(&a,audio,1024);for(i=0;i<2048;i++)CHECK(audio[i]==0);
    for(i=0;i<7;i++){
        acid_init(&a,44100);acid_set(&a,P_DIST,(float)i);acid_set(&a,P_DIST_MIX,1);
        acid_process(&a,audio,1024);for(j=0;j<2048;j++)CHECK(audio[j]==0);
    }
    v=a.project.p[P_CUTOFF];acid_set(&a,P_CUTOFF,NAN);CHECK(a.project.p[P_CUTOFF]==v);
    acid_set(&a,P_LENGTH,-99);CHECK(a.project.p[P_LENGTH]==1);acid_set(&a,P_LENGTH,900);CHECK(a.project.p[P_LENGTH]==64);
    acid_set(&a,P_LENGTH,3.6f);CHECK(a.project.p[P_LENGTH]==4);
    for(i=0;i<8;i++){
        acid_init(&a,44100);acid_preset(&a,i);CHECK(acid_project_valid(&a.project));
        if(i==7)acid_note_on(&a,57,110);else acid_start(&a);
        render(&a,44100);safety();CHECK(a.peak>.005f);
    }
    acid_init(&a,44100);acid_init(&b,44100);acid_start(&a);acid_start(&b);
    for(i=0;i<80;i++){acid_process(&a,audio,1024);acid_process(&b,other,1024);CHECK(!memcmp(audio,other,sizeof audio));}
    acid_init(&a,48000);acid_set(&a,P_TEMPO,120);acid_start(&a);render(&a,48000);CHECK(a.step_serial==8);
    /* Block size cannot change sequencing or audio. */
    acid_init(&a,44100);acid_init(&b,44100);acid_start(&a);acid_start(&b);
    acid_process(&a,audio,1024);for(i=0;i<8;i++)acid_process(&b,other+i*256,128);CHECK(!memcmp(audio,other,sizeof audio));
    acid_init(&a,48000);acid_set(&a,P_TEMPO,120);acid_set(&a,P_SWING,.5f);acid_start(&a);
    render(&a,9000);CHECK(a.step_serial==1);render(&a,3000);CHECK(a.step_serial==2);
    acid_init(&a,44100);nofx(&a);for(i=0;i<64;i++)a.project.step[i].probability=0;
    acid_start(&a);render(&a,44100);CHECK(a.peak==0);
    acid_init(&a,44100);acid_set(&a,P_MODE,1);acid_note_on(&a,50,110);acid_note_on(&a,60,80);
    CHECK(a.note==60&&a.held_count==2);acid_note_off(&a,60);CHECK(a.note==50&&a.gate);acid_note_off(&a,50);CHECK(!a.gate&&!a.held_count);
    acid_init(&a,44100);acid_set(&a,P_MODE,2);acid_note_on(&a,48,100);acid_note_on(&a,55,100);acid_start(&a);render(&a,1);CHECK(a.note==48);
    render(&a,5100);CHECK(a.note==55);acid_note_off(&a,48);acid_note_off(&a,55);render(&a,5300);CHECK(!a.gate);
    acid_init(&a,44100);acid_set(&a,P_CLOCK,1);acid_midi(&a,0xfa,0,0);render(&a,1000);CHECK(a.position==-1);
    acid_midi(&a,0xf8,0,0);CHECK(a.position==0);for(i=0;i<6;i++){render(&a,849);acid_midi(&a,0xf8,0,0);}CHECK(a.position==1);
    render(&a,88201);CHECK(a.external_lost&&!a.gate);acid_midi(&a,0xfc,0,0);CHECK(!a.playing);
    acid_init(&a,44100);acid_set(&a,P_LENGTH,1);a.project.step[0].ratchet=4;a.project.step[0].slide=0;
    acid_start(&a);render(&a,4500);CHECK(a.ratchet_index==3);CHECK(a.gate);
    acid_init(&a,44100);saved=a.project;acid_mutate(&a,123,0);CHECK(!memcmp(&saved,&a.project,sizeof saved));
    acid_generate(&a,123,.6f);saved=a.project;acid_generate(&a,123,.6f);CHECK(!memcmp(&saved,&a.project,sizeof saved));
    saved.p[P_LENGTH]=NAN;CHECK(!acid_load(&a,&saved));saved=a.project;saved.step[2].ratchet=0;CHECK(!acid_load(&a,&saved));
    for(i=0;i<5;i++)for(j=-24;j<36;j++){n=acid_quantize(j,i);CHECK(n<=j&&n>=j-3);}
    /* Adversarial valid parameter combinations, all shapes and distortion types. */
    for(j=0;j<80;j++){
        acid_init(&a,j%2?48000:44100);acid_start(&a);
        for(i=0;i<P_COUNT;i++){
            const AcidParamInfo *p=&acid_params[i];float t=(float)((i*19+j*31)%101)/100;
            acid_set(&a,i,p->lo+(p->hi-p->lo)*t);
        }
        acid_set(&a,P_MODE,0);acid_set(&a,P_CLOCK,0);acid_start(&a);render(&a,8192);safety();
        CHECK(acid_project_valid(&a.project));
    }
    acid_panic(&a);render(&a,2048);CHECK(a.peak==0);
    printf("PASS %d checks; project=%zu bytes, engine=%zu bytes\n",checks,sizeof(AcidProject),sizeof(Acid));
    /* A deterministic stereo demo for listening and automated spectrum checks. */
    {FILE *f=fopen("build/demo.f32","wb");CHECK(f!=NULL);acid_init(&a,44100);acid_start(&a);
     for(i=0;i<6*44100;i+=1024){int count=6*44100-i;if(count>1024)count=1024;acid_process(&a,audio,(uint32_t)count);fwrite(audio,sizeof(float),count*2,f);}fclose(f);}
    return 0;
}
