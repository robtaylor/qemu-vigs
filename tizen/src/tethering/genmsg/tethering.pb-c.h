/* Generated by the protocol buffer compiler.  DO NOT EDIT! */

#ifndef PROTOBUF_C_tethering_2eproto__INCLUDED
#define PROTOBUF_C_tethering_2eproto__INCLUDED

#include "../../../distrib/protobuf/protobuf-c.h"

PROTOBUF_C_BEGIN_DECLS


typedef struct _Injector__HandShakeReq Injector__HandShakeReq;
typedef struct _Injector__HandShakeAns Injector__HandShakeAns;
typedef struct _Injector__EmulatorState Injector__EmulatorState;
typedef struct _Injector__AppState Injector__AppState;
typedef struct _Injector__StartReq Injector__StartReq;
typedef struct _Injector__StartAns Injector__StartAns;
typedef struct _Injector__SetEventStatus Injector__SetEventStatus;
typedef struct _Injector__EventMsg Injector__EventMsg;
typedef struct _Injector__EventTerminate Injector__EventTerminate;
typedef struct _Injector__SetSensorStatus Injector__SetSensorStatus;
typedef struct _Injector__SensorData Injector__SensorData;
typedef struct _Injector__SensorMsg Injector__SensorMsg;
typedef struct _Injector__MultiTouchMaxCount Injector__MultiTouchMaxCount;
typedef struct _Injector__MultiTouchData Injector__MultiTouchData;
typedef struct _Injector__MultiTouchMsg Injector__MultiTouchMsg;
typedef struct _Injector__InjectorMsg Injector__InjectorMsg;


/* --- enums --- */

typedef enum _Injector__EventMsg__TYPE {
  INJECTOR__EVENT_MSG__TYPE__START_REQ = 2,
  INJECTOR__EVENT_MSG__TYPE__START_ANS = 3,
  INJECTOR__EVENT_MSG__TYPE__TERMINATE = 4,
  INJECTOR__EVENT_MSG__TYPE__EVENT_STATUS = 5
} Injector__EventMsg__TYPE;
typedef enum _Injector__SensorMsg__Type {
  INJECTOR__SENSOR_MSG__TYPE__START_REQ = 2,
  INJECTOR__SENSOR_MSG__TYPE__START_ANS = 3,
  INJECTOR__SENSOR_MSG__TYPE__TERMINATE = 4,
  INJECTOR__SENSOR_MSG__TYPE__SENSOR_STATUS = 5,
  INJECTOR__SENSOR_MSG__TYPE__SENSOR_DATA = 6
} Injector__SensorMsg__Type;
typedef enum _Injector__MultiTouchMsg__Type {
  INJECTOR__MULTI_TOUCH_MSG__TYPE__START_REQ = 2,
  INJECTOR__MULTI_TOUCH_MSG__TYPE__START_ANS = 3,
  INJECTOR__MULTI_TOUCH_MSG__TYPE__TERMINATE = 4,
  INJECTOR__MULTI_TOUCH_MSG__TYPE__MAX_COUNT = 5,
  INJECTOR__MULTI_TOUCH_MSG__TYPE__TOUCH_DATA = 6
} Injector__MultiTouchMsg__Type;
typedef enum _Injector__InjectorMsg__Type {
  INJECTOR__INJECTOR_MSG__TYPE__HANDSHAKE_REQ = 2,
  INJECTOR__INJECTOR_MSG__TYPE__HANDSHAKE_ANS = 3,
  INJECTOR__INJECTOR_MSG__TYPE__EMUL_STATE = 4,
  INJECTOR__INJECTOR_MSG__TYPE__APP_STATE = 5,
  INJECTOR__INJECTOR_MSG__TYPE__EVENT_MSG = 6,
  INJECTOR__INJECTOR_MSG__TYPE__SENSOR_MSG = 7,
  INJECTOR__INJECTOR_MSG__TYPE__TOUCH_MSG = 8
} Injector__InjectorMsg__Type;
typedef enum _Injector__Result {
  INJECTOR__RESULT__SUCCESS = 1,
  INJECTOR__RESULT__FAILURE = 2,
  INJECTOR__RESULT__CANCEL = 3
} Injector__Result;
typedef enum _Injector__ConnectionState {
  INJECTOR__CONNECTION_STATE__CONNECT = 1,
  INJECTOR__CONNECTION_STATE__DISCONNECT = 2,
  INJECTOR__CONNECTION_STATE__TERMINATE = 3
} Injector__ConnectionState;
typedef enum _Injector__Event {
  INJECTOR__EVENT__SENSOR = 1,
  INJECTOR__EVENT__MULTITOUCH = 2
} Injector__Event;
typedef enum _Injector__Status {
  INJECTOR__STATUS__ENABLE = 1,
  INJECTOR__STATUS__DISABLE = 2
} Injector__Status;
typedef enum _Injector__SensorType {
  INJECTOR__SENSOR_TYPE__ACCEL = 1,
  INJECTOR__SENSOR_TYPE__MAGNETIC = 2,
  INJECTOR__SENSOR_TYPE__GYROSCOPE = 3,
  INJECTOR__SENSOR_TYPE__PROXIMITY = 4,
  INJECTOR__SENSOR_TYPE__LIGHT = 5
} Injector__SensorType;
typedef enum _Injector__TouchStatus {
  INJECTOR__TOUCH_STATUS__PRESS = 1,
  INJECTOR__TOUCH_STATUS__RELEASE = 2
} Injector__TouchStatus;

