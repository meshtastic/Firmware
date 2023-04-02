/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.7 */

#ifndef PB_MESHTASTIC_MESHTASTIC_LOCALONLY_PB_H_INCLUDED
#define PB_MESHTASTIC_MESHTASTIC_LOCALONLY_PB_H_INCLUDED
#include <pb.h>
#include "meshtastic/config.pb.h"
#include "meshtastic/module_config.pb.h"

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Struct definitions */
typedef struct _meshtastic_LocalConfig {
    /* The part of the config that is specific to the Device */
    bool has_device;
    meshtastic_Config_DeviceConfig device;
    /* The part of the config that is specific to the GPS Position */
    bool has_position;
    meshtastic_Config_PositionConfig position;
    /* The part of the config that is specific to the Power settings */
    bool has_power;
    meshtastic_Config_PowerConfig power;
    /* The part of the config that is specific to the Wifi Settings */
    bool has_network;
    meshtastic_Config_NetworkConfig network;
    /* The part of the config that is specific to the Display */
    bool has_display;
    meshtastic_Config_DisplayConfig display;
    /* The part of the config that is specific to the Lora Radio */
    bool has_lora;
    meshtastic_Config_LoRaConfig lora;
    /* The part of the config that is specific to the Bluetooth settings */
    bool has_bluetooth;
    meshtastic_Config_BluetoothConfig bluetooth;
    /* A version integer used to invalidate old save files when we make
 incompatible changes This integer is set at build time and is private to
 NodeDB.cpp in the device code. */
    uint32_t version;
} meshtastic_LocalConfig;

typedef struct _meshtastic_LocalModuleConfig {
    /* The part of the config that is specific to the MQTT module */
    bool has_mqtt;
    meshtastic_ModuleConfig_MQTTConfig mqtt;
    /* The part of the config that is specific to the Serial module */
    bool has_serial;
    meshtastic_ModuleConfig_SerialConfig serial;
    /* The part of the config that is specific to the ExternalNotification module */
    bool has_external_notification;
    meshtastic_ModuleConfig_ExternalNotificationConfig external_notification;
    /* The part of the config that is specific to the Store & Forward module */
    bool has_store_forward;
    meshtastic_ModuleConfig_StoreForwardConfig store_forward;
    /* The part of the config that is specific to the RangeTest module */
    bool has_range_test;
    meshtastic_ModuleConfig_RangeTestConfig range_test;
    /* The part of the config that is specific to the Telemetry module */
    bool has_telemetry;
    meshtastic_ModuleConfig_TelemetryConfig telemetry;
    /* The part of the config that is specific to the Canned Message module */
    bool has_canned_message;
    meshtastic_ModuleConfig_CannedMessageConfig canned_message;
    /* A version integer used to invalidate old save files when we make
 incompatible changes This integer is set at build time and is private to
 NodeDB.cpp in the device code. */
    uint32_t version;
    /* The part of the config that is specific to the Audio module */
    bool has_audio;
    meshtastic_ModuleConfig_AudioConfig audio;
    /* The part of the config that is specific to the Remote Hardware module */
    bool has_remote_hardware;
    meshtastic_ModuleConfig_RemoteHardwareConfig remote_hardware;
} meshtastic_LocalModuleConfig;


#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define meshtastic_LocalConfig_init_default      {false, meshtastic_Config_DeviceConfig_init_default, false, meshtastic_Config_PositionConfig_init_default, false, meshtastic_Config_PowerConfig_init_default, false, meshtastic_Config_NetworkConfig_init_default, false, meshtastic_Config_DisplayConfig_init_default, false, meshtastic_Config_LoRaConfig_init_default, false, meshtastic_Config_BluetoothConfig_init_default, 0}
#define meshtastic_LocalModuleConfig_init_default {false, meshtastic_ModuleConfig_MQTTConfig_init_default, false, meshtastic_ModuleConfig_SerialConfig_init_default, false, meshtastic_ModuleConfig_ExternalNotificationConfig_init_default, false, meshtastic_ModuleConfig_StoreForwardConfig_init_default, false, meshtastic_ModuleConfig_RangeTestConfig_init_default, false, meshtastic_ModuleConfig_TelemetryConfig_init_default, false, meshtastic_ModuleConfig_CannedMessageConfig_init_default, 0, false, meshtastic_ModuleConfig_AudioConfig_init_default, false, meshtastic_ModuleConfig_RemoteHardwareConfig_init_default}
#define meshtastic_LocalConfig_init_zero         {false, meshtastic_Config_DeviceConfig_init_zero, false, meshtastic_Config_PositionConfig_init_zero, false, meshtastic_Config_PowerConfig_init_zero, false, meshtastic_Config_NetworkConfig_init_zero, false, meshtastic_Config_DisplayConfig_init_zero, false, meshtastic_Config_LoRaConfig_init_zero, false, meshtastic_Config_BluetoothConfig_init_zero, 0}
#define meshtastic_LocalModuleConfig_init_zero   {false, meshtastic_ModuleConfig_MQTTConfig_init_zero, false, meshtastic_ModuleConfig_SerialConfig_init_zero, false, meshtastic_ModuleConfig_ExternalNotificationConfig_init_zero, false, meshtastic_ModuleConfig_StoreForwardConfig_init_zero, false, meshtastic_ModuleConfig_RangeTestConfig_init_zero, false, meshtastic_ModuleConfig_TelemetryConfig_init_zero, false, meshtastic_ModuleConfig_CannedMessageConfig_init_zero, 0, false, meshtastic_ModuleConfig_AudioConfig_init_zero, false, meshtastic_ModuleConfig_RemoteHardwareConfig_init_zero}

