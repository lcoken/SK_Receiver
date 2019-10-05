#ifndef __SKHL_APP_USR_CONFIG_H__
#define __SKHL_APP_USR_CONFIG_H__

#define USER_CONFIG_FILE_NAME "sk.dat"

#define USER_KEY_LEN        6
#define USER_SECRET_LEN     64
#define USER_DEVICE_ID_LEN  32
#define USER_DEVICE_TYPE_LEN 32


#define ERROR_CODE_DEVICE_LINK      0xF0
#define ERROR_CODE_SETTING_TIMEOUT  0xF1
#define ERROR_CODE_AKAS_INVALID     0xF2

typedef struct
{
    char usr_key[USER_KEY_LEN];
    char usr_secret[USER_SECRET_LEN];
    char dev_id[USER_DEVICE_ID_LEN];
    char dev_type[USER_DEVICE_TYPE_LEN];
} sk_user_config_file_t;

typedef struct
{
    int setting_ing;
    sk_user_config_file_t new_setting;
} sk_user_setting_ctx;

skhl_result skhl_usr_config(void *cfg, uint8_t *result);
#ifndef SK_WINDOWS
skhl_result skhl_phrase_usrkey_usrsecret(sk_user_config_file_t *usr_config);
skhl_result skhl_writing_usrkey_usrsecret(sk_user_config_file_t *usr_config);
#endif

#endif