/* --- messages --- */

struct  _Injector__HandShakeReq
{
  ProtobufCMessage base;
  int32_t key;
};
#define INJECTOR__HAND_SHAKE_REQ__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&injector__hand_shake_req__descriptor) \
    , 0 }


struct  _Injector__HandShakeAns
{
  ProtobufCMessage base;
  Injector__Result result;
};
#define INJECTOR__HAND_SHAKE_ANS__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&injector__hand_shake_ans__descriptor) \
    , 0 }


struct  _Injector__EmulatorState
{
  ProtobufCMessage base;
  Injector__ConnectionState state;
};
#define INJECTOR__EMULATOR_STATE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&injector__emulator_state__descriptor) \
    , 0 }


struct  _Injector__AppState
{
  ProtobufCMessage base;
  Injector__ConnectionState state;
};
#define INJECTOR__APP_STATE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&injector__app_state__descriptor) \
    , 0 }


struct  _Injector__StartReq
{
  ProtobufCMessage base;
};
#define INJECTOR__START_REQ__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&injector__start_req__descriptor) \
     }


struct  _Injector__StartAns
{
  ProtobufCMessage base;
  Injector__Result result;
};
#define INJECTOR__START_ANS__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&injector__start_ans__descriptor) \
    , 0 }


struct  _Injector__SetEventStatus
{
  ProtobufCMessage base;
  Injector__Event event;
  Injector__Status status;
};
#define INJECTOR__SET_EVENT_STATUS__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&injector__set_event_status__descriptor) \
    , 0, 0 }


struct  _Injector__EventMsg
{
  ProtobufCMessage base;
  Injector__EventMsg__TYPE type;
  Injector__StartReq *startreq;
  Injector__StartAns *startans;
  Injector__EventTerminate *terminate;
  Injector__SetEventStatus *setstatus;
};
#define INJECTOR__EVENT_MSG__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&injector__event_msg__descriptor) \
    , 0, NULL, NULL, NULL, NULL }


struct  _Injector__EventTerminate
{
  ProtobufCMessage base;
};
#define INJECTOR__EVENT_TERMINATE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&injector__event_terminate__descriptor) \
     }


struct  _Injector__SetSensorStatus
{
  ProtobufCMessage base;
  Injector__SensorType sensor;
  Injector__Status status;
};
#define INJECTOR__SET_SENSOR_STATUS__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&injector__set_sensor_status__descriptor) \
    , 0, 0 }


struct  _Injector__SensorData
{
  ProtobufCMessage base;
  Injector__SensorType sensor;
  char *x;
  char *y;
  char *z;
};
extern char injector__sensor_data__x__default_value[];
extern char injector__sensor_data__y__default_value[];
extern char injector__sensor_data__z__default_value[];
#define INJECTOR__SENSOR_DATA__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&injector__sensor_data__descriptor) \
    , 0, injector__sensor_data__x__default_value, injector__sensor_data__y__default_value, injector__sensor_data__z__default_value }


