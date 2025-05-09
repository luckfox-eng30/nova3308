/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(RT_USING_OLPC_DEMO)
#include "hal_base.h"
#include "olpc_ap.h"

/*
 **************************************************************************************************
 *
 * Macro define
 *
 **************************************************************************************************
 */
#define AP_ACK_OK       (0x01UL << 0)
#define AP_ACK_ERROR    (0x01UL << 1)

#define AP_MQ_SIZE      8

struct ap_cmd_msg
{
    rt_uint32_t cmd;
    void        *msg;
};

static rt_mq_t    ap_cmd_mq;
static rt_event_t ap_ack_event;

struct olpc_ap_data
{
    uint8_t mboxIsA2B;          /**< 1: AP to BB; 0: BB to AP */
    struct  MBOX_REG *mboxReg;
    int     mboxIrq[MBOX_CHAN_CNT];

    uint8_t irq_chan;
    struct  MBOX_CMD_DAT cmdData[MBOX_CHAN_CNT];
};
static struct olpc_ap_data *g_ap_data = RT_NULL;

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */
static void ap_mbox_callback(struct MBOX_CMD_DAT *msg, void *arg);
static void ap_mbox_isr(int irq, void *param);
static struct MBOX_CLIENT mbox_client[MBOX_CHAN_CNT] =
{
    {"mbox0_chan0", NUM_INTERRUPTS, ap_mbox_callback, (void *)MBOX_CH_0},
    {"mbox0_chan1", NUM_INTERRUPTS, ap_mbox_callback, (void *)MBOX_CH_1},
    {"mbox0_chan2", NUM_INTERRUPTS, ap_mbox_callback, (void *)MBOX_CH_2},
    {"mbox0_chan3", NUM_INTERRUPTS, ap_mbox_callback, (void *)MBOX_CH_3},
};

#define AP_MBOX_ISR_FUNC(ID) \
static void ap_mbox_isr##ID(int irq, void *param) \
{ \
    ap_mbox_isr(mbox_client[(int)ID].irq,param);      \
}

AP_MBOX_ISR_FUNC(0)
AP_MBOX_ISR_FUNC(1)
AP_MBOX_ISR_FUNC(2)
AP_MBOX_ISR_FUNC(3)

static const rt_isr_handler_t ap_mbox_isr_func[MBOX_CHAN_CNT] =
{
    ap_mbox_isr0,
    ap_mbox_isr1,
    ap_mbox_isr2,
    ap_mbox_isr3
};

static void ap_mbox_callback(struct MBOX_CMD_DAT *msg, void *arg)
{
    rt_base_t level;
    struct olpc_ap_data *olpc_data = g_ap_data;

    //rt_kprintf("mbox_callback: ch = %d, CMD = 0x%08x, DATA = 0x%08x\n", (eMBOX_CH)arg, msg->CMD, msg->DATA);
    level = rt_hw_interrupt_disable();
    olpc_data->irq_chan = (eMBOX_CH)arg;
    olpc_data->cmdData[olpc_data->irq_chan].DATA = msg->DATA;
    olpc_data->cmdData[olpc_data->irq_chan].CMD  = msg->CMD;
    rt_hw_interrupt_enable(level);
}

static void ap_mbox_isr(int irq, void *param)
{
    struct olpc_ap_data *olpc_data = g_ap_data;

    /* enter interrupt */
    rt_interrupt_enter();

    HAL_MBOX_IrqHandler(irq, olpc_data->mboxReg);

    /* leave interrupt */
    rt_interrupt_leave();
}

static void ap_mbox_int_install(int id)
{
    rt_hw_interrupt_install(mbox_client[id].irq, ap_mbox_isr_func[id], RT_NULL, RT_NULL);
    rt_hw_interrupt_umask(mbox_client[id].irq);
}

