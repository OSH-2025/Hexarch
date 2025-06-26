#include "FreeRTOS.h"
#include "task.h"
#include "list.h"
#include <string.h>

/* 获取TCB指针的私有宏 */
#define prvGetTCBFromHandle( pxHandle ) ( ( ( pxHandle ) == NULL ) ? ( tskTCB * ) pxCurrentTCB : ( tskTCB * ) ( pxHandle ) )

/* 私有函数声明和实现 */

static void prvInitialiseTCBVariables( tskTCB *pxTCB, const signed char * const pcName, unsigned portBASE_TYPE uxPriority, const xMemoryRegion * const xRegions, unsigned short usStackDepth )
{
    #if configMAX_TASK_NAME_LEN > 1
    {
        strncpy( ( char * ) pxTCB->pcTaskName, ( const char * ) pcName, ( unsigned short ) configMAX_TASK_NAME_LEN );
    }
    #endif
    pxTCB->pcTaskName[ ( unsigned short ) configMAX_TASK_NAME_LEN - ( unsigned short ) 1 ] = ( signed char ) '\0';
    if( uxPriority >= configMAX_PRIORITIES )
    {
        uxPriority = configMAX_PRIORITIES - ( unsigned portBASE_TYPE ) 1U;
    }
    pxTCB->uxPriority = uxPriority;
    #if ( configUSE_MUTEXES == 1 )
    {
        pxTCB->uxBasePriority = uxPriority;
    }
    #endif
    vListInitialiseItem( &( pxTCB->xGenericListItem ) );
    vListInitialiseItem( &( pxTCB->xEventListItem ) );
    listSET_LIST_ITEM_OWNER( &( pxTCB->xGenericListItem ), pxTCB );
    listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), configMAX_PRIORITIES - ( portTickType ) uxPriority );
    listSET_LIST_ITEM_OWNER( &( pxTCB->xEventListItem ), pxTCB );
    #if ( portCRITICAL_NESTING_IN_TCB == 1 )
    {
        pxTCB->uxCriticalNesting = ( unsigned portBASE_TYPE ) 0U;
    }
    #endif
    #if ( configUSE_APPLICATION_TASK_TAG == 1 )
    {
        pxTCB->pxTaskTag = NULL;
    }
    #endif
    #if ( configGENERATE_RUN_TIME_STATS == 1 )
    {
        pxTCB->ulRunTimeCounter = 0UL;
    }
    #endif
    #if ( portUSING_MPU_WRAPPERS == 1 )
    {
        vPortStoreTaskMPUSettings( &( pxTCB->xMPUSettings ), xRegions, pxTCB->pxStack, usStackDepth );
    }
    #else
    {
        ( void ) xRegions;
        ( void ) usStackDepth;
    }
    #endif
}

static void prvInitialiseTaskLists( void )
{
    unsigned portBASE_TYPE uxPriority;
    for( uxPriority = ( unsigned portBASE_TYPE ) 0U; uxPriority < configMAX_PRIORITIES; uxPriority++ )
    {
        vListInitialise( ( xList * ) &( pxReadyTasksLists[ uxPriority ] ) );
    }
    vListInitialise( ( xList * ) &xDelayedTaskList1 );
    vListInitialise( ( xList * ) &xDelayedTaskList2 );
    vListInitialise( ( xList * ) &xPendingReadyList );
    #if ( INCLUDE_vTaskDelete == 1 )
    {
        vListInitialise( ( xList * ) &xTasksWaitingTermination );
    }
    #endif
    #if ( INCLUDE_vTaskSuspend == 1 )
    {
        vListInitialise( ( xList * ) &xSuspendedTaskList );
    }
    #endif
    pxDelayedTaskList = &xDelayedTaskList1;
    pxOverflowDelayedTaskList = &xDelayedTaskList2;
}

