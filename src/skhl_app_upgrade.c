#include <stdio.h>
#include <string.h>

#include "osal.h"
#include "md5.h"
#include "debug.h"
#include "skhl_comm_define.h"
#include "skhl_comm_core.h"

enum
{
    UPGRADE_GET_VERSION = 0,
    UPGRADE_START_TRANS,
    UPGRADE_TRANS_FW,
    UPGRADE_END_TRANS,
    UPGRADE_FINISH,
};

extern skhl_upgrade_ack upgrade_ack;
uint8_t target_rule = COMM_TARGET_ID_CENTER_BOARD;

static skhl_result skhl_send_data(skhl_local_pack_attr_t *attr)
{
    skhl_local_pack_attr_t pack;

    pack.cmd_set    = attr->cmd_set;
    pack.cmd_id     = attr->cmd_id;
    pack.cmd_dir    = attr->cmd_dir;
    pack.target     = attr->target;
    pack.seq_id     = attr->seq_id;
    pack.data       = attr->data;
    pack.data_len   = attr->data_len;
    pack.version    = attr->version;

    return skhl_comm_send_data(&pack);
}

skhl_result skhl_app_send_get_version(void)
{
    skhl_local_pack_attr_t pack_attr  = {0};
    static uint32_t seq_id = 0;

    pack_attr.cmd_set = CMD_SET_COMMON;
    pack_attr.cmd_dir = PACKAGE_DIR_REQ;
    pack_attr.cmd_id = CMD_ID_GET_VERSION;
    pack_attr.target = target_rule;
    pack_attr.seq_id = seq_id++;
    pack_attr.data   = NULL;
    pack_attr.data_len = 0;
    pack_attr.version  = COMM_PROTOCOL_V0;

    return skhl_send_data(&pack_attr);
}

skhl_result skhl_app_send_start_trans(const char *file_name, uint32_t file_size)
{
    skhl_local_pack_attr_t pack_attr  = {0};
    static uint32_t seq_id = 0;
    upgrade_start_req_t req = {0};

    // memcpy(req.file_name, file_name, FILE_NAME_SIZE_MAX);
    memcpy(req.file_name, "sk_demo", FILE_NAME_SIZE_MAX);
    req.file_size = file_size;
    pack_attr.cmd_set = CMD_SET_COMMON;
    pack_attr.cmd_dir = PACKAGE_DIR_REQ;
    pack_attr.cmd_id = CMD_ID_START_TRANS;
    pack_attr.target = target_rule;
    pack_attr.seq_id = seq_id++;
    pack_attr.data   = (uint8_t *)&req;
    pack_attr.data_len = sizeof(upgrade_start_req_t);
    pack_attr.version  = COMM_PROTOCOL_V0;

    return skhl_send_data(&pack_attr);
}

skhl_result skhl_app_send_trans_fw(uint8_t *buff, uint32_t len, uint32_t seq)
{
    skhl_local_pack_attr_t pack_attr  = {0};
    static uint32_t seq_id = 0;
    upgrade_trans_req_t req = {0};

    skhl_print_str("@@@@@@ fread data: ", buff, len);

    req.data_size = len;
    memcpy(req.data, buff, len);
    req.data_seq = seq;

    pack_attr.cmd_set = CMD_SET_COMMON;
    pack_attr.cmd_dir = PACKAGE_DIR_REQ;
    pack_attr.cmd_id = CMD_ID_TRANS_FW;
    pack_attr.target = target_rule;
    pack_attr.seq_id = seq_id++;
    pack_attr.data   = (uint8_t *)&req;
    pack_attr.data_len = sizeof(upgrade_trans_req_t);
    pack_attr.version  = COMM_PROTOCOL_V0;

    return skhl_send_data(&pack_attr);
}

skhl_result skhl_app_send_end_trans(uint8_t *buff, uint32_t size)
{
    skhl_local_pack_attr_t pack_attr  = {0};
    static uint32_t seq_id = 0;
    upgrade_end_trans_req_t req = {0};

    memcpy(req.fw_md5, buff, size);
    pack_attr.cmd_set = CMD_SET_COMMON;
    pack_attr.cmd_dir = PACKAGE_DIR_REQ;
    pack_attr.cmd_id = CMD_ID_END_TRANS;
    pack_attr.target = target_rule;
    pack_attr.seq_id = seq_id++;
    pack_attr.data   = (uint8_t *)&req;
    pack_attr.data_len = sizeof(upgrade_end_trans_req_t);
    pack_attr.version  = COMM_PROTOCOL_V0;

    return skhl_send_data(&pack_attr);
}


