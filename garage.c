#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include <assert.h>
#include <etstimer.h>
#include <esplibs/libmain.h>
#include <wifi_config.h>
#include <homekit/homekit.h>
#include <homekit/characteristics.h>

#include <toggle.h>

// Possible values for characteristic CURRENT_DOOR_STATE:
#define HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPEN 0
#define HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED 1
#define HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPENING 2
#define HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSING 3
#define HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_STOPPED 4
#define HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_UNKNOWN 255

#define HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_OPEN 0
#define HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_CLOSED 1
#define HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_UNKNOWN 255

#define OPEN_CLOSE_DURATION 23000
#define RELAY_HOLD_DURATION 100

typedef enum
{
    DOOR_OPEN,
    DOOR_BETWEEN,
    DOOR_CLOSED,
    DOOR_UNKNOWN,
} sensor_state_t;


homekit_value_t homekit_get_target_state();
void homekit_set_target_state(homekit_value_t new_value);
homekit_value_t homekit_get_current_state();
homekit_value_t homekit_get_obstruction();
void homekit_identify(homekit_value_t _value);

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(
            .id = 1,
            .category = homekit_accessory_category_garage,
            .services = (homekit_service_t *[]){
                HOMEKIT_SERVICE(
                    ACCESSORY_INFORMATION,
                    .characteristics = (homekit_characteristic_t *[]){
                        HOMEKIT_CHARACTERISTIC(NAME, "Garage Door"),
                        HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Chamberlain"),
                        // TODO: use mac address
                        HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "1"),
                        HOMEKIT_CHARACTERISTIC(MODEL, "Merlin Professional MT60P"),
                        HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "2.0"),
                        HOMEKIT_CHARACTERISTIC(IDENTIFY, homekit_identify),
                        NULL}),
                HOMEKIT_SERVICE(GARAGE_DOOR_OPENER, .primary = true, .characteristics = (homekit_characteristic_t *[]){HOMEKIT_CHARACTERISTIC(NAME, "Garage Door"), HOMEKIT_CHARACTERISTIC(CURRENT_DOOR_STATE, HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED, .getter = homekit_get_current_state, .setter = NULL), HOMEKIT_CHARACTERISTIC(TARGET_DOOR_STATE, HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_CLOSED, .getter = homekit_get_target_state, .setter = homekit_set_target_state), HOMEKIT_CHARACTERISTIC(OBSTRUCTION_DETECTED, HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_CLOSED, .getter = homekit_get_obstruction, .setter = NULL), NULL}), NULL}),
    NULL};

// #region strings
const char *current_state_description(uint8_t state)
{
    const char *description = "unknown";
    switch (state)
    {
    case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPEN:
        description = "open";
        break;
    case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPENING:
        description = "opening";
        break;
    case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED:
        description = "closed";
        break;
    case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSING:
        description = "closing";
        break;
    case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_STOPPED:
        description = "stopped";
        break;
    default:;
    }
    return description;
}

const char *sensor_state_description(sensor_state_t state)
{
    const char *description = "unknown";
    switch (state)
    {
    case DOOR_OPEN:
        description = "open";
        break;
    case DOOR_CLOSED:
        description = "closed";
        break;
    case DOOR_BETWEEN:
        description = "between sensors";
        break;
    case DOOR_UNKNOWN:
        description = "unknown";
        break;
    }
    return description;
}

const char *sensor_description(uint8_t gpio)
{
    // TODO: remove
    printf("Describing GPIO %d\n", gpio);
    const char *description = "unknown";
    switch (gpio)
    {
    case TOP_PIN:
        description = "top";
        break;
    case BOTTOM_PIN:
        description = "bottom";
        break;
    }
    return description;
}
// #endregion

// #region state
uint8_t current_door_state = HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_UNKNOWN;

// uint8_t get_current_state_safely()
// {
//     if (current_door_state == HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_UNKNOWN)
//     {
//         update_current_state_from_sensor_state();
//     }

//     return current_door_state
// }

uint8_t target_state_from_current()
{
    uint8_t result = current_door_state;
    if (result == HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPENING)
    {
        result = HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_OPEN;
    }
    else if (result == HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSING)
    {
        result = HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_CLOSED;
    }
    return result;
}

