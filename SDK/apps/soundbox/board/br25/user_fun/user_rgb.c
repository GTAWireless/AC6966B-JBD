#include "user_fun_cfg.h"

#if USER_RGB_EN
#if (defined(TCFG_HW_SPI0_ENABLE) && TCFG_HW_SPI0_ENABLE)
#define USER_RGB_DATA   SPI0
#endif
#if (defined(TCFG_HW_SPI1_ENABLE) && TCFG_HW_SPI1_ENABLE)
#define USER_RGB_DATA   SPI1
#endif
#if (defined(TCFG_HW_SPI2_ENABLE) && TCFG_HW_SPI2_ENABLE)
#define USER_RGB_DATA   SPI2
#endif
#ifndef USER_RGB_DATA
error 没有开spi
#endif
RGB_INFO user_rgb_info={
    .time_id   = 0,
    .power_off = 0,
    .init_flag = 0,
    .rend_flag = 0,
    .updata_flag = 0,
    .updata_only = 0,
    .number = USER_RGB_NUMBER,
    .spi_scan_time = 20,
    .spi_port = USER_RGB_DATA,//SPI2,
    .code = {
        .code_0 = USER_RGB_CODE0,
        .code_1 = USER_RGB_CODE1,
    },

    #if USER_RGB_BUFF_MALLOC_EN
    .rgb_buff = NULL,
    .spi_buff = NULL,
    #else
    .rgb_buff = {0},
    .spi_buff = {0},    
    #endif
};
RGB_FUN user_rgb_fun = {
    .power_off = 0,
    .step_value = 1,
    .interrupt_id =0,
    .interrupt = 0,
    .info = &user_rgb_info,
    .cur_colour = {0xff,0x0,0x0},
    .cur_mode = USER_RGB_MODE_MAX,
    .mode_lock = USER_RGB_NULL,
    .mode_scan_time = 100,
    .light_number = 0,
    .dac_energy_scan_id = 0,
};
#define P_RGB_FUN (&user_rgb_fun)
// #else
// #define P_RGB_FUN &NULL
// #endif

//伪随机数
u32 random_number(u32 start, u32 end){
    if (end <= start) {
        return start;
    }

    return JL_TIMER0->CNT % (end - start + 1) + start;
}

//获取dac最大能量值
#define USER_SAVE_SEC_MAX_DAC   3//单位s
int user_max_dac_energy(int _dac_energy){
    int dac_energy_max = 0;
    // int dac_energy = audio_dac_energy_get();
    static int energy_table[USER_SAVE_SEC_MAX_DAC]={0};
    static u32 timer_cnt = 0;
    static u8 cnt_data = 0;
    //保存最近3秒最大能量值
    u32 cur_sec = timer_get_sec();
    cur_sec-=timer_cnt;

    if(cur_sec>=(sizeof(energy_table)/sizeof(energy_table[0]))){
        timer_cnt = timer_get_sec();
        cur_sec = 0;
    }
    if(energy_table[cur_sec]<_dac_energy){
        energy_table[cur_sec] = _dac_energy;
    }
    if(cnt_data!=cur_sec){
        energy_table[cnt_data] = 0;
        cnt_data = cur_sec;
    }

    //最大能量值
    for(int i = 0;i<(sizeof(energy_table)/sizeof(energy_table[0]));i++){
        if(dac_energy_max<energy_table[i]){
            dac_energy_max = energy_table[i];
        }
    }
    return  dac_energy_max;
}

//dac能量值转换为点亮灯数量
void user_rgb_dac_energy_to_light_number(void * priv){
    RGB_FUN *rgb = (RGB_FUN *)priv;

    if(!rgb || !rgb->info || !rgb->dac_energy_scan_id || !rgb->info->number){
        return;
    }

    if(rgb->dac_energy_scan_id){
        rgb->dac_energy_scan_id = 0;
    }
    rgb->dac_energy_scan_id = sys_hi_timeout_add(rgb,user_rgb_dac_energy_to_light_number,rgb->light_number?20:100);

    int fre_cnt = 0;
    int dac_energy_max=0;
    int dac_energy = audio_dac_energy_get();

    dac_energy_max = user_max_dac_energy(dac_energy);
    if(dac_energy_max && dac_energy){
        fre_cnt = ((rgb->info->number)*dac_energy)/dac_energy_max;
    }else{
        fre_cnt = 0;
    }

    if(rgb->light_number<fre_cnt){
        rgb->light_number++;
    }else if(rgb->light_number>fre_cnt){
        rgb->light_number--;
    }

    // printf(">>> dac energy %d\n",rgb->light_number);
}

