#ifndef __tm1637_h__
#define __tm1637_h__

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

// should be used in conjunction with tm_brightness_t
#define TM_CMD_BRIGHTNESS 0b10000000
#define TM_CMD_WRITE 0b01000000
#define TM_CMD_WRITE_FIXED 0b01000100
#define TM_CMD_ADDR 0b11000000

#ifndef TM_TICK
// works fine on STM32F103, more MHZ more delay
#define TM_TICK 0 // system ticks delay
#endif

typedef enum
{
    tm_bright_off = 0,
    tm_bright_2_16 = 1,
    tm_bright_4_16 = 2,
    tm_bright_10_16 = 3,
    tm_bright_11_16 = 4,
    tm_bright_12_16 = 5,
    tm_bright_13_16 = 6,
    tm_bright_14_16 = 7,
    tm_bright_full = 8
} tm_brightness_t;

typedef enum
{
    SYM_0,
    SYM_1,
    SYM_2,
    SYM_3,
    SYM_4,
    SYM_5,
    SYM_6,
    SYM_7,
    SYM_8,
    SYM_9,
    SYM_A,
    SYM_B,
    SYM_C,
    SYM_D,
    SYM_E,
    SYM_F,
    SYM_EMPTY
} tm_symbols_t;

extern uint8_t tm_symbols[SYM_EMPTY];

typedef struct
{
    const struct gpio_dt_spec *dio;
    const struct gpio_dt_spec *clk;
} tm_conn_t;

typedef struct
{
    uint8_t pos;
    uint8_t bit;
} bit_pos_t;

typedef struct
{
    bit_pos_t *variant;
    size_t frames;
    size_t tail_length;
} snake_t;

#define SNAKE_FRAMES 12
extern bit_pos_t tm_snake_frames[SNAKE_FRAMES];

#define ZIGZAG_FRAMES 24
extern bit_pos_t tm_zigzag_frames[ZIGZAG_FRAMES];

tm_conn_t tm_make(const struct gpio_dt_spec *dio, const struct gpio_dt_spec *clk);
void tm_init(tm_conn_t *tm);

void tm_start(tm_conn_t *tm);
void tm_stop(tm_conn_t *tm);
int tm_send_byte(tm_conn_t *tm, uint8_t data);
void tm_set_state(tm_conn_t *tm, uint8_t *data);
void tm_set_brightnes(tm_conn_t *tm, tm_brightness_t brightness);
void tm_play_snake(tm_conn_t *tm, snake_t *snake, tm_brightness_t brightness, struct k_sem *sem);

#endif