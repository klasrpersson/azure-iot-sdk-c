// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <string.h>
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/constbuffer.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/envvariable.h"

#include "iothub_client_core_ll.h"
#include "internal/iothub_client_authorization.h"
#include "iothub_module_client_ll.h"
#include "iothub_transport_ll.h"
#include "internal/iothub_client_private.h"
#include "iothub_client_options.h"
#include "iothub_client_version.h"
#include <stdint.h>

#ifndef DONT_USE_UPLOADTOBLOB
#include "internal/iothub_client_ll_uploadtoblob.h"
#endif


#include "internal/iothub_client_edge.h"


typedef struct IOTHUB_MODULE_CLIENT_LL_HANDLE_DATA_TAG
{
    IOTHUB_CLIENT_CORE_LL_HANDLE coreHandle;

    IOTHUB_CLIENT_EDGE_HANDLE methodHandle;

} IOTHUB_MODULE_CLIENT_LL_HANDLE_DATA;


IOTHUB_MODULE_CLIENT_LL_HANDLE IoTHubModuleClient_LL_CreateFromConnectionString(const char* connectionString, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    IOTHUB_MODULE_CLIENT_LL_HANDLE_DATA* result;

    if ((result = malloc(sizeof(IOTHUB_MODULE_CLIENT_LL_HANDLE_DATA))) == NULL)
    {
        LogError("Failed to allocate module client ll handle");
    }
    else
    {
        memset(result, 0, sizeof(IOTHUB_MODULE_CLIENT_LL_HANDLE_DATA));

        if ((result->coreHandle = IoTHubClientCore_LL_CreateFromConnectionString(connectionString, protocol)) == NULL)
        {
            LogError("Failed to create core handle");
            IoTHubModuleClient_LL_Destroy(result);
            result = NULL;
        }
        //Edge handle is not added
    }

    return result;
}

void IoTHubModuleClient_LL_Destroy(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle)
{
    if (iotHubModuleClientHandle != NULL)
    {
        IoTHubClientCore_LL_Destroy(iotHubModuleClientHandle->coreHandle);
        //Note that we don't have to free the module method handle because it's (currently, until major refactor) owned by the core
        free(iotHubModuleClientHandle);
    }
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SendEventAsync(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_SendEventAsync(iotHubModuleClientHandle->coreHandle, eventMessageHandle, eventConfirmationCallback, userContextCallback);
    }
    else
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_GetSendStatus(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_GetSendStatus(iotHubModuleClientHandle->coreHandle, iotHubClientStatus);
    }
    else
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SetMessageCallback(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC messageCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_SetInputMessageCallback(iotHubModuleClientHandle->coreHandle, NULL, messageCallback, userContextCallback);
    }
    else
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SetConnectionStatusCallback(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK connectionStatusCallback, void * userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_SetConnectionStatusCallback(iotHubModuleClientHandle->coreHandle, connectionStatusCallback, userContextCallback);
    }
    else
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SetRetryPolicy(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_SetRetryPolicy(iotHubModuleClientHandle->coreHandle, retryPolicy, retryTimeoutLimitInSeconds);
    }
    else
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_GetRetryPolicy(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, IOTHUB_CLIENT_RETRY_POLICY* retryPolicy, size_t* retryTimeoutLimitInSeconds)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_GetRetryPolicy(iotHubModuleClientHandle->coreHandle, retryPolicy, retryTimeoutLimitInSeconds);
    }
    else
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_GetLastMessageReceiveTime(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, time_t* lastMessageReceiveTime)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_GetLastMessageReceiveTime(iotHubModuleClientHandle->coreHandle, lastMessageReceiveTime);
    }
    else
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

