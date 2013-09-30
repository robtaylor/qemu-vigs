/* Generated by the protocol buffer compiler.  DO NOT EDIT! */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C_NO_DEPRECATED
#define PROTOBUF_C_NO_DEPRECATED
#endif

#include "ecs_ids.pb-c.h"
const ProtobufCEnumValue ecs__master__type__enum_values_by_number[15] =
{
  { "CHECKVERSION_REQ", "ECS__MASTER__TYPE__CHECKVERSION_REQ", 2 },
  { "CHECKVERSION_ANS", "ECS__MASTER__TYPE__CHECKVERSION_ANS", 3 },
  { "KEEPALIVE_REQ", "ECS__MASTER__TYPE__KEEPALIVE_REQ", 4 },
  { "KEEPALIVE_ANS", "ECS__MASTER__TYPE__KEEPALIVE_ANS", 5 },
  { "INJECTOR_REQ", "ECS__MASTER__TYPE__INJECTOR_REQ", 6 },
  { "INJECTOR_ANS", "ECS__MASTER__TYPE__INJECTOR_ANS", 7 },
  { "INJECTOR_NTF", "ECS__MASTER__TYPE__INJECTOR_NTF", 8 },
  { "DEVICE_REQ", "ECS__MASTER__TYPE__DEVICE_REQ", 9 },
  { "DEVICE_ANS", "ECS__MASTER__TYPE__DEVICE_ANS", 10 },
  { "DEVICE_NTF", "ECS__MASTER__TYPE__DEVICE_NTF", 11 },
  { "MONITOR_REQ", "ECS__MASTER__TYPE__MONITOR_REQ", 12 },
  { "MONITOR_ANS", "ECS__MASTER__TYPE__MONITOR_ANS", 13 },
  { "MONITOR_NTF", "ECS__MASTER__TYPE__MONITOR_NTF", 14 },
  { "NFC_REQ", "ECS__MASTER__TYPE__NFC_REQ", 101 },
  { "NFC_NTF", "ECS__MASTER__TYPE__NFC_NTF", 102 },
};
static const ProtobufCIntRange ecs__master__type__value_ranges[] = {
{2, 0},{101, 13},{0, 15}
};
const ProtobufCEnumValueIndex ecs__master__type__enum_values_by_name[15] =
{
  { "CHECKVERSION_ANS", 1 },
  { "CHECKVERSION_REQ", 0 },
  { "DEVICE_ANS", 8 },
  { "DEVICE_NTF", 9 },
  { "DEVICE_REQ", 7 },
  { "INJECTOR_ANS", 5 },
  { "INJECTOR_NTF", 6 },
  { "INJECTOR_REQ", 4 },
  { "KEEPALIVE_ANS", 3 },
  { "KEEPALIVE_REQ", 2 },
  { "MONITOR_ANS", 11 },
  { "MONITOR_NTF", 12 },
  { "MONITOR_REQ", 10 },
  { "NFC_NTF", 14 },
  { "NFC_REQ", 13 },
};
const ProtobufCEnumDescriptor ecs__master__type__descriptor =
{
  PROTOBUF_C_ENUM_DESCRIPTOR_MAGIC,
  "ECS.Master_Type",
  "Master_Type",
  "ECS__MasterType",
  "ECS",
  15,
  ecs__master__type__enum_values_by_number,
  15,
  ecs__master__type__enum_values_by_name,
  2,
  ecs__master__type__value_ranges,
  NULL,NULL,NULL,NULL   /* reserved[1234] */
};
