/*
   SPDX-License-Identifier: GPL-3.0-or-later

   Based on the http_server example code
*/

#include <ctype.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#define LOCAL_IP_ADDR
#include <esp_log.h>
#include <esp_system.h>
//#include <nvs_flash.h>
#include <sys/param.h>

#include <esp_http_server.h>
#include "led.h"
#include "httpd.h"

/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 * The examples use simple WiFi configuration that you can set via
 * 'make menuconfig'.
 * If you'd rather not, just change the below entries to strings
 * with the config you want -
 * ie. #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD

static const char TAG[]="WEB";

static esp_err_t status_get_handler(httpd_req_t *req)
{
    char query[32]; /* We only require short queries */
    char param[16] = ""; 
    if (httpd_req_get_url_query_str(req, query, sizeof(query) - 1) == ESP_OK) {
        if (httpd_query_key_value(query, "s", param, sizeof(param)) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query parameter => s=%s", param);
        }
    }

    httpd_resp_send_chunk(req, 
        "<html>"
        "<head>"
        "<style>"
        ".button {"
        "border: none;"
        "padding: 40px;"
        "border-radius: 50%;"
        "color: white;"
        "font-size: 16px;"
        "text-align: center;"
        "}"
        ".btn_failure  {background-color: #FF6666}"
        ".btn_building {background-color: #FF9933}"
        ".btn_succes   {background-color: #008000}"
        "</style>"
        "</head>"
        "<body>"
        "These are the states:</br>"
        "<button class='button btn_failure'>Failure</button></br>"
        "<button class='button btn_building'>Building</button></br>"
        "<button class='button btn_succes'>Succes</button></br>"
        "<a href='/?s=failure'>Failure</a></br>"
        "<a href='/?s=building'>Building</a></br>"
        "<a href='/?s=succes'>Succes</a></br>",
        -1);

    if(0)
    {
        httpd_resp_send_chunk(req, 
        "<a href='/?s=0'> Led 0 </a></br>"
        "<a href='/?s=1'> Led 1 </a></br>"
        "<a href='/?s=2'> Led 2 </a></br>"
        "<a href='/?s=3'> Led 3 </a></br>"
        "<a href='/?s=4'> Led 4 </a></br>"
        "<a href='/?s=5'> Led 5 </a></br>"
        "<a href='/?s=6'> Led 6 </a></br>"
        "<a href='/?s=7'> Led 7 </a></br>"
        "<a href='/?s=8'> Led 8 </a></br>"
        "<a href='/?s=9'> Led 9 </a></br>",
        -1);

        int digit = param[0];
        if(isdigit(digit))
        {
            led_set(digit - '0');
        }
    }
    else {
        if(strlen(param) > 0)
        {
            char resp_str[60];
            size_t len = snprintf(resp_str, sizeof(resp_str),
                    "Current mode = %s</br>", param);
            httpd_resp_send_chunk(req, resp_str, len);
            if(strncmp(param, "succes", sizeof(param)) == 0)
            {
                led_set_mask(1<<FRONT_GREEN | 1<<REAR_GREEN);
            }
            else if(strncmp(param, "failure", sizeof(param)) == 0)
            {
                led_set_mask(1<<FRONT_RED | 1<<REAR_RED);
            }
            else if(strncmp(param, "building", sizeof(param)) == 0)
            {
                led_set_mask(1<<FRONT_ORANGE | 1<<REAR_ORANGE);
            }
        }
    }

    /* Finish up */
    httpd_resp_send_chunk(req, 
            "</body>"
            "</html>"
            , -1);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t stoplicht_status = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = status_get_handler,
};

static httpd_handle_t start_webserver(void)
{
    static const httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &stoplicht_status);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    static httpd_handle_t server = NULL;
    /* For accessing reason codes in case of disconnection */
    system_event_info_t *info = &event->event_info;

    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
        ESP_ERROR_CHECK(esp_wifi_connect());
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        ESP_LOGI(TAG, "Got IP: '%s'",
                ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));

        /* Start the web server */
        if (server == NULL) {
            server = start_webserver();
        }
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
        ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);
        if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
            /*Switch to 802.11 bgn mode */
            esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCAL_11B | WIFI_PROTOCAL_11G | WIFI_PROTOCAL_11N);
        }
        ESP_ERROR_CHECK(esp_wifi_connect());

        /* Stop the web server */
        if (server) {
            stop_webserver(server);
            server = NULL;
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void *arg)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    static const wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}


void webserver_start(void)
{
    initialise_wifi(NULL);
}
