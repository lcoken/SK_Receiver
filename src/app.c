#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "qxwz_sdk.h"

#include "debug.h"
#include "skhl_data_typedef.h"
#include "skhl_hal_uart.h"
#include "skhl_app_usr_config.h"

#define GGA_STR_MAX_LEN     512
#define DEMO_GGA_STR        "$GPGGA,000001,3112.518576,N,12127.901251,E,1,8,1,0,M,-32,M,3,0*4B"

#define DEMO_LOG(fmt, ...)  log_info("[DEMO]"fmt, ##__VA_ARGS__)


static qxwz_uint32_t sdk_auth_flag = 0;
static qxwz_uint32_t sdk_start_flag = 0;
qxwz_sdk_cap_info_t sdk_cap_info = {0};

extern sk_user_setting_ctx setting_ctx;
extern uint8_t fw_upgrading;

static char gga_data_tmp[GGA_STR_MAX_LEN] = {0};
static char gga_data[GGA_STR_MAX_LEN] = DEMO_GGA_STR;
static skhl_handle gga_handle  = NULL;

static qxwz_void_t demo_show_caps(qxwz_sdk_cap_info_t *cap_info)
{
    qxwz_int32_t loop = 0;

    DEMO_LOG("total capabilities: %d\n", cap_info->caps_num);
    for (loop = 0; loop < cap_info->caps_num; ++loop) {
        DEMO_LOG("idx: %d, cap_id: %u, state: %d, act_method: %d, expire_time: %llu\n",
            loop + 1,
            cap_info->caps[loop].cap_id,
            cap_info->caps[loop].state,
            cap_info->caps[loop].act_method,
            cap_info->caps[loop].expire_time);
    }
}

static qxwz_void_t demo_on_auth(qxwz_int32_t status_code, qxwz_sdk_cap_info_t *cap_info) {
    if (status_code == QXWZ_SDK_STAT_AUTH_SUCC) {
        sdk_auth_flag = 1;
        sdk_cap_info = *cap_info;
        demo_show_caps(cap_info);
        DEMO_LOG("auth successfully!\n");
    } else {
        DEMO_LOG("auth failed, code=%d\n", status_code);
    }
}

static qxwz_void_t demo_on_start(qxwz_int32_t status_code, qxwz_uint32_t cap_id) {
    DEMO_LOG("on start cap:status_code=%d, cap_id=%d\n", status_code, cap_id);
    sdk_start_flag = 1;
}


static qxwz_void_t demo_on_status(int code)
{
    DEMO_LOG(" on status code: %d\n", code);
}

static qxwz_void_t demo_on_data(qxwz_uint32_t type, const qxwz_void_t *data, qxwz_uint32_t len)
{
    DEMO_LOG(" on data: %d, ptr: %p, len: %d\n", type, data, len);

    switch (type) {
        case QXWZ_SDK_DATA_TYPE_RAW_NOSR:
            DEMO_LOG("QXWZ_SDK_DATA_TYPE_RAW_NOSR\n");
            skhl_print_str("Write GGA:", (uint8_t *)data, (uint32_t)len);

            if (gga_handle != NULL &&
                (setting_ctx.setting_ing == FALSE) && (fw_upgrading == FALSE))
            {
                skhl_hal_uart_write_data(gga_handle, (uint8_t *)data, (uint32_t)len);
            }
            break;
        default:
            DEMO_LOG("unknown type: %d\n", type);
    }
}

typedef enum
{
    SDK_STARTUP = 0,
    SDK_WORKING,
    SDK_REFRESHING_CONFIG,
} sdk_step_e;