/*
*r,g,b:基色值
*umode：模式
*kk:不进值
*rgb_min_value:另一个基色开始变化时的初始值
*/
void user_rgb_gradient(u16 *r,u16 *g,u16 *b,u8 *umode,u16 kk){
	s16 sr=*r,sg=*g,sb=*b;
	u8 mode = *umode;

    if(0==mode){
        if((sr+sg+sb)!=0xff){
            sr = 0xff;
            sg = sb = 0;
        }
        sg+=kk;
        sr = 0xff-sg;

    	if(0xff <= sg){
            sg = 0xff;
            sr = sb = 0;
    		mode = 1;
    	}
    }else if(1==mode){
        if((sr+sg+sb)!=0xff){
            sg = 0xff;
            sr = sb = 0;
            sb = 0;
        }
        sb+=kk;
        sg = 0xff-sb;

    	if(0xff <= sb){
            sb = 0xff;
            sr = sg = 0;
    		mode =2;
    	}
    }else if(2==mode){
        if((sr+sg+sb)!=0xff){
            sb = 0xff;
            sr = sg = 0;
        }
        sr+=kk;
        sb = 0xff-sr;
    	if(0xff <= sr){
            sr = 0xff;
            sb = sg = 0;
    		mode =3;
    	}
    }else if(3==mode){
        if(sr!=0xff && !sb){
            sr = 0xff;
            sb = sg = 0;
        }
        sg+=kk;
        sr = 0xff;
        sb = 0;
    	if(0xff <= sg){
            sr = sg= 0xff;
            sb = 0;
    		mode =4;
    	}
    }else if(4==mode){
        if((sr+sg+sb)!=(0xff*2)){
            sr = sg= 0xff;
            sb = 0;
        }
        sb+=kk;
        sr = 0xff-sb;

    	if(0xff <= sb){
            sg = sb = 0xff;
            sr = 0;
    		mode =5;
    	}
    }else if(5==mode){
        if((sr+sg+sb)!=(0xff*2)){
            sg = sb = 0xff;
            sr = 0;
        }
        sr+=kk;
        sg = 0xff-sr;

    	if(0xff <= sr){
            sr = sb = 0xff;
            sg = 0;
    		mode =6;
    	}
    }else if(6==mode){
        if(sr!=0xff && !sg){
            sr = sb = 0xff;
            sg = 0;
        }
        sb -=kk;

    	if(0x0>=sb){
            sr = 0xff;
            sg = sb = 0x0;
    		mode =0;
    	}
    }
	*r = sr;
	*g = sg;
	*b = sb;
	*umode = mode;
}


void user_rgb_colour_gradient(void *priv){
    RGB_FUN *rgb = (RGB_FUN *)priv;
    if(!rgb || !rgb->info || rgb->info->rend_flag){
        return;
    }
    static u8 mode = 0;

    u16 sr=rgb->cur_colour.r,sg=rgb->cur_colour.g,sb=rgb->cur_colour.b;
    user_rgb_gradient(&sr,&sg,&sb,&mode,1);
    rgb->cur_colour.r=sr;
    rgb->cur_colour.g=sg;
    rgb->cur_colour.b=sb;

}

//清除打断标志
void user_rgb_clean_interrupt(void *priv){

    RGB_FUN *rgb = (RGB_FUN *)priv;
    if(!rgb){
        return;
    }
    rgb->interrupt_id = 0;
    rgb->interrupt = 0;
    rgb->info->updata_only = 0;
}
//bass状态显示
void user_rgb_bass_display(void *priv,void *data){
    RGB_FUN *rgb = (RGB_FUN *)priv;
    RGB_DISPLAY_DATA *play_data = (RGB_DISPLAY_DATA*)data;

    if(!rgb || !play_data || !rgb->info || rgb->info->rend_flag || !rgb->info->number || !play_data->display_time || rgb->power_off){
        return;
    }

    rgb->interrupt = 1;
    rgb->info->updata_flag = 1;
    if(rgb->interrupt_id){
        sys_timeout_del(rgb->interrupt_id);
    }

    user_rgb_clear_colour(rgb->info);
    RGB_COLOUR colour={0x0,0x0,0x0};
    if(play_data->bass){
        colour.r = 0xff;
        colour.b = 0;
        colour.g = 0;
    }else{
        colour.r = 0xff;
        colour.b = 0xff;
        colour.g = 0;
    }

    user_rgb_same_colour(rgb->info,&colour);

    rgb->info->updata_flag = 0;
    rgb->interrupt_id = sys_timeout_add(rgb,user_rgb_clean_interrupt,1000*(play_data->display_time));

}

