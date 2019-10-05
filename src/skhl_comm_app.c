#include <stdio.h>
#include <string.h>

// #include <process.h>

#include "skhl_data_typedef.h"
#include "skhl_comm_define.h"
#include "skhl_comm_core.h"
#include "skhl_comm_uart.h"
#include "skhl_comm_app.h"

#include "debug.h"
#include "osal.h"
#include "md5.h"

#include "skhl_app_usr_config.h"

static skhl_comm_router_t user_router[] = {
    {OPT_UART, COMM_TARGET_ID_PC},
};

skhl_upgrade_ack upgrade_ack = {
    .seq_id = -1,
};

extern sk_user_setting_ctx setting_ctx;

static FILE *pFile = NULL;
static uint32_t fw_file_size = 0;
static MD5_CTX md5_context  = {0};
static int32_t fw_seq_last = 0;
static uint8_t file_name[32] = {0};
uint8_t fw_upgrading = FALSE;

/*============================================================================*/
static skhl_result skhl_get_version(skhl_local_pack_attr_t *pack)
{
    skhl_result ret             = 0;
    skhl_local_pack_attr_t pack_attr  = {0};
    uint32_t version            = VERS;

    log_info("%s...\n", __func__);

    setting_ctx.setting_ing = TRUE;

    if (pack->cmd_dir == PACKAGE_DIR_ACK)
    {
        upgrade_ack.get_version_ack = 1;
    }
    else
    {
        pack_attr.cmd_set   = pack->cmd_set;
        pack_attr.cmd_id    = pack->cmd_id;
        pack_attr.target    = pack->source;
        pack_attr.seq_id    = pack->seq_id;
        pack_attr.data      = (uint8_t *)&version;
        pack_attr.data_len  = sizeof(uint32_t);
        pack_attr.cmd_dir   = PACKAGE_DIR_ACK;
        pack_attr.version   = COMM_PROTOCOL_V0;
        ret = skhl_comm_send_data(&pack_attr);
    }

    return ret;
}

void skhl_send_testing(void)
{
    skhl_local_pack_attr_t pack_attr  = {0};

    pack_attr.cmd_set   = 0xAA;
    pack_attr.cmd_id    = 0X55;
    pack_attr.target    = COMM_TARGET_ID_PC;
    pack_attr.seq_id    = 0x1234;
    pack_attr.data      = NULL;
    pack_attr.data_len  = 0;
    pack_attr.cmd_dir   = PACKAGE_DIR_REQ;
    pack_attr.version   = COMM_PROTOCOL_V0;
    skhl_comm_send_data(&pack_attr);
}


static skhl_result skhl_reboot(skhl_local_pack_attr_t *pack)
{
    skhl_result ret             = 0;
    skhl_local_pack_attr_t pack_attr  = {0};

    log_info("%s...\n", __func__);

    if (pack->cmd_dir == PACKAGE_DIR_ACK)
    {
        // do nothing.
    }
    else
    {
        pack_attr.cmd_set   = pack->cmd_set;
        pack_attr.cmd_id    = pack->cmd_id;
        pack_attr.target    = pack->source;
        pack_attr.seq_id    = pack->seq_id;
        pack_attr.data      = NULL;
        pack_attr.data_len  = 0;
        pack_attr.cmd_dir   = PACKAGE_DIR_ACK;
        pack_attr.version   = COMM_PROTOCOL_V0;
        ret = skhl_comm_send_data(&pack_attr);
    }
    system("./upgrade_deal.sh reboot");

    return ret;
}

