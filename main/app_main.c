#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_scenes.h>
#include <esp_rmaker_ota.h>
#include <esp_rmaker_console.h>

// Change these 4 pins to match your relay board!
// Common relay boards use these pins:
#define RELAY_1_GPIO    18
#define RELAY_2_GPIO    19
#define RELAY_3_GPIO    21
#define RELAY_4_GPIO    22

// Active HIGH or Active LOW? Most cheap relay modules are Active LOW
#define RELAY_ACTIVE_LEVEL  0   // 0 = Active LOW (most common), 1 = Active HIGH

static bool relay_state[4] = {false, false, false, false};

void relay_write(int channel, bool state)
{
    relay_state[channel] = state;
    gpio_set_level(channel == 0 ? RELAY_1_GPIO :
                   channel == 1 ? RELAY_2_GPIO :
                   channel == 2 ? RELAY_3_GPIO : RELAY_4_GPIO,
                   state ? !RELAY_ACTIVE_LEVEL : RELAY_ACTIVE_LEVEL);
}

// This function is called when user toggles relay from RainMaker phone app
static esp_err_t relay_write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                                const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (strcmp(esp_rmaker_param_get_name(param), "Power") == 0) {
        int channel = (int)priv_data;  // We passed channel number as private data
        printf("Relay %d turned %s\n", channel + 1, val.val.b ? "ON" : "OFF");
        relay_write(channel, val.val.b);
    }
    esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}

void app_main()
{
    // Initialize GPIOs
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RELAY_1_GPIO) | (1ULL << RELAY_2_GPIO) |
                        (1ULL << RELAY_3_GPIO) | (1ULL << RELAY_4_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Turn all relays OFF at start
    for (int i = 0; i < 4; i++) {
        relay_write(i, false);
    }

    // Start RainMaker
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_log = true,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP32 Relay Board", "Switch");

    // Create 4 separate Switch devices (they will appear as 4 switches in the app)
    esp_rmaker_device_t *switch1 = esp_rmaker_switch_device_create("Relay 1", NULL, false);
    esp_rmaker_device_t *switch2 = esp_rmaker_switch_device_create("Relay 2", NULL, false);
    esp_rmaker_device_t *switch3 = esp_rmaker_switch_device_create("Relay 3", NULL, false);
    esp_rmaker_device_t *switch4 = esp_rmaker_switch_device_create("Relay 4", NULL, false);

    // Add callback with channel number
    esp_rmaker_device_add_cb(switch1, relay_write_cb, (void *)0);
    esp_rmaker_device_add_cb(switch2, relay_write_cb, (void *)1);
    esp_rmaker_device_add_cb(switch3, relay_write_cb, (void *)2);
    esp_rmaker_device_add_cb(switch4, relay_write_cb, (void *)3);

    // Add the switches to the node
    esp_rmaker_node_add_device(node, switch1);
    esp_rmaker_node_add_device(node, switch2);
    esp_rmaker_node_add_device(node, switch3);
    esp_rmaker_node_add_device(node, switch4);

    // Optional: Enable OTA & scheduling
    esp_rmaker_ota_enable_default();
    esp_rmaker_schedule_enable();
    esp_rmaker_scenes_enable();

    // Start RainMaker
    esp_rmaker_start();

    printf("4-Channel Relay RainMaker Ready!\n");
    printf("Open RainMaker app → Add Device → Scan QR or BLE → Control your relays!\n");
}
