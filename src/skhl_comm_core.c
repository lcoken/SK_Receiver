#include <stdio.h>
#include <string.h>

// #include <process.h>

#include "skhl_data_typedef.h"
#include "skhl_comm_define.h"
#include "skhl_comm_core.h"

#include "debug.h"
#include "crc.h"
#include "osal.h"
#include "ring_buffer.h"

/*========================Defination==========================================*/
#define DATA_BUFFER_SIZE (512 * 1024)

/*========================Struction & Enum====================================*/
typedef struct
{
    ring_buffer_t           rb_desc;
    skhl_comm_item_t        *cb_array;
    uint32_t                cb_array_size;
    skhl_opt_t              *device_opts[COMM_OPT_MAX];
    skhl_handle             device_handle[COMM_OPT_MAX];
    comm_attr_t             device_attr[COMM_OPT_MAX];
    skhl_handle             phrase_event;
    skhl_handle             phrase_core_task;
    skhl_handle             read_data_task;
    skhl_comm_router_t      *router;
    uint32_t                router_size;
    uint8_t                 this_host;
} skhl_comm_t;

enum
{
    LOOKING_FOR_HEADER = 0,
    CHECKING_CRC8,
    CHECKING_CRC16,
    GO_OUT,
};
/*========================Global var==========================================*/

extern uint32_t quit;

static skhl_comm_t comm_context;
static uint8_t comm_data_buffer[DATA_BUFFER_SIZE];

/*========================Global var==========================================*/
static skhl_result skhl_find_link(skhl_local_pack_attr_t *attr, uint8_t *link)
{
    static skhl_comm_t  *context            = &comm_context;
    skhl_local_pack_attr_t *attr_local       = attr;
    skhl_comm_router_t *router              = context->router;

    if (attr->version == COMM_PROTOCOL_RAW)
    {
        for (uint8_t i = 0; i < context->router_size; i++)
        {
            if (COMM_TARGET_ID_PC == router[i].target)
            {
                *link = router[i].link_device;
                return 0;
            }
        }
    }
    else if (attr->version == COMM_PROTOCOL_V0)
    {
        for (uint8_t i = 0; i < context->router_size; i++)
        {
            if (attr_local->target == router[i].target)
            {
                *link = router[i].link_device;
                return 0;
            }
        }
    }
    else
    {
        log_err("Comm pack version not support!(pack ver = %d)\n", attr->version);
    }

    return -1;
}

static skhl_result skhl_pack_data(uint8_t *data, skhl_local_pack_attr_t *attr, uint32_t *real_size)
{
    static skhl_comm_t  *context            = &comm_context;

    if ((NULL == data) || (NULL == attr) || (NULL == real_size))
    {
        return -1;
    }

    if (attr->version == COMM_PROTOCOL_V0)
    {
        uint16_t crc16                  = 0;
        skhl_pack_v0_pack_t *package    = (skhl_pack_v0_pack_t *)data;

        if (attr->data_len > SKHL_V0_PAYLOAD_MAX_LEN)
        {
            log_err("error param len for send data!\n");
            return -1;
        }

        package->header.sof         = SKHL_PACK_V0_HEAD_SOF;
        package->header.pack_len    = SKHL_PACK_LEN(attr->data_len);
        package->header.crc8        = Get_Crc8(data, 3);

        package->attribute.cmd_set  = attr->cmd_set;
        package->attribute.cmd_id   = attr->cmd_id;
        package->attribute.seq_id   = attr->seq_id;
        package->attribute.cmd_dir  = attr->cmd_dir;
        package->attribute.target   = attr->target;
        package->attribute.source   = context->this_host;
        log_info("package rule : 0x%x\n", context->this_host);

        if ((attr->data_len !=  0) && (attr->data != NULL))
        {
            memcpy(package->data, attr->data, attr->data_len);
        }

        crc16 = Get_Crc16(data, SKHL_PACK_LEN(attr->data_len) - 2);
        package->data[attr->data_len]     = crc16 & 0x00FF;
        package->data[attr->data_len + 1] = crc16 >> 8;

        *real_size = SKHL_PACK_LEN(attr->data_len);
    }
    else if (attr->version == COMM_PROTOCOL_RAW)
    {
        skhl_pack_raw_pack_t *package = (skhl_pack_raw_pack_t *)data;

        if ((attr->data_len !=  0) && (attr->data != NULL))
        {
            if (attr->data_len <= SKHL_RAW_PAYLOAD_MAX_LEN)
            {
                memcpy(package->data, attr->data, attr->data_len);
            }
            else
            {
                log_err("Potocol RAW only support %d bytes Max!\n", SKHL_RAW_PAYLOAD_MAX_LEN);
                return -1;
            }
            *real_size = attr->data_len;
        }
    }
    else
    {
        *real_size = 0;
        log_err("Potocol not support!\n");
        return -1;
    }

    return 0;
}

