#include "user_fun_cfg.h"


#if USER_LED_EN
LED_IO user_led_io={
    .por = USER_LED_POR,
    .on  = 1,//高亮

    .status = 0xff,
    .init_ok = 0,
};
#endif

#if (USER_LED_EN && USER_RGB_EN && (USER_LED_POR == IO_PORTB_07))
#error 请确保 led与rgb 使用的io不为同一个io
#endif

int user_led_io_fun(void *priv,u8 cmd){
    LED_IO *io = (LED_IO *)priv;
    if(!io){
        return -1;
    }

#if USER_LED_EN
    // LED_IO *io_p = io;

    if(!io || io->por == NO_CONFIG_PORT){
        puts("led io strl error\n");
        return -1;
    }

    if(!io->init_ok && cmd != LED_IO_INIT){
        user_led_io_fun(io,LED_IO_INIT);
    }

    if(LED_IO_INIT == cmd){
        user_print("LED_IO_INIT \n");
        io->init_ok = 1;
        user_led_io_fun(io,LED_POWER_OFF);
    }else if(LED_POWER_ON == cmd){
        user_print("LED_POWER_ON \n");        
        user_attr_gpio_set(io->por,io->on?USER_GPIO_OUT_H:USER_GPIO_OUT_L,0);
        io->status = cmd;
    }else if(LED_POWER_OFF == cmd){
        user_print("LED_POWER_OFF \n");
        user_attr_gpio_set(io->por,io->on?USER_GPIO_OUT_L:USER_GPIO_OUT_H,0);
        io->status = cmd;
    }else if(LED_IO_FLIP == cmd){
        user_print("LED_IO_FLIP \n");
        // io->status = (LED_POWER_OFF == io->status)?LED_POWER_ON:LED_POWER_OFF;
        if(LED_POWER_OFF == io->status){
            user_led_io_fun(io,LED_POWER_ON);
        }else{
            user_led_io_fun(io,LED_POWER_OFF);
        }
    }//LED_IO_STATUS
#endif
    return io->status;
}
