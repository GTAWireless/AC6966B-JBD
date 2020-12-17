#ifndef _USER_RGB_DEV_H_
#define _USER_RGB_DEV_H_

#define USER_RGB_CODE0  0X40
#define USER_RGB_CODE1  0X70

#define USER_RGB_BUFF_MALLOC_EN 0//rgb、spi buff是否为动态分配空间
typedef struct _USER_RGB_COLOUR_{
    u8 g;
    u8 r;
    u8 b;
}RGB_COLOUR;

typedef struct _USER_RGB_SPI_DATA_COLOUR_{
    u8 g[8];
    u8 r[8];
    u8 b[8];
}SPI_COLOUR;

typedef struct _USER_RGB_CODE_{
    u8 code_0;
    u8 code_1;
}RGB_CODE;

typedef struct _USER_RGB_INFO_{
    u16 time_id;
    bool power_off;
    bool init_flag;
    bool rend_flag;
    bool updata_flag;//灯颜色更新标志
    u8   updata_only;//只更新一次数据
    RGB_CODE code;
    u8 spi_port;//spi
    u16 number;//灯总颗数
    u16 spi_scan_time;//spi 数据更新并发送间隔时间
    
    #if USER_RGB_BUFF_MALLOC_EN
    RGB_COLOUR *rgb_buff;//[USER_RGB_NUMBER];//灯颜色 存储buff
    SPI_COLOUR *spi_buff;//[USER_RGB_NUMBER] __attribute__((aligned(4)));//spi 数据buff
    #else
    RGB_COLOUR rgb_buff[USER_RGB_NUMBER];//灯颜色 存储buff
    SPI_COLOUR spi_buff[USER_RGB_NUMBER] __attribute__((aligned(4)));//spi 数据buff
    #endif
}RGB_INFO;

void user_rgb_init(void *priv);
void user_rgb_power_off(void *priv);
//设置单颗灯颜色
void user_rgb_colour_only_set(void *priv,RGB_COLOUR *colour,s16 number);
//设置为同样颜色
void user_rgb_same_colour(void *priv,RGB_COLOUR *colour);
//清除颜色
void user_rgb_clear_colour(void *priv);
//注销定时器
void user_rgb_del(void *priv);
#endif