skhl_result skhl_comm_send_data(void *attr)
{
    static skhl_comm_t  *context            = &comm_context;
    uint8_t             link_device         = 0;
    skhl_opt_t          *ops_local          = NULL;
    skhl_handle         ops_handle_local    = NULL;
    uint8_t             pack_data[512]      = {0};
    uint32_t            ret                 = 0;
    uint32_t            real_len            = 0;
    skhl_local_pack_attr_t *attr_local      = attr;

    if (NULL == attr)
    {
        log_err("error param pack for send data!\n");
        return -1;
    }

    ret = skhl_pack_data(pack_data, attr, &real_len);
    if (ret != 0)
    {
        log_err("can not pack data!\n");
        return -1;
    }

    ret = skhl_find_link(attr, &link_device);
    if (ret != 0)
    {
        log_err("can not find router to (%d)...\n", attr_local->target);
        return -1;
    }
    ops_local = context->device_opts[link_device];
    ops_handle_local = context->device_handle[link_device];
    log_info("opts device : %d\n", link_device);

    skhl_print_str("Sending data:", pack_data, real_len);
    ret = ops_local->write(ops_handle_local, pack_data, real_len);
    if (ret != real_len)
    {
        log_warn("short cut send!\n");
    }

    return 0;
}

skhl_result skhl_register_comm_device(skhl_opt_t *opt)
{
    static skhl_comm_t *context     = &comm_context;

    if (opt == NULL)
    {
        log_err("comm device register param error!\n");
        return -1;
    }

    if (!strncmp((const char *)opt->name, "uart", 4))
    {
        context->device_opts[OPT_UART] = opt;
    }
    else
    {
        log_err("device opt not support!\n");
        return -1;
    }

    return 0;
}

skhl_result skhl_unregister_comm_device(skhl_opt_t *opt)
{
    static skhl_comm_t *context     = &comm_context;

    if (opt == NULL)
    {
        log_err("comm device unregister param error!\n");
        return -1;
    }

    if (!strncmp((const char *)opt->name, "uart", 4))
    {
        context->device_opts[OPT_UART] = NULL;
    }
    else
    {
        log_err("device opt not support!\n");
        return -1;
    }

    return 0;
}

