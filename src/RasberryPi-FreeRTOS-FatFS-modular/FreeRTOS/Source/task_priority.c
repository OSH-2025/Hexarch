#include "FreeRTOS.h"
#include "task.h"
#include "list.h"

#if ( INCLUDE_uxTaskPriorityGet == 1 )
unsigned portBASE_TYPE uxTaskPriorityGet( xTaskHandle pxTask )
{
    tskTCB *pxTCB;
    unsigned portBASE_TYPE uxReturn;

    taskENTER_CRITICAL();
    {
        pxTCB = prvGetTCBFromHandle( pxTask );
        uxReturn = pxTCB->uxPriority;
    }
    taskEXIT_CRITICAL();

    return uxReturn;
}
#endif

#if ( INCLUDE_vTaskPrioritySet == 1 )
void vTaskPrioritySet( xTaskHandle pxTask, unsigned portBASE_TYPE uxNewPriority )
{
    tskTCB *pxTCB;
    unsigned portBASE_TYPE uxCurrentPriority;
    portBASE_TYPE xYieldRequired = pdFALSE;

    configASSERT( ( uxNewPriority < configMAX_PRIORITIES ) );

    if( uxNewPriority >= configMAX_PRIORITIES )
    {
        uxNewPriority = configMAX_PRIORITIES - ( unsigned portBASE_TYPE ) 1U;
    }

    taskENTER_CRITICAL();
    {
        if( pxTask == pxCurrentTCB )
        {
            pxTask = NULL;
        }
        pxTCB = prvGetTCBFromHandle( pxTask );
        traceTASK_PRIORITY_SET( pxTCB, uxNewPriority );
#if ( configUSE_MUTEXES == 1 )
        uxCurrentPriority = pxTCB->uxBasePriority;
#else
        uxCurrentPriority = pxTCB->uxPriority;
#endif
        if( uxCurrentPriority != uxNewPriority )
        {
            if( uxNewPriority > uxCurrentPriority )
            {
                if( pxTask != NULL )
                {
                    xYieldRequired = pdTRUE;
                }
            }
            else if( pxTask == NULL )
            {
                xYieldRequired = pdTRUE;
            }
#if ( configUSE_MUTEXES == 1 )
            if( pxTCB->uxBasePriority == pxTCB->uxPriority )
            {
                pxTCB->uxPriority = uxNewPriority;
            }
            pxTCB->uxBasePriority = uxNewPriority;
#else
            pxTCB->uxPriority = uxNewPriority;
#endif
            listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), ( configMAX_PRIORITIES - ( portTickType ) uxNewPriority ) );
            if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ uxCurrentPriority ] ), &( pxTCB->xGenericListItem ) ) )
            {
                vListRemove( &( pxTCB->xGenericListItem ) );
                prvAddTaskToReadyQueue( pxTCB );
            }
        }
    }
    taskEXIT_CRITICAL();
    if( xYieldRequired != pdFALSE )
    {
        portYIELD_WITHIN_API();
    }
}
#endif

#if ( configUSE_MUTEXES == 1 )
void vTaskPriorityInherit( xTaskHandle * const pxMutexHolder )
{
    tskTCB * const pxTCB = ( tskTCB * ) pxMutexHolder;
    configASSERT( pxMutexHolder );
    if( pxTCB->uxPriority < pxCurrentTCB->uxPriority )
    {
        listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), configMAX_PRIORITIES - ( portTickType ) pxCurrentTCB->uxPriority );
        if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ pxTCB->uxPriority ] ), &( pxTCB->xGenericListItem ) ) != pdFALSE )
        {
            vListRemove( &( pxTCB->xGenericListItem ) );
            pxTCB->uxPriority = pxCurrentTCB->uxPriority;
            prvAddTaskToReadyQueue( pxTCB );
        }
        else
        {
            pxTCB->uxPriority = pxCurrentTCB->uxPriority;
        }
        traceTASK_PRIORITY_INHERIT( pxTCB, pxCurrentTCB->uxPriority );
    }
}

void vTaskPriorityDisinherit( xTaskHandle * const pxMutexHolder )
{
    tskTCB * const pxTCB = ( tskTCB * ) pxMutexHolder;
    if( pxMutexHolder != NULL )
    {
        if( pxTCB->uxPriority != pxTCB->uxBasePriority )
        {
            vListRemove( &( pxTCB->xGenericListItem ) );
            traceTASK_PRIORITY_DISINHERIT( pxTCB, pxTCB->uxBasePriority );
            pxTCB->uxPriority = pxTCB->uxBasePriority;
            listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), configMAX_PRIORITIES - ( portTickType ) pxTCB->uxPriority );
            prvAddTaskToReadyQueue( pxTCB );
        }
    }
}
#endif 