//音量显示
void user_rgb_vol_display(void *priv,void *data){
    RGB_FUN *rgb = (RGB_FUN *)priv;
    RGB_DISPLAY_DATA *play_data = (RGB_DISPLAY_DATA*)data;

    if(!rgb || !play_data || !rgb->info || rgb->info->rend_flag || !rgb->info->number || !play_data->display_time || rgb->power_off){
        return;
    }
    rgb->interrupt = 1;
    rgb->info->updata_flag = 1;
    if(rgb->interrupt_id){
        sys_timeout_del(rgb->interrupt_id);
    }

    user_rgb_clear_colour(rgb->info);

    u16 tp = 0;
    u16 number = (play_data->sys_vol*rgb->info->number/play_data->sys_vol_max);//被点亮的颗数
    RGB_COLOUR colour={0x0,0x0,0x0};

    for(int i=0;i<number;i++){
        tp = i*(0xff-0xa)/rgb->info->number;
        tp = !tp?0xa:tp;
        tp = tp>0xff?0xff:tp;
        colour.r = tp;

        user_rgb_colour_only_set(rgb->info,&colour,i);
    }

    rgb->info->updata_flag = 0;
    rgb->interrupt_id = sys_timeout_add(rgb,user_rgb_clean_interrupt,1000*(play_data->display_time));
}


//淡入淡出 环形灯圈
void user_rgb_display_mode_1(void *priv){

    RGB_FUN *rgb = (RGB_FUN *)priv;
    if(!rgb || !rgb->info || rgb->info->rend_flag || !rgb->info->number){
        return;
    }

    RGB_COLOUR tp_buff;
    u32 color_brightness = 0;
    u32 tp_brightness = 0;

    /*
        颜色： r:g:b(比值)
        亮度：0~255(光的强弱)
        效果：颜色与亮度的叠加
    */
   static int tp_light =0;
   static int tp_kk = 0;

    tp_kk = rgb->light_number/6;
    tp_light += tp_kk;

    tp_light %=rgb->info->number;
    // r_printf(".......... lin %d",tp_light);
    for(int i = 0;i<rgb->info->number;i++){
        if(rgb->brightness_table[i]<=(RGB_BRIGHTNESS_LEVEL/RGB_BRIGHTNESS_SEGMENT)){
            color_brightness = (255/RGB_BRIGHTNESS_LEVEL)*RGB_BRIGHTNESS_SEGMENT*rgb->brightness_table[i];
        }else if (rgb->brightness_table[i]<((RGB_BRIGHTNESS_LEVEL/RGB_BRIGHTNESS_SEGMENT)*2)){
            color_brightness = (255/RGB_BRIGHTNESS_LEVEL)*(RGB_BRIGHTNESS_LEVEL-RGB_BRIGHTNESS_SEGMENT*(rgb->brightness_table[i]%(RGB_BRIGHTNESS_LEVEL/RGB_BRIGHTNESS_SEGMENT)));
        }else{
            color_brightness = 0;
        }

        u16 tp_value = rgb->brightness_table[i];
        tp_value = (rgb->step_value+tp_value)%0xff;
        rgb->brightness_table[i]= tp_value;
        rgb->brightness_table[i] %= ((RGB_BRIGHTNESS_LEVEL/rgb->info->number))*(rgb->info->number);//RGB_BRIGHTNESS_LEVEL;

        tp_brightness = (color_brightness*rgb->cur_colour.r)/0xff;
        tp_buff.r = tp_brightness;

        tp_brightness = (color_brightness*rgb->cur_colour.g)/0xff;
        tp_buff.g = tp_brightness;

        tp_brightness = (color_brightness*rgb->cur_colour.b)/0xff;
        tp_buff.b = tp_brightness;

        user_rgb_colour_only_set(rgb->info,&tp_buff,(rgb->info->number-i)+tp_light);
    }

}