static void* phrase_data(void)
{
    static skhl_comm_t *context     = &comm_context;
    ring_buffer_t *desc             = &context->rb_desc;
    static uint32_t phrase_addr     = 0;
    uint32_t clean_count            = 0;
    uint32_t ring_buffer_size       = ring_buffer_data_size(desc);
    uint8_t step                    = LOOKING_FOR_HEADER;
    uint8_t protocol_data[PACK_MAX_LEN] = {0};

    if (ring_buffer_size == 0)
    {
        log_warn("Ring buffer empty!\n");
        return NULL;
    }

    log_debug("phrasing data...\n");

    while (step != GO_OUT)
    {
        switch (step)
        {
            case LOOKING_FOR_HEADER:
                {
                    while (1)
                    {
                        if (desc->base_addr[phrase_addr] == SKHL_PACK_V0_HEAD_SOF)
                        {
                            step = CHECKING_CRC8;
                            break;
                        }

                        phrase_addr++;
                        phrase_addr = phrase_addr % desc->buffer_size;
                        clean_count++;
                        ring_buffer_size--;
                        if (!ring_buffer_size)
                        {
                            log_debug("ring buffer null!\n");
                            step = GO_OUT;
                            break;
                        }
                    }
                }
                break;
            case CHECKING_CRC8:
                {
                    skhl_pack_v0_head_t *header = (skhl_pack_v0_head_t *)&desc->base_addr[phrase_addr];
                    uint8_t crc8                = 0;

                    crc8 = Get_Crc8((uint8_t *)header, 3);
                    if ((header->crc8 == crc8)/* && (header->pack_len <= PACK_MAX_LEN)*/)
                    {
                        log_debug("crc8 pass!    \n");
                        log_debug("pack len : %d\n", header->pack_len);
                        step = CHECKING_CRC16;
                    }
                    else
                    {
                        log_debug("crc8 fail!    \n");
                        step = LOOKING_FOR_HEADER;
                    }
                }
                break;
            case CHECKING_CRC16:
                {
                    skhl_pack_v0_head_t *header = (skhl_pack_v0_head_t *)&desc->base_addr[phrase_addr];
                    skhl_pack_v0_pack_t *pack   = (skhl_pack_v0_pack_t *)&desc->base_addr[phrase_addr];
                    uint16_t pack_crc16         = *(uint16_t *)&desc->base_addr[phrase_addr + header->pack_len - 2];
                    uint16_t crc16              = 0;
                    skhl_comm_item_t *item      = NULL;

                    crc16 = Get_Crc16((uint8_t *)pack, header->pack_len - 2);
                    if (crc16 == pack_crc16)
                    {
                        uint8_t i = 0;
                        skhl_local_pack_attr_t pack_info;

                        log_debug("crc16 pass!\n");
                        ring_buffer_pop(desc, protocol_data, clean_count);
                        ring_buffer_pop(desc, protocol_data, header->pack_len);

                        pack   = (skhl_pack_v0_pack_t *)protocol_data;
                        header = (skhl_pack_v0_head_t *)protocol_data;

                        if (context->cb_array != NULL)
                        {
                            for (i = 0; i < context->cb_array_size; i++)
                            {
                                item = &context->cb_array[i];
                                if ((pack->attribute.cmd_set == item->cmd_set) &&
                                    (pack->attribute.cmd_id == item->cmd_id))
                                {
                                    log_debug("Get callback!\n");
                                    log_debug("attr.cmd = 0x%02x 0x%02x\n", pack->attribute.cmd_set, pack->attribute.cmd_id);
                                    log_debug("item.cmd = 0x%02x 0x%02x\n", item->cmd_set, item->cmd_id);

                                    pack_info.cmd_id    = pack->attribute.cmd_id;
                                    pack_info.cmd_dir   = pack->attribute.cmd_dir;
                                    pack_info.cmd_set   = pack->attribute.cmd_set;
                                    pack_info.seq_id    = pack->attribute.seq_id;
                                    pack_info.source    = pack->attribute.source;
                                    pack_info.data      = pack->data;
                                    pack_info.data_len  = SKHL_DATA_LEN(header->pack_len);
                                    item->callback(&pack_info);
                                    break;
                                }
                            }

                            if (i == context->cb_array_size)
                            {
                                skhl_print_str("no call back: ", (uint8_t *)pack, header->pack_len);
                            }
                        }

                        phrase_addr += header->pack_len;
                        phrase_addr = phrase_addr % desc->buffer_size;
                        clean_count = 0;
                        ring_buffer_size = ring_buffer_data_size(desc);
                        if (ring_buffer_size == 0)
                        {
                            step = GO_OUT;
                            break;
                        }
                        else
                        {
                            step = LOOKING_FOR_HEADER;
                        }
                    }
                    else
                    {
                        clean_count++;
                        phrase_addr++;
                        phrase_addr = phrase_addr % desc->buffer_size;

                        step = LOOKING_FOR_HEADER;
                        log_info("crc16 failed should be (0x%x), but it got (0x%x)!\n", crc16, pack_crc16);

                    }
                }
                break;
            case GO_OUT:
            default:
                break;
        }
    }

    return NULL;
}

static void* skhl_phrase_core(void *p)
{
    static skhl_comm_t *context     = &comm_context;
    skhl_result ret                 = 0;

    while (!quit)
    {
        ret = event_wait(context->phrase_event);
        if (ret != 0)
        {
            log_err("event wait error!|n");
            continue;
        }

        log_info("get event!\n");
        phrase_data();
    }
    log_warn("Thread (%s) exit...\n", __FUNCTION__);

    return NULL;
}

static void* read_data_core(void *p)
{
    #define DATA_RECV_SIZE          2048

    static skhl_comm_t *context     = &comm_context;
    uint8_t data_recv[DATA_RECV_SIZE] = {0};
    int32_t read_size               = 0;
    skhl_result ret                 = 0;

    while (!quit)
    {
        for (uint8_t i = 0; i < COMM_OPT_MAX; i++)
        {
            if (context->device_opts[i] != NULL)
            {
                read_size = context->device_opts[i]->read(context->device_handle[i], data_recv, DATA_RECV_SIZE);
                if (read_size > 0)
                {
                    skhl_print_str("Recv data: ", data_recv, read_size);
                    ret = ring_buffer_push(&context->rb_desc, data_recv, read_size);
                    if (ret != 0)
                    {
                        log_err("ring buffer push failed!\n");
                    }
                    else
                    {
                        event_post(context->phrase_event);
                    }
                    memset(data_recv, 0, DATA_RECV_SIZE);
                }
                else
                {
                    log_warn("read nothing...\n");
                }
            }
        }
        skhl_sleep(1000);
    }

    log_warn("Thread (%s) exit...\n", __FUNCTION__);
    return NULL;
}