skhl_result skhl_upgrade_fw(void *file)
{
    #define TRANS_FILE_DATA_SIZE    FILE_PACKAGE_SIZE
    #define MD5_SIZE                16

    FILE *pFile                 = NULL;
    uint8_t file_data[TRANS_FILE_DATA_SIZE];
    uint32_t file_size          = 0;
    uint8_t step                = UPGRADE_GET_VERSION;
    static uint8_t open_flag    = 0;
    uint32_t read_size          = 0;
    uint32_t re_trans           = 0;
    static uint32_t trans_size  = 0;
    static MD5_CTX md5_context  = {0};
    uint8_t md5_result[MD5_SIZE] = {0};
    skhl_result upgrade_result  = 0;
    static uint32_t send_seq    = 0;

    while (step != UPGRADE_FINISH)
    {
        log_info("Upgrading...\n");
        switch (step)
        {
            case UPGRADE_GET_VERSION:
                if (!upgrade_ack.get_version_ack)
                {
                    skhl_app_send_get_version();
                    log_info("sending get version ...\n");
                }
                else
                {
                    upgrade_ack.get_version_ack = 0;
                    step = UPGRADE_START_TRANS;
                }
                break;
            case UPGRADE_START_TRANS:
                if (FALSE == open_flag)
                {
                    open_flag = TRUE;
                    pFile = fopen(file, "rb+");
                    if (NULL == pFile)
                    {
                        step = UPGRADE_FINISH;
                        log_err("fopen %s failed!\n", (int8_t *)file);
                        upgrade_result = -1;
                        goto ERR_OUT;
                    }

                    fseek(pFile, 0, SEEK_END);
                    file_size = ftell(pFile);
                    rewind(pFile);
                }

                if (!upgrade_ack.start_trans_ack)
                {
                    MD5Init(&md5_context);
                    skhl_app_send_start_trans(file, file_size);
                    log_info("sending start trans ...\n");
                }
                else
                {
                    upgrade_ack.start_trans_ack = 0;
                    step = UPGRADE_TRANS_FW;
                    // the first package to send. avoid re-send a NULL buffer.
                    upgrade_ack.trans_fw_ack = 1;
                    log_info("Start to trans fw...\n");
                }
                break;
            case UPGRADE_TRANS_FW:
                if (upgrade_ack.trans_fw_ack == 0)
                {
                    log_info("did not get ack....\n");
                    re_trans = TRUE;
                }
                else
                {
                    log_info("get ack....send seq(%d) ack seq(%d)\n", send_seq, upgrade_ack.seq_id);
                    upgrade_ack.trans_fw_ack = 0;
                    /*if (send_seq == upgrade_ack.seq_id)
                    {
                        re_trans = FALSE;
                    }
                    else
                    {
                        // should not happend!
                        re_trans = TRUE;
                    }*/
                }

                if (re_trans == FALSE)
                {
                    if (trans_size < file_size)
                    {
                        memset(file_data, 0, TRANS_FILE_DATA_SIZE);
                        // get new data to trans from file.
                        read_size = fread(file_data, 1, TRANS_FILE_DATA_SIZE, pFile);
                        if (read_size < TRANS_FILE_DATA_SIZE)
                        {
                            log_warn("short read when read EOF!\n");
                        }

                        // if last read not enough to size TRANS_FILE_DATA_SIZE, pick zero in the end.
                        trans_size += TRANS_FILE_DATA_SIZE;
                        skhl_app_send_trans_fw(file_data, TRANS_FILE_DATA_SIZE, send_seq);
                        send_seq++;

                        MD5Update(&md5_context, file_data, TRANS_FILE_DATA_SIZE);
                        skhl_print_str("send fw data: ", file_data, TRANS_FILE_DATA_SIZE);
                    }
                    else
                    {
                        // if read to EOF...
                        step = UPGRADE_END_TRANS;
                        fclose(pFile);
                        open_flag = FALSE;
                    }
                }
                else
                {
                    // re-trans data last time we've read.
                    skhl_app_send_trans_fw(file_data, TRANS_FILE_DATA_SIZE, upgrade_ack.seq_id + 1);
                    skhl_print_str("re-send fw data: ", file_data, TRANS_FILE_DATA_SIZE);
                }
                break;
            case UPGRADE_END_TRANS:
                if (!upgrade_ack.end_trans_ack)
                {
                    MD5Final(md5_result, &md5_context);
                    skhl_app_send_end_trans(md5_result, MD5_SIZE);
                    skhl_print_str("MD5 result :", md5_result, MD5_SIZE);
                    log_info("sending end trans and check md5 ...\n");
                }
                else
                {
                    upgrade_ack.end_trans_ack = 0;
                    step = UPGRADE_FINISH;
                    if (upgrade_ack.upgrade_result == UPGRADE_SUCCESS)
                    {
                        log_info("upgrade successful!!!\n");
                        upgrade_result = 0;
                    }
                    else
                    {
                        log_info("upgrade failed!!! Re-try again!!!\n");
                        upgrade_result = -1;
                    }
                }
                break;
            case UPGRADE_FINISH:
                break;
            default:
                step = UPGRADE_FINISH;
                upgrade_result = -1;
                break;
        }
ERR_OUT:
        skhl_sleep(1000);
    }

    return upgrade_result;
}

