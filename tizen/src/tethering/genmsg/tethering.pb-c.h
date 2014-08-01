/* Generated by the protocol buffer compiler.  DO NOT EDIT! */

#ifndef PROTOBUF_C_tethering_2eproto__INCLUDED
#define PROTOBUF_C_tethering_2eproto__INCLUDED

#include "../../../distrib/protobuf/protobuf-c.h"

PROTOBUF_C_BEGIN_DECLS


typedef struct _Tethering__HandShakeReq Tethering__HandShakeReq;
typedef struct _Tethering__HandShakeAns Tethering__HandShakeAns;
typedef struct _Tethering__EmulatorState Tethering__EmulatorState;
typedef struct _Tethering__AppState Tethering__AppState;
typedef struct _Tethering__StartReq Tethering__StartReq;
typedef struct _Tethering__StartAns Tethering__StartAns;
typedef struct _Tethering__SetEventStatus Tethering__SetEventStatus;
typedef struct _Tethering__EventMsg Tethering__EventMsg;
typedef struct _Tethering__EventTerminate Tethering__EventTerminate;
typedef struct _Tethering__SetSensorStatus Tethering__SetSensorStatus;
typedef struct _Tethering__SensorData Tethering__SensorData;
typedef struct _Tethering__SensorMsg Tethering__SensorMsg;
typedef struct _Tethering__Resolution Tethering__Resolution;
typedef struct _Tethering__DisplayMsg Tethering__DisplayMsg;
typedef struct _Tethering__TouchMaxCount Tethering__TouchMaxCount;
typedef struct _Tethering__TouchData Tethering__TouchData;
typedef struct _Tethering__HWKeyMsg Tethering__HWKeyMsg;
typedef struct _Tethering__TouchMsg Tethering__TouchMsg;
typedef struct _Tethering__TetheringMsg Tethering__TetheringMsg;


/* --- enums --- */

typedef enum _Tethering__EventMsg__TYPE {
  TETHERING__EVENT_MSG__TYPE__START_REQ = 2,
  TETHERING__EVENT_MSG__TYPE__START_ANS = 3,
  TETHERING__EVENT_MSG__TYPE__TERMINATE = 4,
  TETHERING__EVENT_MSG__TYPE__EVENT_STATUS = 5
} Tethering__EventMsg__TYPE;
typedef enum _Tethering__SensorMsg__Type {
  TETHERING__SENSOR_MSG__TYPE__START_REQ = 2,
  TETHERING__SENSOR_MSG__TYPE__START_ANS = 3,
  TETHERING__SENSOR_MSG__TYPE__TERMINATE = 4,
  TETHERING__SENSOR_MSG__TYPE__SENSOR_STATUS = 5,
  TETHERING__SENSOR_MSG__TYPE__SENSOR_DATA = 6
} Tethering__SensorMsg__Type;
typedef enum _Tethering__TouchMsg__Type {
  TETHERING__TOUCH_MSG__TYPE__START_REQ = 2,
  TETHERING__TOUCH_MSG__TYPE__START_ANS = 3,
  TETHERING__TOUCH_MSG__TYPE__TERMINATE = 4,
  TETHERING__TOUCH_MSG__TYPE__MAX_COUNT = 5,
  TETHERING__TOUCH_MSG__TYPE__TOUCH_DATA = 6,
  TETHERING__TOUCH_MSG__TYPE__RESOLUTION = 7,
  TETHERING__TOUCH_MSG__TYPE__DISPLAY_MSG = 8,
  TETHERING__TOUCH_MSG__TYPE__HWKEY_MSG = 9
} Tethering__TouchMsg__Type;
typedef enum _Tethering__TetheringMsg__Type {
  TETHERING__TETHERING_MSG__TYPE__HANDSHAKE_REQ = 2,
  TETHERING__TETHERING_MSG__TYPE__HANDSHAKE_ANS = 3,
  TETHERING__TETHERING_MSG__TYPE__EMUL_STATE = 4,
  TETHERING__TETHERING_MSG__TYPE__APP_STATE = 5,
  TETHERING__TETHERING_MSG__TYPE__EVENT_MSG = 6,
  TETHERING__TETHERING_MSG__TYPE__SENSOR_MSG = 7,
  TETHERING__TETHERING_MSG__TYPE__TOUCH_MSG = 8
} Tethering__TetheringMsg__Type;
typedef enum _Tethering__MessageResult {
  TETHERING__MESSAGE_RESULT__SUCCESS = 1,
  TETHERING__MESSAGE_RESULT__FAILURE = 2,
  TETHERING__MESSAGE_RESULT__CANCEL = 3
} Tethering__MessageResult;
typedef enum _Tethering__ConnectionState {
  TETHERING__CONNECTION_STATE__CONNECTED = 1,
  TETHERING__CONNECTION_STATE__DISCONNECTED = 2,
  TETHERING__CONNECTION_STATE__TERMINATED = 3
} Tethering__ConnectionState;
typedef enum _Tethering__EventType {
  TETHERING__EVENT_TYPE__SENSOR = 1,
  TETHERING__EVENT_TYPE__TOUCH = 2
} Tethering__EventType;
typedef enum _Tethering__State {
  TETHERING__STATE__ENABLED = 1,
  TETHERING__STATE__DISABLED = 2
} Tethering__State;
typedef enum _Tethering__SensorType {
  TETHERING__SENSOR_TYPE__ACCEL = 1,
  TETHERING__SENSOR_TYPE__MAGNETIC = 2,
  TETHERING__SENSOR_TYPE__GYROSCOPE = 3,
  TETHERING__SENSOR_TYPE__PROXIMITY = 4,
  TETHERING__SENSOR_TYPE__LIGHT = 5
} Tethering__SensorType;
typedef enum _Tethering__TouchState {
  TETHERING__TOUCH_STATE__PRESSED = 1,
  TETHERING__TOUCH_STATE__RELEASED = 2
} Tethering__TouchState;
typedef enum _Tethering__HWKeyType {
  TETHERING__HWKEY_TYPE__MENU = 1,
  TETHERING__HWKEY_TYPE__HOME = 2,
  TETHERING__HWKEY_TYPE__BACK = 3,
  TETHERING__HWKEY_TYPE__POWER = 4,
  TETHERING__HWKEY_TYPE__VOLUME_UP = 5,
  TETHERING__HWKEY_TYPE__VOLUME_DOWN = 6
} Tethering__HWKeyType;