skhl_result skhl_comm_set_attr(COMM_CHANNEL_E chnl, comm_attr_t *attr)
{
    static skhl_comm_t *context     = &comm_context;

    if ((chnl >= COMM_OPT_MAX) || (NULL == attr))
    {
        log_err("error comm set attr!\n");
        return -1;
    }

    memcpy(&context->device_attr[chnl], attr, sizeof(comm_attr_t));

    return 0;
}

skhl_result skhl_comm_clear_attr(COMM_CHANNEL_E chnl)
{
    static skhl_comm_t *context     = &comm_context;

    if ((chnl >= COMM_OPT_MAX))
    {
        log_err("error comm set attr!\n");
        return -1;
    }
    memset(&context->device_attr[chnl], 0, sizeof(comm_attr_t));

    return 0;
}

skhl_result skhl_comm_core_init(void *config)
{
    static skhl_comm_t *context     = &comm_context;
    skhl_result ret                 = 0;
    skhl_opt_t *ops_local           = NULL;
    task_attr task                  = {0};
    event_attr event                = {0};
    skhl_comm_core_config_t *config_local = config;

    context->cb_array       = config_local->cb;
    context->cb_array_size  = config_local->cb_size;
    context->router         = config_local->router;
    context->router_size    = config_local->router_size;
    context->this_host      = config_local->this_host;

    for (uint8_t i = 0; i < COMM_OPT_MAX; i++)
    {
        ops_local = context->device_opts[i];
        if (ops_local != NULL)
        {
            context->device_handle[i] = ops_local->init(&context->device_attr[i]);
            if (context->device_handle[i] == NULL)
            {
                log_err("err handle when open %s\n", context->device_opts[i]->name);
                // look out to check return value!!!
                ret = -1;
                goto ERR_OPEN_HANDLE;
            }
        }
    }

    ret = ring_buffer_init(&context->rb_desc, comm_data_buffer, DATA_BUFFER_SIZE);
    if (ret != 0)
    {
        log_err("Create ring buffer Error!\n");
        goto ERR_RING_BUFFER_INIT;
    }

    event.name = "phrase_data";
    context->phrase_event = event_init(&event);
    if (NULL == context->phrase_event)
    {
        log_err("CreateThread failed\n");
        ret = -1;
        goto ERR_ENENT_INIT;
    }

    // Create phrase core.
    task.auto_start     = 0;
    task.fn             = skhl_phrase_core;
    task.name           = "phrase core";
    task.stack_size     = 0;
    task.prio           = TASK_PRIO_HIGHEST;
    context->phrase_core_task = task_init(&task);
    if (NULL == context->phrase_core_task)
    {
        log_err("Create Thread phrase core failed!\n");
        ret = -1;
        goto ERR_PHRASE_TASK_INIT;
    }

    // Create read data core
    task.auto_start     = 0;
    task.fn             = read_data_core;
    task.name           = "read data";
    task.stack_size     = 0;
    task.prio           = TASK_PRIO_NORMAL;
    context->read_data_task = task_init(&task);
    if (NULL == context->read_data_task)
    {
        log_err("Create Thread read data core failed!\n");
        ret = -1;
        goto ERR_READ_DATA_TASK_INIT;
    }

    return 0;

ERR_READ_DATA_TASK_INIT:
    task_destory(context->phrase_core_task);
ERR_PHRASE_TASK_INIT:
    event_destory(context->phrase_event);
ERR_ENENT_INIT:
    ring_buffer_destory(&context->rb_desc);
ERR_RING_BUFFER_INIT:
    for (uint8_t i = 0; i < COMM_OPT_MAX; i++)
    {
        ops_local = context->device_opts[i];
        if (ops_local != NULL)
        {
            ops_local->destory(context->device_handle[i]);
        }
    }
ERR_OPEN_HANDLE:
    return ret;
}

skhl_result skhl_comm_core_destory(void)
{
    static skhl_comm_t *context     = &comm_context;
    skhl_opt_t *ops_local           = NULL;

    event_post(context->phrase_event);
    task_destory(context->read_data_task);
    task_destory(context->phrase_core_task);
    event_destory(context->phrase_event);
    ring_buffer_destory(&context->rb_desc);

    for (uint8_t i = 0; i < COMM_OPT_MAX; i++)
    {
        ops_local = context->device_opts[i];
        if (ops_local != NULL)
        {
            ops_local->destory(context->device_handle[i]);
        }
    }

    return 0;
}