static void olpc_ap_data_init(struct olpc_ap_data *olpc_data)
{
    olpc_data->mboxReg = MBOX1;
    olpc_data->mboxIsA2B = 0;
    olpc_data->mboxIrq[0] = MAILBOX1_BB_IRQn;
    olpc_data->mboxIrq[1] = MAILBOX1_BB_IRQn;
    olpc_data->mboxIrq[2] = MAILBOX1_BB_IRQn;
    olpc_data->mboxIrq[3] = MAILBOX1_BB_IRQn;
}

static void olpc_ap_mbox_init(struct olpc_ap_data *olpc_data)
{
    rt_err_t ret;
    uint32_t chan;

    ret = HAL_MBOX_Init(olpc_data->mboxReg, olpc_data->mboxIsA2B);
    RT_ASSERT(!ret);
    for (chan = 0; chan < MBOX_CHAN_CNT; chan++)
    {
        mbox_client[chan].irq = olpc_data->mboxIrq[chan];
        ret = HAL_MBOX_RegisterClient(olpc_data->mboxReg, chan, &mbox_client[chan]);
        RT_ASSERT(ret == RT_EOK);
        ap_mbox_int_install(chan);
    }
}

static rt_err_t olpc_ap_mbox_send(rt_uint32_t cmd, void *param, rt_uint32_t paramlen)
{
    struct MBOX_CMD_DAT cmdData;
    struct MBOX_REG *pReg;
    uint32_t intc1, timeout, status = AP_STATUS_OK;
    uint32_t flag = 0;
    struct olpc_ap_data *olpc_data = g_ap_data;

    RT_ASSERT(param);

    intc1 = INTC1_BASE;
    *(uint32_t *)intc1 = 0x10;

    pReg = olpc_data->mboxReg;
    pReg->B2A_STATUS  = 1;
    pReg->B2A_INTEN  |= 1;

    // send request & wait load
    cmdData.DATA = (rt_uint32_t)param;
    cmdData.CMD  = cmd;

    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, param, paramlen);

    olpc_data->irq_chan = 0xff;
    olpc_data->cmdData[0].DATA = -1;
    olpc_data->cmdData[0].CMD = -1;

    flag = 0;
    HAL_MBOX_SendMsg2(olpc_data->mboxReg, 0, &cmdData, olpc_data->mboxIsA2B);
    timeout = 1000;
    do
    {
        rt_thread_delay(1);

        rt_enter_critical();
        if ((olpc_data->irq_chan == 0) &&
                (olpc_data->cmdData[0].DATA == AP_STATUS_OK) &&
                (olpc_data->cmdData[0].CMD  == AP_COMMAND_ACK))
        {
            flag = 1;
        }
        rt_exit_critical();

        if (flag == 1)
        {
            break;
        }
    }
    while (--timeout);

    rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE, param, paramlen);

    if (timeout == 0)
    {
        return RT_ETIMEOUT;
    }

    if (status != AP_STATUS_OK)
    {
        return RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t olpc_file_cache_invalidate(void *p)
{
    FILE_READ_REQ_PARAM *param = (FILE_READ_REQ_PARAM *)p;

    // invalidate cache
    if ((SRAM_IADDR_START <= (rt_uint32_t)param->buf) && ((rt_uint32_t)param->buf < SRAM_IADDR_START + SRAM_SIZE))
        rt_hw_cpu_icache_ops(RT_HW_CACHE_INVALIDATE, param->buf, (rt_uint32_t)param->rdlen);
    else
        rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE, param->buf, (rt_uint32_t)param->rdlen);

    return RT_EOK;
}

/*
 **************************************************************************************************
 *
 * Global static struct & data define
 *
 **************************************************************************************************
 */
