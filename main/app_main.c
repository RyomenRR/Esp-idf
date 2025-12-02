#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_ota.h>

#define RELAY_1_GPIO    18
#define RELAY_2_GPIO    19
#define RELAY_3_GPIO    21
#define RELAY_4_GPIO    22
#define RELAY_ACTIVE_LEVEL  0   // 0 = Active LOW (most relay boards)

static void relay_write(int ch, bool on) {
    int gpio = (ch==0)?RELAY_1_GPIO:(ch==1)?RELAY_2_GPIO:(ch==2)?RELAY_3_GPIO:RELAY_4_GPIO;
    gpio_set_level(gpio, on ? !RELAY_ACTIVE_LEVEL : RELAY_ACTIVE_LEVEL);
}

static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                          const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx) {
    if (strcmp(esp_rmaker_param_get_name(param), ESP_RMAKER_PARAM_POWER) == 0) {
        int ch = (int)priv_data;
        relay_write(ch, val.val.b);
        esp_rmaker_param_update_and_report(param, val);
    }
    return ESP_OK;
}

void app_main(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL<<RELAY_1_GPIO)|(1ULL<<RELAY_2_GPIO)|(1ULL<<RELAY_3_GPIO)|(1ULL<<RELAY_4_GPIO),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    esp_rmaker_config_t rainmaker_cfg = {.enable_time_sync = false};
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "4CH Relay", "Switch");

    for (int i = 0; i < 4; i++) {
        char name[20];
        snprintf(name, sizeof(name), "Relay %d", i+1);
        esp_rmaker_device_t *sw = esp_rmaker_switch_device_create(name, NULL, false);
        esp_rmaker_device_add_cb(sw, write_cb, (void*)i);
        esp_rmaker_node_add_device(node, sw);
    }

    esp_rmaker_ota_enable_default();
    esp_rmaker_start();
    printf("4-Channel Relay RainMaker Ready!\n");
}