void homekit_notify_state()
{
    uint8_t target_door_state = target_state_from_current();

    homekit_accessory_t *accessory = accessories[0];
    homekit_service_t *service = accessory->services[1];
    homekit_characteristic_t *current = service->characteristics[1];
    homekit_characteristic_t *target = service->characteristics[2];

    assert(current);
    assert(target);

    if (current_door_state != target_door_state)
    {
        printf("Notifying homekit that current door state is '%s' with a target of '%s'\n", current_state_description(current_door_state), current_state_description(target_door_state));
    } else {
        printf("Notifying homekit that target and current door state is '%s'\n", current_state_description(current_door_state));
    }

    homekit_value_t current_value = HOMEKIT_UINT8(current_door_state);
    homekit_value_t target_value = HOMEKIT_UINT8(target_door_state);

    homekit_characteristic_notify(current, current_value);
    homekit_characteristic_notify(target, target_value);
}

void current_state_set(uint8_t new_state)
{
    printf("Setting current state to %s\n", current_state_description(new_state));
    if (current_door_state != new_state)
    {
        current_door_state = new_state;
        homekit_notify_state();
    }
}
// #endregion

// #region relay
bool relay_on = false;

void relay_write(bool on)
{
    gpio_write(RELAY_PIN, on);
}

homekit_value_t relay_on_get()
{
    return HOMEKIT_BOOL(relay_on);
}