void IoTHubModuleClient_LL_DoWork(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle)
{
    if (iotHubModuleClientHandle != NULL)
    {
        IoTHubClientCore_LL_DoWork(iotHubModuleClientHandle->coreHandle);
    }
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SetOption(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, const char* optionName, const void* value)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_SetOption(iotHubModuleClientHandle->coreHandle, optionName, value);
    }
    else
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SetModuleTwinCallback(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK moduleTwinCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_SetDeviceTwinCallback(iotHubModuleClientHandle->coreHandle, moduleTwinCallback, userContextCallback);
    }
    else
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SendReportedState(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, const unsigned char* reportedState, size_t size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_SendReportedState(iotHubModuleClientHandle->coreHandle, reportedState, size, reportedStateCallback, userContextCallback);
    }
    else
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_GetTwinAsync(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK moduleTwinCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_GetTwinAsync(iotHubModuleClientHandle->coreHandle, moduleTwinCallback, userContextCallback);
    }
    else
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SetModuleMethodCallback(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC moduleMethodCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_SetDeviceMethodCallback(iotHubModuleClientHandle->coreHandle, moduleMethodCallback, userContextCallback);
    }
    else
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SendEventToOutputAsync(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, const char* outputName, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_SendEventToOutputAsync(iotHubModuleClientHandle->coreHandle, eventMessageHandle, outputName, eventConfirmationCallback, userContextCallback);
    }
    else
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SetInputMessageCallback(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, const char* inputName, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC eventHandlerCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_SetInputMessageCallback(iotHubModuleClientHandle->coreHandle, inputName, eventHandlerCallback, userContextCallback);
    }
    else
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SendMessageDisposition(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, IOTHUB_MESSAGE_HANDLE message, IOTHUBMESSAGE_DISPOSITION_RESULT disposition)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_SendMessageDisposition(iotHubModuleClientHandle->coreHandle, message, disposition);
    }
    else
    {
        LogError("iotHubModuleClientHandle cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}



IOTHUB_MODULE_CLIENT_LL_HANDLE IoTHubModuleClient_LL_CreateFromEnvironment(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    IOTHUB_MODULE_CLIENT_LL_HANDLE_DATA* result;

    if ((result = malloc(sizeof(IOTHUB_MODULE_CLIENT_LL_HANDLE_DATA))) == NULL)
    {
        LogError("Failed to allocate module client ll handle");
    }
    else
    {
        memset(result, 0, sizeof(IOTHUB_MODULE_CLIENT_LL_HANDLE_DATA));

        if ((result->coreHandle = IoTHubClientCore_LL_CreateFromEnvironment(protocol)) == NULL)
        {
            LogError("Failed to create core handle");
            IoTHubModuleClient_LL_Destroy(result);
            result = NULL;
        }
        else if ((result->methodHandle = IoTHubClientCore_LL_GetEdgeHandle(result->coreHandle)) == NULL)
        {
            LogError("Failed to get module method handle");
            IoTHubModuleClient_LL_Destroy(result);
            result = NULL;
        }
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_DeviceMethodInvoke(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, const char* deviceId, const char* methodName, const char* methodPayload, unsigned int timeout, int* responseStatus, unsigned char** responsePayload, size_t* responsePayloadSize)
{
    IOTHUB_CLIENT_RESULT result;

    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClient_Edge_DeviceMethodInvoke(iotHubModuleClientHandle->methodHandle, deviceId, methodName, methodPayload, timeout, responseStatus, responsePayload, responsePayloadSize);
    }
    else
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_ModuleMethodInvoke(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, const char* deviceId, const char* moduleId, const char* methodName, const char* methodPayload, unsigned int timeout, int* responseStatus, unsigned char** responsePayload, size_t* responsePayloadSize)
{
    IOTHUB_CLIENT_RESULT result;

    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClient_Edge_ModuleMethodInvoke(iotHubModuleClientHandle->methodHandle, deviceId, moduleId, methodName, methodPayload, timeout, responseStatus, responsePayload, responsePayloadSize);
    }
    else
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}


IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SendTelemetryAsync(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle,  IOTHUB_MESSAGE_HANDLE telemetryMessageHandle, IOTHUB_CLIENT_TELEMETRY_CALLBACK telemetryConfirmationCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_SendEventAsync(iotHubModuleClientHandle->coreHandle, telemetryMessageHandle, (IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK)telemetryConfirmationCallback, userContextCallback);
    }
    else
    {
        LogError("iotHubModuleClientHandle parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SendPropertiesAsync(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, const unsigned char* properties, size_t propertiesLength, IOTHUB_CLIENT_PROPERTY_ACKNOWLEDGED_CALLBACK propertyAcknowledgedCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_SendReportedState(iotHubModuleClientHandle->coreHandle, properties, propertiesLength, (IOTHUB_CLIENT_REPORTED_STATE_CALLBACK)propertyAcknowledgedCallback, userContextCallback);
    }
    else
    {
        LogError("iotHubModuleClientHandle parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SubscribeToCommands(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, IOTHUB_CLIENT_COMMAND_CALLBACK_ASYNC commandCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_SubscribeToCommands(iotHubModuleClientHandle->coreHandle, commandCallback, userContextCallback);
    }
    else
    {
        LogError("iotHubModuleClientHandle parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_GetPropertiesAsync(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle,  IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK propertyCallback,  void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_GetTwinAsync(iotHubModuleClientHandle->coreHandle, (IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK)propertyCallback, userContextCallback);
    }
    else
    {
        LogError("iotHubModuleClientHandle parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_GetPropertiesAndSubscribeToUpdatesAsync(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK propertiesCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubModuleClientHandle != NULL)
    {
        result = IoTHubClientCore_LL_SetDeviceTwinCallback(iotHubModuleClientHandle->coreHandle, (IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK)propertiesCallback, userContextCallback);
    }
    else
    {
        LogError("iotHubModuleClientHandle parameter cannot be NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}
