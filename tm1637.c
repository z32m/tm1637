#include "tm1637.h"
#include <string.h>

void tm_bit_delay()
{
    if (TM_TICK)
        k_sleep(K_TICKS(TM_TICK));
}

uint8_t tm_symbols[SYM_EMPTY] = {
    1 + 2 + 4 + 8 + 16 + 32,  // SYM_0
    2 + 4,                    // SYM_1
    1 + 2 + 64 + 16 + 8,      // SYM_2
    1 + 2 + 64 + 4 + 8,       // SYM_3
    32 + 64 + 2 + 4,          // SYM_4
    1 + 32 + 64 + 4 + 8,      // SYM_5
    1 + 32 + 64 + 4 + 8 + 16, // SYM_6
    1 + 2 + 4,                // SYM_7,
    127,                      // SYM_8,
    127 - 16,                 // SYM_9,
    127 - 8,                  // SYM_A,
    127 - 2 - 1,              // SYM_B
    127 - 64 - 2 - 4,         // SYM_C,
    127 - 32 - 1,             // SYM_D,
    127 - 2 - 4,              // SYM_E,
    127 - 2 - 4 - 8,          // SYM_F
};

bit_pos_t tm_snake_frames[SNAKE_FRAMES] = {
    {.pos = 0, .bit = 3},
    {.pos = 0, .bit = 4},
    {.pos = 0, .bit = 5},
    {.pos = 0, .bit = 0},
    {.pos = 1, .bit = 0},
    {.pos = 2, .bit = 0},
    {.pos = 3, .bit = 0},
    {.pos = 3, .bit = 1},
    {.pos = 3, .bit = 2},
    {.pos = 3, .bit = 3},
    {.pos = 2, .bit = 3},
    {.pos = 1, .bit = 3},
};

bit_pos_t tm_zigzag_frames[ZIGZAG_FRAMES] = {
    {.pos = 0, .bit = 4},
    {.pos = 0, .bit = 5},
    {.pos = 0, .bit = 0},
    {.pos = 1, .bit = 5},
    {.pos = 1, .bit = 4},
    {.pos = 1, .bit = 3},
    {.pos = 2, .bit = 4},
    {.pos = 2, .bit = 5},
    {.pos = 2, .bit = 0},
    {.pos = 3, .bit = 5},
    {.pos = 3, .bit = 4},
    {.pos = 3, .bit = 3},
    {.pos = 3, .bit = 2},
    {.pos = 3, .bit = 1},
    {.pos = 3, .bit = 0},
    {.pos = 2, .bit = 1},
    {.pos = 2, .bit = 2},
    {.pos = 2, .bit = 3},
    {.pos = 1, .bit = 2},
    {.pos = 1, .bit = 1},
    {.pos = 1, .bit = 0},
    {.pos = 0, .bit = 1},
    {.pos = 0, .bit = 2},
    {.pos = 0, .bit = 3},
};

void tm_init(tm_conn_t *tm)
{
    gpio_pin_configure_dt(tm->clk, GPIO_OUTPUT_LOW);
    gpio_pin_configure_dt(tm->dio, GPIO_OUTPUT_LOW);
}

tm_conn_t tm_make(const struct gpio_dt_spec *dio, const struct gpio_dt_spec *clk)
{
    return (tm_conn_t){
        .dio = dio,
        .clk = clk};
}

void tm_start(tm_conn_t *tm)
{
    // When CLK is a high level and DIO changes from high to low level, data input starts.
    gpio_pin_set_dt(tm->clk, 1);
    gpio_pin_set_dt(tm->dio, 1);
    tm_bit_delay();
    gpio_pin_set_dt(tm->dio, 0);
    tm_bit_delay();
}

void tm_stop(tm_conn_t *tm)
{
    // When CLK is a high level and DIO changes from low level to high level, data input ends.
    gpio_pin_set_dt(tm->clk, 1);
    gpio_pin_set_dt(tm->dio, 0);
    tm_bit_delay();
    gpio_pin_set_dt(tm->dio, 1);
    tm_bit_delay();
}

int tm_send_byte(tm_conn_t *tm, uint8_t data)
{
    uint8_t i, v = data;
    for (i = 0; i < 8; i++, v >>= 1)
    {
        gpio_pin_set_dt(tm->clk, 0);
        gpio_pin_set_dt(tm->dio, v & 1);
        tm_bit_delay();
        gpio_pin_set_dt(tm->clk, 1);
        tm_bit_delay();
    }

    // ack
    // ACK is generated inside the chip to lower the DIO pin at the failing edge of the 8th clock
    gpio_pin_set_dt(tm->clk, 0);
    tm_bit_delay();

    gpio_pin_configure_dt(tm->dio, GPIO_INPUT);

    gpio_pin_set_dt(tm->clk, 1);
    tm_bit_delay();

    int ack = gpio_pin_get_dt(tm->dio);
    gpio_pin_configure_dt(tm->dio, GPIO_OUTPUT_LOW);

    gpio_pin_set_dt(tm->clk, 0);
    tm_bit_delay();

    return ack;
}

void tm_set_state(tm_conn_t *tm, uint8_t *data)
{
    tm_start(tm);
    tm_send_byte(tm, TM_CMD_WRITE);
    tm_stop(tm);

    tm_start(tm);
    tm_send_byte(tm, TM_CMD_ADDR);

    int i;
    for (i = 0; i < 5; i++)
    {
        tm_send_byte(tm, *(data + i));
    }
    tm_stop(tm);
}

void tm_set_brightnes(tm_conn_t *tm, tm_brightness_t brightness)
{
    tm_start(tm);
    tm_send_byte(tm, TM_CMD_BRIGHTNESS | brightness);
    tm_stop(tm);
}

void tm_play_snake(tm_conn_t *tm, snake_t *snake, tm_brightness_t brightness, struct k_sem *sem)
{
    int i = 0, j;
    uint8_t state[5];

    while (1)
    {
        if (k_sem_take(sem, K_MSEC(1)) != 0)
            continue;
            
        k_sem_give(sem);

        if (i == snake->frames)
            i = 0;

        memset(state, 0, sizeof(state));

        for (j = 0; j < snake->tail_length; j++)
        {
            int f = i - j < 0 ? (snake->frames + (i - j)) : i - j;
            bit_pos_t *r = &snake->variant[f];
            state[r->pos] |= 1 << r->bit;
            tm_set_state(tm, state);
        }

        tm_set_brightnes(tm, tm_bright_full);

        i++;
        k_msleep(40);
    }
}
