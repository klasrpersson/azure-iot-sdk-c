// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file iothub_module_client_ll.h
*    @brief   APIs that allow a user to communicate
*             with an Azure IoT Hub.
*
*    @details IoTHubModuleClient_LL allows a user 
*             to communicate with an Azure IoT Hub. It can send events
*             and receive messages. At any given moment in time there can only
*             be at most 1 message callback function.
*
*             This API surface contains a set of APIs that allows the user to
*             interact with the lower layer portion of the IoTHubModuleClient. These APIs
*             contain @c _LL_ in their name, but retain the same functionality like the
*             @c IoTHubModuleClient_... APIs, with one difference. If the @c _LL_ APIs are
*             used then the user is responsible for scheduling when the actual work done
*             by the IoTHubModuleClient happens (when the data is sent/received on/from the network).
*             This is useful for constrained devices where spinning a separate thread is
*             often not desired.
*
*     @warning IoTHubModuleClient_LL_* functions are NOT thread safe.  See
*              https://github.com/Azure/azure-iot-sdk-c/blob/main/doc/threading_notes.md for more details.
*
*/

#ifndef IOTHUB_MODULE_CLIENT_LL_H
#define IOTHUB_MODULE_CLIENT_LL_H

#include <time.h>
#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "iothub_transport_ll.h"
#include "iothub_client_core_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** 
*  @brief   Handle corresponding to a lower layer (LL) module client instance.
*
*  @warning The API functions that use this handle are not thread safe.  See https://github.com/Azure/azure-iot-sdk-c/blob/main/doc/threading_notes.md for more details.
*/
typedef struct IOTHUB_MODULE_CLIENT_LL_HANDLE_DATA_TAG* IOTHUB_MODULE_CLIENT_LL_HANDLE;