/* --- messages --- */

struct  _Tethering__HandShakeReq
{
  ProtobufCMessage base;
  int32_t key;
};
#define TETHERING__HAND_SHAKE_REQ__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__hand_shake_req__descriptor) \
    , 0 }


struct  _Tethering__HandShakeAns
{
  ProtobufCMessage base;
  Tethering__MessageResult result;
};
#define TETHERING__HAND_SHAKE_ANS__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__hand_shake_ans__descriptor) \
    , 0 }


struct  _Tethering__EmulatorState
{
  ProtobufCMessage base;
  Tethering__ConnectionState state;
};
#define TETHERING__EMULATOR_STATE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__emulator_state__descriptor) \
    , 0 }


struct  _Tethering__AppState
{
  ProtobufCMessage base;
  Tethering__ConnectionState state;
};
#define TETHERING__APP_STATE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__app_state__descriptor) \
    , 0 }


struct  _Tethering__StartReq
{
  ProtobufCMessage base;
};
#define TETHERING__START_REQ__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__start_req__descriptor) \
     }


struct  _Tethering__StartAns
{
  ProtobufCMessage base;
  Tethering__MessageResult result;
};
#define TETHERING__START_ANS__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__start_ans__descriptor) \
    , 0 }


struct  _Tethering__SetEventStatus
{
  ProtobufCMessage base;
  Tethering__EventType type;
  Tethering__State state;
};
#define TETHERING__SET_EVENT_STATUS__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__set_event_status__descriptor) \
    , 0, 0 }


struct  _Tethering__EventMsg
{
  ProtobufCMessage base;
  Tethering__EventMsg__TYPE type;
  Tethering__StartReq *startreq;
  Tethering__StartAns *startans;
  Tethering__EventTerminate *terminate;
  Tethering__SetEventStatus *setstatus;
};
#define TETHERING__EVENT_MSG__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__event_msg__descriptor) \
    , 0, NULL, NULL, NULL, NULL }


struct  _Tethering__EventTerminate
{
  ProtobufCMessage base;
};
#define TETHERING__EVENT_TERMINATE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__event_terminate__descriptor) \
     }


struct  _Tethering__SetSensorStatus
{
  ProtobufCMessage base;
  Tethering__SensorType type;
  Tethering__State state;
};
#define TETHERING__SET_SENSOR_STATUS__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__set_sensor_status__descriptor) \
    , 0, 0 }