struct  _Injector__SensorMsg
{
  ProtobufCMessage base;
  Injector__SensorMsg__Type type;
  Injector__StartReq *startreq;
  Injector__StartAns *startans;
  Injector__EventTerminate *terminate;
  Injector__SetSensorStatus *setstatus;
  Injector__SensorData *data;
};
#define INJECTOR__SENSOR_MSG__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&injector__sensor_msg__descriptor) \
    , 0, NULL, NULL, NULL, NULL, NULL }


struct  _Injector__MultiTouchMaxCount
{
  ProtobufCMessage base;
  protobuf_c_boolean has_max;
  int32_t max;
};
#define INJECTOR__MULTI_TOUCH_MAX_COUNT__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&injector__multi_touch_max_count__descriptor) \
    , 0,10 }


struct  _Injector__MultiTouchData
{
  ProtobufCMessage base;
  protobuf_c_boolean has_index;
  int32_t index;
  protobuf_c_boolean has_xpoint;
  float xpoint;
  protobuf_c_boolean has_ypoint;
  float ypoint;
  protobuf_c_boolean has_status;
  Injector__TouchStatus status;
};
#define INJECTOR__MULTI_TOUCH_DATA__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&injector__multi_touch_data__descriptor) \
    , 0,0, 0,0, 0,0, 0,0 }


struct  _Injector__MultiTouchMsg
{
  ProtobufCMessage base;
  Injector__MultiTouchMsg__Type type;
  Injector__StartReq *startreq;
  Injector__StartAns *startans;
  Injector__EventTerminate *terminate;
  Injector__MultiTouchMaxCount *maxcount;
  Injector__MultiTouchData *touchdata;
};
#define INJECTOR__MULTI_TOUCH_MSG__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&injector__multi_touch_msg__descriptor) \
    , 0, NULL, NULL, NULL, NULL, NULL }


struct  _Injector__InjectorMsg
{
  ProtobufCMessage base;
  Injector__InjectorMsg__Type type;
  Injector__HandShakeReq *handshakereq;
  Injector__HandShakeAns *handshakeans;
  Injector__EmulatorState *emulstate;
  Injector__AppState *appstate;
  Injector__EventMsg *eventmsg;
  Injector__SensorMsg *sensormsg;
  Injector__MultiTouchMsg *touchmsg;
};
#define INJECTOR__INJECTOR_MSG__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&injector__injector_msg__descriptor) \
    , 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL }


/* Injector__HandShakeReq methods */
void   injector__hand_shake_req__init
                     (Injector__HandShakeReq         *message);
size_t injector__hand_shake_req__get_packed_size
                     (const Injector__HandShakeReq   *message);
size_t injector__hand_shake_req__pack
                     (const Injector__HandShakeReq   *message,
                      uint8_t             *out);
size_t injector__hand_shake_req__pack_to_buffer
                     (const Injector__HandShakeReq   *message,
                      ProtobufCBuffer     *buffer);
Injector__HandShakeReq *
       injector__hand_shake_req__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   injector__hand_shake_req__free_unpacked
                     (Injector__HandShakeReq *message,
                      ProtobufCAllocator *allocator);
/* Injector__HandShakeAns methods */
void   injector__hand_shake_ans__init
                     (Injector__HandShakeAns         *message);
size_t injector__hand_shake_ans__get_packed_size
                     (const Injector__HandShakeAns   *message);
size_t injector__hand_shake_ans__pack
                     (const Injector__HandShakeAns   *message,
                      uint8_t             *out);
size_t injector__hand_shake_ans__pack_to_buffer
                     (const Injector__HandShakeAns   *message,
                      ProtobufCBuffer     *buffer);
Injector__HandShakeAns *
       injector__hand_shake_ans__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   injector__hand_shake_ans__free_unpacked
                     (Injector__HandShakeAns *message,
                      ProtobufCAllocator *allocator);
/* Injector__EmulatorState methods */
void   injector__emulator_state__init
                     (Injector__EmulatorState         *message);
size_t injector__emulator_state__get_packed_size
                     (const Injector__EmulatorState   *message);
size_t injector__emulator_state__pack
                     (const Injector__EmulatorState   *message,
                      uint8_t             *out);
size_t injector__emulator_state__pack_to_buffer
                     (const Injector__EmulatorState   *message,
                      ProtobufCBuffer     *buffer);
Injector__EmulatorState *
       injector__emulator_state__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   injector__emulator_state__free_unpacked
                     (Injector__EmulatorState *message,
                      ProtobufCAllocator *allocator);