//升降 环形灯圈
void user_rgb_display_mode_2(void *priv){

    RGB_FUN *rgb = (RGB_FUN *)priv;
    if(!rgb || !rgb->info || rgb->info->rend_flag || !rgb->info->number){
        return;
    }
    // random_number(7,30);
    // timer_get_ms();

    s32 display_number = rgb->light_number%rgb->info->number;//timer_get_sec()%(rgb->info->number);
    // display_number=display_number<rgb->info->number?display_number+1:display_number;
    display_number /=2;
    printf(">>>>>>>>>>>>%d\n",display_number);
    display_number = display_number>=(rgb->info->number>>1)?(rgb->info->number>>1)-(display_number%(rgb->info->number>>1))-2:display_number;

    for(int i = 0;i<=display_number;i++){
        user_rgb_colour_only_set(rgb->info,&rgb->cur_colour,(rgb->info->number/2)-i-(!(rgb->info->number%2)));
        user_rgb_colour_only_set(rgb->info,&rgb->cur_colour,(rgb->info->number/2)+i);
    }
}

//渐变
void user_rgb_display_mode_3(void *priv){
    RGB_FUN *rgb = (RGB_FUN *)priv;
    if(!rgb || !rgb->info || rgb->info->rend_flag || !rgb->info->number){
        return;
    }

    user_rgb_same_colour(rgb->info,&rgb->cur_colour);
}

//三色旋转
void user_rgb_display_mode_4(void *priv){
    RGB_FUN *rgb = (RGB_FUN *)priv;
    if(!rgb || !rgb->info || rgb->info->rend_flag || !rgb->info->number){
        return;
    }

    RGB_COLOUR colour[3] = {
        {0xff,0x00,0x00},
        {0x00,0xff,0x00},
        {0x00,0x00,0xff}
    };

    u16 grade = rgb->info->number/3;
    s16 rand  = random_number(7,20);
    for(int i=0;i<rgb->info->number;i++){
        user_rgb_colour_only_set(rgb->info,&colour[i/grade],i+rand);
    }
}

//单色闪烁
#define USER_RGB_FLASH_FRE_GRADE    11//0-11级别
void user_rgb_display_mode_5(void *priv){
    RGB_FUN *rgb = (RGB_FUN *)priv;
    if(!rgb || !rgb->info || rgb->info->rend_flag || !rgb->info->number){
        return;
    }

    u32 tp_time = timer_get_ms();
    static u32 sys_time_old = 0;
    static u8 tp_freq = 0;
    u8 flash_fre_cnt= 0;

    u32 dac_energy = audio_dac_energy_get();
    u32 dac_energy_max = user_max_dac_energy(dac_energy);

    if(dac_energy>(dac_energy_max*5/10)){
        flash_fre_cnt = (USER_RGB_FLASH_FRE_GRADE-1);
    }else if(dac_energy>(dac_energy_max/3)){
        flash_fre_cnt = USER_RGB_FLASH_FRE_GRADE/2;
    }else if(dac_energy>(dac_energy_max/4)){
        flash_fre_cnt = 1;
    }else{
        flash_fre_cnt = 0;
    }
    if(flash_fre_cnt > tp_freq){
        tp_freq++;
    }else if(flash_fre_cnt < tp_freq){
        tp_freq--;
    }

    if((tp_time-sys_time_old)>1000){
        sys_time_old = tp_time;
        if(tp_freq){
            rgb->freq = (USER_RGB_FLASH_FRE_GRADE-tp_freq)*100;
        }else{
            rgb->freq = 0;
        }
        r_printf("freq %d",tp_freq);        
    }

    if(!rgb->freq || ((tp_time%(rgb->freq)) >= ((rgb->freq)/3))){
        user_rgb_same_colour(rgb->info,&rgb->cur_colour);
    }
}