struct  _Tethering__SensorData
{
  ProtobufCMessage base;
  Tethering__SensorType sensor;
  char *x;
  char *y;
  char *z;
};
extern char tethering__sensor_data__x__default_value[];
extern char tethering__sensor_data__y__default_value[];
extern char tethering__sensor_data__z__default_value[];
#define TETHERING__SENSOR_DATA__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__sensor_data__descriptor) \
    , 0, tethering__sensor_data__x__default_value, tethering__sensor_data__y__default_value, tethering__sensor_data__z__default_value }


struct  _Tethering__SensorMsg
{
  ProtobufCMessage base;
  Tethering__SensorMsg__Type type;
  Tethering__StartReq *startreq;
  Tethering__StartAns *startans;
  Tethering__EventTerminate *terminate;
  Tethering__SetSensorStatus *setstatus;
  Tethering__SensorData *data;
};
#define TETHERING__SENSOR_MSG__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__sensor_msg__descriptor) \
    , 0, NULL, NULL, NULL, NULL, NULL }


struct  _Tethering__Resolution
{
  ProtobufCMessage base;
  int32_t width;
  int32_t height;
};
#define TETHERING__RESOLUTION__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__resolution__descriptor) \
    , 0, 0 }


struct  _Tethering__DisplayMsg
{
  ProtobufCMessage base;
  protobuf_c_boolean has_framerate;
  int32_t framerate;
  protobuf_c_boolean has_imagedata;
  ProtobufCBinaryData imagedata;
};
#define TETHERING__DISPLAY_MSG__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__display_msg__descriptor) \
    , 0,0, 0,{0,NULL} }


struct  _Tethering__TouchMaxCount
{
  ProtobufCMessage base;
  protobuf_c_boolean has_max;
  int32_t max;
};
#define TETHERING__TOUCH_MAX_COUNT__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__touch_max_count__descriptor) \
    , 0,10 }


struct  _Tethering__TouchData
{
  ProtobufCMessage base;
  protobuf_c_boolean has_index;
  int32_t index;
  protobuf_c_boolean has_xpoint;
  float xpoint;
  protobuf_c_boolean has_ypoint;
  float ypoint;
  protobuf_c_boolean has_state;
  Tethering__TouchState state;
};
#define TETHERING__TOUCH_DATA__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__touch_data__descriptor) \
    , 0,0, 0,0, 0,0, 0,0 }


struct  _Tethering__HWKeyMsg
{
  ProtobufCMessage base;
  Tethering__HWKeyType type;
};
#define TETHERING__HWKEY_MSG__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__hwkey_msg__descriptor) \
    , 0 }


struct  _Tethering__TouchMsg
{
  ProtobufCMessage base;
  Tethering__TouchMsg__Type type;
  Tethering__StartReq *startreq;
  Tethering__StartAns *startans;
  Tethering__EventTerminate *terminate;
  Tethering__TouchMaxCount *maxcount;
  Tethering__TouchData *touchdata;
  Tethering__Resolution *resolution;
  Tethering__DisplayMsg *display;
  Tethering__HWKeyMsg *hwkey;
};
#define TETHERING__TOUCH_MSG__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__touch_msg__descriptor) \
    , 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }


struct  _Tethering__TetheringMsg
{
  ProtobufCMessage base;
  Tethering__TetheringMsg__Type type;
  Tethering__HandShakeReq *handshakereq;
  Tethering__HandShakeAns *handshakeans;
  Tethering__EmulatorState *emulstate;
  Tethering__AppState *appstate;
  Tethering__EventMsg *eventmsg;
  Tethering__SensorMsg *sensormsg;
  Tethering__TouchMsg *touchmsg;
};
#define TETHERING__TETHERING_MSG__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&tethering__tethering_msg__descriptor) \
    , 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL }


/* Tethering__HandShakeReq methods */
void   tethering__hand_shake_req__init
                     (Tethering__HandShakeReq         *message);
size_t tethering__hand_shake_req__get_packed_size
                     (const Tethering__HandShakeReq   *message);
size_t tethering__hand_shake_req__pack
                     (const Tethering__HandShakeReq   *message,
                      uint8_t             *out);
size_t tethering__hand_shake_req__pack_to_buffer
                     (const Tethering__HandShakeReq   *message,
                      ProtobufCBuffer     *buffer);