/* Injector__AppState methods */
void   injector__app_state__init
                     (Injector__AppState         *message);
size_t injector__app_state__get_packed_size
                     (const Injector__AppState   *message);
size_t injector__app_state__pack
                     (const Injector__AppState   *message,
                      uint8_t             *out);
size_t injector__app_state__pack_to_buffer
                     (const Injector__AppState   *message,
                      ProtobufCBuffer     *buffer);
Injector__AppState *
       injector__app_state__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   injector__app_state__free_unpacked
                     (Injector__AppState *message,
                      ProtobufCAllocator *allocator);
/* Injector__StartReq methods */
void   injector__start_req__init
                     (Injector__StartReq         *message);
size_t injector__start_req__get_packed_size
                     (const Injector__StartReq   *message);
size_t injector__start_req__pack
                     (const Injector__StartReq   *message,
                      uint8_t             *out);
size_t injector__start_req__pack_to_buffer
                     (const Injector__StartReq   *message,
                      ProtobufCBuffer     *buffer);
Injector__StartReq *
       injector__start_req__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   injector__start_req__free_unpacked
                     (Injector__StartReq *message,
                      ProtobufCAllocator *allocator);
/* Injector__StartAns methods */
void   injector__start_ans__init
                     (Injector__StartAns         *message);
size_t injector__start_ans__get_packed_size
                     (const Injector__StartAns   *message);
size_t injector__start_ans__pack
                     (const Injector__StartAns   *message,
                      uint8_t             *out);
size_t injector__start_ans__pack_to_buffer
                     (const Injector__StartAns   *message,
                      ProtobufCBuffer     *buffer);
Injector__StartAns *
       injector__start_ans__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   injector__start_ans__free_unpacked
                     (Injector__StartAns *message,
                      ProtobufCAllocator *allocator);
/* Injector__SetEventStatus methods */
void   injector__set_event_status__init
                     (Injector__SetEventStatus         *message);
size_t injector__set_event_status__get_packed_size
                     (const Injector__SetEventStatus   *message);
size_t injector__set_event_status__pack
                     (const Injector__SetEventStatus   *message,
                      uint8_t             *out);
size_t injector__set_event_status__pack_to_buffer
                     (const Injector__SetEventStatus   *message,
                      ProtobufCBuffer     *buffer);
Injector__SetEventStatus *
       injector__set_event_status__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   injector__set_event_status__free_unpacked
                     (Injector__SetEventStatus *message,
                      ProtobufCAllocator *allocator);
/* Injector__EventMsg methods */
void   injector__event_msg__init
                     (Injector__EventMsg         *message);
size_t injector__event_msg__get_packed_size
                     (const Injector__EventMsg   *message);
size_t injector__event_msg__pack
                     (const Injector__EventMsg   *message,
                      uint8_t             *out);
size_t injector__event_msg__pack_to_buffer
                     (const Injector__EventMsg   *message,
                      ProtobufCBuffer     *buffer);
Injector__EventMsg *
       injector__event_msg__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   injector__event_msg__free_unpacked
                     (Injector__EventMsg *message,
                      ProtobufCAllocator *allocator);
/* Injector__EventTerminate methods */
void   injector__event_terminate__init
                     (Injector__EventTerminate         *message);
size_t injector__event_terminate__get_packed_size
                     (const Injector__EventTerminate   *message);
size_t injector__event_terminate__pack
                     (const Injector__EventTerminate   *message,
                      uint8_t             *out);
size_t injector__event_terminate__pack_to_buffer
                     (const Injector__EventTerminate   *message,
                      ProtobufCBuffer     *buffer);
Injector__EventTerminate *
       injector__event_terminate__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   injector__event_terminate__free_unpacked
                     (Injector__EventTerminate *message,
                      ProtobufCAllocator *allocator);
/* Injector__SetSensorStatus methods */
void   injector__set_sensor_status__init
                     (Injector__SetSensorStatus         *message);
size_t injector__set_sensor_status__get_packed_size
                     (const Injector__SetSensorStatus   *message);
size_t injector__set_sensor_status__pack
                     (const Injector__SetSensorStatus   *message,
                      uint8_t             *out);
size_t injector__set_sensor_status__pack_to_buffer
                     (const Injector__SetSensorStatus   *message,
                      ProtobufCBuffer     *buffer);
