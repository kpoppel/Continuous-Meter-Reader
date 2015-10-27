/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.4-dev at Mon Oct 26 22:30:38 2015. */

#ifndef PB_METERREADER_PB_H_INCLUDED
#define PB_METERREADER_PB_H_INCLUDED
#include <pb.h>

#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Enum definitions */
typedef enum _MeterReader_LogMessage_Type {
    MeterReader_LogMessage_Type_ERROR = 0,
    MeterReader_LogMessage_Type_NOTE = 1
} MeterReader_LogMessage_Type;

typedef enum _MeterReader_Settings_SendProtocol {
    MeterReader_Settings_SendProtocol_ASCII = 0,
    MeterReader_Settings_SendProtocol_PROTOBUF = 1
} MeterReader_Settings_SendProtocol;

typedef enum _MeterReader_Settings_CommunicationChannel {
    MeterReader_Settings_CommunicationChannel_SERIAL = 0,
    MeterReader_Settings_CommunicationChannel_WIRELESS = 1
} MeterReader_Settings_CommunicationChannel;

typedef enum _MeterReader_Settings_SamplingMode {
    MeterReader_Settings_SamplingMode_ANALOG = 0,
    MeterReader_Settings_SamplingMode_DIGITAL = 1
} MeterReader_Settings_SamplingMode;

/* Struct definitions */
typedef struct _MeterReader_StartCalibration {
    uint8_t dummy_field;
} MeterReader_StartCalibration;

typedef struct _MeterReader_CounterUpdate {
    uint32_t meterId;
    uint32_t seriesId;
    uint64_t currentCounterValue;
} MeterReader_CounterUpdate;

typedef struct _MeterReader_LogMessage {
    MeterReader_LogMessage_Type type;
    char text[40];
} MeterReader_LogMessage;

typedef struct _MeterReader_Settings {
    uint32_t meterId;
    uint32_t seriesId;
    MeterReader_Settings_SendProtocol sendProtocol;
    MeterReader_Settings_CommunicationChannel communicationChannel;
    MeterReader_Settings_SamplingMode samplingMode;
    uint32_t threshold;
    uint32_t hysteresis;
    pb_size_t risingEdgeAmounts_count;
    uint32_t risingEdgeAmounts[6];
    pb_size_t fallingEdgeAmounts_count;
    uint32_t fallingEdgeAmounts[6];
} MeterReader_Settings;

typedef struct _MeterReader_Message {
    pb_size_t which_message;
    union {
        MeterReader_CounterUpdate update;
        MeterReader_StartCalibration calibrate;
        MeterReader_Settings settings;
        MeterReader_LogMessage log;
    } message;
} MeterReader_Message;

/* Default values for struct fields */

/* Initializer values for message structs */
#define MeterReader_Message_init_default         {0, {MeterReader_CounterUpdate_init_default}}
#define MeterReader_LogMessage_init_default      {(MeterReader_LogMessage_Type)0, ""}
#define MeterReader_CounterUpdate_init_default   {0, 0, 0}
#define MeterReader_StartCalibration_init_default {0}
#define MeterReader_Settings_init_default        {0, 0, (MeterReader_Settings_SendProtocol)0, (MeterReader_Settings_CommunicationChannel)0, (MeterReader_Settings_SamplingMode)0, 0, 0, 0, {0, 0, 0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0}}
#define MeterReader_Message_init_zero            {0, {MeterReader_CounterUpdate_init_zero}}
#define MeterReader_LogMessage_init_zero         {(MeterReader_LogMessage_Type)0, ""}
#define MeterReader_CounterUpdate_init_zero      {0, 0, 0}
#define MeterReader_StartCalibration_init_zero   {0}
#define MeterReader_Settings_init_zero           {0, 0, (MeterReader_Settings_SendProtocol)0, (MeterReader_Settings_CommunicationChannel)0, (MeterReader_Settings_SamplingMode)0, 0, 0, 0, {0, 0, 0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0}}

/* Field tags (for use in manual encoding/decoding) */
#define MeterReader_CounterUpdate_meterId_tag    1
#define MeterReader_CounterUpdate_seriesId_tag   2
#define MeterReader_CounterUpdate_currentCounterValue_tag 3
#define MeterReader_LogMessage_type_tag          1
#define MeterReader_LogMessage_text_tag          2
#define MeterReader_Settings_meterId_tag         1
#define MeterReader_Settings_seriesId_tag        2
#define MeterReader_Settings_sendProtocol_tag    3
#define MeterReader_Settings_communicationChannel_tag 4
#define MeterReader_Settings_samplingMode_tag    5
#define MeterReader_Settings_threshold_tag       6
#define MeterReader_Settings_hysteresis_tag      7
#define MeterReader_Settings_risingEdgeAmounts_tag 8
#define MeterReader_Settings_fallingEdgeAmounts_tag 9
#define MeterReader_Message_update_tag           1

#define MeterReader_Message_calibrate_tag        2

#define MeterReader_Message_settings_tag         3

#define MeterReader_Message_log_tag              4

/* Struct field encoding specification for nanopb */
extern const pb_field_t MeterReader_Message_fields[5];
extern const pb_field_t MeterReader_LogMessage_fields[3];
extern const pb_field_t MeterReader_CounterUpdate_fields[4];
extern const pb_field_t MeterReader_StartCalibration_fields[1];
extern const pb_field_t MeterReader_Settings_fields[10];

/* Maximum encoded size of messages (where known) */
#define MeterReader_Message_size                 116
#define MeterReader_LogMessage_size              48
#define MeterReader_CounterUpdate_size           23
#define MeterReader_StartCalibration_size        0
#define MeterReader_Settings_size                114

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define METERREADER_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