Tethering__HandShakeReq *
       tethering__hand_shake_req__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__hand_shake_req__free_unpacked
                     (Tethering__HandShakeReq *message,
                      ProtobufCAllocator *allocator);
/* Tethering__HandShakeAns methods */
void   tethering__hand_shake_ans__init
                     (Tethering__HandShakeAns         *message);
size_t tethering__hand_shake_ans__get_packed_size
                     (const Tethering__HandShakeAns   *message);
size_t tethering__hand_shake_ans__pack
                     (const Tethering__HandShakeAns   *message,
                      uint8_t             *out);
size_t tethering__hand_shake_ans__pack_to_buffer
                     (const Tethering__HandShakeAns   *message,
                      ProtobufCBuffer     *buffer);
Tethering__HandShakeAns *
       tethering__hand_shake_ans__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__hand_shake_ans__free_unpacked
                     (Tethering__HandShakeAns *message,
                      ProtobufCAllocator *allocator);
/* Tethering__EmulatorState methods */
void   tethering__emulator_state__init
                     (Tethering__EmulatorState         *message);
size_t tethering__emulator_state__get_packed_size
                     (const Tethering__EmulatorState   *message);
size_t tethering__emulator_state__pack
                     (const Tethering__EmulatorState   *message,
                      uint8_t             *out);
size_t tethering__emulator_state__pack_to_buffer
                     (const Tethering__EmulatorState   *message,
                      ProtobufCBuffer     *buffer);
Tethering__EmulatorState *
       tethering__emulator_state__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__emulator_state__free_unpacked
                     (Tethering__EmulatorState *message,
                      ProtobufCAllocator *allocator);
/* Tethering__AppState methods */
void   tethering__app_state__init
                     (Tethering__AppState         *message);
size_t tethering__app_state__get_packed_size
                     (const Tethering__AppState   *message);
size_t tethering__app_state__pack
                     (const Tethering__AppState   *message,
                      uint8_t             *out);
size_t tethering__app_state__pack_to_buffer
                     (const Tethering__AppState   *message,
                      ProtobufCBuffer     *buffer);
Tethering__AppState *
       tethering__app_state__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__app_state__free_unpacked
                     (Tethering__AppState *message,
                      ProtobufCAllocator *allocator);
/* Tethering__StartReq methods */
void   tethering__start_req__init
                     (Tethering__StartReq         *message);
size_t tethering__start_req__get_packed_size
                     (const Tethering__StartReq   *message);
size_t tethering__start_req__pack
                     (const Tethering__StartReq   *message,
                      uint8_t             *out);
size_t tethering__start_req__pack_to_buffer
                     (const Tethering__StartReq   *message,
                      ProtobufCBuffer     *buffer);
Tethering__StartReq *
       tethering__start_req__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__start_req__free_unpacked
                     (Tethering__StartReq *message,
                      ProtobufCAllocator *allocator);
/* Tethering__StartAns methods */
void   tethering__start_ans__init
                     (Tethering__StartAns         *message);
size_t tethering__start_ans__get_packed_size
                     (const Tethering__StartAns   *message);
size_t tethering__start_ans__pack
                     (const Tethering__StartAns   *message,
                      uint8_t             *out);
size_t tethering__start_ans__pack_to_buffer
                     (const Tethering__StartAns   *message,
                      ProtobufCBuffer     *buffer);
Tethering__StartAns *
       tethering__start_ans__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__start_ans__free_unpacked
                     (Tethering__StartAns *message,
                      ProtobufCAllocator *allocator);
/* Tethering__SetEventStatus methods */
void   tethering__set_event_status__init
                     (Tethering__SetEventStatus         *message);
size_t tethering__set_event_status__get_packed_size
                     (const Tethering__SetEventStatus   *message);
size_t tethering__set_event_status__pack
                     (const Tethering__SetEventStatus   *message,
                      uint8_t             *out);
size_t tethering__set_event_status__pack_to_buffer
                     (const Tethering__SetEventStatus   *message,
                      ProtobufCBuffer     *buffer);
Tethering__SetEventStatus *
       tethering__set_event_status__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__set_event_status__free_unpacked
                     (Tethering__SetEventStatus *message,
                      ProtobufCAllocator *allocator);
/* Tethering__EventMsg methods */
void   tethering__event_msg__init
                     (Tethering__EventMsg         *message);