static skhl_result skhl_start_trans(skhl_local_pack_attr_t *pack)
{
    skhl_result ret             = 0;
    skhl_local_pack_attr_t pack_attr  = {0};

    log_info("%s...\n", __func__);

    if (pack->cmd_dir == PACKAGE_DIR_ACK)
    {
        upgrade_ack.start_trans_ack = 1;
    }
    else
    {
        upgrade_start_req_t *req = (upgrade_start_req_t *)pack->data;
        upgrade_start_ack_t ack = {0};

        if (!strncmp(req->file_name, "sk_client", 9))
        {
            ack.ret_code = UP_FILE_START_FAILED;
            goto ERR_OUT;
        }
        memcpy(file_name, req->file_name, 32);
        pFile = fopen(req->file_name, "wb+");
        if (pFile == NULL)
        {
            ack.ret_code = UP_FILE_START_FAILED;
            goto ERR_OUT;
        }
        ack.ret_code = UP_FILE_SUCCESS;
        fw_file_size = req->file_size;
        fw_seq_last = -1;
        MD5Init(&md5_context);
        fw_upgrading = TRUE;

ERR_OUT:
        pack_attr.cmd_set   = pack->cmd_set;
        pack_attr.cmd_id    = pack->cmd_id;
        pack_attr.target    = pack->source;
        pack_attr.seq_id    = pack->seq_id;
        pack_attr.data      = (uint8_t *)&ack;
        pack_attr.data_len  = sizeof(upgrade_start_ack_t);
        pack_attr.cmd_dir   = PACKAGE_DIR_ACK;
        pack_attr.version   = COMM_PROTOCOL_V0;
        ret = skhl_comm_send_data(&pack_attr);
    }

    return ret;
}

static skhl_result skhl_trans_fw(skhl_local_pack_attr_t *pack)
{
    skhl_result ret             = 0;
    skhl_local_pack_attr_t pack_attr  = {0};
    uint32_t write_ret = 0;

    log_info("%s...seq = (%d) \n", __func__, pack->seq_id);

    // deal ack pack. device -> host
    if (pack->cmd_dir == PACKAGE_DIR_ACK)
    {
        upgrade_trans_ack_t *ack = (upgrade_trans_ack_t *)pack->data;

        skhl_print_str("seq debug debug debug :", pack->data, pack->data_len);
        upgrade_ack.trans_fw_ack = 1;
        upgrade_ack.seq_id = ack->data_seq;
        if (ack->ret_code == UP_FILE_SUCCESS)
        {
            log_info("@@@@ trans success! seq: (0x%x)\n", upgrade_ack.seq_id);
        }
        else
        {
            log_info("@@@@ trans fail! need to re-trans! seq: (%d)\n", upgrade_ack.seq_id);
        }
    }
    // deal req pack. host -> device
    else
    {
        upgrade_trans_req_t *req = (upgrade_trans_req_t *)pack->data;
        upgrade_trans_ack_t ack = {0};

        log_info("@@@@trans fw get seq = %d, we want seq = %d + 1\n", req->data_seq, fw_seq_last) ;
        if (req->data_seq != fw_seq_last + 1)
        {
            ack.ret_code = UP_FILE_SEQ_ERR;
            ack.data_seq = fw_seq_last;
            goto ERR_OUT;
        }
        else
        {
            if (req->data_size != FILE_PACKAGE_SIZE)
            {
                ack.ret_code = UP_FILE_PACK_SIZE_ERR;
                goto ERR_OUT;
            }
            else
            {

    skhl_print_str("###### $$$$$$ fwrite data: ", req->data, FILE_PACKAGE_SIZE);

                write_ret = fwrite(req->data, 1, FILE_PACKAGE_SIZE, pFile);
                if (write_ret != FILE_PACKAGE_SIZE)
                {
                    log_err("write failed!\n");
                    ack.ret_code = UP_FILE_WRITE_FAILED;
                    goto ERR_OUT;
                }
                fw_file_size += FILE_PACKAGE_SIZE;
                MD5Update(&md5_context, req->data, FILE_PACKAGE_SIZE);
                fw_seq_last++;
            }
        }
        ack.ret_code = UP_FILE_SUCCESS;
        ack.data_seq = fw_seq_last;

ERR_OUT:
        log_info("$$$ sending seq (%d) err_code = (0x%x)\n", ack.data_seq, ack.ret_code);
        pack_attr.cmd_set   = pack->cmd_set;
        pack_attr.cmd_id    = pack->cmd_id;
        pack_attr.target    = pack->source;
        pack_attr.seq_id    = pack->seq_id;
        pack_attr.data      = (uint8_t *)&ack;
        pack_attr.data_len  = sizeof(upgrade_trans_ack_t);
        pack_attr.cmd_dir   = PACKAGE_DIR_ACK;
        pack_attr.version   = COMM_PROTOCOL_V0;
        ret = skhl_comm_send_data(&pack_attr);
    }

    return ret;
}