void user_rgb_mode_scan(void *priv){
    RGB_FUN *rgb = (RGB_FUN *)priv;
    if(!rgb || !rgb->info || rgb->info->rend_flag || rgb->power_off){
        return;
    }

    rgb->info->updata_flag = 1;
    if(!rgb->interrupt){
        user_rgb_colour_gradient(rgb);
        user_rgb_clear_colour(rgb->info);


        switch (rgb->cur_mode){
        case USER_RGB_MODE_1:
            if(30 != rgb->mode_scan_time){
                rgb->mode_scan_time = 30;
            }
            user_rgb_display_mode_1(rgb);
            break;
        case USER_RGB_MODE_2:
            if(100 != rgb->mode_scan_time){
                rgb->mode_scan_time = 100;
            }
            user_rgb_display_mode_2(rgb);
            break;
        case USER_RGB_MODE_3:
            if(100 != rgb->mode_scan_time){
                rgb->mode_scan_time = 100;
            }
            user_rgb_display_mode_3(rgb);
            break;
        case USER_RGB_MODE_4:
            if(100 != rgb->mode_scan_time){
                rgb->mode_scan_time = 100;
            }
            user_rgb_display_mode_4(rgb);
            break;
        case USER_RGB_MODE_5:
        case USER_RGB_MODE_6:
        case USER_RGB_MODE_7:
        case USER_RGB_MODE_8:
        case USER_RGB_MODE_9:
            if(20 != rgb->mode_scan_time){
                rgb->mode_scan_time = 20;
            }
            if(USER_RGB_MODE_6 == rgb->cur_mode){
                rgb->cur_colour.r = 0xff;
                rgb->cur_colour.g = 0x00;
                rgb->cur_colour.b = 0x00;
            }else if(USER_RGB_MODE_7 == rgb->cur_mode){
                rgb->cur_colour.r = 0x00;
                rgb->cur_colour.g = 0xff;
                rgb->cur_colour.b = 0x00;
            }else if(USER_RGB_MODE_8 == rgb->cur_mode){
                rgb->cur_colour.r = 0x00;
                rgb->cur_colour.g = 0x00;
                rgb->cur_colour.b = 0xff;
            }else if(USER_RGB_MODE_9 == rgb->cur_mode){
                rgb->cur_colour.r = 0xff;
                rgb->cur_colour.g = 0xff;
                rgb->cur_colour.b = 0xff;
            }
            user_rgb_display_mode_5(rgb);
            break;
        case USER_RGB_FM_MODE:
            rgb->cur_colour.r = 0x00;
            rgb->cur_colour.g = 0xff;
            rgb->cur_colour.b = 0x00;
            user_rgb_same_colour(rgb->info,&rgb->cur_colour);
        case USER_RGB_MODE_OFF:
            break;
        default:
            break;
        }
    }

    rgb->info->updata_flag = 0;

    if(rgb->mode_lock != USER_RGB_NULL && !rgb->info->updata_only){
        r_printf(">>>>> updata_only");
        rgb->info->updata_only = 2;
    }
    sys_timeout_add(rgb,user_rgb_mode_scan,rgb->mode_scan_time);
}

void user_rgb_fun_power_off(void *priv){
    RGB_FUN *rgb = (RGB_FUN *)priv;
    if (!rgb || !rgb->info || rgb->power_off){
        return;
    }

    rgb->cur_mode = USER_RGB_POWER_OFF;
    rgb->power_off = 1;
    user_rgb_power_off(rgb->info);

}
#endif

//vol显示外部调用接口
void user_rgb_display_vol(u8 vol,u16 display_time){
#if USER_RGB_EN
    RGB_DISPLAY_DATA data;
    data.display_time = display_time;
    data.sys_vol_max = app_audio_get_max_volume();
    data.sys_vol = vol;//app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    printf(">>>>>> max %d vol %d",data.sys_vol_max,data.sys_vol);
    user_rgb_mode_set_or_get(USER_RGB_SYS_VOL,&data);
#endif
}

//bass状态显示外部调用接口
void user_rgb_display_bass(u8 bass,u16 display_time){
#if USER_RGB_EN
    RGB_DISPLAY_DATA data;
    data.display_time = display_time;
    data.bass = bass;

    user_rgb_mode_set_or_get(USER_RGB_EQ_BASS,&data);
#endif
}


//保存rgb mode
static u16 rgb_save_id = 0;
static void user_save_rgb_mode_do(void *priv)
{
#if USER_RGB_EN
    RGB_FUN *rgb = (RGB_FUN *)P_RGB_FUN;
    u8 tp_rgb_mode = 0xff;
    local_irq_disable();
    syscfg_read(CFG_USER_RGB_MODE_ID, &tp_rgb_mode, 1);        
    if(tp_rgb_mode!=rgb->cur_mode && ( (USER_RGB_MODE_1 <= rgb->cur_mode) && (rgb->cur_mode < USER_RGB_MODE_MAX))){
        r_printf("save rgb mode %d",rgb->cur_mode);
        syscfg_write(CFG_USER_RGB_MODE_ID, &rgb->cur_mode, 1);
    }
    local_irq_enable();
    rgb_save_id = 0;
#endif
}