/* Field tags (for use in manual encoding/decoding) */
#define meshtastic_LocalConfig_device_tag        1
#define meshtastic_LocalConfig_position_tag      2
#define meshtastic_LocalConfig_power_tag         3
#define meshtastic_LocalConfig_network_tag       4
#define meshtastic_LocalConfig_display_tag       5
#define meshtastic_LocalConfig_lora_tag          6
#define meshtastic_LocalConfig_bluetooth_tag     7
#define meshtastic_LocalConfig_version_tag       8
#define meshtastic_LocalModuleConfig_mqtt_tag    1
#define meshtastic_LocalModuleConfig_serial_tag  2
#define meshtastic_LocalModuleConfig_external_notification_tag 3
#define meshtastic_LocalModuleConfig_store_forward_tag 4
#define meshtastic_LocalModuleConfig_range_test_tag 5
#define meshtastic_LocalModuleConfig_telemetry_tag 6
#define meshtastic_LocalModuleConfig_canned_message_tag 7
#define meshtastic_LocalModuleConfig_version_tag 8
#define meshtastic_LocalModuleConfig_audio_tag   9
#define meshtastic_LocalModuleConfig_remote_hardware_tag 10

/* Struct field encoding specification for nanopb */
#define meshtastic_LocalConfig_FIELDLIST(X, a) \
X(a, STATIC,   OPTIONAL, MESSAGE,  device,            1) \
X(a, STATIC,   OPTIONAL, MESSAGE,  position,          2) \
X(a, STATIC,   OPTIONAL, MESSAGE,  power,             3) \
X(a, STATIC,   OPTIONAL, MESSAGE,  network,           4) \
X(a, STATIC,   OPTIONAL, MESSAGE,  display,           5) \
X(a, STATIC,   OPTIONAL, MESSAGE,  lora,              6) \
X(a, STATIC,   OPTIONAL, MESSAGE,  bluetooth,         7) \
X(a, STATIC,   SINGULAR, UINT32,   version,           8)
#define meshtastic_LocalConfig_CALLBACK NULL
#define meshtastic_LocalConfig_DEFAULT NULL
#define meshtastic_LocalConfig_device_MSGTYPE meshtastic_Config_DeviceConfig
#define meshtastic_LocalConfig_position_MSGTYPE meshtastic_Config_PositionConfig
#define meshtastic_LocalConfig_power_MSGTYPE meshtastic_Config_PowerConfig
#define meshtastic_LocalConfig_network_MSGTYPE meshtastic_Config_NetworkConfig
#define meshtastic_LocalConfig_display_MSGTYPE meshtastic_Config_DisplayConfig
#define meshtastic_LocalConfig_lora_MSGTYPE meshtastic_Config_LoRaConfig
#define meshtastic_LocalConfig_bluetooth_MSGTYPE meshtastic_Config_BluetoothConfig

#define meshtastic_LocalModuleConfig_FIELDLIST(X, a) \
X(a, STATIC,   OPTIONAL, MESSAGE,  mqtt,              1) \
X(a, STATIC,   OPTIONAL, MESSAGE,  serial,            2) \
X(a, STATIC,   OPTIONAL, MESSAGE,  external_notification,   3) \
X(a, STATIC,   OPTIONAL, MESSAGE,  store_forward,     4) \
X(a, STATIC,   OPTIONAL, MESSAGE,  range_test,        5) \
X(a, STATIC,   OPTIONAL, MESSAGE,  telemetry,         6) \
X(a, STATIC,   OPTIONAL, MESSAGE,  canned_message,    7) \
X(a, STATIC,   SINGULAR, UINT32,   version,           8) \
X(a, STATIC,   OPTIONAL, MESSAGE,  audio,             9) \
X(a, STATIC,   OPTIONAL, MESSAGE,  remote_hardware,  10)
#define meshtastic_LocalModuleConfig_CALLBACK NULL
#define meshtastic_LocalModuleConfig_DEFAULT NULL
#define meshtastic_LocalModuleConfig_mqtt_MSGTYPE meshtastic_ModuleConfig_MQTTConfig
#define meshtastic_LocalModuleConfig_serial_MSGTYPE meshtastic_ModuleConfig_SerialConfig
#define meshtastic_LocalModuleConfig_external_notification_MSGTYPE meshtastic_ModuleConfig_ExternalNotificationConfig
#define meshtastic_LocalModuleConfig_store_forward_MSGTYPE meshtastic_ModuleConfig_StoreForwardConfig
#define meshtastic_LocalModuleConfig_range_test_MSGTYPE meshtastic_ModuleConfig_RangeTestConfig
#define meshtastic_LocalModuleConfig_telemetry_MSGTYPE meshtastic_ModuleConfig_TelemetryConfig
#define meshtastic_LocalModuleConfig_canned_message_MSGTYPE meshtastic_ModuleConfig_CannedMessageConfig
#define meshtastic_LocalModuleConfig_audio_MSGTYPE meshtastic_ModuleConfig_AudioConfig
#define meshtastic_LocalModuleConfig_remote_hardware_MSGTYPE meshtastic_ModuleConfig_RemoteHardwareConfig

extern const pb_msgdesc_t meshtastic_LocalConfig_msg;
extern const pb_msgdesc_t meshtastic_LocalModuleConfig_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define meshtastic_LocalConfig_fields &meshtastic_LocalConfig_msg
#define meshtastic_LocalModuleConfig_fields &meshtastic_LocalModuleConfig_msg

/* Maximum encoded size of messages (where known) */
#define meshtastic_LocalConfig_size              456
#define meshtastic_LocalModuleConfig_size        420

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
