#include "queue.h"

#if configQUEUE_REGISTRY_SIZE > 0

typedef struct QUEUE_REGISTRY_ITEM {
    signed char *pcQueueName;
    xQueueHandle xHandle;
} xQueueRegistryItem;

xQueueRegistryItem xQueueRegistry[ configQUEUE_REGISTRY_SIZE ];

void vQueueAddToRegistry( xQueueHandle xQueue, signed char *pcQueueName )
{
    unsigned portBASE_TYPE ux;
    for( ux = ( unsigned portBASE_TYPE ) 0U; ux < ( unsigned portBASE_TYPE ) configQUEUE_REGISTRY_SIZE; ux++ )
    {
        if( xQueueRegistry[ ux ].pcQueueName == NULL )
        {
            xQueueRegistry[ ux ].pcQueueName = pcQueueName;
            xQueueRegistry[ ux ].xHandle = xQueue;
            break;
        }
    }
}

static void vQueueUnregisterQueue( xQueueHandle xQueue )
{
    unsigned portBASE_TYPE ux;
    for( ux = ( unsigned portBASE_TYPE ) 0U; ux < ( unsigned portBASE_TYPE ) configQUEUE_REGISTRY_SIZE; ux++ )
    {
        if( xQueueRegistry[ ux ].xHandle == xQueue )
        {
            xQueueRegistry[ ux ].pcQueueName = NULL;
            break;
        }
    }
}

#endif 