static void prvCheckTasksWaitingTermination( void )
{
    #if ( INCLUDE_vTaskDelete == 1 )
    {
        portBASE_TYPE xListIsEmpty;
        if( uxTasksDeleted > ( unsigned portBASE_TYPE ) 0U )
        {
            vTaskSuspendAll();
            xListIsEmpty = listLIST_IS_EMPTY( &xTasksWaitingTermination );
            xTaskResumeAll();
            if( xListIsEmpty == pdFALSE )
            {
                tskTCB *pxTCB;
                taskENTER_CRITICAL();
                pxTCB = ( tskTCB * ) listGET_OWNER_OF_HEAD_ENTRY( ( ( xList * ) &xTasksWaitingTermination ) );
                vListRemove( &( pxTCB->xGenericListItem ) );
                --uxCurrentNumberOfTasks;
                --uxTasksDeleted;
                taskEXIT_CRITICAL();
                prvDeleteTCB( pxTCB );
            }
        }
    }
    #endif
}

static void prvAddCurrentTaskToDelayedList( portTickType xTimeToWake )
{
    listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xGenericListItem ), xTimeToWake );
    if( xTimeToWake < xTickCount )
    {
        vListInsert( ( xList * ) pxOverflowDelayedTaskList, ( xListItem * ) &( pxCurrentTCB->xGenericListItem ) );
    }
    else
    {
        vListInsert( ( xList * ) pxDelayedTaskList, ( xListItem * ) &( pxCurrentTCB->xGenericListItem ) );
        if( xTimeToWake < xNextTaskUnblockTime )
        {
            xNextTaskUnblockTime = xTimeToWake;
        }
    }
}

static tskTCB *prvAllocateTCBAndStack( unsigned short usStackDepth, portSTACK_TYPE *puxStackBuffer )
{
    tskTCB *pxNewTCB;
    pxNewTCB = ( tskTCB * ) pvPortMalloc( sizeof( tskTCB ) );
    if( pxNewTCB != NULL )
    {
        pxNewTCB->pxStack = ( portSTACK_TYPE * ) pvPortMallocAligned( ( ( ( size_t )usStackDepth ) * sizeof( portSTACK_TYPE ) ), puxStackBuffer );
        if( pxNewTCB->pxStack == NULL )
        {
            vPortFree( pxNewTCB );
            pxNewTCB = NULL;
        }
        else
        {
            memset( pxNewTCB->pxStack, ( int ) tskSTACK_FILL_BYTE, ( size_t ) usStackDepth * sizeof( portSTACK_TYPE ) );
        }
    }
#if (configBLUETHUNDER == 1)
    pxNewTCB->pTraceEvent = NULL;
    pxNewTCB->pTraceEventMin = NULL;
    pxNewTCB->pTraceEventMax = NULL;
#endif
    return pxNewTCB;
}

#if ( INCLUDE_vTaskDelete == 1 )
static void prvDeleteTCB( tskTCB *pxTCB )
{
    portCLEAN_UP_TCB( pxTCB );
    vPortFreeAligned( pxTCB->pxStack );
    vPortFree( pxTCB );
}
#endif

#if ( configUSE_TRACE_FACILITY == 1 )
static void prvListTaskWithinSingleList( const signed char *pcWriteBuffer, xList *pxList, signed char cStatus )
{
    /* 实现略，可根据调试需求补充 */
}
#endif

#if ( ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) )
static unsigned short usTaskCheckFreeStackSpace( const unsigned char * pucStackByte )
{
    register unsigned short usCount = 0U;
    while( *pucStackByte == tskSTACK_FILL_BYTE )
    {
        pucStackByte -= portSTACK_GROWTH;
        usCount++;
    }
    usCount /= sizeof( portSTACK_TYPE );
    return usCount;
}
#endif

/* 空闲任务原型声明（实现通常在主文件或端口相关文件中） */
static portTASK_FUNCTION_PROTO( prvIdleTask, pvParameters ); 