void relay_on_set(homekit_value_t value)
{
    if (value.format != homekit_format_bool)
    {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    relay_on = value.bool_value;
    relay_write(relay_on);
}

void relay_init()
{
    gpio_enable(RELAY_PIN, GPIO_OUTPUT);
    relay_write(relay_on);
}
// #endregion

// #region sensor
bool top_sensor_state;
bool bottom_sensor_state;

void update_current_state_from_sensor_state()
{
    sensor_state_t door_state;
    if (top_sensor_state == false)
    {
        if (bottom_sensor_state == false)
        {
            door_state = DOOR_UNKNOWN;
        }
        else
        {
            door_state = DOOR_OPEN;
        }
    }
    else
    {
        if (bottom_sensor_state == false)
        {
            door_state = DOOR_CLOSED;
        }
        else
        {
            door_state = DOOR_BETWEEN;
        }
    }

    printf("Door appears to be %s\n", sensor_state_description(door_state));

    if (door_state == DOOR_UNKNOWN)
    {
        // TODO: handle error - sensor data invalid
        return;
    }

    switch (current_door_state)
    {
    case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPEN:
        switch (door_state)
        {
        case DOOR_OPEN:
            break;
        case DOOR_BETWEEN:
            current_state_set(HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSING);
            break;
        case DOOR_CLOSED:
            current_state_set(HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED);
            break;
        case DOOR_UNKNOWN:
            break;
        }
        break;
    case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPENING:
        switch (door_state)
        {
        case DOOR_OPEN:
            current_state_set(HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPEN);
            break;
        case DOOR_BETWEEN:
            current_state_set(HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_STOPPED);
            break;
        case DOOR_CLOSED:
            current_state_set(HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED);
            break;
        case DOOR_UNKNOWN:
            break;
        }
        break;
    case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED:
        switch (door_state)
        {
        case DOOR_OPEN:
            current_state_set(HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPEN);
            break;
        case DOOR_BETWEEN:
            current_state_set(HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPENING);
            break;
        case DOOR_CLOSED:
            break;
        case DOOR_UNKNOWN:
            break;
        }
        break;
    case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSING:
        switch (door_state)
        {
        case DOOR_OPEN:
            current_state_set(HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPEN);
            break;
        case DOOR_BETWEEN:
            current_state_set(HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_STOPPED);
            break;
        case DOOR_CLOSED:
            current_state_set(HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED);
            break;
        case DOOR_UNKNOWN:
            break;
        }
        break;
    case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_STOPPED:
        switch (door_state)
        {
        case DOOR_OPEN:
            current_state_set(HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPEN);
            break;
        case DOOR_BETWEEN:
            break;
        case DOOR_CLOSED:
            current_state_set(HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED);
            break;
        case DOOR_UNKNOWN:
            break;
        }
        break;
    case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_UNKNOWN:
        switch (door_state)
        {
        case DOOR_OPEN:
            current_state_set(HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPEN);
            break;
        case DOOR_BETWEEN:
            break;
        case DOOR_CLOSED:
            current_state_set(HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED);
            break;
        case DOOR_UNKNOWN:
            break;
        }
        break;
    }
}

static void on_sensor_change(bool high, void *context)
{
    printf("%s sensor went %s\n", sensor_description(*((uint8_t *)context)), high ? "high" : "low");
    update_current_state_from_sensor_state();
}

static void sensor_init()
{
    int result;
    uint8_t *bottom_pin = malloc(sizeof(uint8_t));
    *bottom_pin = BOTTOM_PIN;
    uint8_t *top_pin = malloc(sizeof(uint8_t));
    *top_pin = TOP_PIN;

    gpio_set_pullup(BOTTOM_PIN, true, true);
    bottom_sensor_state = gpio_read(BOTTOM_PIN);
    result = toggle_create(BOTTOM_PIN, on_sensor_change, bottom_pin);
    if (result)
    {
        printf("Failed to initialize bottom sensor\n");
    }

    gpio_set_pullup(TOP_PIN, true, true);
    top_sensor_state = gpio_read(TOP_PIN);
    result = toggle_create(TOP_PIN, on_sensor_change, top_pin);
    if (result)
    {
        printf("Failed to initialize top sensor\n");
    }
}

// #endregion

// #region timer
ETSTimer update_timer;

static void on_timer(void *arg)
{
    printf("Timer fired. Updating state from sensors.\n");
    sdk_os_timer_disarm(&update_timer);
    update_current_state_from_sensor_state();
}

static void timer_init()
{
    sdk_os_timer_disarm(&update_timer);
    sdk_os_timer_setfn(&update_timer, on_timer, NULL);
}
// #endregion

void identify_task(void *_args)
{
    // // 1. move the door, 2. stop it, 3. move it back:
    // for (int i=0; i<3; i++) {
    //         relay_write(true);
    //         vTaskDelay(500 / portTICK_PERIOD_MS);
    //         relay_write(false);
    //         vTaskDelay(3500 / portTICK_PERIOD_MS);
    // }

    // relay_write(false);

    // vTaskDelete(NULL);

    // TODO: Flash LED
}

void homekit_identify(homekit_value_t _value)
{
    printf("GDO identify\n");
    xTaskCreate(identify_task, "GDO identify", 128, NULL, 2, NULL);
}

homekit_value_t homekit_get_obstruction()
{
    return HOMEKIT_BOOL(false);
}

homekit_value_t homekit_get_current_state()
{
    printf("Returning current door state '%s'.\n", current_state_description(current_door_state));

    return HOMEKIT_UINT8(current_door_state);
}

homekit_value_t homekit_get_target_state()
{
    uint8_t target_door_state = target_state_from_current();

    printf("Returning target door state '%s'.\n", current_state_description(target_door_state));

    return HOMEKIT_UINT8(target_door_state);
}

void homekit_set_target_state(homekit_value_t new_value)
{

    if (new_value.format != homekit_format_uint8)
    {
        printf("Invalid value format: %d\n", new_value.format);
        return;
    }

    if (current_door_state == new_value.int_value)
    {
        printf("homekit_set_target_state() ignored: new target state == current state (%s)\n", current_state_description(current_door_state));
        return;
    }

    // Toggle the garage door by toggling the relay connected to the GPIO (on - off):
    // Turn ON GPIO:
    relay_write(true);
    // Wait for some time:
    vTaskDelay(RELAY_HOLD_DURATION / portTICK_PERIOD_MS);
    // Turn OFF GPIO:
    relay_write(false);

    // Wait for the garage door to open / close,
    // then update current_door_state from sensor:
    sdk_os_timer_arm(&update_timer, OPEN_CLOSE_DURATION, false);
}

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"};

void on_wifi_ready()
{
    homekit_server_init(&config);
}

void user_init(void)
{
    uart_set_baud(0, 115200);

    sensor_init();
    relay_init();
    timer_init();

    wifi_config_init("garage-door", NULL, on_wifi_ready);
}
