#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#if ( configUSE_TIMERS == 1 )

/* Misc definitions. */
#define tmrNO_DELAY     ( portTickType ) 0U

/* 定时器控制块结构体 */
typedef struct tmrTimerControl
{
    const signed char        *pcTimerName;
    xListItem                xTimerListItem;
    portTickType             xTimerPeriodInTicks;
    unsigned portBASE_TYPE   uxAutoReload;
    void                    *pvTimerID;
    tmrTIMER_CALLBACK        pxCallbackFunction;
} xTIMER;

/* 定时器队列消息结构体 */
typedef struct tmrTimerQueueMessage
{
    portBASE_TYPE            xMessageID;
    portTickType             xMessageValue;
    xTIMER                  *pxTimer;
} xTIMER_MESSAGE;

/* 活动定时器列表和队列，仅限定时器服务任务访问 */
PRIVILEGED_DATA static xList xActiveTimerList1;
PRIVILEGED_DATA static xList xActiveTimerList2;
PRIVILEGED_DATA static xList *pxCurrentTimerList;
PRIVILEGED_DATA static xList *pxOverflowTimerList;
PRIVILEGED_DATA static xQueueHandle xTimerQueue = NULL;

#if ( INCLUDE_xTimerGetTimerDaemonTaskHandle == 1 )
PRIVILEGED_DATA static xTaskHandle xTimerTaskHandle = NULL;
#endif

#endif /* configUSE_TIMERS == 1 */ 