#ifdef __cplusplus
}
#endif

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
    * @brief    Creates a IoT Hub client for communication with an existing
    *             IoT Hub using the specified connection string parameter.
    *
    * @param    connectionString    Pointer to a character string
    * @param    protocol            Function pointer for protocol implementation
    *
    *            Sample connection string:
    *                <blockquote>
    *                    <pre>HostName=[IoT Hub name goes here].[IoT Hub suffix goes here, e.g., private.azure-devices-int.net];DeviceId=[Device ID goes here];SharedAccessKey=[Device key goes here];ModuleId=[Module ID goes here]</pre>
    *                </blockquote>
    *
    * @return    A non-NULL #IOTHUB_MODULE_CLIENT_LL_HANDLE value that is used when
    *             invoking other functions for IoT Hub client and @c NULL on failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_MODULE_CLIENT_LL_HANDLE, IoTHubModuleClient_LL_CreateFromConnectionString, const char*, connectionString, IOTHUB_CLIENT_TRANSPORT_PROVIDER, protocol);

    /**
    * @brief    Disposes of resources allocated by the IoT Hub client. This is a
    *             blocking call.
    *
    * @param    iotHubModuleClientHandle    The handle created by a call to the create function.
    *
    * @warning  Do not call this function from inside any application callbacks from this SDK, e.g. your IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK handler.
    */
     MOCKABLE_FUNCTION(, void, IoTHubModuleClient_LL_Destroy, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle);

    /**
    * @brief    Asynchronous call to send the message specified by @p eventMessageHandle.
    *
    * @param    iotHubModuleClientHandle         The handle created by a call to the create function.
    * @param    eventMessageHandle               The handle to an IoT Hub message.
    * @param    eventConfirmationCallback        The callback specified by the module for receiving
    *                                            confirmation of the delivery of the IoT Hub message.
    *                                            This callback can be expected to invoke the
    *                                            IoTHubModuleClient_LL_SendEventAsync function for the
    *                                            same message in an attempt to retry sending a failing
    *                                            message. The user can specify a @c NULL value here to
    *                                            indicate that no callback is required.
    * @param    userContextCallback              User specified context that will be provided to the
    *                                            callback. This can be @c NULL.
    *
    * @warning: Do not call IoTHubModuleClient_LL_Destroy() or IoTHubModuleClient_LL_DoWork() from inside your application's callback.
    *    
    * @remarks
    *            The IOTHUB_MESSAGE_HANDLE instance provided as argument is copied by the function,
    *            so this argument can be destroyed by the calling application right after IoTHubModuleClient_LL_SendEventAsync returns.
    *            The copy of @c eventMessageHandle is later destroyed by the iothub client when the message is effectively sent, if a failure sending it occurs, or if the client is destroyed.
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SendEventAsync, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_MESSAGE_HANDLE, eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, eventConfirmationCallback, void*, userContextCallback);

    /**
    * @brief    This function returns the current sending status for IoTHubModuleClient.
    *
    * @param    iotHubModuleClientHandle  The handle created by a call to the create function.
    * @param    iotHubClientStatus        The sending state is populated at the address pointed
    *                                     at by this parameter. The value will be set to
    *                                     @c IOTHUB_CLIENT_SEND_STATUS_IDLE if there is currently
    *                                     no item to be sent and @c IOTHUB_CLIENT_SEND_STATUS_BUSY
    *                                     if there are.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_GetSendStatus, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_CLIENT_STATUS*, iotHubClientStatus);

    /**
    * @brief    Sets up the message callback to be invoked when Edge issues a
    *             message to the module. This is a blocking call.
    *
    * @param    iotHubModuleClientHandle      The handle created by a call to the create function.
    * @param    messageCallback               The callback specified by the module for receiving
    *                                         messages from IoT Hub.
    * @param    userContextCallback           User specified context that will be provided to the
    *                                         callback. This can be @c NULL.
    *
    * @warning: Do not call IoTHubModuleClient_LL_Destroy() or IoTHubModuleClient_LL_DoWork() from inside your application's callback.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SetMessageCallback, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC, messageCallback, void*, userContextCallback);

    /**
    * @brief    Sets up the connection status callback to be invoked representing the status of
    * the connection to IOT Hub. This is a blocking call.
    *
    * @param    iotHubModuleClientHandle      The handle created by a call to the create function.
    * @param    connectionStatusCallback      The callback specified by the module for receiving
    *                                         updates about the status of the connection to IoT Hub.
    * @param    userContextCallback           User specified context that will be provided to the
    *                                         callback. This can be @c NULL.
    *
    * @warning: Do not call IoTHubModuleClient_LL_Destroy() or IoTHubModuleClient_LL_DoWork() from inside your application's callback.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SetConnectionStatusCallback, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK, connectionStatusCallback, void*, userContextCallback);

    /**
    * @brief    Sets the Retry Policy feature to control how immediatelly and frequently the SDK will attempt to re-connect to the Azure IoT Hub in case a connection issue occurs.
    *
    * @param    iotHubModuleClientHandle      The handle created by a call to the create function.
    * @param    retryPolicy                   The policy to use to reconnect to IoT Hub when a
    *                                         connection drops.
    * @param    retryTimeoutLimitInSeconds    Maximum amount of time(seconds) to attempt reconnection when a
    *                                         connection drops to IOT Hub.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SetRetryPolicy, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_CLIENT_RETRY_POLICY, retryPolicy, size_t, retryTimeoutLimitInSeconds);


    /**
    * @brief    Gets the Retry Policy setting and timeout value for the current retry policy.
    *
    * @param    iotHubModuleClientHandle      The handle created by a call to the create function.
    * @param    retryPolicy                   Out parameter containing the policy to use to reconnect to IoT Hub.
    * @param    retryTimeoutLimitInSeconds    Out parameter containing maximum amount of time, in seconds, to attempt reconnection
    *                                         to IOT Hub for the specified retry policy.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_GetRetryPolicy, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_CLIENT_RETRY_POLICY*, retryPolicy, size_t*, retryTimeoutLimitInSeconds);

    /**
    * @brief    This function returns in the out parameter @p lastMessageReceiveTime
    *             what was the value of the @c time function when the last message was
    *             received at the client.
    *
    * @param    iotHubModuleClientHandle       The handle created by a call to the create function.
    * @param    lastMessageReceiveTime         Out parameter containing the value of @c time function
    *                                          when the last message was received.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_GetLastMessageReceiveTime, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, time_t*, lastMessageReceiveTime);

    /**
    * @brief    This function is meant to be called by the user when work
    *             (sending/receiving) can be done by the IoTHubModuleClient.
    *
    * @param    iotHubModuleClientHandle    The handle created by a call to the create function.
    *
    *            All IoTHubModuleClient interactions (in regards to network traffic
    *            and/or user level callbacks) are the effect of calling this
    *            function and they take place synchronously inside _DoWork.
    *
    * @warning  Do not call this function from inside any application callbacks from this SDK, e.g. your IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK handler.
    */
     MOCKABLE_FUNCTION(, void, IoTHubModuleClient_LL_DoWork, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle);

    /**
    * @brief    This API sets a runtime option identified by parameter @p optionName
    *             to a value pointed to by @p value. @p optionName and the data type
    *             @p value is pointing to are specific for every option.
    *
    * @param    iotHubModuleClientHandle    The handle created by a call to the create function.
    * @param    optionName                  Name of the option.
    * @param    value                       The value.
    *
    * @remarks  Documentation for configuration options is available at https://github.com/Azure/azure-iot-sdk-c/blob/main/doc/Iothub_sdk_options.md.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SetOption, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, const char*, optionName, const void*, value);

    /**
    * @brief    This API specifies a call back to be used when the module receives a desired state update.
    *
    * @param    iotHubModuleClientHandle  The handle created by a call to the create function.
    * @param    moduleTwinCallback        The callback specified by the module client to be used for updating
    *                                     the desired state. The callback will be called in response to patch
    *                                     request send by the IoT Hub services. The payload will be passed to the
    *                                     callback, along with two version numbers:
    *                                        - Desired:
    *                                        - LastSeenReported:
    * @param    userContextCallback       User specified context that will be provided to the
    *                                     callback. This can be @c NULL.
    *
    * @warning: Do not call IoTHubModuleClient_LL_Destroy() or IoTHubModuleClient_LL_DoWork() from inside your application's callback.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SetModuleTwinCallback, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, moduleTwinCallback, void*, userContextCallback);

    /**
    * @brief    This API sneds a report of the module's properties and their current values.
    *
    * @param    iotHubModuleClientHandle The handle created by a call to the create function.
    * @param    reportedState            The current module property values to be 'reported' to the IoT Hub.
    * @param    size                     Number of bytes in @c reportedState.
    * @param    reportedStateCallback    The callback specified by the module client to be called with the
    *                                    result of the transaction.
    * @param    userContextCallback      User specified context that will be provided to the
    *                                    callback. This can be @c NULL.
    *
    * @warning: Do not call IoTHubModuleClient_LL_Destroy() or IoTHubModuleClient_LL_DoWork() from inside your application's callback.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SendReportedState, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, const unsigned char*, reportedState, size_t, size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK, reportedStateCallback, void*, userContextCallback);

     /**
     * @brief	This API enabled the device to request the full module twin (with all the desired and reported properties) on demand.
     *
     * @param	iotHubModuleClientHandle	The handle created by a call to the create function.
     * @param	deviceTwinCallback	        The callback specified by the module client to receive the Twin document.
     * @param	userContextCallback		    User specified context that will be provided to the
     * 									    callback. This can be @c NULL.
     *
     * @warning: Do not call IoTHubModuleClient_LL_Destroy() or IoTHubModuleClient_LL_DoWork() from inside your application's callback.
     *
     * @return	IOTHUB_CLIENT_OK upon success or an error code upon failure.
     */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_GetTwinAsync, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, deviceTwinCallback, void*, userContextCallback);

     /**
     * @brief    This API sets callback for async cloud to module method call.
     *
     * @param    iotHubModuleClientHandle        The handle created by a call to the create function.
     * @param    moduleMethodCallback            The callback which will be called by IoT Hub.
     * @param    userContextCallback             User specified context that will be provided to the
     *                                           callback. This can be @c NULL.
     *
     * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
     */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SetModuleMethodCallback, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC, moduleMethodCallback, void*, userContextCallback);

    /**
    * @brief    Asynchronous call to send the message specified by @p eventMessageHandle.
    *
    * @param    iotHubModuleClientHandle      The handle created by a call to the create function.
    * @param    eventMessageHandle            The handle to an IoT Hub message.
    * @param    outputName                    The name of the queue to send the message to.
    * @param    eventConfirmationCallback     The callback specified by the module for receiving
    *                                         confirmation of the delivery of the IoT Hub message.
    *                                         This callback can be expected to invoke the
    *                                         IoTHubModuleClient_LL_SendEventAsync function for the
    *                                         same message in an attempt to retry sending a failing
    *                                         message. The user can specify a @c NULL value here to
    *                                         indicate that no callback is required.
    * @param    userContextCallback           User specified context that will be provided to the
    *                                         callback. This can be @c NULL.
    *
    * @warning: Do not call IoTHubModuleClient_LL_Destroy() or IoTHubModuleClient_LL_DoWork() from inside your application's callback.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SendEventToOutputAsync, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_MESSAGE_HANDLE, eventMessageHandle, const char*, outputName, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, eventConfirmationCallback, void*, userContextCallback);

    /**
    * @brief    This API sets callback for method call that is directed to specified 'inputName' queue (e.g. messages from IoTHubModuleClient_LL_SendEventToOutputAsync)
    *
    * @param    iotHubModuleClientHandle      The handle created by a call to the create function.
    * @param    inputName                     The name of the queue to listen on for this moduleMethodCallback/userContextCallback.
    * @param    eventHandlerCallback          The callback which will be called by IoT Hub.
    * @param    userContextCallback           User specified context that will be provided to the
    *                                         callback. This can be @c NULL.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SetInputMessageCallback, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, const char*, inputName, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC, eventHandlerCallback, void*, userContextCallback);


    /**
    * @brief    This API creates a module handle based on environment variables set in the Edge runtime.
    *           NOTE: It is *ONLY* valid when the code is running in a container initiated by Edge.
    *
    * @param    protocol            Function pointer for protocol implementation.  This *MUST* be MQTT_Protocol.
    *
    * @remarks  The protocol parameter MUST be set to MQTT_Protocol.  Using other values will cause undefined behavior.
    *
    * @return   A non-NULL #IOTHUB_MODULE_CLIENT_LL_HANDLE value that is used when
    *           invoking other functions for IoT Hub client and @c NULL on failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_MODULE_CLIENT_LL_HANDLE, IoTHubModuleClient_LL_CreateFromEnvironment, IOTHUB_CLIENT_TRANSPORT_PROVIDER, protocol);

    /**
    * @brief    This API invokes a device method on a specified device
    *
    * @param    iotHubModuleClientHandle        The handle created by a call to a create function
    * @param    deviceId                        The device id of the device to invoke a method on
    * @param    methodName                      The name of the method
    * @param    methodPayload                   The method payload (in json format)
    *
    * @warning  The timeout parameter is ignored.  See https://github.com/Azure/azure-iot-sdk-c/issues/1378.
    *           The timeout used will be the default for IoT Edge.
    *
    * @param    responseStatus                  This pointer will be filled with the response status after invoking the device method
    * @param    responsePayload                 This pointer will be filled with the response payload
    * @param    responsePayloadSize             This pointer will be filled with the response payload size
    *
    * @warning  Other _LL_ functions such as IoTHubModuleClient_LL_SendEventAsync queue work to be performed later and do not block.  IoTHubModuleClient_LL_DeviceMethodInvoke
    *           will block however until the method invocation is completed or fails, which may take a while.
    *
    * @return   IOTHUB_CLIENT_OK upon success, or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_DeviceMethodInvoke, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, const char*, deviceId, const char*, methodName, const char*, methodPayload, unsigned int, timeout, int*, responseStatus, unsigned char**, responsePayload, size_t*, responsePayloadSize);

    /**
    * @brief    This API invokes a module method on a specified module
    *
    * @param    iotHubModuleClientHandle        The handle created by a call to a create function
    * @param    deviceId                        The device id of the device to invoke a method on
    * @param    moduleId                        The module id of the module to invoke a method on
    * @param    methodName                      The name of the method
    * @param    methodPayload                   The method payload (in json format)
    * @param    timeout                         The time in seconds before a timeout occurs
    *
    * @warning  The timeout parameter is ignored.  See https://github.com/Azure/azure-iot-sdk-c/issues/1378.
    *           The timeout used will be the default for IoT Edge.
    *
    * @param    responseStatus                  This pointer will be filled with the response status after invoking the module method
    * @param    responsePayload                 This pointer will be filled with the response payload
    * @param    responsePayloadSize             This pointer will be filled with the response payload size
    *
    * @warning  Other _LL_ functions such as IoTHubModuleClient_LL_SendEventAsync queue work to be performed later and do not block.  IoTHubModuleClient_LL_ModuleMethodInvoke
    *           will block however until the method invocation is completed or fails, which may take a while.
    *
    *
    * @return   IOTHUB_CLIENT_OK upon success, or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_ModuleMethodInvoke, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, const char*, deviceId, const char*, moduleId, const char*, methodName, const char*, methodPayload, unsigned int, timeout, int*, responseStatus, unsigned char**, responsePayload, size_t*, responsePayloadSize);

    /**
    * @brief    This API sends an acknowledgement to Azure IoT Hub that a cloud-to-device message has been received and frees resources associated with the message.
    *
    * @param    module_ll_handle                The handle created by a call to a create function.
    * @param    message                         The cloud-to-device message received through the callback provided to IoTHubModuleClient_LL_SetMessageCallback or IoTHubModuleClient_LL_SetInputMessageCallback.
    * @param    disposition                     Acknowledgement option for the message.
    *
    * @warning  This function is to be used only when IOTHUBMESSAGE_ASYNC_ACK is used in the callback for incoming cloud-to-device messages.
    * @remarks
    *           If your cloud-to-device message callback returned IOTHUBMESSAGE_ASYNC_ACK, it MUST call this API eventually.
    *           Beyond sending acknowledgment to the service, this method also handles freeing message's memory.
    *           Not calling this function will result in memory leaks.
    *           Depending on the protocol used, this API will acknowledge cloud-to-device messages differently:
    *           AMQP: A MESSAGE DISPOSITION is sent using the `disposition` option provided.
    *           MQTT: A PUBACK is sent if `disposition` is `IOTHUBMESSAGE_ACCEPTED`. Passing any other option results in no PUBACK sent for the message.
    *           HTTP: A HTTP request is sent using the `disposition` option provided.
    *           
    * @return   IOTHUB_CLIENT_OK upon success, or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SendMessageDisposition, IOTHUB_MODULE_CLIENT_LL_HANDLE, module_ll_handle, IOTHUB_MESSAGE_HANDLE, message, IOTHUBMESSAGE_DISPOSITION_RESULT, disposition);

    /**
    * @brief      Asynchronous call to send the telemetry message specified by @p telemetryMessageHandle.
    *
    * @param[in]  iotHubModuleClientHandle        The handle created by a call to the create function.
    * @param[in]  telemetryMessageHandle          The handle to an IoT Hub message.
    * @param[in]  telemetryConfirmationCallback   Optional callback specified by the module for receiving
    *                                             confirmation of the delivery of the telemetry.
    * @param[in]  userContextCallback             User specified context that will be provided to the
    *                                             callback. This can be @c NULL.
    *
    * @remarks    The application behavior is undefined if the user calls
    *             the IoTHubModuleClient_LL_Destroy function from within any callback.
    *
    *             The IOTHUB_MESSAGE_HANDLE instance provided as an argument is copied by the function,
    *             so this argument can be destroyed by the calling application right after IoTHubModuleClient_LL_SendTelemetryAsync returns.
    *             The copy of @p telemetryMessageHandle is later destroyed by the iothub client when the message is successfully sent, if a failure sending it occurs, or if the client is destroyed.
    *
    * @return     IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SendTelemetryAsync, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_MESSAGE_HANDLE, telemetryMessageHandle, IOTHUB_CLIENT_TELEMETRY_CALLBACK, telemetryConfirmationCallback, void*, userContextCallback);
    
    /**
    * @brief      Subscribe to incoming commands from IoT Hub.
    *
    * @param[in]  iotHubModuleClientHandle  The handle created by a call to the create function.
    * @param[in]  commandCallback           The callback which will be called when a command request arrives.
    * @param[in]  userContextCallback       User specified context that will be provided to the
    *                                       callback. This can be @c NULL.
    *
    * @remarks    The application behavior is undefined if the user calls
    *             the IoTHubModuleClient_LL_Destroy function from within any callback.
    *    
    * @return     IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SubscribeToCommands, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_CLIENT_COMMAND_CALLBACK_ASYNC, commandCallback,  void*, userContextCallback);

    /**
    * @brief      Sends module properties to IoT Hub.
    *
    * @param[in]  iotHubModuleClientHandle     The handle created by a call to the create function.
    * @param[in]  properties                   Serialized property data to be sent to IoT Hub.  This buffer can either be
    *                                          manually serialized created with IoTHubClient_Properties_Serializer_CreateReported() 
    *                                          or IoTHubClient_Properties_Serializer_CreateWritableResponse().
    * @param[in]  propertiesLength             Number of bytes in the properties buffer.
    * @param[in]  propertyAcknowledgedCallback Optional callback specified by the application to be called with the
    *                                          result of the transaction.
    * @param[in]  userContextCallback          User specified context that will be provided to the
    *                                          callback. This can be @c NULL.
    *
    * @remarks   The application behavior is undefined if the user calls
    *            the IoTHubModuleClient_LL_Destroy function from within any callback.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SendPropertiesAsync, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle,  const unsigned char*, properties, size_t, propertiesLength, IOTHUB_CLIENT_PROPERTY_ACKNOWLEDGED_CALLBACK, propertyAcknowledgedCallback, void*, userContextCallback);

    /**
    * @brief      Retrieves all module properties from IoT Hub.
    *
    * @param[in]  iotHubModuleClientHandle  The handle created by a call to the create function.
    * @param[in]  propertyCallback          Callback invoked when properties are retrieved.
    *                                       The API IoTHubClient_Deserialize_Properties() can help deserialize the raw 
    *                                       payload stream.
    * @param[in]  userContextCallback       User specified context that will be provided to the
    *                                       callback. This can be @c NULL.
    *
    * @remarks    The application behavior is undefined if the user calls
    *             the IoTHubModuleClient_LL_Destroy function from within any callback.
    *
    * @return     IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_GetPropertiesAsync, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle,  IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK, propertyCallback, void*, userContextCallback);
    
    /**
    * @brief      Retrieves all module properties from IoT Hub and also subscribes for updates to writable properties.
    *
    * @param[in]  iotHubModuleClientHandle The handle created by a call to the create function.
    * @param[in]  propertyUpdateCallback   Callback both on initial retrieval of properties stored on IoT Hub
    *                                      and subsequent service-initiated modifications of writable properties.
    *                                      The API IoTHubClient_Deserialize_Properties() can help deserialize the raw 
    *                                      payload stream.
    * @param[in]  userContextCallback      User specified context that will be provided to the
    *                                      callback. This can be @c NULL.
    *
    * @remarks    The application behavior is undefined if the user calls
    *             the IoTHubModuleClient_LL_Destroy function from within any callback.
    *
    * @return     IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_GetPropertiesAndSubscribeToUpdatesAsync, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK, propertyUpdateCallback, void*, userContextCallback);

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_MODULE_CLIENT_LL_H */
