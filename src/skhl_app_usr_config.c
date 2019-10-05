#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include "debug.h"
#include "osal.h"

#include "skhl_data_typedef.h"
#include "skhl_app_usr_config.h"

sk_user_setting_ctx setting_ctx = {
    .setting_ing = TRUE,
};


skhl_result skhl_phrase_usrkey_usrsecret(sk_user_config_file_t *usr_config)
{
    skhl_result ret = 0;
    // skhl_handle file_handle = NULL;
    FILE *file_handle = NULL;
    file_attr  file = {
        .name = USER_CONFIG_FILE_NAME,
    };
    int32_t real_size = 0;

    if (NULL == usr_config)
    {
        log_err("param error when phrasing setting...\n");
        return -1;
    }

    /* file_handle = file_init(&file);
    if (NULL == file_handle)
    {
        log_err("file init %s error!\n", file.name);
        return -1;
    }*/
    log_err("fopen %s!\n", file.name);
    file_handle = fopen(file.name, "r+");
    if (file_handle == NULL)
    {
        log_err("fopen %s error!\n", file.name);
        return -1;
    }

    log_err("fread %s!\n", file.name);
    ret = fread(usr_config, 1, sizeof(sk_user_config_file_t), file_handle);
    // ret = file_read(file_handle, (uint8_t *)&usr_config, sizeof(sk_user_config_file_t), &real_size);
    if ((ret != sizeof(sk_user_config_file_t)))
    {
        log_err("file read %s error! Read = %d Real = %d\n", file.name, (int32_t)sizeof(sk_user_config_file_t), real_size);
        fclose(file_handle);
        //file_close(file_handle);
        return -1;
    }
    log_err("fread %s = %d!\n", file.name, ret);

    fclose(file_handle);
    // file_close(file_handle);
    return 0;
}


skhl_result skhl_writing_usrkey_usrsecret(sk_user_config_file_t *usr_config)
{
    skhl_result ret = 0;
    // skhl_handle file_handle = NULL;
    FILE *file_handle = NULL;
    file_attr  file = {
        .name = USER_CONFIG_FILE_NAME,
    };
    int32_t real_size = 0;

    if (NULL == usr_config)
    {
        log_err("param error when writing setting...\n");
        return -1;
    }

    log_err("fopen %s!\n", file.name);
    file_handle = fopen(file.name, "w+");
    if (file_handle == NULL)
    {
        log_err("fopen %s error!\n", file.name);
        return -1;
    }

    /* file_handle = file_init(&file);
    if (NULL == file_handle)
    {
        log_err("file init %s error!\n", file.name);
        return -1;
    } */

    // ret = file_write(file_handle, (uint8_t *)&usr_config, sizeof(sk_user_config_file_t), &real_size);
    // if ((ret < 0) || (real_size != sizeof(sk_user_config_file_t)))
    ret = fwrite(usr_config, 1, sizeof(sk_user_config_file_t), file_handle);
    if (ret != sizeof(sk_user_config_file_t))
    {
        log_err("file write %s error! Write = %d Real = %d\n", file.name, (int32_t)sizeof(sk_user_config_file_t), real_size);
        fclose(file_handle);
        // file_close(file_handle);
        return -1;
    }

    fclose(file_handle);
    //file_close(file_handle);
    return 0;
}


