/* SPDX-License-Identifier: AGPL-3.0-only
 * ACID FM1: original application on the unmodified MPL-2.0 fm1-base platform. */
static Acid synth PLAT_POOL;
static AcidProject disk_project;
static struct {
    uint32_t keys,buttons,last_draw;
    int page,cursor,preset,octave,slot,dirty,redraw,recording,home_used;
    char message[24]; uint32_t message_until;
} acid_ui;
static const char *const page_names[]={"VOICE","ENVELOPE","EFFECTS","MOD 1","MOD 2","MOD 3","SEQUENCER","CIRCUIT","CLOCK","DISTORTION","PATTERN","FX SPACE","MOD 1 ROUTE","MOD 2 ROUTE","MOD 3 ROUTE","UTILITY"};
#define NPAGES 16
static const int page_params[NPAGES][4]={
    {P_CUTOFF,P_RESONANCE,P_ENV,P_DRIVE},
    {P_DECAY,P_ACCENT,P_SLIDE,P_GATE},
    {P_DELAY,P_FEEDBACK,P_REVERB,P_CHORUS},
    {P_MOD1_RATE,P_MOD1_DEPTH,P_MOD1_SHAPE,P_MOD1_LENGTH},
    {P_MOD2_RATE,P_MOD2_DEPTH,P_MOD2_SHAPE,P_MOD2_LENGTH},
    {P_MOD3_RATE,P_MOD3_DEPTH,P_MOD3_SHAPE,P_MOD3_LENGTH},
    {-1,-2,-3,-4},
    {P_PULSE,P_SUB,P_BASS,P_NOISE},
    {P_TEMPO,P_SWING,P_CLOCK,P_MODE},
    {P_DIST,P_DIST_MIX,P_CLIP,P_VOLUME},
    {P_LENGTH,P_DIRECTION,P_ACC_LENGTH,P_SLIDE_LENGTH},
    {P_DELAY_TIME,P_VIB_RATE,P_VIB_DEPTH,P_TRACKING},
    {P_MOD1_TARGET,P_MOD1_RATE,P_MOD1_DEPTH,P_MOD1_LENGTH},
    {P_MOD2_TARGET,P_MOD2_RATE,P_MOD2_DEPTH,P_MOD2_LENGTH},
    {P_MOD3_TARGET,P_MOD3_RATE,P_MOD3_DEPTH,P_MOD3_LENGTH},
    {P_ROOT,P_SCALE,P_ACC_ATTACK,P_MUTATION}
};
static void app_number(int v,char *s)
{
    char tmp[12];int i=0;unsigned u=(unsigned)(v<0?-v:v);
    if(v<0)*s++='-';
    do{tmp[i++]=(char)('0'+u%10);u/=10;}while(u);
    while(i)*s++=tmp[--i];
    *s=0;
}
void app_say(const char *a,const char *b)
{
    unsigned i=0,j=0;while(a&&a[j]&&i<22)acid_ui.message[i++]=a[j++];
    if(b&&*b&&i<22)acid_ui.message[i++]=' ';
    j=0;while(b&&b[j]&&i<23)acid_ui.message[i++]=b[j++];acid_ui.message[i]=0;
    acid_ui.message_until=plat_ms()+1400;acid_ui.redraw=1;
    if(ann_on())ann_say(acid_ui.message,0,ANN_NOW);
}
void app_init(void)
{
    memset(&acid_ui,0,sizeof acid_ui);
    acid_init(&synth,44100.0f);acid_ui.octave=-1;acid_ui.redraw=1;
    if(plat_store_load(OBJ_PATCH0,&disk_project,sizeof disk_project)==(int)sizeof disk_project)
        acid_load(&synth,&disk_project);
}
void app_render(int32_t *lr,uint32_t n,uint32_t master)
{
    float tmp[256],gain=(float)master*(8380000.0f/4096.0f);uint32_t i,offset=0,pkt;int port;
    while(plat_midi_in(&pkt,&port))acid_midi(&synth,(uint8_t)(pkt>>8),(uint8_t)(pkt>>16),(uint8_t)(pkt>>24));
    while(offset<n){uint32_t block=n-offset;if(block>128)block=128;
        acid_process(&synth,tmp,block);
        for(i=0;i<block*2;i++)lr[offset*2+i]=(int32_t)(tmp[i]*gain);
        offset+=block;
    }
}
void app_panic(void){acid_stop(&synth);acid_panic(&synth);}
static void change_param(int id,int detents)
{
    float v,step;uint32_t lock;
    if(id<0)return;
    step=acid_params[id].discrete?1.0f:(acid_params[id].hi-acid_params[id].lo)/100.0f;
    if(id==P_TEMPO)step=1;
    v=synth.project.p[id]+step*(float)detents;
    lock=plat_lock();acid_set(&synth,id,v);plat_unlock(lock);acid_ui.dirty=1;acid_ui.redraw=1;
}
static void save_project(void)
{
    uint32_t lock=plat_lock();memcpy(&disk_project,&synth.project,sizeof disk_project);plat_unlock(lock);
    if(plat_store_save((uint32_t)(OBJ_PATCH0+acid_ui.slot),&disk_project,sizeof disk_project)==0){acid_ui.dirty=0;app_say("SAVED SLOT","");}
    else app_say("SAVE FAILED","");
}
static void load_project(void)
{
    int size=plat_store_load((uint32_t)(OBJ_PATCH0+acid_ui.slot),&disk_project,sizeof disk_project);
    if(size==(int)sizeof disk_project && acid_project_valid(&disk_project)){
        uint32_t lock=plat_lock();acid_load(&synth,&disk_project);plat_unlock(lock);acid_ui.dirty=0;app_say("LOADED SLOT","");
    }else app_say("SLOT EMPTY","");
}
static void panel_input(void)
{
    uint32_t keys=plat_keys(),buttons=plat_buttons(),rise=buttons&~acid_ui.buttons,lock;int i,d;
    int shift=(buttons>>B_HOME)&1u;
    if(rise&(1u<<B_HOME))acid_ui.home_used=0;
    if(shift&&(rise&~(1u<<B_HOME)))acid_ui.home_used=1;
    if((acid_ui.buttons&(1u<<B_HOME))&&!shift&&!acid_ui.home_used)acid_ui.page=0;
    if(rise&(1u<<B_PLAY)){lock=plat_lock();if(shift)app_panic();else if(synth.playing)acid_stop(&synth);else acid_start(&synth);plat_unlock(lock);}
    if(rise&(1u<<B_ENV))acid_ui.page=1;
    if(rise&(1u<<B_FX))acid_ui.page=acid_ui.page==2?9:acid_ui.page==9?11:2;
    if(rise&(1u<<B_LFO))acid_ui.page=acid_ui.page>=3&&acid_ui.page<5?acid_ui.page+1:3;
    if(rise&(1u<<B_SEQ))acid_ui.page=acid_ui.page==6?10:6;
    if(rise&(1u<<B_GLO))acid_ui.page=acid_ui.page==8?15:8;
    if(rise&(1u<<B_EDIT))acid_ui.page=acid_ui.page==6?0:6;
    if(rise&(1u<<B_SEL))change_param(P_SCALE,synth.project.p[P_SCALE]>=4?-4:1);
    if(rise&(1u<<B_ARP)){lock=plat_lock();acid_set(&synth,P_MODE,synth.project.p[P_MODE]==2?0:2);plat_unlock(lock);}
    if(rise&(1u<<B_REC)){acid_ui.recording=!acid_ui.recording;app_say(acid_ui.recording?"STEP RECORD ON":"STEP RECORD OFF","");}
    if(rise&(1u<<B_OCTDN))acid_ui.octave=clamp(acid_ui.octave-1,-3,2);
    if(rise&(1u<<B_OCTUP))acid_ui.octave=clamp(acid_ui.octave+1,-3,2);
    if(rise&(1u<<B_SAVE)){if(shift)load_project();else save_project();}
    d=plat_enc(EN_SELECT);
    if(d){if(shift)acid_ui.home_used=1;if(acid_ui.page==6)acid_ui.cursor=clamp(acid_ui.cursor+d,0,63);else acid_ui.page=(acid_ui.page+d%NPAGES+NPAGES)%NPAGES;acid_ui.redraw=1;}
    d=plat_enc(EN_ALGO);
    if(d){if(shift){acid_ui.home_used=1;acid_ui.slot=clamp(acid_ui.slot+d,0,3);}else if(acid_ui.page>=3&&acid_ui.page<=5)change_param(P_MOD1_TARGET+(acid_ui.page-3)*5,d);else change_param(P_WAVE,d>0?100:-100);}
    d=plat_enc(EN_PRESET);
    if(d){if(shift)acid_ui.home_used=1;acid_ui.preset=(acid_ui.preset+d%8+8)%8;lock=plat_lock();acid_preset(&synth,acid_ui.preset);plat_unlock(lock);acid_ui.dirty=1;app_say(acid_preset_name(acid_ui.preset),"");}
    for(i=0;i<4;i++)if((d=plat_enc(EN_K1+i))!=0){
        if(shift)acid_ui.home_used=1;
        if(acid_ui.page!=6)change_param(page_params[acid_ui.page][i],d);
        else {
            AcidStep *s=&synth.project.step[acid_ui.cursor];lock=plat_lock();
            if(shift){
                if(i==0)acid_step_set(&synth,acid_ui.cursor,4,(float)s->probability+(float)d*5);
                if(i==1)acid_step_set(&synth,acid_ui.cursor,5,(float)s->ratchet+(float)d);
                if(i==2)acid_step_set(&synth,acid_ui.cursor,6,s->cutoff+(float)d*.02f);
                if(i==3)acid_step_set(&synth,acid_ui.cursor,6,-1);
            }else{
                if(i==0)acid_step_set(&synth,acid_ui.cursor,0,(float)s->note+(float)d);
                if(i>0)acid_step_set(&synth,acid_ui.cursor,i,d>0?1:0);
            }plat_unlock(lock);acid_ui.dirty=1;
        }
    }
    for(i=0;i<NKEYS;i++){
        uint32_t bit=1u<<i;int note=53+i+12*acid_ui.octave;
        if((keys&bit)&&!(acid_ui.keys&bit)){
            lock=plat_lock();
            if(acid_ui.recording){acid_step_set(&synth,acid_ui.cursor,0,(float)(note-(int)synth.project.p[P_ROOT]));acid_step_set(&synth,acid_ui.cursor,1,1);acid_ui.cursor=(acid_ui.cursor+1)%(int)synth.project.p[P_LENGTH];acid_ui.dirty=1;}
            acid_note_on(&synth,note,100);plat_unlock(lock);
        }else if(!(keys&bit)&&(acid_ui.keys&bit)){
            /* key releases must match the octave at note-on, handled below by
             * resetting held notes when changing octaves. */
            lock=plat_lock();acid_note_off(&synth,note);plat_unlock(lock);
        }
    }
    if(rise&((1u<<B_OCTDN)|(1u<<B_OCTUP))){lock=plat_lock();acid_midi(&synth,0xb0,123,0);plat_unlock(lock);}
    if(keys!=acid_ui.keys||buttons!=acid_ui.buttons||rise)acid_ui.redraw=1;
    acid_ui.keys=keys;acid_ui.buttons=buttons;
    plat_glow(1);
    plat_leds((synth.playing?1u<<B_PLAY:0)|(acid_ui.dirty?1u<<B_SAVE:0)|(acid_ui.recording?1u<<B_REC:0),keys);
}
void app_input_only(void){panel_input();}
void app_redraw(void){acid_ui.redraw=1;}
static void draw_panel(void)
{
    char text[24];int i;uint16_t lime=RGB(185,241,90);
    cv_begin(240,240,C_BLACK);
    cv_text(9,5,&FONT_B,"ACID FM1",lime);
    cv_text(9,30,&FONT_XS,page_names[acid_ui.page],C_WHITE);
    app_number((int)synth.project.p[P_TEMPO],text);cv_text(178,7,&FONT_B,text,C_WHITE);
    cv_text(178,28,&FONT_XS,synth.playing?"PLAY":"STOP",synth.playing?lime:C_GRAY);
    if(acid_ui.page==6){
        AcidStep *s=&synth.project.step[acid_ui.cursor];
        const char *names[4]={"NOTE","GATE","ACCENT","SLIDE"};
        int val[4]={s->note,s->gate,s->accent,s->slide};
        for(i=0;i<4;i++){int x=9+(i%2)*116,y=60+(i/2)*43;cv_text(x,y,&FONT_XS,names[i],C_GRAY);app_number(val[i],text);cv_text(x,y+15,&FONT_B,text,lime);}
        cv_text(9,147,&FONT_XS,"HOME: PROB / RATCH / LOCK",C_GRAY);
    }else{
        for(i=0;i<4;i++){
            int id=page_params[acid_ui.page][i],x=9+(i%2)*116,y=60+(i/2)*44;
            float val=synth.project.p[id];
            cv_text(x,y,&FONT_XS,acid_params[id].name,C_GRAY);
            app_number((int)(acid_params[id].discrete||id==P_TEMPO?val:val*100),text);
            cv_text(x,y+15,&FONT_B,text,lime);
        }
    }
    for(i=0;i<16;i++){
        int index=(acid_ui.cursor/16)*16+i,x=8+i*14,active=index==synth.position;
        cv_rect(x,174,11,18,active?lime:synth.project.step[index].gate?C_LINE:C_DIM);
        if(synth.project.step[index].accent)cv_rect(x+3,177,5,3,C_WHITE);
        if(synth.project.step[index].slide)cv_rect(x+2,187,7,2,C_GRAY);
        if(acid_ui.page==6&&index==acid_ui.cursor)cv_rect(x,195,11,2,C_WHITE);
    }
    if(plat_ms()<acid_ui.message_until)cv_text(8,208,&FONT_XS,acid_ui.message,lime);
    else {
        cv_text(8,207,&FONT_XS,acid_preset_name(acid_ui.preset),C_WHITE);
        app_number(acid_ui.slot+1,text);cv_text(190,207,&FONT_XS,"SLOT",C_GRAY);cv_text(219,207,&FONT_XS,text,lime);
    }
    if(synth.external_lost)cv_text(8,224,&FONT_XS,"WAITING FOR MIDI CLOCK",C_WHITE);
    else {app_number((int)plat_cpu_pct(),text);cv_text(8,224,&FONT_XS,"CPU",C_GRAY);cv_text(36,224,&FONT_XS,text,C_WHITE);cv_text(92,224,&FONT_XS,"SELECT: PAGE",C_GRAY);}
    cv_blit(0,0);
}
void app_ui_init(void){lcd_fill(0,0,240,240,C_BLACK);acid_ui.redraw=1;}
void app_frame(void)
{
    uint32_t now=plat_ms();panel_input();ann_tick(now);
    if(ann_on()&&ann_where())ann_say(page_names[acid_ui.page],0,ANN_NOW);
    if(acid_ui.redraw||now-acid_ui.last_draw>=100){acid_ui.redraw=0;acid_ui.last_draw=now;draw_panel();}
}

#ifdef PLAT_HOST
int app_expect(const char *what,int *got)
{
    if(!strcmp(what,"playing"))*got=synth.playing;
    else if(!strcmp(what,"page"))*got=acid_ui.page;
    else if(!strcmp(what,"cursor"))*got=acid_ui.cursor;
    else if(!strcmp(what,"dirty"))*got=acid_ui.dirty;
    else if(!strcmp(what,"mode"))*got=(int)synth.project.p[P_MODE];
    else if(!strcmp(what,"note"))*got=synth.note;
    else if(!strcmp(what,"gate"))*got=synth.gate;
    else if(!strcmp(what,"slot"))*got=acid_ui.slot;
    else if(!strcmp(what,"root"))*got=(int)synth.project.p[P_ROOT];
    else return 0;
    return 1;
}

#endif
