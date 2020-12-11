#include "user_fun_cfg.h"
#if (!TCFG_UI_LED7_ENABLE)
#define led7_clear_all_icon()   
#endif
#if USER_VBAT_CHECK_EN
USER_POWER_INFO user_power_io={
    .pro = IO_PORTB_06,
    .ch  = AD_CH_PB6,
    .vol = 0,
};
#endif


void user_udelay_init(void){
    bit_clr_ie(USER_UDELAY_TIMER_IRQ);
}

__attribute__((weak)) void user_udelay(u32 usec)
{
    USER_UDELAY_TIMER->CON = BIT(14);//0x4000;
    USER_UDELAY_TIMER->CNT = 0;
    USER_UDELAY_TIMER->PRD = (24/4)  * usec;//延时时间=时钟源频率/分频系数*计数次数
    USER_UDELAY_TIMER->CON = BIT(0)|BIT(3)|BIT(4);//BIT(0)定时计数模式 BIT(3):晶振为时钟源 BIT(4):4分频
    while ((USER_UDELAY_TIMER->CON & BIT(15)) == 0);
    USER_UDELAY_TIMER->CON = BIT(14); 
}

__attribute__((weak)) void user_attr_gpio_set(u32 gpio,u8 cmd,int delay_cnt){
    JL_PORT_FLASH_TypeDef *p =NULL;
    u32 por = NO_CONFIG_PORT;

    if(gpio<=IO_PORTA_15){
        p = JL_PORTA;
        por = gpio-IO_PORTA_00;
    }else if(gpio<=IO_PORTB_15){
        p = JL_PORTB;
        por = gpio-IO_PORTB_00;
    }else if(gpio<=IO_PORTC_15){
        p = JL_PORTC;
        por = gpio-IO_PORTC_00;
    }else if(gpio<=IO_PORTD_07){
        p = JL_PORTD;
        por = gpio-IO_PORTD_00;
    }

    if(!p || NO_CONFIG_PORT==por){
        return;
    }

    switch(cmd){
    case USER_GPIO_HI:
    case USER_GPIO_IN_IO:
        p->DIR  |=   BIT(por);
        p->PD   &=~  BIT(por);
        p->PU   &=~  BIT(por);
        p->DIE  |=   BIT(por);
        break;
    case USER_GPIO_IN_AD:
        p->DIR  |=   BIT(por);
        p->PD   &=~  BIT(por);
        p->PU   &=~  BIT(por);
        p->DIE  &=~  BIT(por); 
        break;
    case USER_GPIO_OUT_H:
        p->DIR  &=~  BIT(por);
        p->PD   &=~  BIT(por);
        p->PU   &=~  BIT(por);
        p->DIE  |=   BIT(por);
        p->OUT  |=   BIT(por);
        break;
    case USER_GPIO_OUT_L:
        p->DIR  &=~  BIT(por);
        p->PD   &=~  BIT(por);
        p->PU   &=~  BIT(por);
        p->DIE  |=   BIT(por);
        p->OUT  &=~  BIT(por);
        break;
    case USER_GPIO_IN_IO_PU:
        p->DIR  |=   BIT(por);
        p->PD   &=~  BIT(por);
        p->PU   |=   BIT(por);
        p->DIE  |=   BIT(por);
        break;
    case USER_GPIO_IN_IO_PD:
        p->DIR  |=   BIT(por);
        p->PD   |=   BIT(por);
        p->PU   &=~  BIT(por);
        p->DIE  |=   BIT(por);
        break;
    case USER_GPIO_IN_AD_PU:
        p->DIR  |=   BIT(por);
        p->PD   &=~  BIT(por);
        p->PU   |=   BIT(por);
        p->DIE  &=~  BIT(por);
        break;
    case USER_GPIO_IN_AD_PD:
        p->DIR  |=   BIT(por);
        p->PD   |=   BIT(por);
        p->PU   &=~  BIT(por);
        p->DIE  &=~  BIT(por);
        break;
    default:
        break;
    }

    if(delay_cnt){
        delay(delay_cnt);
    }
}
void user_message_filtering(int key_event){   
    switch(key_event){
        case KEY_VOL_DOWN:
        case KEY_VOL_UP:
            user_key_set_sys_vol_flag(1);
        case KEY_ENC_START:
        case KEY_MUSIC_PP:
        case KEY_MUSIC_PREV:
        case KEY_MUSIC_NEXT:
        #if (defined(USER_IR_MUTE_INTERRUPT_EN) && USER_IR_MUTE_INTERRUPT_EN) 
            user_pa_ex_manual(0);
        #endif
    default:
        break;
    }
}