size_t tethering__event_msg__get_packed_size
                     (const Tethering__EventMsg   *message);
size_t tethering__event_msg__pack
                     (const Tethering__EventMsg   *message,
                      uint8_t             *out);
size_t tethering__event_msg__pack_to_buffer
                     (const Tethering__EventMsg   *message,
                      ProtobufCBuffer     *buffer);
Tethering__EventMsg *
       tethering__event_msg__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__event_msg__free_unpacked
                     (Tethering__EventMsg *message,
                      ProtobufCAllocator *allocator);
/* Tethering__EventTerminate methods */
void   tethering__event_terminate__init
                     (Tethering__EventTerminate         *message);
size_t tethering__event_terminate__get_packed_size
                     (const Tethering__EventTerminate   *message);
size_t tethering__event_terminate__pack
                     (const Tethering__EventTerminate   *message,
                      uint8_t             *out);
size_t tethering__event_terminate__pack_to_buffer
                     (const Tethering__EventTerminate   *message,
                      ProtobufCBuffer     *buffer);
Tethering__EventTerminate *
       tethering__event_terminate__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__event_terminate__free_unpacked
                     (Tethering__EventTerminate *message,
                      ProtobufCAllocator *allocator);
/* Tethering__SetSensorStatus methods */
void   tethering__set_sensor_status__init
                     (Tethering__SetSensorStatus         *message);
size_t tethering__set_sensor_status__get_packed_size
                     (const Tethering__SetSensorStatus   *message);
size_t tethering__set_sensor_status__pack
                     (const Tethering__SetSensorStatus   *message,
                      uint8_t             *out);
size_t tethering__set_sensor_status__pack_to_buffer
                     (const Tethering__SetSensorStatus   *message,
                      ProtobufCBuffer     *buffer);
Tethering__SetSensorStatus *
       tethering__set_sensor_status__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__set_sensor_status__free_unpacked
                     (Tethering__SetSensorStatus *message,
                      ProtobufCAllocator *allocator);
/* Tethering__SensorData methods */
void   tethering__sensor_data__init
                     (Tethering__SensorData         *message);
size_t tethering__sensor_data__get_packed_size
                     (const Tethering__SensorData   *message);
size_t tethering__sensor_data__pack
                     (const Tethering__SensorData   *message,
                      uint8_t             *out);
size_t tethering__sensor_data__pack_to_buffer
                     (const Tethering__SensorData   *message,
                      ProtobufCBuffer     *buffer);
Tethering__SensorData *
       tethering__sensor_data__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__sensor_data__free_unpacked
                     (Tethering__SensorData *message,
                      ProtobufCAllocator *allocator);
/* Tethering__SensorMsg methods */
void   tethering__sensor_msg__init
                     (Tethering__SensorMsg         *message);
size_t tethering__sensor_msg__get_packed_size
                     (const Tethering__SensorMsg   *message);
size_t tethering__sensor_msg__pack
                     (const Tethering__SensorMsg   *message,
                      uint8_t             *out);
size_t tethering__sensor_msg__pack_to_buffer
                     (const Tethering__SensorMsg   *message,
                      ProtobufCBuffer     *buffer);
Tethering__SensorMsg *
       tethering__sensor_msg__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__sensor_msg__free_unpacked
                     (Tethering__SensorMsg *message,
                      ProtobufCAllocator *allocator);
/* Tethering__Resolution methods */
void   tethering__resolution__init
                     (Tethering__Resolution         *message);
size_t tethering__resolution__get_packed_size
                     (const Tethering__Resolution   *message);
size_t tethering__resolution__pack
                     (const Tethering__Resolution   *message,
                      uint8_t             *out);
size_t tethering__resolution__pack_to_buffer
                     (const Tethering__Resolution   *message,
                      ProtobufCBuffer     *buffer);
Tethering__Resolution *
       tethering__resolution__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__resolution__free_unpacked
                     (Tethering__Resolution *message,
                      ProtobufCAllocator *allocator);
/* Tethering__DisplayMsg methods */
void   tethering__display_msg__init
                     (Tethering__DisplayMsg         *message);
size_t tethering__display_msg__get_packed_size
                     (const Tethering__DisplayMsg   *message);
size_t tethering__display_msg__pack
                     (const Tethering__DisplayMsg   *message,
                      uint8_t             *out);
size_t tethering__display_msg__pack_to_buffer
                     (const Tethering__DisplayMsg   *message,
                      ProtobufCBuffer     *buffer);
