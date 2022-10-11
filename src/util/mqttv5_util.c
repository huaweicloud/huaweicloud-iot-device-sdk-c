#include <stdio.h>
#include <stdlib.h>
#include "mqttv5_util.h"

#if defined(MQTTV5)
//Linked list deletion
int listFree(MQTTV5_USER_PRO *mqtt_data)
{
    if (mqtt_data == NULL)
    {
        return 0;
    }
    MQTTV5_USER_PRO *propertiesA = mqtt_data;
    MQTTV5_USER_PRO *propertiesB = mqtt_data->nex;

    while (propertiesA != NULL)
    {
        free(propertiesA);
        propertiesA = propertiesB;
        if (propertiesA != NULL)
        {
            propertiesB = propertiesA->nex;
        }
    }
    return 0;
}
#endif