//设置低电图标 显示、取消；获取低电图标显示状态
//cmd 1:常亮 2：闪烁 0：熄灭
u8 user_low_power_show(u8 cmd){
    static u8 low_power_icon = 0;

    //获取状态 开机过滤时间 关机状态返回
    if(0xff == cmd || timer_get_sec()<=5 || 0x55 == low_power_icon || low_power_icon == cmd){
        return low_power_icon;
    }

    //关机设置
    if(0x55 == low_power_icon || 0x55 == cmd){
        low_power_icon = cmd;
        // led7_clear_icon(LED7_CHARGE);
        led7_clear_all_icon();
        return 0;
    }


    if(0 == cmd || 1 == cmd || 2 == cmd){
        low_power_icon = cmd;
        printf(">>>>>     low power icon %d\n",low_power_icon);
    }

    return low_power_icon;//?1:0;
}

void user_led7_flash_lowpower(void){
    u8 ret = user_low_power_show(0xff);
    if(2 == ret){
        led7_flash_icon(LED7_CHARGE);
    }else if(1 == ret){
        led7_show_icon(LED7_CHARGE);
    }else if(0 == ret){
        led7_clear_icon(LED7_CHARGE);
    }else if(0x55 == ret){
        led7_clear_icon(LED7_CHARGE);
        led7_clear_all_icon();
    }
}

