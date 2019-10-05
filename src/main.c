#include <stdio.h>
#include <string.h>
#ifdef SK_WINDOWS
#include <stdlib.h>
#endif
#include <getopt.h>

#include "debug.h"
#include "osal.h"

#include "skhl_data_typedef.h"
#include "skhl_comm_define.h"
#include "skhl_comm_app.h"
#ifndef SK_WINDOWS
#include "skhl_app_upgrade.h"
#endif
#include "skhl_app_usr_config.h"

uint32_t quit = 0;
uint32_t app_key_secret_done = 0;

const char *gga_port   = "/dev/ttyS1";
const char *comm_port  = "/dev/ttyS2";

extern sk_user_setting_ctx setting_ctx;

// : 带一个参数
// :: 带不定长参数
// 空 不带参数

extern void sdk_startup(char *gga_port,
                    char *usr_key,
                    char *usr_secret,
                    char *dev_id,
                    char *dev_type);

extern void skhl_send_testing(void);


int main(int32_t argc, char **argv)
{
    skhl_result         ret             = 0;
    comm_user_config_t  config          = {0};
    sk_user_config_file_t usr_setting   = {0};

    /* Device Mode. */
    /* 1. opem comm port and initial for communication. */
    /* 2. monitor command from host. */

    log_info("Work in device mode!\n");

    config.rule = COMM_TARGET_ID_CENTER_BOARD;
    config.port = comm_port;
    ret = skhl_comm_init((void *)&config);
    if (ret != 0)
    {
        log_err("comm init err!\n");
        goto COMM_INIT_ERR;
    }

    ret = skhl_phrase_usrkey_usrsecret(&usr_setting);
    if (ret != 0)
    {
        log_err("pharse usr key and secret err!\n");

        // wait for user setting...
        while (!app_key_secret_done)
        {
            log_err("waiting for app key and app secret config...\n");
            // skhl_send_testing();
            skhl_sleep(1000); /* 1s */;
        }

        // goto PHRASE_ERR;
        ret = skhl_phrase_usrkey_usrsecret(&usr_setting);
        if (ret != 0)
        {
            log_err("pharse usr key and secret err!\n");
            goto PHRASE_ERR;
        }
    }


    setting_ctx.setting_ing = FALSE;

    log_info("user setting  :\n");
    skhl_print_str("user key : ", usr_setting.usr_key, USER_KEY_LEN);
    skhl_print_str("user secret : ", usr_setting.usr_secret, USER_SECRET_LEN);
    skhl_print_str("user dev id : ", usr_setting.dev_id, USER_DEVICE_ID_LEN);
    skhl_print_str("user dev type : ", usr_setting.dev_type, USER_DEVICE_TYPE_LEN);

    log_info("user key      : %s\n", usr_setting.usr_key);
    log_info("user secret   : %s\n", usr_setting.usr_secret);
    log_info("user dev id   : %s\n", usr_setting.dev_id);
    log_info("user dev type : %s\n", usr_setting.dev_type);
    log_info("\nGGA port      : %s!\n", gga_port);

    sdk_startup((char *)gga_port,
                usr_setting.usr_key,
                usr_setting.usr_secret,
                usr_setting.dev_id,
                usr_setting.dev_type);

    quit = 1;
    skhl_comm_destory();

    return 0;

PHRASE_ERR:
    quit = 1;
    skhl_comm_destory();
COMM_INIT_ERR:
    return -1;
}