static void user_save_rgb_mode(void *priv,u32 msec)
{

    local_irq_disable();
    if (rgb_save_id) {
        sys_hi_timer_del(rgb_save_id);
        rgb_save_id = 0;
    }
    if(msec){
        rgb_save_id = sys_hi_timeout_add(priv, user_save_rgb_mode_do, msec);
    }else{
        r_printf("user_save_rgb_mode_do");
        user_save_rgb_mode_do(priv);
    }
    local_irq_enable();
}

//模式设置
u8 user_rgb_mode_set_or_get(USER_GRB_MODE cmd,void *priv){
    u8 ret = 0;
#if USER_RGB_EN
    RGB_FUN *rgb = (RGB_FUN *)P_RGB_FUN;
    USER_GRB_MODE mode = cmd;
    if (!rgb || !rgb->info || USER_RGB_POWER_OFF==rgb->cur_mode
    ){
        printf(">>>>> user_rgb_mode_set_or_get rgb p error %d %d %d\n",!rgb,!rgb->info , USER_RGB_POWER_OFF==rgb->cur_mode);
        return rgb->cur_mode;
    }

    #if (USER_RGB_LOOP_MODE == USER_RGB_LOOP_MODE_1)
    USER_GRB_MODE cycle_mode[]={
    USER_RGB_MODE_1,//节奏渐变 旋转
    USER_RGB_MODE_2,//对称 升降
    USER_RGB_MODE_3,//渐变
    USER_RGB_MODE_4,//三色 旋转
    USER_RGB_MODE_5,//渐变 闪烁
    USER_RGB_MODE_6,//红色 闪烁
    USER_RGB_MODE_7,//绿色 闪烁
    USER_RGB_MODE_8,//蓝色 闪烁
    USER_RGB_MODE_9,//白色 闪烁
    USER_RGB_MODE_OFF,//关灯
    };
    #elif (USER_RGB_LOOP_MODE == USER_RGB_LOOP_MODE_2)
    USER_GRB_MODE cycle_mode[]={
    USER_RGB_MODE_1,//节奏渐变 旋转
    USER_RGB_MODE_2,//对称 升降
    USER_RGB_MODE_3,//渐变
    USER_RGB_MODE_OFF,//关灯
    };
    #elif (USER_RGB_LOOP_MODE == USER_RGB_LOOP_MODE_3)
    USER_GRB_MODE cycle_mode[]={
    USER_RGB_MODE_5,//渐变 闪烁
    USER_RGB_MODE_6,//红色 闪烁
    USER_RGB_MODE_7,//绿色 闪烁
    USER_RGB_MODE_8,//蓝色 闪烁
    USER_RGB_MODE_9,//白色 闪烁
    USER_RGB_MODE_OFF,//关灯
    };

    if(app_check_curr_task(APP_FM_TASK) &&
    ( (USER_RGB_MODE_1 <= mode && USER_RGB_MODE_MAX > mode) || (mode == USER_RGB_AUTO_SW))
    ){
        if(rgb->cur_mode!=USER_RGB_MODE_OFF){
            mode = USER_RGB_MODE_OFF;
        }else{
            mode = USER_RGB_FM_MODE;
        }
    }
    #endif

    static u8 cur_mode = -1;
    switch (mode){
    case USER_RGB_STATUS_ULOCK:
        r_printf("USER_RGB_STATUS_ULOCK mode=%d",rgb->mode_lock);
        rgb->cur_mode = rgb->mode_lock;
        rgb->mode_lock = USER_RGB_NULL;
        break;
    case USER_RGB_STATUS_LOCK:
        r_printf("USER_RGB_STATUS_LOCK mode=%d",rgb->cur_mode);
        user_save_rgb_mode(rgb,0);
        rgb->mode_lock = rgb->cur_mode;
        break;
    case USER_RGB_AUTO_SW:
        r_printf("cur_mode %d %d",cur_mode,sizeof(cycle_mode)/sizeof(cycle_mode[0]));
        cur_mode++;
        if(cur_mode>=sizeof(cycle_mode)/sizeof(cycle_mode[0])){
            cur_mode = 0;
        }
        rgb->cur_mode = cycle_mode[cur_mode];
        printf("user rgb mode %d\n",rgb->cur_mode);
        user_save_rgb_mode(rgb,5000);
        break;
    case USER_RGB_SYS_VOL:
        puts("USER_RGB_SYS_VOL\n");
        user_rgb_vol_display(rgb,priv);
        break;
    case USER_RGB_EQ_BASS:
        puts("USER_RGB_EQ_BASS\n");
        user_rgb_bass_display(rgb,priv);
        break;
    case USER_RGB_POWER_OFF:
        user_rgb_fun_power_off(rgb);
        break;
    case USER_RGB_MODE_1://节奏渐变 旋转
    case USER_RGB_MODE_2://对称 升降
    case USER_RGB_MODE_3://渐变
    case USER_RGB_MODE_4://三色 旋转
    case USER_RGB_MODE_5://渐变 闪烁
    case USER_RGB_MODE_6://红色 闪烁
    case USER_RGB_MODE_7://绿色 闪烁
    case USER_RGB_MODE_8://蓝色 闪烁
    case USER_RGB_MODE_9://白色 闪烁
        user_save_rgb_mode(rgb,5000);
    case USER_RGB_MODE_OFF:
    case USER_RGB_FM_MODE:
        rgb->cur_mode = mode;
        break;
    default:
        break;
    }
    
    rgb->info->updata_only = 0;
    ret = rgb->cur_mode;
#endif
    return ret;
}


