/*
   SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>

#include "httpd.h"
#include "led.h"


void app_main(void)
{
    printf("SDK version:%s\n", esp_get_idf_version());

    ESP_ERROR_CHECK(nvs_flash_init());

    led_init();

    webserver_start();

}