static void sdk_refresh_gga(char *usr_key,
                            char *usr_secret,
                            char *dev_id,
                            char *dev_type)
{
    struct timeval      tval        = {0};
    qxwz_sdk_config_t   sdk_config  = {0};
    unsigned int        tick        = 0;
    uint32_t            data_size   = 0;
    sdk_step_e          step        = SDK_STARTUP;

    sdk_config.key_type = QXWZ_SDK_KEY_TYPE_AK;
    strcpy(sdk_config.key, usr_key);
    strcpy(sdk_config.secret, usr_secret);
    strcpy(sdk_config.dev_id, dev_id);
    strcpy(sdk_config.dev_type, dev_type);

    DEMO_LOG("user app key: #%s#\n", usr_key);
    DEMO_LOG("user app secret: #%s#\n", usr_secret);
    DEMO_LOG("user dev id: #%s#\n", dev_id);
    DEMO_LOG("user dev type: #%s#\n", dev_type);

    sdk_config.status_cb    = demo_on_status;
    sdk_config.data_cb      = demo_on_data;
    sdk_config.auth_cb      = demo_on_auth;
    sdk_config.start_cb     = demo_on_start;

    gettimeofday(&tval, NULL);

    while (1) {
        switch (step)
        {
            case SDK_STARTUP:
                qxwz_sdk_init(&sdk_config);
                qxwz_sdk_auth();
                step = SDK_WORKING;
                break;
            case SDK_WORKING:
                if (setting_ctx.setting_ing == FALSE || fw_upgrading == FALSE)
                {
                    if (gga_handle != NULL)
                    {
                        data_size = skhl_hal_uart_read_data(gga_handle,
                                                (uint8_t *)gga_data_tmp, GGA_STR_MAX_LEN);
                        if (data_size != 0)
                        {
                           skhl_print_str("Read GGA:", (uint8_t *)gga_data, data_size);
                           /* upload GGA */
                           memcpy(gga_data, gga_data_tmp, GGA_STR_MAX_LEN);
                        }
                    }

                    if ((tick++ % 10) == 0)
                    {
                        qxwz_sdk_upload_gga(gga_data, strlen(gga_data));
                        DEMO_LOG("uploading gga data: %s ...\n", gga_data);
                    }

                    gettimeofday(&tval, NULL);
                    usleep(1000 * 1000); /* 1000ms */
                    // qxwz_sdk_tick(tval.tv_sec);

                    if (sdk_auth_flag > 0) {
                        sdk_auth_flag = 0;
                        if (sdk_cap_info.caps_num > 0) {
                            DEMO_LOG("starting sdk capability...\n");
                            qxwz_sdk_start(QXWZ_SDK_CAP_ID_NOSR);   /* start NOSR capability */
                        }
                    }
                }
                else
                {
                    step = SDK_REFRESHING_CONFIG;
                }
                break;
            case SDK_REFRESHING_CONFIG:
                qxwz_sdk_stop(QXWZ_SDK_CAP_ID_NOSR);   /* stop NOSR capability */
                qxwz_sdk_cleanup();
                if (setting_ctx.setting_ing == FALSE)
                {
                    strcpy(sdk_config.key, setting_ctx.new_setting.usr_key);
                    strcpy(sdk_config.secret, setting_ctx.new_setting.usr_secret);
                    strcpy(sdk_config.dev_id, setting_ctx.new_setting.dev_id);
                    strcpy(sdk_config.dev_type, setting_ctx.new_setting.dev_type);

                    if (fw_upgrading == FALSE)
                    {
                        step = SDK_STARTUP;
                    }
                }
                usleep(1000 * 1000); /* 1s */
                break;
            default:
                break;
        }

    }

    /* close gga uart */
    if (gga_handle != NULL)
    {
        skhl_hal_uart_close(gga_handle);
        gga_handle = NULL;
    }
}

void sdk_startup(char *gga_port,
                    char *usr_key,
                    char *usr_secret,
                    char *dev_id,
                    char *dev_type)
{
    if (gga_port != NULL)
    {
        gga_handle = skhl_hal_uart_init(gga_port);
        if (gga_handle == NULL)
        {
            DEMO_LOG("hal uart init %s failed!\n", gga_port);
            return;
        }

        sdk_refresh_gga(usr_key, usr_secret, dev_id, dev_type);
    }
}