//设置本地music音量 并同步到对箱
void user_set_and_sync_sys_vol(u8 vol){
    
    extern bool user_linein_set_vol(u8 vol);
    if(user_linein_set_vol(vol)/*linein*/ == false){
        //其他模式
        app_audio_set_volume(APP_AUDIO_STATE_MUSIC, vol, 1);
    }    
    user_key_set_sys_vol_flag(2);
    if(tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED){
        extern int user_get_tws_state(void);
        if(1 == user_get_tws_state()){
            bt_tws_sync_volume();
        }
    }
    
}
//低电降音量
void user_power_low_dow_sys_vol(u8 vol){
    // return;
    #if (defined(USER_POWER_LOW_DOW_VOL_EN) && USER_POWER_LOW_DOW_VOL_EN)
    puts(">>>>>>>>>>>>> low dow sys vol\n");
    if(tone_get_status()){
        return;
    }

    if(vol < app_audio_get_volume(APP_AUDIO_STATE_MUSIC)){
        user_set_and_sync_sys_vol(vol);
    }
    printf(">>>>>>>>>>>>> low sys vol %d\n",app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
    #endif
}

void user_dow_sys_vol_20(void){

    if(tone_get_status()){
        sys_hi_timeout_add(NULL,user_dow_sys_vol_20,200);
        return;
    }

    user_power_low_dow_sys_vol(20);
}

void user_dow_sys_vol_10(void){

    if(tone_get_status()){
        sys_hi_timeout_add(NULL,user_dow_sys_vol_10,200);
        return;
    }

    user_power_low_dow_sys_vol(10);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws tws info同步
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
#if TCFG_USER_TWS_ENABLE
static void bt_tws_user_info_sync(void *_data, u16 len, bool rx)
{
    if (rx/*&& len==2*/) {
        u8 *data = (u8 *)_data;

        switch(data[0]){
        //SYNC LED
        case USER_TWS_SYNC_LED:
            user_led_io_fun(USER_IO_LED,data[1]);
            break;
        //SYNC RGB
        case USER_TWS_SYNC_RGB:
            user_led_io_fun(USER_IO_LED,data[1]);
            break;
        //SYNC EQ MODE
        case USER_TWS_SYNC_EQ_MODE:
            user_eq_mode_sw(data[1]);
            break;
        //DOW VOL 10
        case USER_TWS_SYNC_DOW_VOL_10:
            user_dow_sys_vol_10();
            break;
        //DOW VOL 20
        case USER_TWS_SYNC_DOW_VOL_20:
            user_dow_sys_vol_20();
            break;
        default:
            printf("unknow tws user sync\n");
            break;
        }

        printf(">>>> tws sync DATA0:%d DATA1:%d\n",data[0],data[1]);
    }

}

//TWS同步信息
REGISTER_TWS_FUNC_STUB(app_led_mode_sync_stub) = {
    .func_id = USER_TWS_FUNC_ID_USER_INFO_SYNC,
    .func    = bt_tws_user_info_sync,
};

void user_bt_tws_sync_msg_send(u8 sync_type,u8 value){
    u8 data[2];
    if((tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) && (sync_type < USER_TWS_SYNC_MAX)){
        data[0] = sync_type;
        data[1] = value;
        tws_api_send_data_to_sibling(data, 2, USER_TWS_FUNC_ID_USER_INFO_SYNC);
    }
}
#endif

void user_tws_sync_info(void){
    #if TCFG_USER_TWS_ENABLE
    if(APP_BT_TASK != app_get_curr_task() && (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED)){
        return;
    }

    int ret = user_led_io_fun(USER_IO_LED,LED_IO_STATUS);
    if(-1 != ret){
        // user_bt_tws_send_led_mode(0,ret);
        user_bt_tws_sync_msg_send(USER_TWS_SYNC_LED,ret);
    }

    ret = user_rgb_mode_set(USER_RGB_STATUS,NULL);
    if(ret){
        // user_bt_tws_send_led_mode(1,ret);
        user_bt_tws_sync_msg_send(USER_TWS_SYNC_RGB,ret);
    }

    #endif
}

//adkey 与 irkey复用时滤波
bool user_adkey_mult_irkey(u8 key_type){
    u32 static ir_time = 0;
    bool tp = 0;
    #if ((TCFG_ADKEY_ENABLE && TCFG_IRKEY_ENABLE) && (TCFG_IRKEY_PORT == TCFG_ADKEY_PORT))
    if(KEY_DRIVER_TYPE_IR == key_type){
        ir_time = timer_get_ms();
    }else if(KEY_DRIVER_TYPE_AD == key_type){
        if(timer_get_ms()-ir_time<200){
            // printf(">>> %d %d\n",timer_get_ms(),ir_time);
            tp = 1;
        }
    }
    #endif
    return tp;
}

//设置、获取 录音状态
u8 user_record_status(u8 cmd){
    static u8 user_record_status  = 0;
    #if USER_RECORD_EN
    if(0 == cmd || 1 == cmd){
        user_record_status = cmd;

        // user_mic_check_en(!user_record_status);
        // printf(">>>>>>>>>>>>>>>>>>>>  record status  %d\n",user_record_status);
    }
    #endif
    return user_record_status;//USER_KEY_RECORD_START
}

/*
cmd:    1、其他值
    1：已启用 其他：获取状态
ret：   1：已启用 0：为启用过
*/
u8 user_eq_init_ok(u8 cmd){
    static u8 sys_init_ok_flag = 0;

    if(1 == cmd){
        sys_init_ok_flag = cmd;
    }

    return sys_init_ok_flag;
}

//cmd 0xff:获取 其他：设置
int user_eq_mode_sw(u8 cmd){
        static int user_eq_mode = 0;
    #if USER_EQ_FILE_ADD_EQ_TABLE

        if(!user_eq_init_ok(0xff) || (cmd>EQ_MODE_COUNTRY && EQ_MODE_NEXT!=cmd) || 0xff == cmd){
            return user_eq_mode;
        }

        if(EQ_MODE_NEXT == cmd){
            user_eq_mode++;
        }else{
            user_eq_mode = cmd;
        }

        if(user_eq_mode>EQ_MODE_COUNTRY){
            user_eq_mode = EQ_MODE_NORMAL;
        }
        printf(">>>> eq mode %d\n",user_eq_mode);

        #if USER_EQ_LIVE_UPDATE
        int tp_bass = (eq_tab_custom[USER_EQ_BASS_INDEX].gain)>>20;
        int tp_terble = (eq_tab_custom[USER_EQ_TERBLE_INDEX].gain)>>20;
        #endif

        // if(user_eq_mode<EQ_MODE_CUSTOM){
        //     memcpy(eq_tab_custom,eq_type_tab[user_eq_mode],sizeof(EQ_CFG_SEG)*SECTION_MAX);
        // }else{
        //     memcpy(eq_tab_custom,user_eq_tab_custom,sizeof(EQ_CFG_SEG)*SECTION_MAX);
        // }

        memcpy(eq_tab_custom,eq_type_tab[user_eq_mode],sizeof(EQ_CFG_SEG)*SECTION_MAX);

        #if USER_EQ_LIVE_UPDATE
        eq_tab_custom[USER_EQ_BASS_INDEX].gain = tp_bass<<20;
        eq_tab_custom[USER_EQ_TERBLE_INDEX].gain = tp_terble<<20;
        #endif

        eq_mode_set(EQ_MODE_CUSTOM);
#if TCFG_UI_ENABLE
        ui_set_tmp_menu(MENU_SET_EQ, 1000, user_eq_mode, NULL);
#endif
    #else
    eq_mode_sw();
    #endif
    return user_eq_mode;
}

//低音
void user_eq_bass_terble_set(int bass,int terble){
    #if USER_EQ_LIVE_UPDATE
    eq_tab_custom[USER_EQ_BASS_INDEX].gain = bass<<20;
    eq_tab_custom[USER_EQ_TERBLE_INDEX].gain = terble<<20;

    eq_mode_set(EQ_MODE_CUSTOM);
    #endif
    return;
}

//高音
void user_bass_terble_updata(u32 bass_ad,u32 terble_ad){
    static int bass_old =0,terble_old = 0;
    static s32 bass_ad_old = 0,terble_ad_old = 0;
    s32 bass  = bass_ad,terble = terble_ad;
    bool updata_flag = 0;

    //无效值
    if(user_record_status(0xff) || !user_eq_init_ok(0xff) || \
    timer_get_sec()<6 || APP_RECORD_TASK == app_get_curr_task() || \
    app_get_curr_task() == APP_FM_TASK){
        return;
    }

    if( USER_EQ_BASS_AD_MIN > bass_ad){
        bass = USER_EQ_BASS_AD_MIN;
    }else if( USER_EQ_BASS_AD_MAX < bass_ad){
        bass = USER_EQ_BASS_AD_MAX;
    }

    if(USER_EQ_TERBLE_AD_MIN > terble_ad){
        terble = USER_EQ_TERBLE_AD_MIN;
    }else if( USER_EQ_TERBLE_AD_MAX < terble_ad){
        terble = USER_EQ_TERBLE_AD_MAX;
    }

    // r_printf("         bass=%d  %d  terble=%d %d",bass,bass_ad,terble,terble_ad);
    //ad 滤抖
    if((USER_EQ_BASS_AD_MIN != bass_ad_old) && (USER_EQ_BASS_AD_MIN == bass)){
        r_printf("bass=%d terble=%d 1",bass,terble);
        bass_ad_old = bass;
        updata_flag = 1;
    }else if((USER_EQ_BASS_AD_MAX != bass_ad_old) && (USER_EQ_BASS_AD_MAX == bass)){
        r_printf("2");
        bass_ad_old = bass;
        updata_flag = 1;
    }else if(DIFFERENCE(bass_ad_old,bass)>8){
        r_printf("3");
        bass_ad_old = bass;
        updata_flag = 1;
    }else if((USER_EQ_TERBLE_AD_MIN != terble_ad_old) && (USER_EQ_TERBLE_AD_MIN == terble)){
        r_printf(">>>>>>4");
        terble_ad_old = terble;
        updata_flag = 1;
    }else if((USER_EQ_TERBLE_AD_MAX != terble_ad_old) && (USER_EQ_TERBLE_AD_MAX == terble)){
        r_printf(">>>>>>>>>>>5");
        terble_ad_old = terble;
        updata_flag = 1;
    }else if(DIFFERENCE(terble_ad_old,terble)>5){
        r_printf(">>>>>>>>>>>>>>>>>>>>>>6");
        terble_ad_old = terble;
        updata_flag = 1;
    }else{
        // r_printf("7");
        return;
    }

    bass = ((bass-USER_EQ_BASS_AD_MIN)*(USER_EQ_BASS_GAIN_MAX-USER_EQ_BASS_GAIN_MIN))/(USER_EQ_BASS_AD_MAX-USER_EQ_BASS_AD_MIN);
    terble = ((terble-USER_EQ_TERBLE_AD_MIN)*(USER_EQ_TERBLE_GAIN_MAX-USER_EQ_TERBLE_GAIN_MIN))/(USER_EQ_TERBLE_AD_MAX-USER_EQ_TERBLE_AD_MIN);
    bass += USER_EQ_BASS_GAIN_MIN;
    terble += USER_EQ_TERBLE_GAIN_MIN;

    if(bass_old!=bass || terble_old!=terble){
        if(bass_old!=bass){
            bass_old = bass;
            updata_flag = 1;
        }
        if(terble_old!=terble){
            terble_old = terble;
            updata_flag = 1;
        }
        


        if(updata_flag){
            printf("                                               eq bass %d terble %d\n",bass_old,terble_old);
            user_eq_bass_terble_set(bass_old,terble_old);            
        }

    }
}

//mic 开关
void user_mic_en(u8 en){
    SFR(JL_ANA->ADA_CON0,29,1,en);
}

//旋钮设置mic 音量
void user_mic_vol_update(u8 vol){
    static u8 mic_old = 0;
    
    bool mic_in_status = user_get_mic_status();
    if(mic_old != vol){
        #if USER_MIC_MUSIC_VOL_SEPARATE
        mic_effect_set_dvol(mic_old);
        #else
        printf("                  set mic vol %d\n",vol);
        audio_mic_set_gain(vol);
        if(!vol && mic_old){
            user_mic_en(0);
        }else if(vol && !mic_old){
            user_mic_en(1);
        }
        #endif
        mic_old = vol;
    }
}

u8 user_mic_ad_2_vol(u8 cmd,u32 vol_ad){
    static int mic_old =-1;

#if (defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE)
    static int vol_table[20] ={0};
    static u8 table_cnt = 0;
    static bool for_flag = 0;
    u32 average = 0;
    u32 tp_ad;

    u8 mic_vol = 0;

    //获取mic 旋钮音量
    if(1 == cmd){
        return mic_old;
    }


    //ir key mute
    if(user_pa_ex_manual(0xff)){
        tp_ad = USER_EQ_MIC_AD_MIN;
    }else{
        tp_ad = vol_ad;
    }

    //平均值
    if(DIFFERENCE(vol_table[table_cnt?table_cnt-1:table_cnt],tp_ad)>50){
        memset(vol_table,0,sizeof(vol_table));
        table_cnt= 0;
        for_flag = 0;
    }
    vol_table[table_cnt] = tp_ad;
    table_cnt++;
    if(table_cnt>=20){
        table_cnt = 0;
        for_flag = 1;
    }
    for(int i = 0;i<(sizeof(vol_table)/sizeof(vol_table[0]))/*sizeof(vol_table)*/;i++){
        average+=vol_table[i];
    }
    if(for_flag||!table_cnt){
        average/=20;
    }else{
        average/=table_cnt;
    }

    tp_ad = average;
    tp_ad = tp_ad>USER_EQ_MIC_AD_MAX?USER_EQ_MIC_AD_MAX:tp_ad;
    tp_ad = tp_ad<USER_EQ_MIC_AD_MIN?USER_EQ_MIC_AD_MIN:tp_ad;

    u32 level[]={USER_EQ_MIC_AD_MAX,792,700,651,630,609,595,0};
    // r_printf("level >>>>>>>>>>> %d",(sizeof(level)/sizeof(level[0])));
    for(int n = 0;n<(sizeof(level)/sizeof(level[0]));n++){
        if(tp_ad>=level[n]){
            mic_vol = n;
            break;
        }
    }
    mic_old = mic_vol = mic_vol?mic_vol+USER_EQ_MIC_GAIN_MIN:mic_vol;
    // printf(">>>>>>>> mic vol %d",mic_old);
    user_mic_vol_update(mic_old);

#endif
    return mic_old;
}

//按键固定调节mic vol
void user_mic_vol_key_set(void){
    static u8 user_mic_vol_grade = 0xff;
    u8 tp_grade[]={0,1,2,3,4,5,6,7,8,9,10};

    bool mic_in_status = user_get_mic_status();
    // printf(">>>>>> mic in status %d\n",mic_in_status);
    if(user_record_status(0xff) ||  !mic_in_status ){
        return;
    }
    
    if(0xff == user_mic_vol_grade){
        for(int i = 0;i<sizeof(tp_grade);i++){
            if(USER_MIC_DEFAULT_GAIN<=tp_grade[i]){
                user_mic_vol_grade = i;
                r_printf(">>>>>>>>>>>>>  mic vol default grad %d %d",i,tp_grade[i]);
                break;
            }
        }
    }
    user_mic_vol_grade++;
    if(user_mic_vol_grade>=sizeof(tp_grade)){
        user_mic_vol_grade = 0;
    }

    if(!user_mic_vol_grade){
        user_mic_en(0);
    }else if(1==user_mic_vol_grade){
        user_mic_en(1);
    }
    r_printf(">>>>>>>>>>>>>  key set mic vol %d",tp_grade[user_mic_vol_grade]);
    audio_mic_set_gain(tp_grade[user_mic_vol_grade]);
    UI_SHOW_MENU(MENU_MIC_VOL, 1000, tp_grade[user_mic_vol_grade], NULL);
}

u8 user_ex_mic_get_vol(void){
    int tp_vol = user_mic_ad_2_vol(1,0);
    if(tp_vol<0){
        return 0xff;
    }
    return tp_vol;
}


void user_mic_reverb_updata(u32 vol){
    #if (defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE)
    static int rev_old =0;
    
    static int mic_effect_dvol = -1;
    static bool mic_status_old = 0;
    
    if(!user_record_status(0xff) && \
    ((user_get_mic_status() && !mic_status_old)||
     rev_old != vol)){
        rev_old = vol;
        mic_status_old = user_get_mic_status();
        printf("reverb %d\n",rev_old);
        mic_effect_set_echo_delay(rev_old);
     }

    #endif
}

int user_mic_ad_2_reverb(u8 cmd, u32 reverb_ad){
    static int reverb_value =-1;
    s32 tp_value = 0;

    //获取reverb 旋钮值
    if(1 == cmd){
        return reverb_value;
    }

    //ad 512 无效
    // if(DIFFERENCE(reverb_ad,512)<60){
    //     return reverb_value;
    // }

    u32 tp_ad=reverb_ad;
    tp_ad = tp_ad>USER_EQ_REV_AD_MAX?USER_EQ_REV_AD_MAX:tp_ad;
    tp_ad = tp_ad<USER_EQ_REV_AD_MIN?USER_EQ_REV_AD_MIN:tp_ad;


    tp_value = ((USER_EQ_REV_AD_MAX-tp_ad)*(USER_EQ_REV_GAIN_MAX-USER_EQ_REV_GAIN_MIN)*10)/(USER_EQ_REV_AD_MAX-USER_EQ_REV_AD_MIN);    
    tp_value = (tp_value/10)+(tp_value%10>5?1:0);

    if(!reverb_value && DIFFERENCE(tp_value,reverb_value)<3){
        return reverb_value;
    }

    if(tp_value){
        tp_value += USER_EQ_REV_GAIN_MIN;
    }else{
        tp_value = 0;
    }

    if(tp_value && DIFFERENCE(tp_value,reverb_value)<2){
        return reverb_value;
    }
    // printf(">>>>> reverb %d reverb_value %d ad %d\n",reverb_value,tp_value,reverb_ad);//return reverb_value;

    reverb_value = tp_value;
    user_mic_reverb_updata(reverb_value);
    return reverb_value;
}

int user_ex_mic_get_reverb(void){
    int tp_vol = user_mic_ad_2_reverb(1,0);
    if(tp_vol<0){
        return 0xff;
    }
    return tp_vol;
}

void user_4ad_fun_features(u32 *vol){
    
    if(!(DIFFERENCE(vol[USER_EQ_BASS_BIT],512)<50)) 
        user_bass_terble_updata(vol[USER_EQ_BASS_BIT],vol[USER_EQ_TERBLE_BIT]);

    if(user_record_status(0xff) || (!user_get_mic_status())){
        return;
    }

    if(!(DIFFERENCE(vol[USER_MIC_VOL_BIT],512)<50))
        user_mic_ad_2_vol(0,vol[USER_MIC_VOL_BIT]);
    if(!(DIFFERENCE(vol[USER_REVER_BOL_BIT],512)<50))
        user_mic_ad_2_reverb(0,vol[USER_REVER_BOL_BIT]);
}
// cmd 0:ad 1:ir key 2：低电 3:对箱同步 4:开机记忆音量
u8 user_key_set_sys_vol_flag(u8 cmd){
    static u8 user_ir_key_set_sys_bol_flag = 0;
#if (defined(USER_SYS_VOL_CHECK_EN) && USER_SYS_VOL_CHECK_EN)  
    if(0xff == cmd){
        return user_ir_key_set_sys_bol_flag;
    }
    user_ir_key_set_sys_bol_flag = cmd;
#endif
    return user_ir_key_set_sys_bol_flag;
}

u16 user_delay_set_vol_id = 0;
void user_daley_set_sys_vol(void *priv){
    u32 tp = (u32)priv;
    s8 vol = tp;

    if(vol<0 && vol>get_max_sys_vol()){
        user_delay_set_vol_id = 0;
        return;
    }

    s8 get_vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    s8 set_sys_vol = 0;
    #define USER_SET_SYS_VOL_STEP_VALUE 3//Step value
    if(vol != get_vol){
        if(DIFFERENCE(vol,get_vol)>USER_SET_SYS_VOL_STEP_VALUE){
            set_sys_vol = vol>get_vol?get_vol+USER_SET_SYS_VOL_STEP_VALUE:get_vol-USER_SET_SYS_VOL_STEP_VALUE;
        }else{
            set_sys_vol = vol;
        }

        extern int linein_volume_set(u8 vol);
        if(APP_LINEIN_TASK == app_get_curr_task()){
            linein_volume_set(set_sys_vol);
        }else{
            app_audio_set_volume(APP_AUDIO_STATE_MUSIC, set_sys_vol, 1);
        }
    
        UI_SHOW_MENU(MENU_MAIN_VOL, 1000, app_audio_get_volume(APP_AUDIO_STATE_MUSIC), NULL);
        bt_tws_sync_volume();
        get_vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
        if(vol!=get_vol){
            user_delay_set_vol_id = sys_s_hi_timerout_add(priv,user_daley_set_sys_vol,100);
            return;
        }
    }

    user_delay_set_vol_id = 0;
}

//旋钮 调系统音量
void user_sys_vol_callback_fun(u32 *vol){
#if (defined(USER_SYS_VOL_CHECK_EN) && USER_SYS_VOL_CHECK_EN)    
    #define USER_SYS_VOL_AD_MAX 1000
    #define USER_SYS_VOL_AD_MIN 20

    //Rate of change
    static u32 dif_ad_old = 0;
    u32 cur_vol_ad = 0;//当前系统音量转换成ad值
    u32 cur_ad_vol = 0;//当前ad值转换成系统音量
    u32 level_ad = (USER_SYS_VOL_AD_MAX-USER_SYS_VOL_AD_MIN)/get_max_sys_vol();
    u32 cur_ad = vol[0];
    u32 cur_vol = 0;
    bool sys_vol_update_flag = 0;

    if(timer_get_sec<4 || tone_get_status()){
        return;
    }

    //滤除两端ad值
    cur_ad = cur_ad>USER_SYS_VOL_AD_MAX?USER_SYS_VOL_AD_MAX:cur_ad;
    cur_ad = cur_ad<USER_SYS_VOL_AD_MIN?0:cur_ad;

    //按键调音量之后 如果ad比上一次ad变化不大不设置 系统音量
    static u8 set_sys_vol_flag = 0;
    if(user_key_set_sys_vol_flag(0xff) != set_sys_vol_flag){
        set_sys_vol_flag = user_key_set_sys_vol_flag(0xff);
        dif_ad_old = cur_ad;
    }

    if(set_sys_vol_flag && (DIFFERENCE(cur_ad,dif_ad_old)<(level_ad*2))){
        //dif_ad_old = cur_ad;
        return;
    }else{
        user_key_set_sys_vol_flag(0);
    }

    //当前系统音量转换成ad值
    cur_vol_ad = 10*(USER_SYS_VOL_AD_MAX-USER_SYS_VOL_AD_MIN)*app_audio_get_volume(APP_AUDIO_STATE_MUSIC)/get_max_sys_vol();
    cur_vol_ad = cur_vol_ad/10+(cur_vol_ad%10>5?1:0);

    //当前ad值转换成系统音量
    cur_ad_vol = (cur_ad * get_max_sys_vol()*10 / (USER_SYS_VOL_AD_MAX-USER_SYS_VOL_AD_MIN));
    cur_ad_vol = cur_ad_vol/10+(cur_ad_vol%10>5?1:0);
    cur_ad_vol = cur_ad_vol>get_max_sys_vol()?get_max_sys_vol():cur_ad_vol;

    if(DIFFERENCE(cur_ad,cur_vol_ad)>=level_ad/*2*level_ad/3*/){
        sys_vol_update_flag = 1;
    }else if((cur_vol_ad!=get_max_sys_vol() && cur_ad_vol == get_max_sys_vol())){
        sys_vol_update_flag = 1;
    }else if(!cur_ad_vol && get_max_sys_vol()){
        sys_vol_update_flag = 1;
    }else if(DIFFERENCE(cur_ad,dif_ad_old)>60){
        sys_vol_update_flag = 1;
    }
// app_audio_state_switch(APP_AUDIO_STATE_MUSIC, get_max_sys_vol());
    if(sys_vol_update_flag){
        if(app_audio_get_volume(APP_AUDIO_STATE_MUSIC) != cur_ad_vol){
            u32 volume = cur_ad_vol;

            if(user_delay_set_vol_id)sys_s_hi_timeout_del(user_delay_set_vol_id);

            if(APP_BT_TASK == app_get_curr_task()){
                r_printf("ad vol %d tws sync\n",volume);
                extern int user_get_tws_state(void);

                if((tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) && (tws_api_get_role() == TWS_ROLE_SLAVE)){
                    //对箱从机旋钮不设置音量
                }else{
                    user_delay_set_vol_id = sys_s_hi_timerout_add((void *)volume,user_daley_set_sys_vol,100);
                    // app_audio_set_volume(APP_AUDIO_STATE_MUSIC, volume, 1);
                    // UI_SHOW_MENU(MENU_MAIN_VOL, 1000, app_audio_get_volume(APP_AUDIO_STATE_MUSIC), NULL);
                    // bt_tws_sync_volume();
                }

            }else{
                user_delay_set_vol_id = sys_s_hi_timerout_add((void *)volume,user_daley_set_sys_vol,100);
                // app_audio_set_volume(APP_AUDIO_STATE_MUSIC, volume, 1);
                // UI_SHOW_MENU(MENU_MAIN_VOL, 1000, app_audio_get_volume(APP_AUDIO_STATE_MUSIC), NULL);
            }
            // user_set_and_sync_sys_vol(volume);
            
        }
    }

    dif_ad_old = cur_ad;//vol[0];
    // printf(">>>> sys vol ad %d %d %d %d %d %d\n",vol[0],cur_ad,sys_vol_update_flag,cur_ad_vol,cur_vol_ad,app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
#endif    
}

static u16 auto_time_id = 0;
void user_music_play_finle_number(void *priv){
    int *filenum = (int *)priv;
    auto_time_id = 0;
    printf(">>> kkkk number %d %d\n",*filenum);
    if(*filenum){
        music_player_play_by_number(music_player_get_dev_cur(),*filenum);
        // music_play_msg_post(3, MPLY_MSG_FILE_BY_DEV_FILENUM, (int)music_play_get_cur_dev(), *filenum);
    }
    *filenum = 0;
    printf(">>>> 000 %d\n",*filenum);
}
void user_music_set_file_number(int number){
    #if USER_IR_PLAY_FILE_NUMBER
    static int filenum = 0;
    s32 tp = filenum*10+number;

    printf(">>>>>music_play_get_file_total_file %d\n",music_player_get_file_total());

    if(auto_time_id){
        sys_timeout_del(auto_time_id);
    }

    if((tp>music_player_get_file_total()) || /*!tp ||*/ tp/10000){
        filenum = 0;
        #if TCFG_UI_ENABLE
        ui_set_tmp_menu(MENU_RECODE_ERR, 3000, 0, NULL);
        #endif
        return;
    }else{
        filenum = tp;
    }

    printf(">>>>> filenum %d\n",filenum);


    #if TCFG_UI_ENABLE
    ui_set_tmp_menu(MENU_FILENUM, 4000, filenum, NULL);
    #endif

    if(filenum>1000){
        user_music_play_finle_number(&filenum);
        filenum = 0;
    }else{
        auto_time_id = sys_timeout_add(&filenum,user_music_play_finle_number,3000);
    }
    #endif
}


//spi pb6 clk 引脚复用设置
void user_fun_spi_pb6_mult(void){
    #if (defined(USER_VBAT_CHECK_EN) && USER_VBAT_CHECK_EN)
    user_attr_gpio_set(user_power_io.pro,USER_GPIO_IN_AD,35);
    #endif
}

void user_vbat_check_init(void){
    #if (defined(USER_VBAT_CHECK_EN) && USER_VBAT_CHECK_EN)
    user_fun_spi_pb6_mult();
    adc_add_sample_ch(user_power_io.ch);          //注意：初始化AD_KEY之前，先初始化ADC
    
    adc_set_sample_freq(user_power_io.ch,5);
    // if(timer_get_ms()>3000){
    //     adc_set_sample_freq(user_power_io.ch,10);
    // }
    #endif
    return;
}

u16 user_fun_get_vbat(void){
    static u16 tp = 0;
    #if (defined(USER_VBAT_CHECK_EN) && USER_VBAT_CHECK_EN)
    u32 vddio_m = 0;
    u32 tp_ad = 0;//adc_get_value(user_power_io.ch);
    tp_ad = adc_get_value(user_power_io.ch);

    // //立即获取dac值
    // extern void adc_enter_occupy_mode(u32 ch);
    // extern u32 adc_occupy_run();
    // extern void adc_exit_occupy_mode();
    // adc_enter_occupy_mode(user_power_io.ch);
    // tp_ad = user_power_io.vol = adc_occupy_run();
    // adc_exit_occupy_mode();


    vddio_m  = 220+(TCFG_LOWPOWER_VDDIOM_LEVEL)*20;

    user_power_io.vol = (vddio_m*2*tp_ad*10)/0x3ffL;
    user_power_io.vol = (user_power_io.vol+5)/10;
    user_power_io.vol-=6;//修正
    // printf("user vbat >>>> ad:%d %d vbat:%dV\n",tp_ad,tp_ad*20*vddio_m/0x3ffL,user_power_io.vol);
    tp = user_power_io.vol;
    #endif
    return tp;
}


void user_fm_vol_set(bool cmd){
#if (defined(USER_FM_MODE_SYS_VOL) && USER_FM_MODE_SYS_VOL)

    static u8 user_fm_sys_vol = 0xff;
    u8 tp_vol = 0;

    if(cmd){//进fm 保存音量
        tp_vol = user_fm_sys_vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
        while (tp_vol > USER_FM_MODE_SYS_VOL){
            app_audio_set_volume(APP_AUDIO_STATE_MUSIC, USER_FM_MODE_SYS_VOL, 1);
            tp_vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
        }
    }else{//退出FM
        while(app_audio_get_volume(APP_AUDIO_STATE_MUSIC)!=user_fm_sys_vol){
            app_audio_set_volume(APP_AUDIO_STATE_MUSIC, user_fm_sys_vol, 1);
        }
    }

#endif
}

//sd pg power
void user_sd_power(u8 cmd){
    #if(defined(USER_SD_POWER_SWITCH_EN) && USER_SD_POWER_SWITCH_EN)
    //绑定引脚高阻态
    #if (USER_SD_POWER_IO != NO_CONFIG_PORT)
    gpio_set_die(USER_SD_POWER_IO, 0);
    gpio_set_direction(USER_SD_POWER_IO, 1);
    gpio_set_pull_down(USER_SD_POWER_IO, 0);
    gpio_set_pull_up(USER_SD_POWER_IO, 0);
    #endif

    sdpg_config(cmd?1:0);
    #endif
}

//获取、设置 关机类型
//cmd 1:低电关机 2:遥控器关机
u8 user_power_off_class(u8 cmd){
    static u8 power_class = 0;

    if(1 == cmd){//低电关机
        power_class = cmd;
    }else if((1 != power_class) && (2 == cmd)){//遥控器关机
        power_class = cmd;        
    }

    return power_class;
}
//关机
void user_power_off(void){
    user_low_power_show(0x55);
    UI_SHOW_MENU(MENU_POWER_OFF, 0, 0, NULL);
    user_led_io_fun(USER_IO_LED,LED_POWER_OFF);
    // user_pa_ex_strl(PA_POWER_OFF);
    // user_sd_power(0);
    user_rgb_mode_set(USER_RGB_POWER_OFF,NULL);

    user_mic_check_en(0);
}

//注销 定时器
void user_del_time(void){
    user_mic_check_del();
    user_ad_total_check_del();
    user_rgb_fun_del();
    user_pa_ex_strl(PA_POWER_OFF);
    user_pa_ex_del();
}

//开机 io口初始化
void user_fun_io_init(void){
    user_mic_en(0);
    user_pa_ex_io_init();
    user_sd_power(1);
    user_vbat_check_init();
}

//开机 功能初始化
void user_fun_init(void){
    user_udelay_init();
    user_pa_ex_init();

    user_4ad_check_init(user_4ad_fun_features);
    user_sys_vol_ad_check_init(user_sys_vol_callback_fun);

    // user_led_io_fun(USER_IO_LED,LED_IO_INIT);
    // user_led_io_fun(USER_IO_LED,LED_POWER_ON);

    // #if (USER_IC_MODE == USER_IC_6966B)
    // ex_dev_detect_init(& user_mic_check);
    // #endif
    //第一次上电开机 mic上线的消息会丢失 延时检测
    // sys_timeout_add(NULL,user_mic_check_init,3000);

    user_rgb_fun_init();
    user_low_power_show(0);
	// user_mic_en(1);
    // extern void user_print_timer(void);
    // sys_s_hi_timer_add(NULL,user_print_timer,500);
    // user_print_timer();
}