static skhl_result skhl_end_trans(skhl_local_pack_attr_t *pack)
{
    skhl_result ret             = 0;
    skhl_local_pack_attr_t pack_attr  = {0};
    uint8_t md5_result[16]      = {0};
    char cmd_string[64]         = {0};

    log_info("%s...\n", __func__);

    if (pack->cmd_dir == PACKAGE_DIR_ACK)
    {
        upgrade_end_trans_ack_t *ack = (upgrade_end_trans_ack_t *)pack->data;

        if (ack->ret_code == UP_FILE_SUCCESS)
        {
            upgrade_ack.end_trans_ack = 1;
            upgrade_ack.upgrade_result = UPGRADE_SUCCESS;
        }
        else
        {
            upgrade_ack.end_trans_ack = 1;
            upgrade_ack.upgrade_result = UPGRADE_FAILED;
        }
    }
    else
    {
        upgrade_end_trans_req_t *req = (upgrade_end_trans_req_t *)pack->data;
        upgrade_end_trans_ack_t ack = {0};

        if (pFile)
        {
            fclose(pFile);
            pFile = NULL;
        }

        MD5Final(md5_result, &md5_context);
        if (strncmp((const char *)md5_result, (const char *)req->fw_md5, 16) != 0)
        {
            ack.ret_code = UP_FILE_MD5_ERR;
            goto ERR_OUT;
        }
        log_info("Refresh File %s success!!!\n", file_name);
        ack.ret_code = UP_FILE_SUCCESS;
        sprintf(cmd_string, "./sk_dela.sh refresh %s", file_name);
        system(cmd_string);
        fw_upgrading = FALSE;

ERR_OUT:
        log_info("End trans result: 0x%x\n", ack.ret_code);
        skhl_print_str("MD5 want: ", req->fw_md5, 16);
        skhl_print_str("MD5 get : ", md5_result, 16);
        pack_attr.cmd_set   = pack->cmd_set;
        pack_attr.cmd_id    = pack->cmd_id;
        pack_attr.target    = pack->source;
        pack_attr.seq_id    = pack->seq_id;
        pack_attr.data      = (uint8_t *)&ack;
        pack_attr.data_len  = sizeof(upgrade_end_trans_ack_t);
        pack_attr.cmd_dir   = PACKAGE_DIR_ACK;
        pack_attr.version   = COMM_PROTOCOL_V0;
        ret = skhl_comm_send_data(&pack_attr);
    }

    return ret;
}

static skhl_result skhl_usr_setting(skhl_local_pack_attr_t *pack)
{
    skhl_result ret = 0;

    log_info("%s...\n", __func__);

    if (pack->cmd_dir == PACKAGE_DIR_ACK)
    {

    }
    else
    {
        usr_setting_req_t *setting = (usr_setting_req_t *)pack->data;
        usr_setting_ack_t ack = {0};
        sk_user_config_file_t usr_config = {0};
        skhl_local_pack_attr_t pack_attr  = {0};

        if (pack->data_len != sizeof(usr_setting_req_t))
        {
            ack.ret_code = USR_SETTING_LEN_ERR;
            goto ERR_OUT;
        }

        log_info("user setting  :\n");
        log_info("user key      : %s\n", setting->app_key);
        log_info("user secret   : %s\n", setting->app_secret);
        log_info("user dev id   : %s\n", setting->device_id);
        log_info("user dev type : %s\n", setting->device_type);

        memcpy(usr_config.usr_key, setting->app_key, USER_KEY_LEN);
        memcpy(usr_config.usr_secret, setting->app_secret, USER_SECRET_LEN);
        memcpy(usr_config.dev_id, setting->device_id, USER_DEVICE_ID_LEN);
        memcpy(usr_config.dev_type, setting->device_type, USER_DEVICE_TYPE_LEN);

        ret = skhl_writing_usrkey_usrsecret(&usr_config);
        if (ret != 0)
        {
            ack.ret_code = USR_SETTING_WRITE_ERR;
            goto ERR_OUT;
        }
        ack.ret_code = USR_SETTING_SUCCESS;

        memcpy(&setting_ctx.new_setting, &usr_config, sizeof(sk_user_config_file_t));
        setting_ctx.setting_ing = FALSE;

ERR_OUT:
        log_info("usr setting result: 0x%x\n", ack.ret_code);
        pack_attr.cmd_set   = pack->cmd_set;
        pack_attr.cmd_id    = pack->cmd_id;
        pack_attr.target    = pack->source;
        pack_attr.seq_id    = pack->seq_id;
        pack_attr.data      = (uint8_t *)&ack;
        pack_attr.data_len  = sizeof(usr_setting_ack_t);
        pack_attr.cmd_dir   = PACKAGE_DIR_ACK;
        pack_attr.version   = COMM_PROTOCOL_V0;
        ret = skhl_comm_send_data(&pack_attr);
    }

    return ret;
}