Injector__SetSensorStatus *
       injector__set_sensor_status__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   injector__set_sensor_status__free_unpacked
                     (Injector__SetSensorStatus *message,
                      ProtobufCAllocator *allocator);
/* Injector__SensorData methods */
void   injector__sensor_data__init
                     (Injector__SensorData         *message);
size_t injector__sensor_data__get_packed_size
                     (const Injector__SensorData   *message);
size_t injector__sensor_data__pack
                     (const Injector__SensorData   *message,
                      uint8_t             *out);
size_t injector__sensor_data__pack_to_buffer
                     (const Injector__SensorData   *message,
                      ProtobufCBuffer     *buffer);
Injector__SensorData *
       injector__sensor_data__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   injector__sensor_data__free_unpacked
                     (Injector__SensorData *message,
                      ProtobufCAllocator *allocator);
/* Injector__SensorMsg methods */
void   injector__sensor_msg__init
                     (Injector__SensorMsg         *message);
size_t injector__sensor_msg__get_packed_size
                     (const Injector__SensorMsg   *message);
size_t injector__sensor_msg__pack
                     (const Injector__SensorMsg   *message,
                      uint8_t             *out);
size_t injector__sensor_msg__pack_to_buffer
                     (const Injector__SensorMsg   *message,
                      ProtobufCBuffer     *buffer);
Injector__SensorMsg *
       injector__sensor_msg__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   injector__sensor_msg__free_unpacked
                     (Injector__SensorMsg *message,
                      ProtobufCAllocator *allocator);
/* Injector__MultiTouchMaxCount methods */
void   injector__multi_touch_max_count__init
                     (Injector__MultiTouchMaxCount         *message);
size_t injector__multi_touch_max_count__get_packed_size
                     (const Injector__MultiTouchMaxCount   *message);
size_t injector__multi_touch_max_count__pack
                     (const Injector__MultiTouchMaxCount   *message,
                      uint8_t             *out);
size_t injector__multi_touch_max_count__pack_to_buffer
                     (const Injector__MultiTouchMaxCount   *message,
                      ProtobufCBuffer     *buffer);
Injector__MultiTouchMaxCount *
       injector__multi_touch_max_count__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   injector__multi_touch_max_count__free_unpacked
                     (Injector__MultiTouchMaxCount *message,
                      ProtobufCAllocator *allocator);
/* Injector__MultiTouchData methods */
void   injector__multi_touch_data__init
                     (Injector__MultiTouchData         *message);
size_t injector__multi_touch_data__get_packed_size
                     (const Injector__MultiTouchData   *message);
size_t injector__multi_touch_data__pack
                     (const Injector__MultiTouchData   *message,
                      uint8_t             *out);
size_t injector__multi_touch_data__pack_to_buffer
                     (const Injector__MultiTouchData   *message,
                      ProtobufCBuffer     *buffer);
Injector__MultiTouchData *
       injector__multi_touch_data__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   injector__multi_touch_data__free_unpacked
                     (Injector__MultiTouchData *message,
                      ProtobufCAllocator *allocator);
/* Injector__MultiTouchMsg methods */
void   injector__multi_touch_msg__init
                     (Injector__MultiTouchMsg         *message);
size_t injector__multi_touch_msg__get_packed_size
                     (const Injector__MultiTouchMsg   *message);
size_t injector__multi_touch_msg__pack
                     (const Injector__MultiTouchMsg   *message,
                      uint8_t             *out);
size_t injector__multi_touch_msg__pack_to_buffer
                     (const Injector__MultiTouchMsg   *message,
                      ProtobufCBuffer     *buffer);
Injector__MultiTouchMsg *
       injector__multi_touch_msg__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   injector__multi_touch_msg__free_unpacked
                     (Injector__MultiTouchMsg *message,
                      ProtobufCAllocator *allocator);
/* Injector__InjectorMsg methods */
void   injector__injector_msg__init
                     (Injector__InjectorMsg         *message);
size_t injector__injector_msg__get_packed_size
                     (const Injector__InjectorMsg   *message);
size_t injector__injector_msg__pack
                     (const Injector__InjectorMsg   *message,
                      uint8_t             *out);
size_t injector__injector_msg__pack_to_buffer
                     (const Injector__InjectorMsg   *message,
                      ProtobufCBuffer     *buffer);
