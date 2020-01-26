#include "accessory.h"

#include <assert.h>
#include <espressif/esp_wifi.h>
#include <homekit/characteristics.h>
#include <homekit/homekit.h>
#include <stdio.h>

#include "machine.h"
#include "timed_latch.h"
#include "transitions.h"

static uint8_t
accessory_target_state_from_state(machine_state_t machine_state) {
  const char state_map[] = {
      [MACHINE_STATE_OPEN] = MACHINE_STATE_OPEN,
      [MACHINE_STATE_CLOSED] = MACHINE_STATE_CLOSED,
      [MACHINE_STATE_OPENING] = MACHINE_STATE_OPEN,
      [MACHINE_STATE_CLOSING] = MACHINE_STATE_CLOSED,
      [MACHINE_STATE_STOPPED] = MACHINE_STATE_UNKNOWN,
      [MACHINE_STATE_UNKNOWN] = MACHINE_STATE_UNKNOWN,
  };

  return state_map[machine_state];
}

static homekit_value_t accessory_get_current_state() {
  machine_state_t current_state = machine_current_state();
  printf("Returning current door state '%s'.\n",
         machine_state_description(current_state));

  return HOMEKIT_UINT8(current_state);
}

static homekit_value_t accessory_get_target_state() {
  machine_state_t target_state =
      accessory_target_state_from_state(machine_current_state());

  printf("Returning target door state '%s'.\n",
         machine_state_description(target_state));

  return HOMEKIT_UINT8(target_state);
}

static void accessory_set_target_state(homekit_value_t new_value) {
  if (new_value.format != homekit_format_uint8) {
    printf("Invalid value format: %d\n", new_value.format);
    return;
  }

  machine_state_t current_state = machine_current_state();

  if (current_state == new_value.int_value) {
    printf("accessory_set_target_state() ignored: target state == current "
           "state (%s)\n",
           machine_state_description(current_state));
    return;
  }

  timed_latch_trigger();
}

static void accessory_identify() {
  // TODO: Implement identify function
  printf("Identify\n");
}

static homekit_value_t accessory_get_obstruction() {
  // TODO: Implement obstruction sensor
  return HOMEKIT_BOOL(false);
}

static void accessory_handle_transition(machine_state_t new_state) {
  accessory_notify_state();
}

static homekit_server_config_t g_accessory_config = {
    .password = "",
    .accessories = (homekit_accessory_t *[]){
        HOMEKIT_ACCESSORY(
                .id = 1, .category = homekit_accessory_category_garage,
                .services =
                    (homekit_service_t *[]){
                        HOMEKIT_SERVICE(
                            ACCESSORY_INFORMATION,
                            .characteristics =
                                (homekit_characteristic_t *[]){
                                    HOMEKIT_CHARACTERISTIC(NAME, ""),
                                    HOMEKIT_CHARACTERISTIC(MANUFACTURER, ""),
                                    // TODO: use mac address when no serial
                                    // provided
                                    HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, ""),
                                    HOMEKIT_CHARACTERISTIC(MODEL, ""),
                                    HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION,
                                                           ""),
                                    HOMEKIT_CHARACTERISTIC(IDENTIFY,
                                                           accessory_identify),
                                    NULL}),
                        HOMEKIT_SERVICE(
                            GARAGE_DOOR_OPENER, .primary = true,
                            .characteristics =
                                (homekit_characteristic_t *[]){
                                    HOMEKIT_CHARACTERISTIC(NAME, ""),
                                    HOMEKIT_CHARACTERISTIC(
                                        CURRENT_DOOR_STATE,
                                        MACHINE_STATE_UNKNOWN,
                                        .getter = accessory_get_current_state,
                                        .setter = NULL),
                                    HOMEKIT_CHARACTERISTIC(
                                        TARGET_DOOR_STATE,
                                        MACHINE_STATE_UNKNOWN,
                                        .getter = accessory_get_target_state,
                                        .setter = accessory_set_target_state),
                                    HOMEKIT_CHARACTERISTIC(
                                        OBSTRUCTION_DETECTED, false,
                                        .getter = accessory_get_obstruction,
                                        .setter = NULL),
                                    NULL}),
                        NULL}),
        NULL}};

void accessory_init(const accessory_config_t *config,
                    const machine_state_t initial_state) {
  // TODO: Assert required config fields
  // uint8_t mac_addr[6];
  // char mac_string[18];
  // sdk_wifi_get_macaddr(STATION_IF, macaddr);
  // snprintf(mac_string, sizeof(mac_string), "%02x:%02x:%02x:%02x:%02x:%02x",
  //          mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4],
  //          mac_addr[5]);

  machine_init(g_state_transition_matrix, initial_state,
               accessory_handle_transition);

  g_accessory_config.password = config->password;
  homekit_accessory_t *accessory = g_accessory_config.accessories[0];
  homekit_service_t *information_service = accessory->services[0];
  homekit_service_t *garage_door_service = accessory->services[0];

  information_service->characteristics[0]->value = HOMEKIT_STRING(config->name);
  information_service->characteristics[1]->value =
      HOMEKIT_STRING(config->manufacturer);
  information_service->characteristics[2]->value =
      HOMEKIT_STRING(config->serial_number);
  information_service->characteristics[3]->value =
      HOMEKIT_STRING(config->model);
  information_service->characteristics[4]->value =
      HOMEKIT_STRING(config->version);

  garage_door_service->characteristics[0]->value = HOMEKIT_STRING(config->name);
  garage_door_service->characteristics[1]->value = HOMEKIT_UINT8(initial_state);
  garage_door_service->characteristics[2]->value = HOMEKIT_UINT8(initial_state);

  homekit_server_init(&g_accessory_config);
}

void accessory_notify_state() {
  machine_state_t current_state = machine_current_state();
  machine_state_t target_state =
      accessory_target_state_from_state(current_state);

  homekit_accessory_t *accessories = g_accessory_config.accessories[0];
  homekit_service_t *service = accessories->services[1];
  homekit_characteristic_t *current = service->characteristics[1];
  homekit_characteristic_t *target = service->characteristics[2];

  assert(current);
  assert(target);

  if (current_state != target_state) {
    printf("Notifying homekit that current door state is '%s' with a target of "
           "'%s'\n",
           machine_state_description(current_state),
           machine_state_description(target_state));
  } else {
    printf("Notifying homekit that target and current door state is '%s'\n",
           machine_state_description(current_state));
  }

  homekit_characteristic_notify(current, HOMEKIT_UINT8(current_state));
  homekit_characteristic_notify(target, HOMEKIT_UINT8(target_state));
}
