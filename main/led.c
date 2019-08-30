/*
   SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/pwm.h"
#include "esp_log.h"
#include "led.h"

/* Really? Is this actually necessary? Sigh */
#ifndef ARRAY_SIZE
# define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define MAX_DUTY    (1000)

static const char TAG[] = "pwm";

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

#if 0
static void led_test_task(void *arg)
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
#endif

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
        /* 6 to 11 are used by the SPI. Cannot use them */
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

/* Current task */
static TaskHandle_t led_task = NULL;
static void initial_led_task(void * arg)
{
    /* Blink! */
    for(int i = 0; i < 2000/(100*2); i++)
    {
        led_all_on();
        vTaskDelay(100 / portTICK_RATE_MS);
        led_all_off();
        vTaskDelay(100 / portTICK_RATE_MS);
    }
    vTaskSuspend(NULL);
}

static void building_task(void *arg)
{
    const uint32_t delay = 15 / portTICK_RATE_MS;
    while(1)
    {
        //pwm_stop(~0);
        //led_all_off();
        vTaskDelay(1000 / portTICK_RATE_MS);
        int duty = 0;
        /* Glow-in */
        while(duty < MAX_DUTY)
        {
            pwm_set_duty(0, duty);
            pwm_set_duty(1, duty);
            pwm_start();
            vTaskDelay(delay);
            duty += 10;
        }
        vTaskDelay(500 / portTICK_RATE_MS);

        /* Glow-out */
        while(duty > 0)
        {
            pwm_set_duty(0, duty);
            pwm_set_duty(1, duty);
            pwm_start();
            vTaskDelay(delay);
            duty -= 10;
        }
    }
}

static void failure_task(void *arg)
{
    led_set_mask(1<<FRONT_RED | 1<<REAR_RED);
    vTaskSuspend(NULL);
}

static void succes_task(void *arg)
{
    led_set_mask(1<<FRONT_GREEN | 1<<REAR_GREEN);
    vTaskSuspend(NULL);
}

void led_set_state(enum LedState state)
{
    /* Make sure we stop the PWM */
    led_all_off();
    pwm_stop(~0);

    TaskFunction_t func;
    vTaskDelete(led_task);
    led_task = NULL;
    switch(state)
    {
        case LED_STATE_FAILURE:  func = failure_task; break;
        case LED_STATE_BUILDING: func = building_task; break;
        case LED_STATE_SUCCES:   func = succes_task; break;
        default:
                return;
    }
    xTaskCreate(func, "ledtask", configMINIMAL_STACK_SIZE * 4, NULL, 5, &led_task);
}

void led_init(void)
{
    gpio_config(&gpio_cfg);

#define pwm_size 2
    uint32_t duties[pwm_size] = {1, 1};
    const uint32_t pin_num[pwm_size] = {leds[FRONT_ORANGE], leds[REAR_ORANGE]};
    uint32_t mask = (1<<pwm_size) - 1;
    /* Init PWM */
    pwm_init(MAX_DUTY, duties, pwm_size, pin_num);
    pwm_set_phase(0, 0);
    pwm_set_phase(1, 0);
    pwm_set_channel_invert(mask);

    /* Create task to do the LED toggling */
    xTaskCreate(initial_led_task, "ledtask", configMINIMAL_STACK_SIZE * 4, NULL, 5, &led_task);
}
