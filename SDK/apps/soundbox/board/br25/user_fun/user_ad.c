#include "user_fun_cfg.h"


#if (defined(USER_3IO_CHECK_4AD_EN) && USER_3IO_CHECK_4AD_EN)
IO3_AD4 user_4ad_io={
    .por = IO_PORTA_04,//检测引脚
    .select_por = {IO_PORTA_06,IO_PORTA_05},//选择引脚
    .ch  = AD_CH_PA4,
    .vol = {0},
    .ch_bumber = 0,
    .up  = 0,

    .callback = NULL,//user_4ad_fun_features,
    .check_time = 50,
};
#endif


//输出高低
static void user_io_output(u32 gpio, u32 dir){
    gpio_set_direction(gpio, 0);
    gpio_set_pull_down(gpio, 0);
    gpio_set_pull_up(gpio, 0);
    gpio_set_die(gpio, 1);
    gpio_set_output_value(gpio,dir);
    return;
}
//高阻态
static void user_io_high_impedance(u32 gpio){
    gpio_set_direction(gpio, 1);
    gpio_set_output_value(gpio,0);
    gpio_set_pull_down(gpio, 0);
    gpio_set_pull_up(gpio, 0);
    gpio_set_die(gpio, 1);
    return;
}

void user_4ad_io_init(IO3_AD4 * io){
    if(io->ch_bumber >= 4){
        io->ch_bumber = 0;
    }

    if(0 == io->ch_bumber){
        user_io_output(io->select_por[0],1);
        user_io_high_impedance(io->select_por[1]);
    }else if(1 == io->ch_bumber){
        user_io_output(io->select_por[0],0);
        user_io_high_impedance(io->select_por[1]);
    }else if(2 == io->ch_bumber){
        user_io_output(io->select_por[1],1);
        user_io_high_impedance(io->select_por[0]);
    }else if(3 == io->ch_bumber){
        user_io_output(io->select_por[1],0);
        user_io_high_impedance(io->select_por[0]);
    }
 
}

void user_4ad_check(void *io){
    IO3_AD4 *io_p = (IO3_AD4 *)io;

    io_p->check_id = 0;
    if(!io_p || NO_CONFIG_PORT == io_p->ch || NO_CONFIG_PORT == io_p->por || !io_p->init_ok){
        puts("ad check io error\n");
        io_p->vol[0]= io_p->vol[1]= io_p->vol[2] = -1;
        return;
    }
    io_p->vol[io_p->ch_bumber++] = adc_get_value(io_p->ch);

    user_4ad_io_init(io_p);

    if(io_p->callback){
        io_p->callback(io_p->vol);
    }

    io_p->check_id = sys_hi_timeout_add(io_p,user_4ad_check,io_p->check_time);
    return;
}

void user_4ad_init(IO3_AD4 * io){
    if(!io || NO_CONFIG_PORT == io->ch || NO_CONFIG_PORT == io->por || NO_CONFIG_PORT == io->select_por[0] || NO_CONFIG_PORT == io->select_por[1]){
        puts("ad init error\n");
        io->init_ok = 0;
        io->vol[0]= io->vol[1]= io->vol[2] = -1;
        return ;
    }
    // extern u32 adc_add_sample_ch(u32 ch);
    adc_add_sample_ch(io->ch);          //注意：初始化AD_KEY之前，先初始化ADC


    gpio_set_die(io->por, 0);
    gpio_set_direction(io->por, 1);
    gpio_set_pull_down(io->por, 0);
    gpio_set_pull_up(io->por, io->up?1:0);

    user_4ad_io_init(io);    

    io->init_ok = 1;

    io->check_id =  sys_hi_timeout_add(io,user_4ad_check,io->check_time);
}

void user_4ad_check_init(void (* callback)(u32 *vol)){
    #if (defined(USER_3IO_CHECK_4AD_EN) && USER_3IO_CHECK_4AD_EN)
    if(callback){
        user_4ad_io.callback = callback;
        user_4ad_init(&user_4ad_io);
    }
    #endif
}

