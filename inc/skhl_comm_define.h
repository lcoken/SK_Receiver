#ifndef __SKHL_COMM_DEFINE_H__
#define __SKHL_COMM_DEFINE_H__

#define COMM_PROTOCOL_V0        0
#define COMM_PROTOCOL_RAW       1

#define CMD_SET_COMMON  0

#define CMD_ID_GET_VERSION      1
#define CMD_ID_REBOOT           2
#define CMD_ID_POWER_CTRL       3
#define CMD_ID_ENTER_UPGRADE    4
#define CMD_ID_GET_STATE        5
#define CMD_ID_START_TRANS      6
#define CMD_ID_TRANS_FW         7
#define CMD_ID_END_TRANS        8

#define CMD_ID_USR_SETTING      9
#define CMD_ID_WAIT_VERIFY      10

#define PACKAGE_DIR_REQ         0
#define PACKAGE_DIR_ACK         1


#define COMM_TARGET_ID_PC               0x10
#define COMM_TARGET_ID_UB482            0x20
#define COMM_TARGET_ID_CENTER_BOARD     0x30

#define SKHL_PACK_V0_HEAD_SOF   0x55
#define SKHL_PACK_V0_VERSION    0x00

#define PACK_MAX_LEN            2048

#define SKHL_PACK_LEN(len)      (sizeof(skhl_pack_v0_head_t) + sizeof(skhl_pack_v0_attr_t) + len + 2)
#define SKHL_DATA_LEN(len)      (len - sizeof(skhl_pack_v0_head_t) - sizeof(skhl_pack_v0_attr_t) - 2)
#define SKHL_V0_PAYLOAD_MAX_LEN (PACK_MAX_LEN - (sizeof(skhl_pack_v0_head_t) + sizeof(skhl_pack_v0_attr_t) + 2))
#define SKHL_RAW_PAYLOAD_MAX_LEN (PACK_MAX_LEN)


#define UPGRADE_SUCCESS     0xAA
#define UPGRADE_FAILED      0x55

typedef struct
{
    uint8_t get_version_ack;

    uint8_t start_trans_ack;
    uint8_t trans_fw_ack;
    uint8_t end_trans_ack;

    uint8_t usr_setting_ack;

    uint8_t verify_ack;
    uint8_t verify_result;

    uint8_t upgrade_result;

    int32_t seq_id;
} skhl_upgrade_ack;

typedef struct
{
    uint8_t sof;
    uint8_t protocol_ver;
    uint16_t pack_len;
    uint8_t reserved;
    uint8_t crc8;
} skhl_pack_v0_head_t;

typedef struct
{
    uint8_t             cmd_set;
    uint8_t             cmd_id;
    uint8_t             cmd_dir;
    uint8_t             target;
    uint8_t             source;
    uint8_t             reserved;
    uint16_t            seq_id;
} skhl_pack_v0_attr_t;

typedef struct
{
    skhl_pack_v0_head_t header;
    skhl_pack_v0_attr_t attribute;

    uint8_t             data[SKHL_V0_PAYLOAD_MAX_LEN];
} skhl_pack_v0_pack_t;

typedef struct
{
    uint8_t             data[SKHL_RAW_PAYLOAD_MAX_LEN];
} skhl_pack_raw_pack_t;

// for app layer
typedef struct
{
    uint8_t             cmd_set;
    uint8_t             cmd_id;
    uint8_t             cmd_dir;
    uint8_t             target;
    uint8_t             source;
    uint16_t            seq_id;
    uint8_t             *data;
    uint32_t            data_len;
} skhl_pack_info_t;

/********************* Upgrade Firmware ******************************/
#define FILE_NAME_SIZE_MAX  32
#define FILE_PACKAGE_SIZE   256

#define UP_FILE_SUCCESS         0
#define UP_FILE_START_FAILED    0xF0
#define UP_FILE_START_RETRY     0xF1
#define UP_FILE_SEQ_ERR         0xF2
#define UP_FILE_PACK_SIZE_ERR   0xF3
#define UP_FILE_WRITE_FAILED    0xF4
#define UP_FILE_MD5_ERR         0xF5

#define USR_SETTING_SUCCESS     0
#define USR_SETTING_LEN_ERR     0xE0
#define USR_SETTING_WRITE_ERR   0xE1
#define USR_SETTING_VERIFY_ERR  0xE2

#define USR_VERIFY_SUCCESS      0

/* START UPGRADE */
typedef struct
{
    char        file_name[FILE_NAME_SIZE_MAX];
    uint32_t    file_size;
} __attribute__((packed)) upgrade_start_req_t;

typedef struct
{
    uint8_t     ret_code;
} __attribute__((packed)) upgrade_start_ack_t;


/* TRANS FIRMWARE */
typedef struct
{
    uint32_t    data_size;
    uint8_t     data[FILE_PACKAGE_SIZE];
    int32_t     data_seq;
} __attribute__((packed)) upgrade_trans_req_t;

typedef struct
{
    uint8_t     ret_code;
    int32_t     data_seq;
} __attribute__((packed)) upgrade_trans_ack_t;

/* END TRANS */
typedef struct
{
    uint8_t     fw_md5[16];
} __attribute__((packed)) upgrade_end_trans_req_t;

typedef struct
{
    uint8_t     ret_code;
} __attribute__((packed)) upgrade_end_trans_ack_t;

/* USR setting */
typedef struct
{
    uint8_t     app_key[6];
    uint8_t     app_secret[64];
    uint8_t     device_id[32];
    uint8_t     device_type[32];
} __attribute__((packed)) usr_setting_req_t;

typedef struct
{
    uint8_t     ret_code;
} __attribute__((packed)) usr_setting_ack_t;

/* WAIT VERIFY RESULT */
typedef struct
{
    uint8_t     verify_result;
} __attribute__((packed)) verify_result_ack_t;

/********************* Upgrade Firmware end **************************/

#endif