rt_err_t olpc_ap_command(rt_uint32_t cmd, void *param, rt_uint32_t paramlen)
{
    rt_err_t ret = RT_ERROR;
    rt_uint32_t event = 0;
    struct ap_cmd_msg mq;

    mq.cmd = cmd;
    mq.msg = param;
    ret = rt_mq_send(ap_cmd_mq, &mq, sizeof(struct ap_cmd_msg));
    if (ret == RT_EOK)
    {
        ret = rt_event_recv(ap_ack_event, AP_ACK_OK | AP_ACK_ERROR,
                            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                            RT_WAITING_FOREVER, &event);
        if (ret == RT_EOK)
        {
            if (event == AP_ACK_OK)
            {
                ret = RT_EOK;
            }
        }
    }

    return ret;
}
RTM_EXPORT(olpc_ap_command);

/*
 **************************************************************************************************
 *
 * olpc main thread
 *
 **************************************************************************************************
 */
/**
 * olpc main thread.
 */
static void olpc_ap_thread(void *p)
{
    rt_err_t ret;
    struct ap_cmd_msg mq;
    //rt_uint32_t paramlen;
    struct olpc_ap_data *olpc_data;

    olpc_data = (struct olpc_ap_data *)rt_malloc(sizeof(struct olpc_ap_data));
    RT_ASSERT(olpc_data != RT_NULL);
    rt_memset((void *)olpc_data, 0, sizeof(struct olpc_ap_data));
    g_ap_data = olpc_data;

    olpc_ap_data_init(olpc_data);
    olpc_ap_mbox_init(olpc_data);

    ap_cmd_mq = rt_mq_create("ap_cmd_mq", sizeof(struct ap_cmd_msg), AP_MQ_SIZE,  RT_IPC_FLAG_FIFO);
    RT_ASSERT(ap_cmd_mq != RT_NULL);
    ap_ack_event = rt_event_create("ap_ack_event", RT_IPC_FLAG_FIFO);
    RT_ASSERT(ap_ack_event != RT_NULL);

    while (1)
    {
        ret = rt_mq_recv(ap_cmd_mq, &mq, sizeof(struct ap_cmd_msg), RT_WAITING_FOREVER);
        RT_ASSERT(ret == RT_EOK);

        switch (mq.cmd)
        {
        case FILE_INFO_REQ:
            ret = olpc_ap_mbox_send(FILE_INFO_REQ, mq.msg, sizeof(FILE_READ_REQ_PARAM));
            if (ret == RT_EOK)
            {
                rt_event_send(ap_ack_event, AP_ACK_OK);
            }
            else
            {
                rt_event_send(ap_ack_event, AP_ACK_ERROR);
            }
            break;
        case FIILE_READ_REQ:
            ret = olpc_ap_mbox_send(FIILE_READ_REQ, mq.msg, sizeof(FILE_READ_REQ_PARAM));
            if (ret == RT_EOK)
            {
                olpc_file_cache_invalidate(mq.msg);
                rt_event_send(ap_ack_event, AP_ACK_OK);
            }
            else
            {
                rt_event_send(ap_ack_event, AP_ACK_ERROR);
            }
            break;
        default:
            rt_kprintf("olpc_ap_command: Undefined command!!!\n");
            rt_event_send(ap_ack_event, AP_ACK_ERROR);
            break;
        }
    }

    /* Thread deinit */
    rt_event_delete(ap_ack_event);
    ap_ack_event = RT_NULL;

    rt_mq_delete(ap_cmd_mq);
    ap_cmd_mq = RT_NULL;

    rt_free(olpc_data);
    olpc_data = RT_NULL;
    g_ap_data = RT_NULL;
}

/**
 * olpc clock demo application init.
 */
int olpc_ap_init(void)
{
    rt_thread_t rtt_olpc_ap;

    rtt_olpc_ap = rt_thread_create("olpcap", olpc_ap_thread, RT_NULL, 2048, 3, 10);
    RT_ASSERT(rtt_olpc_ap != RT_NULL);
    rt_thread_startup(rtt_olpc_ap);

    return RT_EOK;
}
INIT_APP_EXPORT(olpc_ap_init);
#endif