void user_rgb_fun_init(void){
#if USER_RGB_EN
    RGB_FUN *rgb = (RGB_FUN *)P_RGB_FUN;
    if (!rgb || !rgb->info || !rgb->info->number){
        printf(">>>>> user_rgb_fun_init rgb p error\n");
        return;
    }

    #if USER_RGB_BUFF_MALLOC_EN
    rgb->info->spi_buff = malloc(sizeof(SPI_COLOUR)*rgb->info->number);
    if(!rgb->info->spi_buff){
        printf("rgb init mallocc error\n");
        return;
    }
    rgb->info->rgb_buff = malloc(sizeof(RGB_COLOUR)*rgb->info->number);
    if(!rgb->info->rgb_buff){
        free(rgb->info->spi_buff);
        printf("rgb init mallocc error 1\n");
        return;
    }
    #endif
    
    //读取vm中保存rgb mode
    u8 tp_rgb_mode = 0xff;
    syscfg_read(CFG_USER_RGB_MODE_ID, &tp_rgb_mode, 1);
    if(0xff != tp_rgb_mode){
        user_rgb_mode_set_or_get(tp_rgb_mode, rgb);
    }else{
        user_rgb_mode_set_or_get(USER_RGB_AUTO_SW,rgb);
    }
    // user_rgb_clear_colour(rgb->info);

    // printf(">>>>>>>>>>>>>> user spi %d\n",user_rgb_fun.info->spi_port);
    for(int i = 0;i<rgb->info->number;i++){
        rgb->brightness_table[i]=(i+1)*(RGB_BRIGHTNESS_LEVEL/rgb->info->number);
        printf("xx %d %d\n",i,rgb->brightness_table[i]);
    }

    user_rgb_same_colour(rgb->info,&user_rgb_fun.cur_colour);

    user_rgb_init(rgb->info);
    user_vbat_check_init();
    if(rgb->info->init_flag){
        user_rgb_mode_scan(rgb);
    }

    rgb->dac_energy_scan_id = sys_hi_timeout_add(rgb,user_rgb_dac_energy_to_light_number,300);
#endif
}
void user_rgb_fun_del(void){
#if USER_RGB_EN
    RGB_FUN *rgb = (RGB_FUN *)P_RGB_FUN;
    if (!rgb || !rgb->info || !rgb->info->number){
        printf(">>>>> user_rgb_fun_del rgb p error\n");
        return;
    }

    if(rgb->dac_energy_scan_id){
        sys_timeout_del(rgb->dac_energy_scan_id);
        rgb->dac_energy_scan_id = 0;
    }
    user_rgb_del(rgb->info);

    #if (defined USER_RGB_BUFF_MALLOC_EN && USER_RGB_BUFF_MALLOC_EN)
    if(rgb->info->spi_buff){
        free(rgb->info->spi_buff);
    }
    if(rgb->info->rgb_buff){
        free(rgb->info->rgb_buff);
    }
    #endif
#endif
}