extern uint32_t app_key_secret_done;

static skhl_result skhl_usr_verify(skhl_local_pack_attr_t *pack)
{
    skhl_result ret = 0;

    if (pack->cmd_dir == PACKAGE_DIR_ACK)
    {
    }
    else
    {
        verify_result_ack_t ack = {0};
        skhl_local_pack_attr_t pack_attr  = {0};

        ack.verify_result = USR_VERIFY_SUCCESS;
        app_key_secret_done = 1;

        log_info("usr verify result: 0x%x\n", ack.verify_result);
        pack_attr.cmd_set   = pack->cmd_set;
        pack_attr.cmd_id    = pack->cmd_id;
        pack_attr.target    = pack->source;
        pack_attr.seq_id    = pack->seq_id;
        pack_attr.data      = (uint8_t *)&ack;
        pack_attr.data_len  = sizeof(verify_result_ack_t);
        pack_attr.cmd_dir   = PACKAGE_DIR_ACK;
        pack_attr.version   = COMM_PROTOCOL_V0;
        ret = skhl_comm_send_data(&pack_attr);
    }

    return ret;
}


static skhl_comm_item_t comm_cb[] =
{
    {CMD_SET_COMMON,    CMD_ID_GET_VERSION,    skhl_get_version    },
    {CMD_SET_COMMON,    CMD_ID_START_TRANS,    skhl_start_trans    },
    {CMD_SET_COMMON,    CMD_ID_TRANS_FW,       skhl_trans_fw       },
    {CMD_SET_COMMON,    CMD_ID_END_TRANS,      skhl_end_trans      },
    {CMD_SET_COMMON,    CMD_ID_REBOOT,         skhl_reboot         },
    {CMD_SET_COMMON,    CMD_ID_USR_SETTING,    skhl_usr_setting    },
    {CMD_SET_COMMON,    CMD_ID_WAIT_VERIFY,    skhl_usr_verify     },
};


/*======================== Entry for Comm ===============================*/
skhl_result skhl_comm_init(void *usr_config)
{
    comm_user_config_t *config_local = usr_config;
    skhl_result             ret     = 0;
    comm_attr_t             attr    = {
        .name = config_local->port,
    };
    skhl_comm_core_config_t config  = {
        .cb             = comm_cb,
        .cb_size        = ARRAY_SIZE(comm_cb),
        .router         = user_router,
        .router_size    = ARRAY_SIZE(user_router),
        .this_host      = config_local->rule,
    };

    log_info("comm rule = 0x%x\n", config_local->rule);
    ret = skhl_comm_set_attr(OPT_UART, &attr);
    if (ret != 0)
    {
        log_err("set attr error!\n");
        goto ERR_SET_ATTR;
    }
    log_info("set port = %s\n", (int8_t *)config_local->port);

    ret = skhl_comm_uart_init();
    if (ret != 0)
    {
        log_err("comm device uart init failed!\n");
        goto ERR_COMM_UART_INIT;
    }

    ret = skhl_comm_core_init(&config);
    if (ret != 0)
    {
        log_err("comm core init failed!\n");
        skhl_comm_uart_destory();
        goto ERR_COMM_CORE_INIT;
    }

    log_debug("Success comm init!\n");

    return 0;

ERR_COMM_CORE_INIT:
    skhl_comm_uart_destory();
ERR_COMM_UART_INIT:
    skhl_comm_clear_attr(OPT_UART);
ERR_SET_ATTR:
    return ret;
}

skhl_result skhl_comm_destory(void)
{
    skhl_comm_core_destory();
    skhl_comm_uart_destory();
    return 0;
}