Tethering__DisplayMsg *
       tethering__display_msg__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__display_msg__free_unpacked
                     (Tethering__DisplayMsg *message,
                      ProtobufCAllocator *allocator);
/* Tethering__TouchMaxCount methods */
void   tethering__touch_max_count__init
                     (Tethering__TouchMaxCount         *message);
size_t tethering__touch_max_count__get_packed_size
                     (const Tethering__TouchMaxCount   *message);
size_t tethering__touch_max_count__pack
                     (const Tethering__TouchMaxCount   *message,
                      uint8_t             *out);
size_t tethering__touch_max_count__pack_to_buffer
                     (const Tethering__TouchMaxCount   *message,
                      ProtobufCBuffer     *buffer);
Tethering__TouchMaxCount *
       tethering__touch_max_count__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__touch_max_count__free_unpacked
                     (Tethering__TouchMaxCount *message,
                      ProtobufCAllocator *allocator);
/* Tethering__TouchData methods */
void   tethering__touch_data__init
                     (Tethering__TouchData         *message);
size_t tethering__touch_data__get_packed_size
                     (const Tethering__TouchData   *message);
size_t tethering__touch_data__pack
                     (const Tethering__TouchData   *message,
                      uint8_t             *out);
size_t tethering__touch_data__pack_to_buffer
                     (const Tethering__TouchData   *message,
                      ProtobufCBuffer     *buffer);
Tethering__TouchData *
       tethering__touch_data__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__touch_data__free_unpacked
                     (Tethering__TouchData *message,
                      ProtobufCAllocator *allocator);
/* Tethering__HWKeyMsg methods */
void   tethering__hwkey_msg__init
                     (Tethering__HWKeyMsg         *message);
size_t tethering__hwkey_msg__get_packed_size
                     (const Tethering__HWKeyMsg   *message);
size_t tethering__hwkey_msg__pack
                     (const Tethering__HWKeyMsg   *message,
                      uint8_t             *out);
size_t tethering__hwkey_msg__pack_to_buffer
                     (const Tethering__HWKeyMsg   *message,
                      ProtobufCBuffer     *buffer);
Tethering__HWKeyMsg *
       tethering__hwkey_msg__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__hwkey_msg__free_unpacked
                     (Tethering__HWKeyMsg *message,
                      ProtobufCAllocator *allocator);
/* Tethering__TouchMsg methods */
void   tethering__touch_msg__init
                     (Tethering__TouchMsg         *message);
size_t tethering__touch_msg__get_packed_size
                     (const Tethering__TouchMsg   *message);
size_t tethering__touch_msg__pack
                     (const Tethering__TouchMsg   *message,
                      uint8_t             *out);
size_t tethering__touch_msg__pack_to_buffer
                     (const Tethering__TouchMsg   *message,
                      ProtobufCBuffer     *buffer);
Tethering__TouchMsg *
       tethering__touch_msg__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__touch_msg__free_unpacked
                     (Tethering__TouchMsg *message,
                      ProtobufCAllocator *allocator);
/* Tethering__TetheringMsg methods */
void   tethering__tethering_msg__init
                     (Tethering__TetheringMsg         *message);
size_t tethering__tethering_msg__get_packed_size
                     (const Tethering__TetheringMsg   *message);
size_t tethering__tethering_msg__pack
                     (const Tethering__TetheringMsg   *message,
                      uint8_t             *out);
size_t tethering__tethering_msg__pack_to_buffer
                     (const Tethering__TetheringMsg   *message,
                      ProtobufCBuffer     *buffer);
Tethering__TetheringMsg *
       tethering__tethering_msg__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   tethering__tethering_msg__free_unpacked
                     (Tethering__TetheringMsg *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*Tethering__HandShakeReq_Closure)
                 (const Tethering__HandShakeReq *message,
                  void *closure_data);
typedef void (*Tethering__HandShakeAns_Closure)
                 (const Tethering__HandShakeAns *message,
                  void *closure_data);
typedef void (*Tethering__EmulatorState_Closure)
                 (const Tethering__EmulatorState *message,
                  void *closure_data);
typedef void (*Tethering__AppState_Closure)
                 (const Tethering__AppState *message,
                  void *closure_data);
typedef void (*Tethering__StartReq_Closure)
                 (const Tethering__StartReq *message,
                  void *closure_data);
