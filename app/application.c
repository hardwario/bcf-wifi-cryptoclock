#include <application.h>

#define WIFI_SSID "my_wifi_ap"
#define WIFI_PASSWORD "pwd1234"
#define TIMEZONE 2
#define HTTP_HOST "api.grames.cz"
#define HTTP_PATH "/bitcoin-price-usd"
#define REFRESH_DATA_INTERVAL (60 * 60 * 1000)
#define CLOCK_12HOUR 1

// LED instance
bc_led_t led;
// WiFi module instance
bc_esp8266_t esp8266;

char btc_price[10];

void esp8266_event_handler(bc_esp8266_t *self, bc_esp8266_event_t event, void *event_param)
{
    if (event == BC_ESP8266_EVENT_WIFI_CONNECT_SUCCESS)
    {
        bc_led_pulse(&led, 500);

        bc_esp8266_tcp_connect(&esp8266, HTTP_HOST, 80);
    }
    else if (event == BC_ESP8266_EVENT_SOCKET_CONNECT_SUCCESS)
    {
        char data[100];
        sprintf(data,
            "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n\r\n",
            HTTP_PATH, HTTP_HOST);

        bc_esp8266_send_data(&esp8266, data, strlen(data));
    }
    else if (event == BC_ESP8266_EVENT_DATA_RECEIVED)
    {
        uint32_t length = bc_esp8266_get_received_message_length(&esp8266);
        char *buffer = malloc(length + 1);
        bc_esp8266_get_received_message_data(&esp8266, (uint8_t *) buffer, length + 1);
        buffer[length] = '\0';

        char *data = strstr(buffer, "\r\n\r\n");
        data += 4;
        length = strlen(data);
        strncpy(btc_price, data, 9);

        free(buffer);

        bc_esp8266_disconnect(&esp8266);
    }
}

static void refresh_data_event_handler(void *param)
{
    bc_esp8266_connect(&esp8266);

    bc_scheduler_plan_current_relative(REFRESH_DATA_INTERVAL);
}

void application_init()
{
    btc_price[0] = '\0';

    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);

    // Initialize WiFi module
    bc_esp8266_init(&esp8266, BC_UART_UART1);
    bc_esp8266_set_event_handler(&esp8266, esp8266_event_handler, NULL);
    bc_esp8266_set_station_mode(&esp8266, WIFI_SSID, WIFI_PASSWORD);
    bc_esp8266_set_sntp(&esp8266, TIMEZONE);

    bc_module_lcd_init();

    bc_scheduler_register(refresh_data_event_handler, NULL, 100);
}

void application_task(void)
{
    if (!bc_module_lcd_is_ready())
    {
        bc_scheduler_plan_current_relative(500);
        return;
    }

    bc_system_pll_enable();

    bc_module_lcd_clear();

    char str[32];
    bc_rtc_t datetime;
    bc_rtc_get_date_time(&datetime);

    snprintf(str, sizeof(str), "%d. %d.", datetime.date, datetime.month);
    bc_module_lcd_set_font(&bc_font_ubuntu_28);
    bc_module_lcd_draw_string(10, 10, str, true);

    uint8_t hours = datetime.hours;
    if (CLOCK_12HOUR)
    {
        hours = datetime.hours % 12;
        if (hours == 0)
        {
            hours = 12;
        }

        snprintf(str, sizeof(str), "%s", datetime.hours / 12 == 0 ? "AM" : "PM");
        bc_module_lcd_set_font(&bc_font_ubuntu_15);
        bc_module_lcd_draw_string(95, 50, str, true);
    }

    snprintf(str, sizeof(str), "%d:%02d", hours, datetime.minutes);
    bc_module_lcd_set_font(&bc_font_ubuntu_28);
    if (hours < 10)
    {
        bc_module_lcd_draw_string(26, 50, str, true);
    }
    else
    {
        bc_module_lcd_draw_string(10, 50, str, true);
    }

    snprintf(str, sizeof(str), "$%s", btc_price);
    bc_module_lcd_set_font(&bc_font_ubuntu_28);
    bc_module_lcd_draw_string(10, 90, str, true);

    bc_system_pll_disable();

    bc_module_lcd_update();

    bc_scheduler_plan_current_relative(1000);
}
