/* SPDX-License-Identifier: MIT */
#include "acid.h"
#include <emscripten/emscripten.h>
static Acid engine;
static float output[2048];
#define API EMSCRIPTEN_KEEPALIVE
API void synth_init(float fs){acid_init(&engine,fs);}
API void synth_set(int id,float v){acid_set(&engine,id,v);}
API float synth_get(int id){return id>=0&&id<P_COUNT?engine.project.p[id]:0;}
API void synth_step(int s,int f,float v){acid_step_set(&engine,s,f,v);}
API float synth_step_get(int s,int f){
    AcidStep *p;if(s<0||s>=64)return 0;p=&engine.project.step[s];
    switch(f){case 0:return p->note;case 1:return p->gate;case 2:return p->accent;case 3:return p->slide;case 4:return p->probability;case 5:return p->ratchet;case 6:return p->cutoff;default:return 0;}
}
API void synth_curve(int m,int i,float v){acid_curve_set(&engine,m,i,v);}
API float synth_curve_get(int m,int i){return m>=0&&m<3&&i>=0&&i<8?engine.project.curve[m][i]:0;}
API void synth_play(int on){if(on)acid_start(&engine);else acid_stop(&engine);}
API void synth_panic(void){acid_stop(&engine);acid_panic(&engine);}
API void synth_preset(int p){acid_preset(&engine,p);}
API void synth_generate(int seed,float density){acid_generate(&engine,(uint32_t)seed,density);}
API void synth_mutate(int seed,float amount){acid_mutate(&engine,(uint32_t)seed,amount);}
API void synth_midi(int st,int d1,int d2){acid_midi(&engine,(uint8_t)st,(uint8_t)d1,(uint8_t)d2);}
API float *synth_render(int n){if(n<0)n=0;if(n>1024)n=1024;acid_process(&engine,output,(uint32_t)n);return output;}
API float synth_status(int id){switch(id){case 0:return (float)engine.position;case 1:return (float)engine.playing;case 2:return engine.peak;case 3:return (float)engine.external_lost;case 4:return engine.env;case 5:return engine.mod_value[0];case 6:return engine.mod_value[1];case 7:return engine.mod_value[2];default:return 0;}}
API int synth_param_count(void){return P_COUNT;}
API int synth_project_size(void){return (int)sizeof(AcidProject);}
API uint32_t synth_seed(void){return engine.project.seed;}
API void synth_seed_set(uint32_t seed){engine.project.seed=seed?seed:1;engine.rng=engine.project.seed;}