Injector__InjectorMsg *
       injector__injector_msg__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   injector__injector_msg__free_unpacked
                     (Injector__InjectorMsg *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*Injector__HandShakeReq_Closure)
                 (const Injector__HandShakeReq *message,
                  void *closure_data);
typedef void (*Injector__HandShakeAns_Closure)
                 (const Injector__HandShakeAns *message,
                  void *closure_data);
typedef void (*Injector__EmulatorState_Closure)
                 (const Injector__EmulatorState *message,
                  void *closure_data);
typedef void (*Injector__AppState_Closure)
                 (const Injector__AppState *message,
                  void *closure_data);
typedef void (*Injector__StartReq_Closure)
                 (const Injector__StartReq *message,
                  void *closure_data);
typedef void (*Injector__StartAns_Closure)
                 (const Injector__StartAns *message,
                  void *closure_data);
typedef void (*Injector__SetEventStatus_Closure)
                 (const Injector__SetEventStatus *message,
                  void *closure_data);
typedef void (*Injector__EventMsg_Closure)
                 (const Injector__EventMsg *message,
                  void *closure_data);
typedef void (*Injector__EventTerminate_Closure)
                 (const Injector__EventTerminate *message,
                  void *closure_data);
typedef void (*Injector__SetSensorStatus_Closure)
                 (const Injector__SetSensorStatus *message,
                  void *closure_data);
typedef void (*Injector__SensorData_Closure)
                 (const Injector__SensorData *message,
                  void *closure_data);
typedef void (*Injector__SensorMsg_Closure)
                 (const Injector__SensorMsg *message,
                  void *closure_data);
typedef void (*Injector__MultiTouchMaxCount_Closure)
                 (const Injector__MultiTouchMaxCount *message,
                  void *closure_data);
typedef void (*Injector__MultiTouchData_Closure)
                 (const Injector__MultiTouchData *message,
                  void *closure_data);
typedef void (*Injector__MultiTouchMsg_Closure)
                 (const Injector__MultiTouchMsg *message,
                  void *closure_data);
typedef void (*Injector__InjectorMsg_Closure)
                 (const Injector__InjectorMsg *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCEnumDescriptor    injector__result__descriptor;
extern const ProtobufCEnumDescriptor    injector__connection_state__descriptor;
extern const ProtobufCEnumDescriptor    injector__event__descriptor;
extern const ProtobufCEnumDescriptor    injector__status__descriptor;
extern const ProtobufCEnumDescriptor    injector__sensor_type__descriptor;
extern const ProtobufCEnumDescriptor    injector__touch_status__descriptor;
extern const ProtobufCMessageDescriptor injector__hand_shake_req__descriptor;
extern const ProtobufCMessageDescriptor injector__hand_shake_ans__descriptor;
extern const ProtobufCMessageDescriptor injector__emulator_state__descriptor;
extern const ProtobufCMessageDescriptor injector__app_state__descriptor;
extern const ProtobufCMessageDescriptor injector__start_req__descriptor;
extern const ProtobufCMessageDescriptor injector__start_ans__descriptor;
extern const ProtobufCMessageDescriptor injector__set_event_status__descriptor;
extern const ProtobufCMessageDescriptor injector__event_msg__descriptor;
extern const ProtobufCEnumDescriptor    injector__event_msg__type__descriptor;
extern const ProtobufCMessageDescriptor injector__event_terminate__descriptor;
extern const ProtobufCMessageDescriptor injector__set_sensor_status__descriptor;
extern const ProtobufCMessageDescriptor injector__sensor_data__descriptor;
extern const ProtobufCMessageDescriptor injector__sensor_msg__descriptor;
extern const ProtobufCEnumDescriptor    injector__sensor_msg__type__descriptor;
extern const ProtobufCMessageDescriptor injector__multi_touch_max_count__descriptor;
extern const ProtobufCMessageDescriptor injector__multi_touch_data__descriptor;
extern const ProtobufCMessageDescriptor injector__multi_touch_msg__descriptor;
extern const ProtobufCEnumDescriptor    injector__multi_touch_msg__type__descriptor;
extern const ProtobufCMessageDescriptor injector__injector_msg__descriptor;
extern const ProtobufCEnumDescriptor    injector__injector_msg__type__descriptor;

PROTOBUF_C_END_DECLS


#endif  /* PROTOBUF_tethering_2eproto__INCLUDED */