typedef void (*Tethering__StartAns_Closure)
                 (const Tethering__StartAns *message,
                  void *closure_data);
typedef void (*Tethering__SetEventStatus_Closure)
                 (const Tethering__SetEventStatus *message,
                  void *closure_data);
typedef void (*Tethering__EventMsg_Closure)
                 (const Tethering__EventMsg *message,
                  void *closure_data);
typedef void (*Tethering__EventTerminate_Closure)
                 (const Tethering__EventTerminate *message,
                  void *closure_data);
typedef void (*Tethering__SetSensorStatus_Closure)
                 (const Tethering__SetSensorStatus *message,
                  void *closure_data);
typedef void (*Tethering__SensorData_Closure)
                 (const Tethering__SensorData *message,
                  void *closure_data);
typedef void (*Tethering__SensorMsg_Closure)
                 (const Tethering__SensorMsg *message,
                  void *closure_data);
typedef void (*Tethering__Resolution_Closure)
                 (const Tethering__Resolution *message,
                  void *closure_data);
typedef void (*Tethering__DisplayMsg_Closure)
                 (const Tethering__DisplayMsg *message,
                  void *closure_data);
typedef void (*Tethering__TouchMaxCount_Closure)
                 (const Tethering__TouchMaxCount *message,
                  void *closure_data);
typedef void (*Tethering__TouchData_Closure)
                 (const Tethering__TouchData *message,
                  void *closure_data);
typedef void (*Tethering__HWKeyMsg_Closure)
                 (const Tethering__HWKeyMsg *message,
                  void *closure_data);
typedef void (*Tethering__TouchMsg_Closure)
                 (const Tethering__TouchMsg *message,
                  void *closure_data);
typedef void (*Tethering__TetheringMsg_Closure)
                 (const Tethering__TetheringMsg *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCEnumDescriptor    tethering__message_result__descriptor;
extern const ProtobufCEnumDescriptor    tethering__connection_state__descriptor;
extern const ProtobufCEnumDescriptor    tethering__event_type__descriptor;
extern const ProtobufCEnumDescriptor    tethering__state__descriptor;
extern const ProtobufCEnumDescriptor    tethering__sensor_type__descriptor;
extern const ProtobufCEnumDescriptor    tethering__touch_state__descriptor;
extern const ProtobufCEnumDescriptor    tethering__hwkey_type__descriptor;
extern const ProtobufCMessageDescriptor tethering__hand_shake_req__descriptor;
extern const ProtobufCMessageDescriptor tethering__hand_shake_ans__descriptor;
extern const ProtobufCMessageDescriptor tethering__emulator_state__descriptor;
extern const ProtobufCMessageDescriptor tethering__app_state__descriptor;
extern const ProtobufCMessageDescriptor tethering__start_req__descriptor;
extern const ProtobufCMessageDescriptor tethering__start_ans__descriptor;
extern const ProtobufCMessageDescriptor tethering__set_event_status__descriptor;
extern const ProtobufCMessageDescriptor tethering__event_msg__descriptor;
extern const ProtobufCEnumDescriptor    tethering__event_msg__type__descriptor;
extern const ProtobufCMessageDescriptor tethering__event_terminate__descriptor;
extern const ProtobufCMessageDescriptor tethering__set_sensor_status__descriptor;
extern const ProtobufCMessageDescriptor tethering__sensor_data__descriptor;
extern const ProtobufCMessageDescriptor tethering__sensor_msg__descriptor;
extern const ProtobufCEnumDescriptor    tethering__sensor_msg__type__descriptor;
extern const ProtobufCMessageDescriptor tethering__resolution__descriptor;
extern const ProtobufCMessageDescriptor tethering__display_msg__descriptor;
extern const ProtobufCMessageDescriptor tethering__touch_max_count__descriptor;
extern const ProtobufCMessageDescriptor tethering__touch_data__descriptor;
extern const ProtobufCMessageDescriptor tethering__hwkey_msg__descriptor;
extern const ProtobufCMessageDescriptor tethering__touch_msg__descriptor;
extern const ProtobufCEnumDescriptor    tethering__touch_msg__type__descriptor;
extern const ProtobufCMessageDescriptor tethering__tethering_msg__descriptor;
extern const ProtobufCEnumDescriptor    tethering__tethering_msg__type__descriptor;

PROTOBUF_C_END_DECLS


#endif  /* PROTOBUF_tethering_2eproto__INCLUDED */
