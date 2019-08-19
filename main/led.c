/*
   SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "led.h"

/* Really? Is this actually necessary? Sigh */
#ifndef ARRAY_SIZE
# define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

static const uint8_t leds[] = 
{
    GPIO_NUM_0,     /* 0 Green traffic 'front' */
    GPIO_NUM_2,     /* 1 Green traffic 'rear' */
    GPIO_NUM_3,     /* 2 Red pedestrian 'right' */
    GPIO_NUM_4,     /* 3 Orange traffic 'rear' */
    GPIO_NUM_5,     /* 4 Red traffic 'rear' */
    GPIO_NUM_12,    /* 5 Red pedestrian 'left' */
    GPIO_NUM_13,    /* 6 Red traffic 'front' */
    GPIO_NUM_14,    /* 7 Green pedestrian 'left' */
    GPIO_NUM_15,    /* 8 Orange traffic 'front' */
    GPIO_NUM_16,    /* 9 Green pedestrian 'right' */
};

static void led_all(bool on)
{
    /* TODO we can probably do this in 1 write... to 
     * GPIO.out_w1ts |= (0x1 << gpio_num);
     */
    for(size_t i = 0; i < ARRAY_SIZE(leds); i++)
    {
        gpio_set_level(leds[i], !on); //Active low, 1 = off, 0 = on
    }

}

void led_all_on(void)
{
    led_all(true);
}

void led_all_off(void)
{
    /* TODO we can probably do this in 1 write... to 
     * GPIO.out_w1ts |= (0x1 << gpio_num);
     */
    led_all(false);
}

static void led_task(void *arg)
{
    for(;;)
    {
        if(0)
        {
            for(size_t i = 0; i < ARRAY_SIZE(leds); i++)
            {
                led_all_off();
                gpio_set_level(leds[i], 0);
                vTaskDelay(1000 / portTICK_RATE_MS);
            }
            led_all_off();
            vTaskDelay(1000 / portTICK_RATE_MS);

            gpio_set_level(GPIO_NUM_0, 0);
            gpio_set_level(GPIO_NUM_2, 0);
            gpio_set_level(GPIO_NUM_3, 0);
            gpio_set_level(GPIO_NUM_4, 0);
            gpio_set_level(GPIO_NUM_5, 0);
            gpio_set_level(GPIO_NUM_12,0);
            gpio_set_level(GPIO_NUM_13,0);
            gpio_set_level(GPIO_NUM_14,0);
            gpio_set_level(GPIO_NUM_15,0);
            gpio_set_level(GPIO_NUM_16,0);
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

void led_set_mask(enum Led led_mask)
{
    led_all_off();
    for(int i = 0; led_mask ; i++)
    {
        if(led_mask & 1)
            gpio_set_level(leds[i], 0);
        led_mask >>= 1;
    }

}

void led_set(enum Led lednr)
{
    led_all_off();
    gpio_set_level(leds[lednr], 0);
}

/* Setup all the GPIO to be output */
static const gpio_config_t gpio_cfg = 
{
    .pin_bit_mask =
        (1<<GPIO_NUM_0) |
        /* GPIO 1 This is the UART TX pin. Better not use it! */
        (1<<GPIO_NUM_2) |
        (1<<GPIO_NUM_3) |
        (1<<GPIO_NUM_4) |
        (1<<GPIO_NUM_5) |
        /* 6 to 11 are used by the SPI. Cannot not use them */
        (1<<GPIO_NUM_12)|
        (1<<GPIO_NUM_13)|
        (1<<GPIO_NUM_14)|
        (1<<GPIO_NUM_15)|
        (1<<GPIO_NUM_16)|
        0,
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE 
};

void led_init(void)
{
    gpio_config(&gpio_cfg);

    /* Blink! */
    for(int i = 0; i < 2000/(100*2); i++)
    {
        led_all_on();
        vTaskDelay(100 / portTICK_RATE_MS);
        led_all_off();
        vTaskDelay(100 / portTICK_RATE_MS);
    }

    /* Create task to do the LED toggling */
    xTaskCreate(led_task, "ledtask", configMINIMAL_STACK_SIZE * 4, NULL, 5, NULL);
}
