#include <application.h>

#define WIFI_SSID "my_wifi_ap"
#define WIFI_PASSWORD "pwd1234"
#define TIMEZONE 2
#define HTTP_HOST "api.grames.cz"
#define HTTP_PATH "/bitcoin-price-usd"
#define REFRESH_DATA_INTERVAL (60 * 60 * 1000)
#define CLOCK_12HOUR 1

// LED instance
bc_led_t led_lcd_green;
// WiFi Module instance
bc_esp8266_t esp8266;
bc_scheduler_task_id_t refresh_data_task;
int battery_percentage = 0;
int btc_price = 0;
uint8_t last_minute = 60;

void esp8266_event_handler(bc_esp8266_t *self, bc_esp8266_event_t event, void *event_param)
{
    if (event == BC_ESP8266_EVENT_WIFI_CONNECT_SUCCESS)
    {
        // Connect to remote server
        bc_led_pulse(&led_lcd_green, 200);

        bc_esp8266_tcp_connect(&esp8266, HTTP_HOST, 80);
    }
    else if (event == BC_ESP8266_EVENT_SOCKET_CONNECT_SUCCESS)
    {
        // Send HTTP request
        char data[100];
        sprintf(data,
            "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n\r\n",
            HTTP_PATH, HTTP_HOST);

        bc_esp8266_send_data(&esp8266, data, strlen(data));
    }
    else if (event == BC_ESP8266_EVENT_DATA_RECEIVED)
    {
        // Read response
        uint32_t length = bc_esp8266_get_received_message_length(&esp8266);
        char *buffer = malloc(length + 1);
        bc_esp8266_get_received_message_data(&esp8266, (uint8_t *) buffer, length + 1);
        buffer[length] = '\0';

        // Skip HTTP header and copy data
        char *data = strstr(buffer, "\r\n\r\n");
        data += 4;
        btc_price = atoi(data);

        free(buffer);

        // Disconnect and turn off WiFi Module
        bc_esp8266_disconnect(&esp8266);
    }
}

static void refresh_data_event_handler(void *param)
{
    // Turn on WiFi Module and connect to WiFi
    bc_esp8266_connect(&esp8266);

    bc_scheduler_plan_current_relative(REFRESH_DATA_INTERVAL);
}

void battery_module_event_handler(bc_module_battery_event_t event, void *event_param)
{
    if (event == BC_MODULE_BATTERY_EVENT_UPDATE)
    {
        bc_module_battery_get_charge_level(&battery_percentage);
    }
}

void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    if (event == BC_BUTTON_EVENT_PRESS)
    {
        bc_scheduler_plan_now(refresh_data_task);
    }
}

void application_init()
{
    // Initialize WiFi Module
    bc_esp8266_init(&esp8266, BC_UART_UART1);
    bc_esp8266_set_event_handler(&esp8266, esp8266_event_handler, NULL);
    bc_esp8266_set_station_mode(&esp8266, WIFI_SSID, WIFI_PASSWORD);
    bc_esp8266_set_sntp(&esp8266, TIMEZONE);

    // Initialize LCD Module
    bc_module_lcd_init();
    bc_led_init_virtual(&led_lcd_green, BC_MODULE_LCD_LED_GREEN, bc_module_lcd_get_led_driver(), true);

    // Initialize button on LCD/Core Module
    static bc_button_t button;
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize Battery Module
    bc_module_battery_init();
    bc_module_battery_set_event_handler(battery_module_event_handler, NULL);
    bc_module_battery_set_update_interval(REFRESH_DATA_INTERVAL);

    refresh_data_task = bc_scheduler_register(refresh_data_event_handler, NULL, 100);
}

void application_task(void)
{
    if (!bc_module_lcd_is_ready())
    {
        bc_scheduler_plan_current_relative(500);
        return;
    }

    bc_rtc_t datetime;
    bc_rtc_get_date_time(&datetime);
    if (datetime.minutes == last_minute)
    {
        bc_scheduler_plan_current_relative(1000);
        return;
    }
    last_minute = datetime.minutes;

    bc_system_pll_enable();

    bc_module_lcd_clear();

    char str[32];

    // Draw date and month
    snprintf(str, sizeof(str), "%d. %d.", datetime.date, datetime.month);
    bc_module_lcd_set_font(&bc_font_ubuntu_28);
    bc_module_lcd_draw_string(10, 10, str, true);

    // Draw battery percentage
    snprintf(str, sizeof(str), "%d%%", battery_percentage);
    bc_module_lcd_set_font(&bc_font_ubuntu_15);
    bc_module_lcd_draw_string(90, 11, str, true);

    // Draw AM/PM
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

    // Draw hours and minutes
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

    // Draw BTC price
    snprintf(str, sizeof(str), "$%d", btc_price);
    bc_module_lcd_set_font(&bc_font_ubuntu_28);
    bc_module_lcd_draw_string(10, 90, str, true);

    bc_system_pll_disable();

    bc_module_lcd_update();

    bc_scheduler_plan_current_relative(1000);
}
