// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/constbuffer.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/safe_math.h"

#include "iothub_client_core_ll.h"
#include "iothub_client_options.h"
#include "iothub_client_version.h"
#include "iothub_transport_ll.h"
#include "internal/iothub_client_authorization.h"
#include "internal/iothub_client_private.h"
#include "internal/iothub_client_diagnostic.h"
#include "internal/iothubtransport.h"

#ifndef DONT_USE_UPLOADTOBLOB
#include "internal/iothub_client_ll_uploadtoblob.h"
#endif

#include "azure_c_shared_utility/envvariable.h"
#include "azure_prov_client/iothub_security_factory.h"
#include "internal/iothub_client_edge.h"

#define LOG_ERROR_RESULT LogError("result = %s", MU_ENUM_TO_STRING(IOTHUB_CLIENT_RESULT, result));
#define INDEFINITE_TIME ((time_t)(-1))
#define ERROR_CODE_BECAUSE_DESTROY 0

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, IOTHUB_CLIENT_FILE_UPLOAD_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_CLIENT_RETRY_POLICY, IOTHUB_CLIENT_RETRY_POLICY_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_IDENTITY_TYPE, IOTHUB_IDENTITY_TYPE_VALUE);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_PROCESS_ITEM_RESULT, IOTHUB_PROCESS_ITEM_RESULT_VALUE);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_CLIENT_IOTHUB_METHOD_STATUS, IOTHUB_CLIENT_IOTHUB_METHOD_STATUS_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_STATUS_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, IOTHUB_CLIENT_CONNECTION_STATUS_REASON_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(TRANSPORT_TYPE, TRANSPORT_TYPE_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(DEVICE_TWIN_UPDATE_STATE, DEVICE_TWIN_UPDATE_STATE_VALUES);
#ifndef DONT_USE_UPLOADTOBLOB
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT_VALUES);
#endif // DONT_USE_UPLOADTOBLOB

#define CALLBACK_TYPE_VALUES     \
    CALLBACK_TYPE_NONE,          \
    CALLBACK_TYPE_MESSAGE_SYNC,  \
    CALLBACK_TYPE_MESSAGE_ASYNC, \
    CALLBACK_TYPE_METHOD,   \
    CALLBACK_TYPE_INBOUND_METHOD,  \
    CALLBACK_TYPE_COMMAND


MU_DEFINE_ENUM(CALLBACK_TYPE, CALLBACK_TYPE_VALUES)
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(CALLBACK_TYPE, CALLBACK_TYPE_VALUES)

static const char COMPONENT_DELIMETER = '*';

typedef struct IOTHUB_METHOD_CALLBACK_DATA_TAG
{
    CALLBACK_TYPE type;
    IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback;
    IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK inboundDeviceMethodCallback;
    IOTHUB_CLIENT_COMMAND_CALLBACK_ASYNC commandCallback;
    void* userContextCallback;
}IOTHUB_METHOD_CALLBACK_DATA;

typedef struct IOTHUB_EVENT_CALLBACK_TAG
{
    STRING_HANDLE inputName;
    IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC callbackAsync;
    IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC_EX callbackAsyncEx;
    void* userContextCallback;
    void* userContextCallbackEx;
}IOTHUB_EVENT_CALLBACK;

typedef struct IOTHUB_MESSAGE_CALLBACK_DATA_TAG
{
    CALLBACK_TYPE type;
    IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC callbackSync;
    IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC_EX callbackAsync;
    void* userContextCallback;
}IOTHUB_MESSAGE_CALLBACK_DATA;

typedef struct GET_TWIN_CONTEXT_TAG
{
    IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK callback;
    void* context;
} GET_TWIN_CONTEXT;

typedef struct IOTHUB_CLIENT_CORE_LL_HANDLE_DATA_TAG
{
    DLIST_ENTRY waitingToSend;
    DLIST_ENTRY iot_msg_queue;
    DLIST_ENTRY iot_ack_queue;
    TRANSPORT_LL_HANDLE transportHandle;
    bool isSharedTransport;
    IOTHUB_DEVICE_HANDLE deviceHandle;
    TRANSPORT_PROVIDER_FIELDS;
    IOTHUB_MESSAGE_CALLBACK_DATA messageCallback;
    IOTHUB_METHOD_CALLBACK_DATA methodCallback;
    IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK conStatusCallback;
    void* conStatusUserContextCallback;
    time_t lastMessageReceiveTime;
    TICK_COUNTER_HANDLE tickCounter; /*shared tickcounter used to track message timeouts in waitingToSend list*/
    tickcounter_ms_t currentMessageTimeout;
    uint64_t current_device_twin_timeout;
    IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback;
    void* deviceTwinContextCallback;
    IOTHUB_CLIENT_RETRY_POLICY retryPolicy;
    size_t retryTimeoutLimitInSeconds;
#ifndef DONT_USE_UPLOADTOBLOB
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE uploadToBlobHandle;
#endif

    IOTHUB_CLIENT_EDGE_HANDLE methodHandle;
    uint32_t data_msg_id;
    bool complete_twin_update_encountered;
    IOTHUB_AUTHORIZATION_HANDLE authorization_module;
    STRING_HANDLE product_info;
    IOTHUB_DIAGNOSTIC_SETTING_DATA diagnostic_setting;
    SINGLYLINKEDLIST_HANDLE event_callbacks;  // List of IOTHUB_EVENT_CALLBACK's
    STRING_HANDLE model_id;
}IOTHUB_CLIENT_CORE_LL_HANDLE_DATA;

static const char HOSTNAME_TOKEN[] = "HostName";
static const char DEVICEID_TOKEN[] = "DeviceId";
static const char X509_TOKEN[] = "x509";
static const char X509_TOKEN_ONLY_ACCEPTABLE_VALUE[] = "true";
static const char DEVICEKEY_TOKEN[] = "SharedAccessKey";
static const char DEVICESAS_TOKEN[] = "SharedAccessSignature";
static const char PROTOCOL_GATEWAY_HOST_TOKEN[] = "GatewayHostName";
static const char MODULE_ID_TOKEN[] = "ModuleId";
static const char PROVISIONING_TOKEN[] = "UseProvisioning";
static const char PROVISIONING_ACCEPTABLE_VALUE[] = "true";

static const int DEFAULT_COMMAND_RESPONSE_STATUS_CODE = 500;

/*The following section should be moved to iothub_module_client_ll.c during impending refactor*/

static const char* ENVIRONMENT_VAR_EDGEHUB_CONNECTIONSTRING = "EdgeHubConnectionString";
static const char* ENVIRONMENT_VAR_EDGEHUB_CACERTIFICATEFILE = "EdgeModuleCACertificateFile";
static const char* ENVIRONMENT_VAR_EDGEAUTHSCHEME = "IOTEDGE_AUTHSCHEME";
static const char* ENVIRONMENT_VAR_EDGEDEVICEID = "IOTEDGE_DEVICEID";
static const char* ENVIRONMENT_VAR_EDGEMODULEID = "IOTEDGE_MODULEID";
static const char* ENVIRONMENT_VAR_EDGEHUBHOSTNAME = "IOTEDGE_IOTHUBHOSTNAME";
static const char* ENVIRONMENT_VAR_EDGEGATEWAYHOST = "IOTEDGE_GATEWAYHOSTNAME";
static const char* SAS_TOKEN_AUTH = "sasToken";


typedef struct EDGE_ENVIRONMENT_VARIABLES_TAG
{
    const char* connection_string;
    const char* ca_trusted_certificate_file;
    const char* auth_scheme;
    const char* device_id;
    const char* iothub_name;
    const char* iothub_suffix;
    const char* gatewayhostname;
    const char* module_id;
    char* iothub_buffer;
} EDGE_ENVIRONMENT_VARIABLES;


static int retrieve_edge_environment_variabes(EDGE_ENVIRONMENT_VARIABLES *edge_environment_variables)
{
    int result;
    const char* edgehubhostname;
    char* edgehubhostname_separator;

    if ((edge_environment_variables->connection_string = environment_get_variable(ENVIRONMENT_VAR_EDGEHUB_CONNECTIONSTRING)) != NULL)
    {
        if ((edge_environment_variables->ca_trusted_certificate_file = environment_get_variable(ENVIRONMENT_VAR_EDGEHUB_CACERTIFICATEFILE)) == NULL)
        {
            LogError("Environment variable %s is missing.  When %s is set, it is required", ENVIRONMENT_VAR_EDGEHUB_CACERTIFICATEFILE, ENVIRONMENT_VAR_EDGEHUB_CONNECTIONSTRING);
            result = MU_FAILURE;
        }
        else
        {
            // If we can read in the connection string and trusted certs, we're done.
            result = 0;
        }
    }
    else
    {
        // We're NOT using pre-configured EdgeConnection string / certificates.  In this case, we use these environment variables when
        // communicating to Edge service.
        if ((edge_environment_variables->auth_scheme = environment_get_variable(ENVIRONMENT_VAR_EDGEAUTHSCHEME)) == NULL)
        {
            LogError("Environment %s not set", ENVIRONMENT_VAR_EDGEAUTHSCHEME);
            result = MU_FAILURE;
        }
        else if (strcmp(edge_environment_variables->auth_scheme, SAS_TOKEN_AUTH) != 0)
        {
            LogError("Environment %s was set to %s, but only support for %s", ENVIRONMENT_VAR_EDGEAUTHSCHEME, edge_environment_variables->auth_scheme, SAS_TOKEN_AUTH);
            result = MU_FAILURE;
        }
        else if ((edge_environment_variables->device_id = environment_get_variable(ENVIRONMENT_VAR_EDGEDEVICEID)) == NULL)
        {
            LogError("Environment %s not set", ENVIRONMENT_VAR_EDGEDEVICEID);
            result = MU_FAILURE;
        }
        else if ((edgehubhostname = environment_get_variable(ENVIRONMENT_VAR_EDGEHUBHOSTNAME)) == NULL)
        {
            LogError("Environment %s not set", ENVIRONMENT_VAR_EDGEHUBHOSTNAME);
            result = MU_FAILURE;
        }
        else if ((edge_environment_variables->gatewayhostname = environment_get_variable(ENVIRONMENT_VAR_EDGEGATEWAYHOST)) == NULL)
        {
            LogError("Environment %s not set", ENVIRONMENT_VAR_EDGEGATEWAYHOST);
            result = MU_FAILURE;
        }
        else if ((edge_environment_variables->module_id = environment_get_variable(ENVIRONMENT_VAR_EDGEMODULEID)) == NULL)
        {
            LogError("Environment %s not set", ENVIRONMENT_VAR_EDGEMODULEID);
            result = MU_FAILURE;
        }
        // Make a copy of just ENVIRONMENT_VAR_EDGEHUBHOSTNAME.  We need to make changes in place (namely inserting a '\0')
        // and can't do this with system environment variable safely.
        else if (mallocAndStrcpy_s(&edge_environment_variables->iothub_buffer, edgehubhostname) != 0)
        {
            LogError("Unable to copy buffer");
            result = MU_FAILURE;
        }
        else if ((edgehubhostname_separator = strchr(edge_environment_variables->iothub_buffer, '.')) == NULL)
        {
            LogError("Environment edgehub %s invalid, requires '.' separator", edge_environment_variables->iothub_buffer);
            result = MU_FAILURE;
        }
        else if (*(edgehubhostname_separator + 1) == 0)
        {
            LogError("Environment edgehub %s invalid, no content after '.' separator", edge_environment_variables->iothub_buffer);
            result = MU_FAILURE;
        }
        else
        {
            edge_environment_variables->iothub_name = edge_environment_variables->iothub_buffer;
            *edgehubhostname_separator = 0;
            edge_environment_variables->iothub_suffix = edgehubhostname_separator + 1;
            result = 0;
        }
    }

    return result;
}

IOTHUB_CLIENT_EDGE_HANDLE IoTHubClientCore_LL_GetEdgeHandle(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle)
{
    IOTHUB_CLIENT_EDGE_HANDLE result;
    if (iotHubClientHandle != NULL)
    {
        result = iotHubClientHandle->methodHandle;
    }
    else
    {
        result = NULL;
    }

    return result;
}

static void setTransportProtocol(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData, TRANSPORT_PROVIDER* protocol)
{
    handleData->IoTHubTransport_SendMessageDisposition = protocol->IoTHubTransport_SendMessageDisposition;
    handleData->IoTHubTransport_GetHostname = protocol->IoTHubTransport_GetHostname;
    handleData->IoTHubTransport_SetOption = protocol->IoTHubTransport_SetOption;
    handleData->IoTHubTransport_Create = protocol->IoTHubTransport_Create;
    handleData->IoTHubTransport_Destroy = protocol->IoTHubTransport_Destroy;
    handleData->IoTHubTransport_Register = protocol->IoTHubTransport_Register;
    handleData->IoTHubTransport_Unregister = protocol->IoTHubTransport_Unregister;
    handleData->IoTHubTransport_Subscribe = protocol->IoTHubTransport_Subscribe;
    handleData->IoTHubTransport_Unsubscribe = protocol->IoTHubTransport_Unsubscribe;
    handleData->IoTHubTransport_DoWork = protocol->IoTHubTransport_DoWork;
    handleData->IoTHubTransport_SetRetryPolicy = protocol->IoTHubTransport_SetRetryPolicy;
    handleData->IoTHubTransport_GetSendStatus = protocol->IoTHubTransport_GetSendStatus;
    handleData->IoTHubTransport_ProcessItem = protocol->IoTHubTransport_ProcessItem;
    handleData->IoTHubTransport_Subscribe_DeviceTwin = protocol->IoTHubTransport_Subscribe_DeviceTwin;
    handleData->IoTHubTransport_Unsubscribe_DeviceTwin = protocol->IoTHubTransport_Unsubscribe_DeviceTwin;
    handleData->IoTHubTransport_GetTwinAsync = protocol->IoTHubTransport_GetTwinAsync;
    handleData->IoTHubTransport_Subscribe_DeviceMethod = protocol->IoTHubTransport_Subscribe_DeviceMethod;
    handleData->IoTHubTransport_Unsubscribe_DeviceMethod = protocol->IoTHubTransport_Unsubscribe_DeviceMethod;
    handleData->IoTHubTransport_DeviceMethod_Response = protocol->IoTHubTransport_DeviceMethod_Response;
    handleData->IoTHubTransport_Subscribe_InputQueue = protocol->IoTHubTransport_Subscribe_InputQueue;
    handleData->IoTHubTransport_Unsubscribe_InputQueue = protocol->IoTHubTransport_Unsubscribe_InputQueue;
    handleData->IoTHubTransport_SetCallbackContext = protocol->IoTHubTransport_SetCallbackContext;
    handleData->IoTHubTransport_GetSupportedPlatformInfo = protocol->IoTHubTransport_GetSupportedPlatformInfo;
}

static bool is_event_equal(IOTHUB_EVENT_CALLBACK *event_callback, const char *input_name)
{
    bool result;

    if (event_callback != NULL)
    {
        const char* event_input_name = STRING_c_str(event_callback->inputName);
        if ((event_input_name != NULL) && (input_name != NULL))
        {
            // Matched the input queue name of a named handler
            result = (strcmp(event_input_name, input_name) == 0);
        }
        else if ((input_name == NULL) && (event_input_name == NULL))
        {
            // Matched the default handler
            result = true;
        }
        else
        {
            result = false;
        }
    }
    else
    {
        result = false;
    }
    return result;
}

static bool is_event_equal_for_match(LIST_ITEM_HANDLE list_item, const void* match_context)
{
    return is_event_equal((IOTHUB_EVENT_CALLBACK*)singlylinkedlist_item_get_value(list_item), (const char*)match_context);
}

static void device_twin_data_destroy(IOTHUB_DEVICE_TWIN* client_item)
{
    CONSTBUFFER_DecRef(client_item->report_data_handle);
    free(client_item);
}

static int create_edge_handle(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handle_data, const IOTHUB_CLIENT_CONFIG* config, const char* module_id)
{
    int result;
    (void)config;
    (void)module_id;

    /* There is no way to currently distinguish a regular module from a edge module, so this handle is created regardless of if appropriate.
    However, as a gateway hostname is required in order to create an Edge Handle, we need to at least make sure that exists
    in order to prevent errors.

    The end result is that all edge modules will have an EdgeHandle, but only some non-edge modules will have it.
    Regardless, non-edge modules will never be able to use the handle.
    */
    if (config->protocolGatewayHostName != NULL)
    {
        handle_data->methodHandle = IoTHubClient_EdgeHandle_Create(config, handle_data->authorization_module, module_id);

        if (handle_data->methodHandle == NULL)
        {
            LogError("Unable to IoTHubModuleClient_LL_MethodHandle_Create");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }
    else
    {
        result = 0;
    }

    return result;
}

static int create_blob_upload_module(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handle_data, const IOTHUB_CLIENT_CONFIG* config, bool use_dev_auth)
{
    int result;
    (void)handle_data;
    (void)config;
    (void)use_dev_auth;
#ifndef DONT_USE_UPLOADTOBLOB
    handle_data->uploadToBlobHandle = IoTHubClient_LL_UploadToBlob_Create(config, handle_data->authorization_module);
    if (handle_data->uploadToBlobHandle == NULL)
    {
        LogError("unable to IoTHubClientCore_LL_UploadToBlob_Create");
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_CREDENTIAL_TYPE credential_type = IoTHubClient_Auth_Get_Credential_Type(handle_data->authorization_module);

        if (use_dev_auth &&
            (credential_type == IOTHUB_CREDENTIAL_TYPE_X509 || credential_type == IOTHUB_CREDENTIAL_TYPE_X509_ECC))
        {
            char* x509_certificate = NULL;
            char* x509_private_key = NULL;

            if (IoTHubClient_Auth_Get_x509_info(handle_data->authorization_module, &x509_certificate, &x509_private_key) != 0)
            {
                LogError("Failed retrieving x509 client certificate and/or private key for upload to blob.");
                result = MU_FAILURE;
            }
            else
            {
                if (IoTHubClient_LL_UploadToBlob_SetOption(handle_data->uploadToBlobHandle, OPTION_X509_CERT, x509_certificate) != IOTHUB_CLIENT_OK)
                {
                    LogError("Failed setting x509 client certificate for upload to blob.");
                    result = MU_FAILURE;
                }
                else if (IoTHubClient_LL_UploadToBlob_SetOption(handle_data->uploadToBlobHandle, OPTION_X509_PRIVATE_KEY, x509_private_key) != IOTHUB_CLIENT_OK)
                {
                    LogError("Failed setting x509 client private key for upload to blob.");
                    result = MU_FAILURE;
                }
                else
                {
                    result = 0;
                }
                
                free(x509_certificate);
                free(x509_private_key);
            }
        }
        else
        {
            result = 0;
        }
    }
#else
    result = 0;
#endif
    return result;
}

static void destroy_blob_upload_module(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handle_data)
{
    (void)handle_data;
#ifndef DONT_USE_UPLOADTOBLOB
    IoTHubClient_LL_UploadToBlob_Destroy(handle_data->uploadToBlobHandle);
#endif
}

static void destroy_module_method_module(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handle_data)
{
    (void)handle_data;

    IoTHubClient_EdgeHandle_Destroy(handle_data->methodHandle);
}

static bool invoke_message_callback(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData, IOTHUB_MESSAGE_HANDLE messageHandle)
{
    bool result;
    handleData->lastMessageReceiveTime = get_time(NULL);

    switch (handleData->messageCallback.type)
    {
        case CALLBACK_TYPE_NONE:
        {
            LogError("Invalid workflow - not currently set up to accept messages");
            result = false;
            break;
        }
        case CALLBACK_TYPE_MESSAGE_SYNC:
        {
            IOTHUBMESSAGE_DISPOSITION_RESULT cb_result = handleData->messageCallback.callbackSync(messageHandle, handleData->messageCallback.userContextCallback);

            if (cb_result != IOTHUBMESSAGE_ASYNC_ACK)
            {
                if (handleData->IoTHubTransport_SendMessageDisposition(handleData->deviceHandle, messageHandle, cb_result) != IOTHUB_CLIENT_OK)
                {
                    LogError("IoTHubTransport_SendMessageDisposition failed");
                }
            }

            result = true;
            break;
        }
        case CALLBACK_TYPE_MESSAGE_ASYNC:
        {
            result = handleData->messageCallback.callbackAsync(messageHandle, handleData->messageCallback.userContextCallback);
            if (!result)
            {
                LogError("messageCallbackEx failed");
            }
            break;
        }
        default:
        {
            LogError("Invalid state");
            result = false;
            break;
        }
    }

    return result;
}

static STRING_HANDLE make_product_info(const char* product, PLATFORM_INFO_OPTION option)
{
    STRING_HANDLE result;
    STRING_HANDLE pfi = platform_get_platform_info(option);
    if (pfi == NULL)
    {
        LogError("Platform get info failed");
        result = NULL;
    }
    else
    {
        if (product == NULL)
        {
            result = STRING_construct_sprintf("%s %s", CLIENT_DEVICE_TYPE_PREFIX CLIENT_DEVICE_BACKSLASH IOTHUB_SDK_VERSION, STRING_c_str(pfi));
        }
        else
        {
            result = STRING_construct_sprintf("%s %s %s", product, CLIENT_DEVICE_TYPE_PREFIX CLIENT_DEVICE_BACKSLASH IOTHUB_SDK_VERSION, STRING_c_str(pfi));
        }
        STRING_delete(pfi);
    }
    return result;
}

static void IoTHubClientCore_LL_SendComplete(PDLIST_ENTRY completed, IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* ctx)
{
    if (
        (ctx == NULL) ||
        (completed == NULL)
        )
    {
        /*"shall return"*/
        LogError("invalid arg");
    }
    else
    {
        PDLIST_ENTRY oldest;
        while ((oldest = DList_RemoveHeadList(completed)) != completed)
        {
            IOTHUB_MESSAGE_LIST* messageList = (IOTHUB_MESSAGE_LIST*)containingRecord(oldest, IOTHUB_MESSAGE_LIST, entry);
            if (messageList->callback != NULL)
            {
                messageList->callback(result, messageList->context);
            }
            IoTHubMessage_Destroy(messageList->messageHandle);
            free(messageList);
        }
    }
}

static void IoTHubClientCore_LL_RetrievePropertyComplete(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* ctx)
{
    if (ctx == NULL)
    {
        LogError("Invalid argument ctx NULL");
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)ctx;
        if (handleData->deviceTwinCallback)
        {
            if (update_state == DEVICE_TWIN_UPDATE_COMPLETE)
            {
                handleData->complete_twin_update_encountered = true;
            }
            if (handleData->complete_twin_update_encountered)
            {
                handleData->deviceTwinCallback(update_state, payLoad, size, handleData->deviceTwinContextCallback);
            }
        }
    }
}

static void IoTHubClientCore_LL_ReportedStateComplete(uint32_t item_id, int status_code, void* ctx)
{
    if (ctx == NULL)
    {
        /*"shall return"*/
        LogError("Invalid argument handle=%p", ctx);
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)ctx;

        DLIST_ENTRY* client_item = handleData->iot_ack_queue.Flink;
        while (client_item != &(handleData->iot_ack_queue)) /*while we are not at the end of the list*/
        {
            PDLIST_ENTRY next_item = client_item->Flink;
            IOTHUB_DEVICE_TWIN* queue_data = containingRecord(client_item, IOTHUB_DEVICE_TWIN, entry);
            if (queue_data->item_id == item_id)
            {
                if (queue_data->reported_state_callback != NULL)
                {
                    queue_data->reported_state_callback(status_code, queue_data->context);
                }
                DList_RemoveEntryList(client_item);
                device_twin_data_destroy(queue_data);
                break;
            }
            client_item = next_item;
        }
    }
}

static void IoTHubClientCore_LL_ConnectionStatusCallBack(IOTHUB_CLIENT_CONNECTION_STATUS status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* ctx)
{
    if (ctx == NULL)
    {
        /*"shall return"*/
        LogError("invalid arg");
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)ctx;

        if (handleData->conStatusCallback != NULL)
        {
            handleData->conStatusCallback(status, reason, handleData->conStatusUserContextCallback);
        }
    }

}

static const char* IoTHubClientCore_LL_GetProductInfo(void* ctx)
{
    const char* result;
    if (ctx == NULL)
    {
        result = NULL;
        LogError("invalid argument ctx %p", ctx);
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* iothub_data = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)ctx;
        result = STRING_c_str(iothub_data->product_info);
    }
    return result;
}

static const char* IoTHubClientCore_LL_GetModelId(void* ctx)
{
    const char* result;
    if (ctx == NULL)
    {
        result = NULL;
        LogError("invalid argument ctx %p", ctx);
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* iothub_data = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)ctx;
        result = STRING_c_str(iothub_data->model_id);
    }
    return result;
}

static bool IoTHubClientCore_LL_MessageCallbackFromInput(IOTHUB_MESSAGE_HANDLE messageHandle, void* ctx)
{
    bool result;
    IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* clientHandleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)ctx;

    if ((clientHandleData == NULL) || messageHandle == NULL)
    {
        LogError("invalid argument: clientHandleData(%p), messageHandle(%p)", clientHandleData, messageHandle);
        result = false;
    }
    else if (clientHandleData->event_callbacks == NULL)
    {
        LogError("Callback from input called but no input specific callbacks registered");
        result = false;
    }
    else
    {
        const char* inputName = IoTHubMessage_GetInputName(messageHandle);

        LIST_ITEM_HANDLE item_handle = NULL;

        item_handle = singlylinkedlist_find(clientHandleData->event_callbacks, is_event_equal_for_match, (const void*)inputName);

        if (item_handle == NULL)
        {
            item_handle = singlylinkedlist_find(clientHandleData->event_callbacks, is_event_equal_for_match, NULL);
        }

        if (item_handle == NULL)
        {
            LogError("Could not find callback (explicit or default) for input queue %s", MU_P_OR_NULL(inputName));
            result = false;
        }
        else
        {
            IOTHUB_EVENT_CALLBACK* event_callback = (IOTHUB_EVENT_CALLBACK*)singlylinkedlist_item_get_value(item_handle);
            if (NULL == event_callback)
            {
                LogError("singlylinkedlist_item_get_value for event_callback failed");
                result = false;
            }
            else
            {
                clientHandleData->lastMessageReceiveTime = get_time(NULL);

                if (event_callback->callbackAsyncEx != NULL)
                {
                    result = event_callback->callbackAsyncEx(messageHandle, event_callback->userContextCallbackEx);
                }
                else
                {
                    IOTHUBMESSAGE_DISPOSITION_RESULT cb_result = event_callback->callbackAsync(messageHandle, event_callback->userContextCallback);

                    if (clientHandleData->IoTHubTransport_SendMessageDisposition(clientHandleData->deviceHandle, messageHandle, cb_result) != IOTHUB_CLIENT_OK)
                    {
                        LogError("IoTHubTransport_SendMessageDisposition failed");
                    }
                    result = true;
                }
            }
        }
    }
    return result;
}

static bool IoTHubClientCore_LL_MessageCallback(IOTHUB_MESSAGE_HANDLE messageHandle, void* ctx)
{
    bool result;
    if ((ctx == NULL) || messageHandle == NULL)
    {
        LogError("invalid argument: ctx(%p), messageHandle(%p)", ctx, messageHandle);
        result = false;
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* clientHandleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)ctx;
        return invoke_message_callback(clientHandleData, messageHandle);
    }
    return result;
}

// IoTHubClientCore_LL_ParseMethodToCommand parses a legacy method into a parsed out command.  Commands are of the form:
// * <component_name>*<command_name> // if a component_name is specified and delitimed via COMPONENT_DELIMETER
// * <command_name> // If no COMPONENT_DELIMETER is specified.
// We guarantee component_name is null-terminated, when set, which means we need to make a copy of it
// that the caller must free.  command_name is a pointer into an existing buffer that is NOT freed.
IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_ParseMethodToCommand(const char* method_name, char** component_name, const char** command_name)
{
    IOTHUB_CLIENT_RESULT result;

    if ((method_name == NULL) || (component_name == NULL) || (command_name == NULL))
    {
        LogError("Invalid parameter for IoTHubClientCore_LL_ParseMethodToCommand");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        const char* componentSplit = strchr(method_name, COMPONENT_DELIMETER);

        if (componentSplit == NULL)
        {
            *component_name = NULL;
            *command_name = method_name;
            result = IOTHUB_CLIENT_OK;
        }
        else
        {
            size_t component_name_length = safe_subtract_size_t(componentSplit, method_name);
            size_t malloc_size = safe_add_size_t(component_name_length, 1);
            if (malloc_size == SIZE_MAX ||
                (*component_name = malloc(malloc_size)) == NULL)
            {
                LogError("Cannot allocate component name, size:%zu", malloc_size);
                result = IOTHUB_CLIENT_ERROR;
                *component_name = NULL;
            }
            else
            {
                memcpy(*component_name, method_name, component_name_length);
                (*component_name)[component_name_length] = 0;
                *command_name = componentSplit + 1;
                result = IOTHUB_CLIENT_OK;
            }
        }
    }

    return result;
}

static int invoke_command_callback(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData, const char* method_name, const unsigned char* payload, size_t size, METHOD_HANDLE response_id)
{
    int result; 

    const char* command_name = NULL;
    char* component_name = NULL;

    // Parse the raw method_name into its constituent (optional) component_name and command_name parts.
    if (IoTHubClientCore_LL_ParseMethodToCommand(method_name, &component_name, &command_name) != IOTHUB_CLIENT_OK)
    {
        LogError("Cannot parse command/component name");
        result = MU_FAILURE;
    }
    else
    {
        // Invoke the application's callback.
        IOTHUB_CLIENT_COMMAND_REQUEST commandRequest;
        IOTHUB_CLIENT_COMMAND_RESPONSE commandResponse;

        commandRequest.structVersion = IOTHUB_CLIENT_COMMAND_REQUEST_STRUCT_VERSION_1;
        commandRequest.componentName = component_name;
        commandRequest.commandName = command_name;
        commandRequest.payload = payload;
        commandRequest.payloadLength = size;

        memset(&commandResponse, 0, sizeof(commandResponse));
        commandResponse.structVersion = IOTHUB_CLIENT_COMMAND_RESPONSE_STRUCT_VERSION_1;
        // Set statusCode of response to a default value so that if application has a bug and doesn't set it, we still return
        // something meaningful to IoT Hub.
        commandResponse.statusCode = DEFAULT_COMMAND_RESPONSE_STATUS_CODE;
        
        handleData->methodCallback.commandCallback(&commandRequest, &commandResponse, handleData->methodCallback.userContextCallback);
        if ((commandResponse.payload != NULL) && (commandResponse.payloadLength > 0))
        {
            result = handleData->IoTHubTransport_DeviceMethod_Response(handleData->deviceHandle, response_id, commandResponse.payload, commandResponse.payloadLength, commandResponse.statusCode);
        }
        else
        {
            result = MU_FAILURE;
        }

        free(commandResponse.payload);
    }

    free(component_name);
    return result;
}

static int IoTHubClientCore_LL_DeviceMethodComplete(const char* method_name, const unsigned char* payLoad, size_t size, METHOD_HANDLE response_id, void* ctx)
{
    int result;
    if (ctx == NULL)
    {
        LogError("Invalid argument ctx=%p", ctx);
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)ctx;
        switch (handleData->methodCallback.type)
        {
            case CALLBACK_TYPE_METHOD:
            {
                unsigned char* payload_resp = NULL;
                size_t response_size = 0;
                result = handleData->methodCallback.deviceMethodCallback(method_name, payLoad, size, &payload_resp, &response_size, handleData->methodCallback.userContextCallback);

                if (payload_resp != NULL && response_size > 0)
                {
                    result = handleData->IoTHubTransport_DeviceMethod_Response(handleData->deviceHandle, response_id, payload_resp, response_size, result);
                }
                else
                {
                    result = MU_FAILURE;
                }
                if (payload_resp != NULL)
                {
                    free(payload_resp);
                }
                break;
            }
            case CALLBACK_TYPE_COMMAND:
                result = invoke_command_callback(handleData, method_name, payLoad, size, response_id);
                break;
            case CALLBACK_TYPE_INBOUND_METHOD:
                result = handleData->methodCallback.inboundDeviceMethodCallback(method_name, payLoad, size, response_id, handleData->methodCallback.userContextCallback);
                break;
            default:
                result = 0;
                break;
        }
    }
    return result;
}

static IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* initialize_iothub_client(const IOTHUB_CLIENT_CONFIG* client_config, const IOTHUB_CLIENT_DEVICE_CONFIG* device_config, bool use_dev_auth, const char* module_id)
{
    IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* result;
    srand((unsigned int)get_time(NULL));

    result = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)malloc(sizeof(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA));
    if (result == NULL)
    {
        LogError("failure allocating IOTHUB_CLIENT_CORE_LL_HANDLE_DATA");
    }
    else
    {
        IOTHUB_CLIENT_CONFIG actual_config;
        const IOTHUB_CLIENT_CONFIG* config = NULL;
        char* IoTHubName = NULL;
        char* IoTHubSuffix = NULL;

        memset(result, 0, sizeof(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA));
        if (use_dev_auth)
        {
            if ((result->authorization_module = IoTHubClient_Auth_CreateFromDeviceAuth(client_config->deviceId, module_id)) == NULL)
            {
                LogError("Failed create authorization module");
                free(result);
                result = NULL;
            }
        }
        else
        {
            const char* device_key;
            const char* device_id;
            const char* sas_token;
            if (device_config == NULL)
            {
                device_key = client_config->deviceKey;
                device_id = client_config->deviceId;
                sas_token = client_config->deviceSasToken;
            }
            else
            {
                device_key = device_config->deviceKey;
                device_id = device_config->deviceId;
                sas_token = device_config->deviceSasToken;
            }

            if ((result->authorization_module = IoTHubClient_Auth_Create(device_key, device_id, sas_token, module_id)) == NULL)
            {
                LogError("Failed create authorization module");
                free(result);
                result = NULL;
            }
        }

        if (result != NULL)
        {
            TRANSPORT_CALLBACKS_INFO transport_cb;
            memset(&transport_cb, 0, sizeof(TRANSPORT_CALLBACKS_INFO));
            transport_cb.send_complete_cb = IoTHubClientCore_LL_SendComplete;
            transport_cb.twin_retrieve_prop_complete_cb = IoTHubClientCore_LL_RetrievePropertyComplete;
            transport_cb.twin_rpt_state_complete_cb = IoTHubClientCore_LL_ReportedStateComplete;
            transport_cb.connection_status_cb = IoTHubClientCore_LL_ConnectionStatusCallBack;
            transport_cb.prod_info_cb = IoTHubClientCore_LL_GetProductInfo;
            transport_cb.msg_input_cb = IoTHubClientCore_LL_MessageCallbackFromInput;
            transport_cb.msg_cb = IoTHubClientCore_LL_MessageCallback;
            transport_cb.method_complete_cb = IoTHubClientCore_LL_DeviceMethodComplete;
            transport_cb.get_model_id_cb = IoTHubClientCore_LL_GetModelId;

            if (client_config != NULL)
            {
                IOTHUBTRANSPORT_CONFIG lowerLayerConfig;
                memset(&lowerLayerConfig, 0, sizeof(IOTHUBTRANSPORT_CONFIG));
                lowerLayerConfig.upperConfig = client_config;
                lowerLayerConfig.waitingToSend = &(result->waitingToSend);
                lowerLayerConfig.auth_module_handle = result->authorization_module;
                lowerLayerConfig.moduleId = module_id;

                setTransportProtocol(result, (TRANSPORT_PROVIDER*)client_config->protocol());
                if ((result->transportHandle = result->IoTHubTransport_Create(&lowerLayerConfig, &transport_cb, result)) == NULL)
                {
                    LogError("underlying transport failed");
                    destroy_blob_upload_module(result);
                    destroy_module_method_module(result);
                    tickcounter_destroy(result->tickCounter);
                    IoTHubClient_Auth_Destroy(result->authorization_module);
                    free(result);
                    result = NULL;
                }
                else
                {
                    result->isSharedTransport = false;
                    config = client_config;
                }
            }
            else if (device_config != NULL)
            {
                STRING_HANDLE transport_hostname = NULL;

                result->transportHandle = device_config->transportHandle;
                setTransportProtocol(result, (TRANSPORT_PROVIDER*)device_config->protocol());

                if (result->IoTHubTransport_SetCallbackContext(result->transportHandle, result) != 0)
                {
                    LogError("unable to set transport callbacks");
                    IoTHubClient_Auth_Destroy(result->authorization_module);
                    free(result);
                    result = NULL;
                }
                else if ((transport_hostname = result->IoTHubTransport_GetHostname(result->transportHandle)) == NULL)
                {
                    LogError("unable to determine the transport IoTHub name");
                    IoTHubClient_Auth_Destroy(result->authorization_module);
                    free(result);
                    result = NULL;
                }
                else
                {
                    const char* hostname = STRING_c_str(transport_hostname);
                    /*the first '.' says where the iothubname finishes*/
                    const char* whereIsDot = strchr(hostname, '.');
                    if (whereIsDot == NULL)
                    {
                        LogError("unable to determine the IoTHub name");
                        IoTHubClient_Auth_Destroy(result->authorization_module);
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        size_t suffix_len = strlen(whereIsDot);
                        size_t calloc_size_hub_name = safe_add_size_t(safe_subtract_size_t(whereIsDot, hostname), 1);
                        size_t malloc_size_hub_suffix = safe_add_size_t(suffix_len, 1);
                        if (calloc_size_hub_name == SIZE_MAX ||
                            (IoTHubName = (char*)calloc(1, calloc_size_hub_name)) == NULL)
                        {
                            LogError("unable to calloc, size:%zu", calloc_size_hub_name);
                            IoTHubClient_Auth_Destroy(result->authorization_module);
                            free(result);
                            result = NULL;
                        }
                        else if (malloc_size_hub_suffix == SIZE_MAX ||
                            (IoTHubSuffix = (char*)malloc(malloc_size_hub_suffix)) == NULL)
                        {
                            LogError("unable to malloc");
                            IoTHubClient_Auth_Destroy(result->authorization_module);
                            free(IoTHubName);
                            free(result);
                            result = NULL;
                        }
                        else
                        {
                            (void)memcpy(IoTHubName, hostname, whereIsDot - hostname);
                            (void)strcpy(IoTHubSuffix, whereIsDot+1);

                            actual_config.deviceId = device_config->deviceId;
                            actual_config.deviceKey = device_config->deviceKey;
                            actual_config.deviceSasToken = device_config->deviceSasToken;
                            actual_config.iotHubName = IoTHubName;
                            actual_config.iotHubSuffix = IoTHubSuffix;
                            actual_config.protocol = NULL; /*irrelevant to IoTHubClientCore_LL_UploadToBlob*/
                            actual_config.protocolGatewayHostName = NULL; /*irrelevant to IoTHubClientCore_LL_UploadToBlob*/

                            config = &actual_config;

                            result->isSharedTransport = true;
                        }
                    }
                }
                STRING_delete(transport_hostname);
            }
        }
        if (result != NULL)
        {
            if (create_blob_upload_module(result, config, use_dev_auth) != 0)
            {
                LogError("unable to create blob upload");
                if (!result->isSharedTransport)
                {
                    result->IoTHubTransport_Destroy(result->transportHandle);
                }
                destroy_blob_upload_module(result);
                IoTHubClient_Auth_Destroy(result->authorization_module);
                free(result);
                result = NULL;
            }
            else if ((module_id != NULL) && create_edge_handle(result, config, module_id) != 0)
            {
                LogError("unable to create module method handle");
                if (!result->isSharedTransport)
                {
                    result->IoTHubTransport_Destroy(result->transportHandle);
                }
                destroy_blob_upload_module(result);
                IoTHubClient_Auth_Destroy(result->authorization_module);
                free(result);
                result = NULL;
            }
            else
            {
                PLATFORM_INFO_OPTION supportedPlatformInfo;
                if ((result->tickCounter = tickcounter_create()) == NULL)
                {
                    LogError("unable to get a tickcounter");
                    if (!result->isSharedTransport)
                    {
                        result->IoTHubTransport_Destroy(result->transportHandle);
                    }
                    destroy_blob_upload_module(result);
                    destroy_module_method_module(result);
                    IoTHubClient_Auth_Destroy(result->authorization_module);
                    free(result);
                    result = NULL;
                }
                // Add extended info to product info if required
                else if (result->IoTHubTransport_GetSupportedPlatformInfo(result->transportHandle, &supportedPlatformInfo) != 0)
                {
                    LogError("failed to get supported platform info");
                    if (!result->isSharedTransport)
                    {
                        result->IoTHubTransport_Destroy(result->transportHandle);
                    }
                    tickcounter_destroy(result->tickCounter);
                    destroy_blob_upload_module(result);
                    destroy_module_method_module(result);
                    IoTHubClient_Auth_Destroy(result->authorization_module);
                    free(result);
                    result = NULL;
                }
                else if ((result->product_info = make_product_info(NULL, supportedPlatformInfo)) == NULL)
                {
                    LogError("failed to initialize product info");
                    if (!result->isSharedTransport)
                    {
                        result->IoTHubTransport_Destroy(result->transportHandle);
                    }
                    tickcounter_destroy(result->tickCounter);
                    destroy_blob_upload_module(result);
                    destroy_module_method_module(result);
                    IoTHubClient_Auth_Destroy(result->authorization_module);
                    free(result);
                    result = NULL;
                }
                else if (config != NULL)
                {
                    DList_InitializeListHead(&(result->waitingToSend));
                    DList_InitializeListHead(&(result->iot_msg_queue));
                    DList_InitializeListHead(&(result->iot_ack_queue));
                    result->messageCallback.type = CALLBACK_TYPE_NONE;
                    result->methodCallback.type = CALLBACK_TYPE_NONE;
                    result->lastMessageReceiveTime = INDEFINITE_TIME;
                    result->data_msg_id = 1;

                    IOTHUB_DEVICE_CONFIG deviceConfig;
                    deviceConfig.deviceId = config->deviceId;
                    deviceConfig.deviceKey = config->deviceKey;
                    deviceConfig.deviceSasToken = config->deviceSasToken;
                    deviceConfig.authorization_module = result->authorization_module;
                    deviceConfig.moduleId = module_id;

                    if ((result->deviceHandle = result->IoTHubTransport_Register(result->transportHandle, &deviceConfig, &(result->waitingToSend))) == NULL)
                    {
                        LogError("Registering device in transport failed");
                        IoTHubClient_Auth_Destroy(result->authorization_module);
                        if (!result->isSharedTransport)
                        {
                            result->IoTHubTransport_Destroy(result->transportHandle);
                        }
                        destroy_blob_upload_module(result);
                        destroy_module_method_module(result);
                        tickcounter_destroy(result->tickCounter);
                        STRING_delete(result->product_info);
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        result->currentMessageTimeout = 0;
                        result->current_device_twin_timeout = 0;

                        result->diagnostic_setting.currentMessageNumber = 0;
                        result->diagnostic_setting.diagSamplingPercentage = 0;
                        if (IoTHubClientCore_LL_SetRetryPolicy(result, IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, 0) != IOTHUB_CLIENT_OK)
                        {
                            LogError("Setting default retry policy in transport failed");
                            result->IoTHubTransport_Unregister(result->deviceHandle);
                            IoTHubClient_Auth_Destroy(result->authorization_module);
                            if (!result->isSharedTransport)
                            {
                                result->IoTHubTransport_Destroy(result->transportHandle);
                            }
                            destroy_blob_upload_module(result);
                            destroy_module_method_module(result);
                            tickcounter_destroy(result->tickCounter);
                            STRING_delete(result->product_info);
                            free(result);
                            result = NULL;
                        }
                    }
                }
            }
        }
        if (IoTHubName)
        {
            free(IoTHubName);
        }
        if (IoTHubSuffix)
        {
            free(IoTHubSuffix);
        }
    }
    return result;
}

static uint32_t get_next_item_id(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData)
{
    if (handleData->data_msg_id+1 >= UINT32_MAX)
    {
        handleData->data_msg_id = 1;
    }
    else
    {
        handleData->data_msg_id++;
    }
    return handleData->data_msg_id;
}

static IOTHUB_DEVICE_TWIN* dev_twin_data_create(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData, uint32_t id, const unsigned char* reportedState, size_t size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback)
{
    IOTHUB_DEVICE_TWIN* result = (IOTHUB_DEVICE_TWIN*)malloc(sizeof(IOTHUB_DEVICE_TWIN) );
    if (result != NULL)
    {
        result->report_data_handle = CONSTBUFFER_Create(reportedState, size);
        if (result->report_data_handle == NULL)
        {
            LogError("Failure allocating reported state data");
            free(result);
            result = NULL;
        }
        else
        {
            result->item_id = id;
            result->ms_timesOutAfter = 0;
            result->context = userContextCallback;
            result->reported_state_callback = reportedStateCallback;
            result->client_handle = handleData;
            result->device_handle = handleData->deviceHandle;
        }
    }
    else
    {
        LogError("Failure allocating device twin information");
    }
    return result;
}

static void on_get_device_twin_completed(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback)
{
    if (userContextCallback == NULL)
    {
        LogError("Invalid argument (userContextCallback=NULL)");
    }
    else
    {
        GET_TWIN_CONTEXT* getTwinCtx = (GET_TWIN_CONTEXT*)userContextCallback;
        getTwinCtx->callback(update_state, payLoad, size, getTwinCtx->context);
        free(getTwinCtx);
    }
}

static void delete_event(IOTHUB_EVENT_CALLBACK* event_callback)
{
    STRING_delete(event_callback->inputName);
    free(event_callback->userContextCallbackEx);
    free(event_callback);
}

static void delete_event_callback(const void* item, const void* action_context, bool* continue_processing)
{
    (void)action_context;
    delete_event((IOTHUB_EVENT_CALLBACK*)item);
    *continue_processing = true;
}

static void delete_event_callback_list(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData)
{
    if (handleData->event_callbacks != NULL)
    {
        singlylinkedlist_foreach(handleData->event_callbacks, delete_event_callback, NULL);
        singlylinkedlist_destroy(handleData->event_callbacks);
        handleData->event_callbacks = NULL;
    }
}


IOTHUB_CLIENT_CORE_LL_HANDLE IoTHubClientCore_LL_CreateFromDeviceAuth(const char* iothub_uri, const char* device_id, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    IOTHUB_CLIENT_CORE_LL_HANDLE result;
    if (iothub_uri == NULL || protocol == NULL || device_id == NULL)
    {
        LogError("Input parameter is NULL: iothub_uri: %p  protocol: %p device_id: %p", iothub_uri, protocol, device_id);
        result = NULL;
    }
    else
    {
#ifdef USE_PROV_MODULE
        IOTHUB_CLIENT_CONFIG* config = (IOTHUB_CLIENT_CONFIG*)malloc(sizeof(IOTHUB_CLIENT_CONFIG));
        if (config == NULL)
        {
            LogError("Malloc failed");
            result = NULL;
        }
        else
        {
            const char* iterator;
            const char* initial;
            char* iothub_name = NULL;
            char* iothub_suffix = NULL;

            memset(config, 0, sizeof(IOTHUB_CLIENT_CONFIG));
            config->protocol = protocol;
            config->deviceId = device_id;

            // Find the iothub suffix
            initial = iterator = iothub_uri;
            while (iterator != NULL && *iterator != '\0')
            {
                if (*iterator == '.')
                {
                    size_t length = safe_subtract_size_t(iterator, initial);
                    size_t calloc_size = safe_add_size_t(length, 1);
                    if (calloc_size != SIZE_MAX &&
                        (iothub_name = (char*)calloc(1, calloc_size)) != NULL)
                    {
                        memcpy(iothub_name, initial, length);
                        config->iotHubName = iothub_name;

                        length = safe_subtract_size_t(safe_subtract_size_t(strlen(initial), length), 1);
                        calloc_size = safe_add_size_t(length, 1);
                        if (calloc_size != SIZE_MAX &&
                            (iothub_suffix = (char*)calloc(1, calloc_size)) != NULL)
                        {
                            memcpy(iothub_suffix, iterator + 1, length);
                            config->iotHubSuffix = iothub_suffix;
                            break;
                        }
                        else
                        {
                            LogError("Failed to allocate iothub suffix, size:%zu", calloc_size);
                            free(iothub_name);
                            iothub_name = NULL;
                            result = NULL;
                        }
                    }
                    else
                    {
                        LogError("Failed to allocate iothub name, size:%zu", calloc_size);
                        result = NULL;
                    }
                }
                iterator++;
            }

            if (config->iotHubName == NULL || config->iotHubSuffix == NULL)
            {
                LogError("initialize iothub client");
                result = NULL;
            }
            else
            {
                IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = initialize_iothub_client(config, NULL, true, NULL);
                if (handleData == NULL)
                {
                    LogError("initialize iothub client");
                    result = NULL;
                }
                else
                {
                    result = handleData;
                }
            }

            free(iothub_name);
            free(iothub_suffix);
            free(config);
        }
#else
        LogError("HSM module is not included");
        result = NULL;
#endif
    }
    return result;
}

IOTHUB_CLIENT_CORE_LL_HANDLE IoTHubClientCore_LL_CreateFromConnectionString(const char* connectionString, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    IOTHUB_CLIENT_CORE_LL_HANDLE result;

    if (connectionString == NULL)
    {
        LogError("Input parameter is NULL: connectionString");
        result = NULL;
    }
    else if (protocol == NULL)
    {
        LogError("Input parameter is NULL: protocol");
        result = NULL;
    }
    else
    {
        IOTHUB_CLIENT_CONFIG* config = (IOTHUB_CLIENT_CONFIG*) malloc(sizeof(IOTHUB_CLIENT_CONFIG));
        if (config == NULL)
        {
            LogError("Malloc failed");
            result = NULL;
        }
        else
        {
            STRING_TOKENIZER_HANDLE tokenizer1 = NULL;
            STRING_HANDLE connString = NULL;
            STRING_HANDLE tokenString = NULL;
            STRING_HANDLE valueString = NULL;
            STRING_HANDLE hostNameString = NULL;
            STRING_HANDLE hostSuffixString = NULL;
            STRING_HANDLE deviceIdString = NULL;
            STRING_HANDLE deviceKeyString = NULL;
            STRING_HANDLE deviceSasTokenString = NULL;
            STRING_HANDLE protocolGateway = NULL;
            STRING_HANDLE moduleId = NULL;

            memset(config, 0, sizeof(*config));
            config->protocol = protocol;

            config->protocolGatewayHostName = NULL;

            if ((connString = STRING_construct(connectionString)) == NULL)
            {
                LogError("Error constructing connection String");
                result = NULL;
            }
            else if ((tokenizer1 = STRING_TOKENIZER_create(connString)) == NULL)
            {
                LogError("Error creating Tokenizer");
                result = NULL;
            }
            else if ((tokenString = STRING_new()) == NULL)
            {
                LogError("Error creating Token String");
                result = NULL;
            }
            else if ((valueString = STRING_new()) == NULL)
            {
                LogError("Error creating Value String");
                result = NULL;
            }
            else if ((hostNameString = STRING_new()) == NULL)
            {
                LogError("Error creating HostName String");
                result = NULL;
            }
            else if ((hostSuffixString = STRING_new()) == NULL)
            {
                LogError("Error creating HostSuffix String");
                result = NULL;
            }
            else
            {
                int isx509found = 0;
                bool use_provisioning = false;
                while ((STRING_TOKENIZER_get_next_token(tokenizer1, tokenString, "=") == 0))
                {
                    if (STRING_TOKENIZER_get_next_token(tokenizer1, valueString, ";") != 0)
                    {
                        LogError("Tokenizer error");
                        break;
                    }
                    else
                    {
                        if (tokenString != NULL)
                        {
                            const char* s_token = STRING_c_str(tokenString);
                            if (strcmp(s_token, HOSTNAME_TOKEN) == 0)
                            {
                                STRING_TOKENIZER_HANDLE tokenizer2 = NULL;
                                if ((tokenizer2 = STRING_TOKENIZER_create(valueString)) == NULL)
                                {
                                    LogError("Error creating Tokenizer");
                                    break;
                                }
                                else
                                {
                                    if (STRING_TOKENIZER_get_next_token(tokenizer2, hostNameString, ".") != 0)
                                    {
                                        LogError("Tokenizer error");
                                        STRING_TOKENIZER_destroy(tokenizer2);
                                        break;
                                    }
                                    else
                                    {
                                        config->iotHubName = STRING_c_str(hostNameString);
                                        if (STRING_TOKENIZER_get_next_token(tokenizer2, hostSuffixString, ";") != 0)
                                        {
                                            LogError("Tokenizer error");
                                            STRING_TOKENIZER_destroy(tokenizer2);
                                            break;
                                        }
                                        else
                                        {
                                            config->iotHubSuffix = STRING_c_str(hostSuffixString);
                                        }
                                    }
                                    STRING_TOKENIZER_destroy(tokenizer2);
                                }
                            }
                            else if (strcmp(s_token, DEVICEID_TOKEN) == 0)
                            {
                                deviceIdString = STRING_clone(valueString);
                                if (deviceIdString != NULL)
                                {
                                    config->deviceId = STRING_c_str(deviceIdString);
                                }
                                else
                                {
                                    LogError("Failure cloning device id string");
                                    break;
                                }
                            }
                            else if (strcmp(s_token, DEVICEKEY_TOKEN) == 0)
                            {
                                deviceKeyString = STRING_clone(valueString);
                                if (deviceKeyString != NULL)
                                {
                                    config->deviceKey = STRING_c_str(deviceKeyString);
                                }
                                else
                                {
                                    LogError("Failure cloning device key string");
                                    break;
                                }
                            }
                            else if (strcmp(s_token, DEVICESAS_TOKEN) == 0)
                            {
                                deviceSasTokenString = STRING_clone(valueString);
                                if (deviceSasTokenString != NULL)
                                {
                                    config->deviceSasToken = STRING_c_str(deviceSasTokenString);
                                }
                                else
                                {
                                    LogError("Failure cloning device sasToken string");
                                    break;
                                }
                            }
                            else if (strcmp(s_token, X509_TOKEN) == 0)
                            {
                                if (strcmp(STRING_c_str(valueString), X509_TOKEN_ONLY_ACCEPTABLE_VALUE) != 0)
                                {
                                    LogError("x509 option has wrong value, the only acceptable one is \"true\"");
                                    break;
                                }
                                else
                                {
                                    isx509found = 1;
                                }
                            }
                            else if (strcmp(s_token, PROVISIONING_TOKEN) == 0)
                            {
                                if (strcmp(STRING_c_str(valueString), PROVISIONING_ACCEPTABLE_VALUE) != 0)
                                {
                                    LogError("provisioning option has wrong value, the only acceptable one is \"true\"");
                                    break;
                                }
                                else
                                {
                                    use_provisioning = 1;
                                }
                            }

                            else if (strcmp(s_token, PROTOCOL_GATEWAY_HOST_TOKEN) == 0)
                            {
                                protocolGateway = STRING_clone(valueString);
                                if (protocolGateway != NULL)
                                {
                                    config->protocolGatewayHostName = STRING_c_str(protocolGateway);
                                }
                                else
                                {
                                    LogError("Failure cloning protocol Gateway Name");
                                    break;
                                }
                            }
                            else if (strcmp(s_token, MODULE_ID_TOKEN) == 0)
                            {
                                moduleId = STRING_clone(valueString);
                                if (moduleId == NULL)
                                {
                                    LogError("Failure cloning moduleId string");
                                    break;
                                }
                            }
                            else
                            {
                                // If we get an unknown token, log it to error stream but do not cause a fatal error.
                                LogError("Unknown token <%s> in connection string.  Ignoring error and continuing to parse", s_token);
                            }
                        }
                    }
                }
                /* parsing is done - check the result */
                if (config->iotHubName == NULL)
                {
                    LogError("iotHubName is not found");
                    result = NULL;
                }
                else if (config->iotHubSuffix == NULL)
                {
                    LogError("iotHubSuffix is not found");
                    result = NULL;
                }
                else if (config->deviceId == NULL)
                {
                    LogError("deviceId is not found");
                    result = NULL;
                }
                else if (!(
                    ((!use_provisioning && !isx509found) && (config->deviceSasToken == NULL) ^ (config->deviceKey == NULL)) ||
                    ((use_provisioning || isx509found) && (config->deviceSasToken == NULL) && (config->deviceKey == NULL))
                    ))
                {
                    LogError("invalid combination of x509, provisioning, deviceSasToken and deviceKey");
                    result = NULL;
                }
                else
                {
                    result = initialize_iothub_client(config, NULL, use_provisioning, STRING_c_str(moduleId));
                    if (result == NULL)
                    {
                        LogError("IoTHubClientCore_LL_Create failed");
                    }
                    else
                    {
                        /*return as is*/
                    }
                }
            }
            if (deviceSasTokenString != NULL)
                STRING_delete(deviceSasTokenString);
            if (deviceKeyString != NULL)
                STRING_delete(deviceKeyString);
            if (deviceIdString != NULL)
                STRING_delete(deviceIdString);
            if (hostSuffixString != NULL)
                STRING_delete(hostSuffixString);
            if (hostNameString != NULL)
                STRING_delete(hostNameString);
            if (valueString != NULL)
                STRING_delete(valueString);
            if (tokenString != NULL)
                STRING_delete(tokenString);
            if (connString != NULL)
                STRING_delete(connString);
            if (protocolGateway != NULL)
                STRING_delete(protocolGateway);
            if (moduleId != NULL)
                STRING_delete(moduleId);

            if (tokenizer1 != NULL)
                STRING_TOKENIZER_destroy(tokenizer1);

            free(config);
        }
    }
    return result;
}

IOTHUB_CLIENT_CORE_LL_HANDLE IoTHubClientCore_LL_CreateImpl(const IOTHUB_CLIENT_CONFIG* config, const char* module_id, bool use_dev_auth)
{
    IOTHUB_CLIENT_CORE_LL_HANDLE result;
    if(
        (config == NULL) ||
        (config->protocol == NULL)
        )
    {
        result = NULL;
        LogError("invalid configuration (NULL detected)");
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = initialize_iothub_client(config, NULL, use_dev_auth, module_id);
        if (handleData == NULL)
        {
            LogError("initialize iothub client");
            result = NULL;
        }
        else
        {
            result = handleData;
        }
    }

    return result;
}

IOTHUB_CLIENT_CORE_LL_HANDLE IoTHubClientCore_LL_Create(const IOTHUB_CLIENT_CONFIG* config)
{
    return IoTHubClientCore_LL_CreateImpl(config, NULL, false);
}


IOTHUB_CLIENT_CORE_LL_HANDLE IoTHubClientCore_LL_CreateFromEnvironment(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* result;
    EDGE_ENVIRONMENT_VARIABLES edge_environment_variables;

    memset(&edge_environment_variables, 0, sizeof(edge_environment_variables));

    if (retrieve_edge_environment_variabes(&edge_environment_variables) != 0)
    {
        LogError("retrieve_edge_environment_variabes failed");
        result = NULL;
    }
    // The presence of a connection string environment variable means we use it, ignoring other settings
    else if (edge_environment_variables.connection_string != NULL)
    {
        if ((result = IoTHubClientCore_LL_CreateFromConnectionString(edge_environment_variables.connection_string, protocol)) == NULL)
        {
            LogError("IoTHubClientCore_LL_CreateFromConnectionString fails");
        }
    }
    else if (iothub_security_init(IOTHUB_SECURITY_TYPE_HTTP_EDGE) != 0)
    {
        LogError("iothub_security_init failed");
        result = NULL;
    }
    else
    {
        IOTHUB_CLIENT_CONFIG client_config;

        memset(&client_config, 0, sizeof(client_config));
        client_config.protocol = protocol;
        client_config.deviceId = edge_environment_variables.device_id;
        client_config.iotHubName = edge_environment_variables.iothub_name;
        client_config.iotHubSuffix = edge_environment_variables.iothub_suffix;
        client_config.protocolGatewayHostName = edge_environment_variables.gatewayhostname;

        if ((result = IoTHubClientCore_LL_CreateImpl(&client_config, edge_environment_variables.module_id, true)) == NULL)
        {
            LogError("IoTHubClientCore_LL_CreateImpl fails");
        }
    }

    if (result != NULL)
    {
        // Because the Edge Hub almost always use self-signed certificates, we need to specify which certificates to trust.  We need to do
        // this regardless of how we created the underlying IOTHUB_CLIENT_CORE_LL_HANDLE_DATA.
        IOTHUB_CLIENT_RESULT setTrustResult;
        char* trustedCertificate = IoTHubClient_Auth_Get_TrustBundle(result->authorization_module, edge_environment_variables.ca_trusted_certificate_file);

        if (trustedCertificate == NULL)
        {
            LogError("IoTHubClient_Auth_Get_TrustBundle failed");
            IoTHubClientCore_LL_Destroy(result);
            result = NULL;
        }
        else if ((setTrustResult = IoTHubClientCore_LL_SetOption(result, OPTION_TRUSTED_CERT, trustedCertificate)) != IOTHUB_CLIENT_OK)
        {
            LogError("IoTHubClientCore_LL_SetOption failed, err = %d", setTrustResult);
            IoTHubClientCore_LL_Destroy(result);
            result = NULL;
        }

        free(trustedCertificate);
    }

    free(edge_environment_variables.iothub_buffer);
    return result;
}


IOTHUB_CLIENT_CORE_LL_HANDLE IoTHubClientCore_LL_CreateWithTransport(const IOTHUB_CLIENT_DEVICE_CONFIG * config)
{
    IOTHUB_CLIENT_CORE_LL_HANDLE result;
    if (
        (config == NULL) ||
        (config->protocol == NULL) ||
        (config->transportHandle == NULL) ||
        ((config->deviceKey == NULL) && (config->deviceSasToken == NULL))
        )
    {
        result = NULL;
        LogError("invalid configuration (NULL detected)");
    }
    else
    {
        result = initialize_iothub_client(NULL, config, false, NULL);
    }
    return result;
}

void IoTHubClientCore_LL_Destroy(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle)
{
    if (iotHubClientHandle != NULL)
    {
        PDLIST_ENTRY unsend;
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;
        handleData->IoTHubTransport_Unregister(handleData->deviceHandle);
        if (handleData->isSharedTransport == false)
        {
            handleData->IoTHubTransport_Destroy(handleData->transportHandle);
        }
        /*if any, remove the items currently not send*/
        while ((unsend = DList_RemoveHeadList(&(handleData->waitingToSend))) != &(handleData->waitingToSend))
        {
            IOTHUB_MESSAGE_LIST* temp = containingRecord(unsend, IOTHUB_MESSAGE_LIST, entry);
            if (temp->callback != NULL)
            {
                temp->callback(IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY, temp->context);
            }
            IoTHubMessage_Destroy(temp->messageHandle);
            free(temp);
        }

        while ((unsend = DList_RemoveHeadList(&(handleData->iot_msg_queue))) != &(handleData->iot_msg_queue))
        {
            IOTHUB_DEVICE_TWIN* temp = containingRecord(unsend, IOTHUB_DEVICE_TWIN, entry);

            // The Twin reported properties status codes are based on HTTP codes and provided by the service.
            // Following design already implemented in the transport layer, the status code shall be artificially 
            // returned as zero to indicate the report was not sent due to the client being destroyed.
            if (temp->reported_state_callback != NULL)
            {
                temp->reported_state_callback(ERROR_CODE_BECAUSE_DESTROY, temp->context);
            }  

            device_twin_data_destroy(temp);
        }
        while ((unsend = DList_RemoveHeadList(&(handleData->iot_ack_queue))) != &(handleData->iot_ack_queue))
        {
            IOTHUB_DEVICE_TWIN* temp = containingRecord(unsend, IOTHUB_DEVICE_TWIN, entry);

            // The Twin reported properties status codes are based on HTTP codes and provided by the service.
            // Following design already implemented in the transport layer, the status code shall be artificially 
            // returned as zero to indicate the report was not sent due to the client being destroyed.
            if (temp->reported_state_callback != NULL)
            {
                temp->reported_state_callback(ERROR_CODE_BECAUSE_DESTROY, temp->context);
            }

            device_twin_data_destroy(temp);
        }

        delete_event_callback_list(handleData);

        IoTHubClient_Auth_Destroy(handleData->authorization_module);
        tickcounter_destroy(handleData->tickCounter);
#ifndef DONT_USE_UPLOADTOBLOB
        IoTHubClient_LL_UploadToBlob_Destroy(handleData->uploadToBlobHandle);
#endif

        IoTHubClient_EdgeHandle_Destroy(handleData->methodHandle);

        STRING_delete(handleData->product_info);
        STRING_delete(handleData->model_id);
        free(handleData);
    }
}

/*returns 0 on success, any other value is error*/
static int attach_ms_timesOutAfter(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData, IOTHUB_MESSAGE_LIST *newEntry)
{
    int result;
    if (handleData->currentMessageTimeout == 0)
    {
        newEntry->ms_timesOutAfter = 0; /*do not timeout*/
        newEntry->message_timeout_value = 0;
        result = 0;
    }
    else
    {
        if (tickcounter_get_current_ms(handleData->tickCounter, &newEntry->ms_timesOutAfter) != 0)
        {
            result = MU_FAILURE;
            LogError("unable to get the current relative tickcount");
        }
        else
        {
            newEntry->message_timeout_value = handleData->currentMessageTimeout;
            result = 0;
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_SendEventAsync(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (
        (iotHubClientHandle == NULL) ||
        (eventMessageHandle == NULL) ||
        ((eventConfirmationCallback == NULL) && (userContextCallback != NULL))
        )
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LOG_ERROR_RESULT;
    }
    else
    {
        IOTHUB_MESSAGE_LIST *newEntry = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST));
        if (newEntry == NULL)
        {
            result = IOTHUB_CLIENT_ERROR;
            LOG_ERROR_RESULT;
        }
        else
        {
            IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;

            if (attach_ms_timesOutAfter(handleData, newEntry) != 0)
            {
                result = IOTHUB_CLIENT_ERROR;
                LOG_ERROR_RESULT;
                free(newEntry);
            }
            else
            {
                if ((newEntry->messageHandle = IoTHubMessage_Clone(eventMessageHandle)) == NULL)
                {
                    result = IOTHUB_CLIENT_ERROR;
                    free(newEntry);
                    LOG_ERROR_RESULT;
                }
                else if (IoTHubClient_Diagnostic_AddIfNecessary(&handleData->diagnostic_setting, newEntry->messageHandle) != 0)
                {
                    result = IOTHUB_CLIENT_ERROR;
                    IoTHubMessage_Destroy(newEntry->messageHandle);
                    free(newEntry);
                    LOG_ERROR_RESULT;
                }
                else
                {
                    newEntry->callback = eventConfirmationCallback;
                    newEntry->context = userContextCallback;
                    DList_InsertTailList(&(iotHubClientHandle->waitingToSend), &(newEntry->entry));
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_SetMessageCallback(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC messageCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubClientHandle == NULL)
    {
        LogError("Invalid argument - iotHubClientHandle is NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;
        if (messageCallback == NULL)
        {
            if (handleData->messageCallback.type == CALLBACK_TYPE_NONE)
            {
                LogError("not currently set to accept or process incoming messages.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else if (handleData->messageCallback.type == CALLBACK_TYPE_MESSAGE_ASYNC)
            {
                LogError("Invalid workflow sequence. Please unsubscribe using the IoTHubClientCore_LL_SetMessageCallback_Ex function.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                handleData->IoTHubTransport_Unsubscribe(handleData->deviceHandle);
                handleData->messageCallback.type = CALLBACK_TYPE_NONE;
                handleData->messageCallback.callbackSync = NULL;
                handleData->messageCallback.callbackAsync = NULL;
                handleData->messageCallback.userContextCallback = NULL;
                result = IOTHUB_CLIENT_OK;
            }
        }
        else
        {
            if (handleData->messageCallback.type == CALLBACK_TYPE_MESSAGE_ASYNC)
            {
                LogError("Invalid workflow sequence. Please unsubscribe using the IoTHubClientCore_LL_SetMessageCallback_Ex function before subscribing with MessageCallback.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                if (handleData->IoTHubTransport_Subscribe(handleData->deviceHandle) == 0)
                {
                    handleData->messageCallback.type = CALLBACK_TYPE_MESSAGE_SYNC;
                    handleData->messageCallback.callbackSync = messageCallback;
                    handleData->messageCallback.userContextCallback = userContextCallback;
                    result = IOTHUB_CLIENT_OK;
                }
                else
                {
                    LogError("IoTHubTransport_Subscribe failed");
                    handleData->messageCallback.type = CALLBACK_TYPE_NONE;
                    handleData->messageCallback.callbackSync = NULL;
                    handleData->messageCallback.callbackAsync = NULL;
                    handleData->messageCallback.userContextCallback = NULL;
                    result = IOTHUB_CLIENT_ERROR;
                }
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_SetMessageCallback_Ex(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC_EX messageCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubClientHandle == NULL)
    {
        LogError("Invalid argument - iotHubClientHandle is NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;
        if (messageCallback == NULL)
        {
            if (handleData->messageCallback.type == CALLBACK_TYPE_NONE)
            {
                LogError("not currently set to accept or process incoming messages.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else if (handleData->messageCallback.type == CALLBACK_TYPE_MESSAGE_SYNC)
            {
                LogError("Invalid workflow sequence. Please unsubscribe using the IoTHubClientCore_LL_SetMessageCallback function.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                handleData->IoTHubTransport_Unsubscribe(handleData->deviceHandle);
                handleData->messageCallback.type = CALLBACK_TYPE_NONE;
                handleData->messageCallback.callbackSync = NULL;
                handleData->messageCallback.callbackAsync = NULL;
                handleData->messageCallback.userContextCallback = NULL;
                result = IOTHUB_CLIENT_OK;
            }
        }
        else
        {
            if (handleData->messageCallback.type == CALLBACK_TYPE_MESSAGE_SYNC)
            {
                LogError("Invalid workflow sequence. Please unsubscribe using the IoTHubClientCore_LL_MessageCallbackEx function before subscribing with MessageCallback.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                if (handleData->IoTHubTransport_Subscribe(handleData->deviceHandle) == 0)
                {
                    handleData->messageCallback.type = CALLBACK_TYPE_MESSAGE_ASYNC;
                    handleData->messageCallback.callbackAsync = messageCallback;
                    handleData->messageCallback.userContextCallback = userContextCallback;
                    result = IOTHUB_CLIENT_OK;
                }
                else
                {
                    LogError("IoTHubTransport_Subscribe failed");
                    handleData->messageCallback.type = CALLBACK_TYPE_NONE;
                    handleData->messageCallback.callbackSync = NULL;
                    handleData->messageCallback.callbackAsync = NULL;
                    handleData->messageCallback.userContextCallback = NULL;
                    result = IOTHUB_CLIENT_ERROR;
                }
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_SendMessageDisposition(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE message_handle, IOTHUBMESSAGE_DISPOSITION_RESULT disposition)
{
    IOTHUB_CLIENT_RESULT result;
    if ((iotHubClientHandle == NULL) || (message_handle == NULL))
    {
        LogError("Invalid argument handle=%p, message_handle=%p", iotHubClientHandle, message_handle);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else if (disposition == IOTHUBMESSAGE_ASYNC_ACK)
    {
        LogError("IOTHUBMESSAGE_ASYNC_ACK is not a valid disposition value for this function");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;
        result = handleData->IoTHubTransport_SendMessageDisposition(handleData->deviceHandle, message_handle, disposition);
    }
    return result;
}

static void DoTimeouts(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData)
{
    tickcounter_ms_t nowTick;
    if (tickcounter_get_current_ms(handleData->tickCounter, &nowTick) != 0)
    {
        LogError("unable to get the current ms, timeouts will not be processed");
    }
    else
    {
        DLIST_ENTRY* currentItemInWaitingToSend = handleData->waitingToSend.Flink;
        while (currentItemInWaitingToSend != &(handleData->waitingToSend)) /*while we are not at the end of the list*/
        {
            IOTHUB_MESSAGE_LIST* fullEntry = containingRecord(currentItemInWaitingToSend, IOTHUB_MESSAGE_LIST, entry);
            if ((fullEntry->ms_timesOutAfter != 0) && ((nowTick - fullEntry->ms_timesOutAfter) > fullEntry->message_timeout_value))
            {
                PDLIST_ENTRY theNext = currentItemInWaitingToSend->Flink; /*need to save the next item, because the below operations are destructive*/
                DList_RemoveEntryList(currentItemInWaitingToSend);
                if (fullEntry->callback != NULL)
                {
                    fullEntry->callback(IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT, fullEntry->context);
                }
                IoTHubMessage_Destroy(fullEntry->messageHandle); /*because it has been cloned*/
                free(fullEntry);
                currentItemInWaitingToSend = theNext;
            }
            else
            {
                currentItemInWaitingToSend = currentItemInWaitingToSend->Flink;
            }
        }
    }
}

void IoTHubClientCore_LL_DoWork(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle)
{
    if (iotHubClientHandle != NULL)
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;
        DoTimeouts(handleData);

        DLIST_ENTRY* client_item = handleData->iot_msg_queue.Flink;
        while (client_item != &(handleData->iot_msg_queue)) /*while we are not at the end of the list*/
        {
            PDLIST_ENTRY next_item = client_item->Flink;

            IOTHUB_DEVICE_TWIN* queue_data = containingRecord(client_item, IOTHUB_DEVICE_TWIN, entry);
            IOTHUB_IDENTITY_INFO identity_info;
            identity_info.device_twin = queue_data;
            IOTHUB_PROCESS_ITEM_RESULT process_results =  handleData->IoTHubTransport_ProcessItem(handleData->transportHandle, IOTHUB_TYPE_DEVICE_TWIN, &identity_info);
            if (process_results == IOTHUB_PROCESS_CONTINUE || process_results == IOTHUB_PROCESS_NOT_CONNECTED)
            {
                break;
            }
            else
            {
                DList_RemoveEntryList(client_item);
                if (process_results == IOTHUB_PROCESS_OK)
                {
                    DList_InsertTailList(&(iotHubClientHandle->iot_ack_queue), &(queue_data->entry));
                }
                else
                {
                    LogError("Failure queue processing item");
                    device_twin_data_destroy(queue_data);
                }
            }
            // Move along to the next item
            client_item = next_item;
        }

        handleData->IoTHubTransport_DoWork(handleData->transportHandle);
    }
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_GetSendStatus(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
{
    IOTHUB_CLIENT_RESULT result;

    if (iotHubClientHandle == NULL || iotHubClientStatus == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LOG_ERROR_RESULT;
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;

        result = handleData->IoTHubTransport_GetSendStatus(handleData->deviceHandle, iotHubClientStatus);
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_SetConnectionStatusCallback(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK connectionStatusCallback, void * userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubClientHandle == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LOG_ERROR_RESULT;
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;
        handleData->conStatusCallback = connectionStatusCallback;
        handleData->conStatusUserContextCallback = userContextCallback;
        result = IOTHUB_CLIENT_OK;
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_SetRetryPolicy(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds)
{
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;

    if (handleData == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LOG_ERROR_RESULT;
    }
    else
    {
        if (handleData->transportHandle == NULL)
        {
            result = IOTHUB_CLIENT_ERROR;
            LOG_ERROR_RESULT;
        }
        else
        {
            if (handleData->IoTHubTransport_SetRetryPolicy(handleData->transportHandle, retryPolicy, retryTimeoutLimitInSeconds) != 0)
            {
                result = IOTHUB_CLIENT_ERROR;
                LOG_ERROR_RESULT;
            }
            else
            {
                handleData->retryPolicy = retryPolicy;
                handleData->retryTimeoutLimitInSeconds = retryTimeoutLimitInSeconds;
                result = IOTHUB_CLIENT_OK;
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_GetRetryPolicy(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY* retryPolicy, size_t* retryTimeoutLimitInSeconds)
{
    IOTHUB_CLIENT_RESULT result;

    if (iotHubClientHandle == NULL || retryPolicy == NULL || retryTimeoutLimitInSeconds == NULL)
    {
        LogError("Invalid parameter IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle = %p, IOTHUB_CLIENT_RETRY_POLICY* retryPolicy = %p, size_t* retryTimeoutLimitInSeconds = %p", iotHubClientHandle, retryPolicy, retryTimeoutLimitInSeconds);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;

        *retryPolicy = handleData->retryPolicy;
        *retryTimeoutLimitInSeconds = handleData->retryTimeoutLimitInSeconds;
        result = IOTHUB_CLIENT_OK;
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_GetLastMessageReceiveTime(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, time_t* lastMessageReceiveTime)
{
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;

    if (handleData == NULL || lastMessageReceiveTime == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LOG_ERROR_RESULT;
    }
    else
    {
        if (handleData->lastMessageReceiveTime == INDEFINITE_TIME)
        {
            result = IOTHUB_CLIENT_INDEFINITE_TIME;
            LOG_ERROR_RESULT;
        }
        else
        {
            *lastMessageReceiveTime = handleData->lastMessageReceiveTime;
            result = IOTHUB_CLIENT_OK;
        }
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_SetOption(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, const char* optionName, const void* value)
{

    IOTHUB_CLIENT_RESULT result;
    if (
        (iotHubClientHandle == NULL) ||
        (optionName == NULL) ||
        (value == NULL)
        )
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("invalid argument (NULL)");
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;

        if (strcmp(optionName, OPTION_MESSAGE_TIMEOUT) == 0)
        {
            /*this is an option handled by IoTHubClientCore_LL*/
            handleData->currentMessageTimeout = *(const tickcounter_ms_t*)value;
            result = IOTHUB_CLIENT_OK;
        }
        else if (strcmp(optionName, OPTION_PRODUCT_INFO) == 0)
        {
            if (handleData->product_info != NULL)
            {
                STRING_delete(handleData->product_info);
                handleData->product_info = NULL;
            }

            PLATFORM_INFO_OPTION supportedPlatformInfo;
            if (handleData->IoTHubTransport_GetSupportedPlatformInfo(handleData->transportHandle, &supportedPlatformInfo) != 0)
            {
                LogError("IoTHubTransport_GetSupportedPlatformInfo failed");
                result = IOTHUB_CLIENT_ERROR;
            }
            else if ((handleData->product_info = make_product_info((const char*)value, supportedPlatformInfo)) == NULL)
            {
                LogError("STRING_construct_sprintf failed");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                result = IOTHUB_CLIENT_OK;
            }
        }
        else if (strcmp(optionName, OPTION_DIAGNOSTIC_SAMPLING_PERCENTAGE) == 0)
        {
            uint32_t percentage = *(uint32_t*)value;
            if (percentage > 100)
            {
                LogError("The value of diag_sampling_percentage is out of range [0, 100]: %u", percentage);
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                handleData->diagnostic_setting.diagSamplingPercentage = percentage;
                handleData->diagnostic_setting.currentMessageNumber = 0;
                result = IOTHUB_CLIENT_OK;
            }
        }
        else if ((strcmp(optionName, OPTION_BLOB_UPLOAD_TIMEOUT_SECS) == 0) || 
                 (strcmp(optionName, OPTION_CURL_VERBOSE) == 0) || 
                 (strcmp(optionName, OPTION_NETWORK_INTERFACE_UPLOAD_TO_BLOB) == 0) ||
                 (strcmp(optionName, OPTION_BLOB_UPLOAD_TLS_RENEGOTIATION) == 0))
        {
#ifndef DONT_USE_UPLOADTOBLOB
            // This option just gets passed down into IoTHubClientCore_LL_UploadToBlob
            result = IoTHubClient_LL_UploadToBlob_SetOption(handleData->uploadToBlobHandle, optionName, value);
            if(result != IOTHUB_CLIENT_OK)
            {
                LogError("unable to IoTHubClientCore_LL_UploadToBlob_SetOption, result=%d", result);
            }
#else
            LogError("%s option being set with DONT_USE_UPLOADTOBLOB compiler switch", optionName);
            result = IOTHUB_CLIENT_ERROR;
#endif /*DONT_USE_UPLOADTOBLOB*/
        }
        // OPTION_SAS_TOKEN_REFRESH_TIME is, but may be updated in the future
        // if this becomes necessary
        else if (strcmp(optionName, OPTION_SAS_TOKEN_REFRESH_TIME) == 0 || strcmp(optionName, OPTION_SAS_TOKEN_LIFETIME) == 0)
        {
            // API compat: while IoTHubClient_Auth_Set_SasToken_Expiry accepts uint64_t, we cannot change the public API.
            if (IoTHubClient_Auth_Set_SasToken_Expiry(handleData->authorization_module, (uint64_t)(*(size_t*)value)) != 0)
            {
                LogError("Failed setting the Token Expiry time");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                result = IOTHUB_CLIENT_OK;
            }
        }
        else if (strcmp(optionName, OPTION_MODEL_ID) == 0)
        {
            if (handleData->model_id != NULL)
            {
                LogError("DT ModelId already specified.");
                result = IOTHUB_CLIENT_ERROR;
            } 
            else if ((handleData->model_id = STRING_construct((const char*)value)) == NULL)
            {
                LogError("STRING_construct failed");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                result = IOTHUB_CLIENT_OK;
            }
        }
        else
        {
            // This section is unusual for SetOption calls because it attempts to pass unhandled options
            // to two downstream targets (IoTHubTransport_SetOption and IoTHubClientCore_LL_UploadToBlob_SetOption) instead of one.

            result = handleData->IoTHubTransport_SetOption(handleData->transportHandle, optionName, value);
            if(result != IOTHUB_CLIENT_OK)
            {
                LogError("unable to IoTHubTransport_SetOption");
            }
#ifndef DONT_USE_UPLOADTOBLOB
            else
            {
                (void)IoTHubClient_LL_UploadToBlob_SetOption(handleData->uploadToBlobHandle, optionName, value);
            }
#endif /*DONT_USE_UPLOADTOBLOB*/
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_SetDeviceTwinCallback(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubClientHandle == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("Invalid argument specified iothubClientHandle=%p", iotHubClientHandle);
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;
        if (deviceTwinCallback == NULL)
        {
            handleData->IoTHubTransport_Unsubscribe_DeviceTwin(handleData->transportHandle);
            handleData->deviceTwinCallback = NULL;
            result = IOTHUB_CLIENT_OK;
        }
        else
        {
            if (handleData->IoTHubTransport_Subscribe_DeviceTwin(handleData->transportHandle) == 0)
            {
                handleData->deviceTwinCallback = deviceTwinCallback;
                handleData->deviceTwinContextCallback = userContextCallback;
                result = IOTHUB_CLIENT_OK;
            }
            else
            {
                result = IOTHUB_CLIENT_ERROR;
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_SendReportedState(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, const unsigned char* reportedState, size_t size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubClientHandle == NULL || (reportedState == NULL || size == 0) )
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("Invalid argument specified iothubClientHandle=%p, reportedState=%p, size=%lu", iotHubClientHandle, reportedState, (unsigned long)size);
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;
        IOTHUB_DEVICE_TWIN* client_data = dev_twin_data_create(handleData, get_next_item_id(handleData), reportedState, size, reportedStateCallback, userContextCallback);
        if (client_data == NULL)
        {
            LogError("Failure constructing device twin data");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            if (handleData->IoTHubTransport_Subscribe_DeviceTwin(handleData->transportHandle) != 0)
            {
                LogError("Failure adding device twin data to queue");
                device_twin_data_destroy(client_data);
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                DList_InsertTailList(&(iotHubClientHandle->iot_msg_queue), &(client_data->entry));

                result = IOTHUB_CLIENT_OK;
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_GetTwinAsync(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;

    if (iotHubClientHandle == NULL || deviceTwinCallback == NULL)
    {
        LogError("Invalid argument iothubClientHandle=%p, deviceTwinCallback=%p", iotHubClientHandle, deviceTwinCallback);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        if (iotHubClientHandle->IoTHubTransport_Subscribe_DeviceTwin(iotHubClientHandle->transportHandle) != 0)
        {
            LogError("Failure adding device twin data to queue");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            GET_TWIN_CONTEXT* getTwinCtx;

            if ((getTwinCtx = (GET_TWIN_CONTEXT*)malloc(sizeof(GET_TWIN_CONTEXT))) == NULL)
            {
                LogError("Failed creating get-twin context");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;

                getTwinCtx->callback = deviceTwinCallback;
                getTwinCtx->context = userContextCallback;

                if (handleData->IoTHubTransport_GetTwinAsync(handleData->deviceHandle, on_get_device_twin_completed, getTwinCtx) != IOTHUB_CLIENT_OK)
                {
                    LogError("Failed getting device twin document");
                    free(getTwinCtx);
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    handleData->complete_twin_update_encountered = true;
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
    }

    return result;
}


static void ResetMethodCallbackData(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData)
{
    handleData->methodCallback.type = CALLBACK_TYPE_NONE;
    handleData->methodCallback.deviceMethodCallback = NULL;
    handleData->methodCallback.commandCallback = NULL;
    handleData->methodCallback.inboundDeviceMethodCallback = NULL;
    handleData->methodCallback.userContextCallback = NULL;
}

static IOTHUB_CLIENT_RESULT VerifyMethodCallbackType(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData, CALLBACK_TYPE desiredCallbackType, bool unsubscribing)
{
    IOTHUB_CLIENT_RESULT result;

    if (unsubscribing)
    {
        // If we are unsubscribing,  method (Ex or the original non-Ex) performing the unsubscribe must be the same function that subscribed initially.
        if (handleData->methodCallback.type == CALLBACK_TYPE_NONE)
        {
            LogError("not currently set to accept or process incoming messages.");
            result = IOTHUB_CLIENT_ERROR;
        }
        else if (handleData->methodCallback.type != desiredCallbackType)
        {
            LogError("Need to unsubscribe with same type of device method function (original or _Ex()");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            result = IOTHUB_CLIENT_OK;
        }
    }
    else
    {
        // If we are already subscribed and changing the callback method, the type of method (Ex or original non-Ex) must match what was used for the original subscribe.
        if ((handleData->methodCallback.type != CALLBACK_TYPE_NONE) &&
            (handleData->methodCallback.type != desiredCallbackType))
        {
            LogError("Need to change callback function with same type of device method function (original or _Ex()");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            result = IOTHUB_CLIENT_OK;
        }
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_SetDeviceMethodCallback(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;

    if (iotHubClientHandle == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LOG_ERROR_RESULT;
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;
        bool unsubscribing = (deviceMethodCallback == NULL);

        if ((result = VerifyMethodCallbackType(handleData, CALLBACK_TYPE_METHOD, unsubscribing)) != IOTHUB_CLIENT_OK)
        {
            LogInfo("Incorrect callback type");
        }
        else if (unsubscribing)
        {
            handleData->IoTHubTransport_Unsubscribe_DeviceMethod(handleData->deviceHandle);
            ResetMethodCallbackData(handleData);
            result = IOTHUB_CLIENT_OK;
        }
        else
        {
            if (handleData->IoTHubTransport_Subscribe_DeviceMethod(handleData->deviceHandle) == 0)
            {
                handleData->methodCallback.type = CALLBACK_TYPE_METHOD;
                handleData->methodCallback.deviceMethodCallback = deviceMethodCallback;
                handleData->methodCallback.userContextCallback = userContextCallback;
                result = IOTHUB_CLIENT_OK;
            }
            else
            {
                LogError("IoTHubTransport_Subscribe_DeviceMethod failed");
                ResetMethodCallbackData(handleData);
                result = IOTHUB_CLIENT_ERROR;
            }
        }
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK inboundDeviceMethodCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubClientHandle == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LOG_ERROR_RESULT;
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;
        bool unsubscribing = (inboundDeviceMethodCallback == NULL);

        if ((result = VerifyMethodCallbackType(handleData, CALLBACK_TYPE_INBOUND_METHOD, unsubscribing)) != IOTHUB_CLIENT_OK)
        {
            LogInfo("Incorrect callback type");
        }
        else if (unsubscribing)
        {
            handleData->IoTHubTransport_Unsubscribe_DeviceMethod(handleData->deviceHandle);
            ResetMethodCallbackData(handleData);
            result = IOTHUB_CLIENT_OK;
        }
        else
        {
            if (handleData->IoTHubTransport_Subscribe_DeviceMethod(handleData->deviceHandle) == 0)
            {
                handleData->methodCallback.type = CALLBACK_TYPE_INBOUND_METHOD;
                handleData->methodCallback.inboundDeviceMethodCallback = inboundDeviceMethodCallback;
                handleData->methodCallback.userContextCallback = userContextCallback;
                result = IOTHUB_CLIENT_OK;
            }
            else
            {
                LogError("IoTHubTransport_Subscribe_DeviceMethod failed");
                ResetMethodCallbackData(handleData);
                result = IOTHUB_CLIENT_ERROR;
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_SubscribeToCommands(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_COMMAND_CALLBACK_ASYNC commandCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;

    if (iotHubClientHandle == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LOG_ERROR_RESULT;
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;
        bool unsubscribing = (commandCallback == NULL);

        if ((result = VerifyMethodCallbackType(handleData, CALLBACK_TYPE_COMMAND, unsubscribing)) != IOTHUB_CLIENT_OK)
        {
            LogInfo("Incorrect callback type");
        }
        else if (unsubscribing)
        {
            handleData->IoTHubTransport_Unsubscribe_DeviceMethod(handleData->deviceHandle);
            ResetMethodCallbackData(handleData);
            result = IOTHUB_CLIENT_OK;
        }
        else
        {
            if (handleData->IoTHubTransport_Subscribe_DeviceMethod(handleData->deviceHandle) == 0)
            {
                handleData->methodCallback.type = CALLBACK_TYPE_COMMAND;
                handleData->methodCallback.commandCallback = commandCallback;
                handleData->methodCallback.userContextCallback = userContextCallback;
                result = IOTHUB_CLIENT_OK;
            }
            else
            {
                LogError("IoTHubTransport_Subscribe_DeviceMethod failed");
                ResetMethodCallbackData(handleData);
                result = IOTHUB_CLIENT_ERROR;
            }
        }
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_DeviceMethodResponse(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, METHOD_HANDLE methodId, const unsigned char* response, size_t response_size, int status_response)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubClientHandle == NULL || methodId == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LOG_ERROR_RESULT;
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;
        if (handleData->IoTHubTransport_DeviceMethod_Response(handleData->deviceHandle, methodId, response, response_size, status_response) != 0)
        {
            LogError("IoTHubTransport_DeviceMethod_Response failed");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            result = IOTHUB_CLIENT_OK;
        }
    }
    return result;
}

#ifndef DONT_USE_UPLOADTOBLOB
IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_UploadToBlob(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, const char* destinationFileName, const unsigned char* source, size_t size)
{
    IOTHUB_CLIENT_RESULT result;
    if (
        (iotHubClientHandle == NULL) ||
        (destinationFileName == NULL) ||
        ((source == NULL) && (size >0))
        )
    {
        LogError("invalid parameters iotHubClientHandle=%p, const char* destinationFileName=%s, const unsigned char* source=%p, size_t size=%lu",
            iotHubClientHandle, MU_P_OR_NULL(destinationFileName), source, (unsigned long)size);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        char* uploadCorrelationId;
        char* azureBlobSasUri;

        if (IoTHubClient_LL_UploadToBlob_InitializeUpload(
                iotHubClientHandle->uploadToBlobHandle, destinationFileName, &uploadCorrelationId, &azureBlobSasUri) != IOTHUB_CLIENT_OK)
        {
            LogError("Failed initializing upload in IoT Hub");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            bool uploadSucceeded;
            IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext = IoTHubClient_LL_UploadToBlob_CreateContext(iotHubClientHandle->uploadToBlobHandle, azureBlobSasUri);

            if (uploadContext == NULL)
            {
                LogError("Failed creating upload to blob context");
                uploadSucceeded = false;
            }
            else
            {
                if (IoTHubClient_LL_UploadToBlob_PutBlock(uploadContext, 0, source, size) != IOTHUB_CLIENT_OK)
                {
                    LogError("Failed uploading block to Azure Blob Storage");
                    uploadSucceeded = false;
                }
                else if (IoTHubClient_LL_UploadToBlob_PutBlockList(uploadContext) != IOTHUB_CLIENT_OK)
                {
                    LogError("Failed completing upload to blob.");
                    uploadSucceeded = false;
                }
                else
                {
                    uploadSucceeded = true; 
                }

                IoTHubClient_LL_UploadToBlob_DestroyContext(uploadContext);
            }

            // TODO (ewertons): fix the http error status below.
            if (IoTHubClient_LL_UploadToBlob_NotifyCompletion(
                    iotHubClientHandle->uploadToBlobHandle, uploadCorrelationId, uploadSucceeded, 400, NULL) != IOTHUB_CLIENT_OK)
            {
                LogError("Failed completing upload to blob.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                result = (uploadSucceeded ? IOTHUB_CLIENT_OK : IOTHUB_CLIENT_ERROR); 
            }

            free(uploadCorrelationId);
            free(azureBlobSasUri);
        }
    }

    return result;
}

typedef struct UPLOAD_MULTIPLE_BLOCKS_WRAPPER_CONTEXT_TAG
{
    IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK getDataCallback;
    void* context;
} UPLOAD_MULTIPLE_BLOCKS_WRAPPER_CONTEXT;


static IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT uploadMultipleBlocksCallbackWrapper(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context)
{
    UPLOAD_MULTIPLE_BLOCKS_WRAPPER_CONTEXT* wrapperContext = (UPLOAD_MULTIPLE_BLOCKS_WRAPPER_CONTEXT*)context;
    wrapperContext->getDataCallback(result, data, size, wrapperContext->context);
    return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_UploadMultipleBlocksToBlob(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, const char* destinationFileName, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK getDataCallback, void* context)
{
    IOTHUB_CLIENT_RESULT result;
    if (
        (iotHubClientHandle == NULL) ||
        (destinationFileName == NULL) ||
        (getDataCallback == NULL)
        )
    {
        LogError("invalid parameters IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle=%p, const char* destinationFileName=%p, getDataCallback=%p", iotHubClientHandle, destinationFileName, getDataCallback);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        char* uploadCorrelationId;
        char* azureBlobSasUri;

        if (IoTHubClient_LL_UploadToBlob_InitializeUpload(
                iotHubClientHandle->uploadToBlobHandle, destinationFileName, &uploadCorrelationId, &azureBlobSasUri) != IOTHUB_CLIENT_OK)
        {
            LogError("Failed initializing upload in IoT Hub");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            bool uploadSucceeded;
            IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext = IoTHubClient_LL_UploadToBlob_CreateContext(iotHubClientHandle->uploadToBlobHandle, azureBlobSasUri);

            if (uploadContext == NULL)
            {
                LogError("Failed creating upload to blob context");
                uploadSucceeded = false;
            }
            else
            {
                UPLOAD_MULTIPLE_BLOCKS_WRAPPER_CONTEXT uploadMultipleBlocksWrapperContext;
                uploadMultipleBlocksWrapperContext.getDataCallback = getDataCallback;
                uploadMultipleBlocksWrapperContext.context = context;

                if (IoTHubClient_LL_UploadToBlob_UploadMultipleBlocks(
                        uploadContext, uploadMultipleBlocksCallbackWrapper, &uploadMultipleBlocksWrapperContext) != IOTHUB_CLIENT_OK)
                {
                    LogError("Failed to upload multiple blocks to Azure blob");
                    uploadSucceeded = false;
                }
                else
                {
                    uploadSucceeded = false;
                }

                IoTHubClient_LL_UploadToBlob_DestroyContext(uploadContext);
            }

            // TODO (ewertons): fix the http error status below.
            if (IoTHubClient_LL_UploadToBlob_NotifyCompletion(
                    iotHubClientHandle->uploadToBlobHandle, uploadCorrelationId, uploadSucceeded, 400, NULL) != IOTHUB_CLIENT_OK)
            {
                LogError("Failed completing upload to blob.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                result = (uploadSucceeded ? IOTHUB_CLIENT_OK : IOTHUB_CLIENT_ERROR); 
            }

            free(uploadCorrelationId);
            free(azureBlobSasUri);
        }
    }
    
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_UploadMultipleBlocksToBlobEx(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, const char* destinationFileName, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX getDataCallbackEx, void* context)
{
    IOTHUB_CLIENT_RESULT result;
    if (
        (iotHubClientHandle == NULL) ||
        (destinationFileName == NULL) ||
        (getDataCallbackEx == NULL)
        )
    {
        LogError("invalid parameters IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle=%p, destinationFileName=%p, getDataCallbackEx=%p", iotHubClientHandle, destinationFileName, getDataCallbackEx);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        char* uploadCorrelationId = NULL;
        char* azureBlobSasUri = NULL;

        if (IoTHubClient_LL_UploadToBlob_InitializeUpload(
                iotHubClientHandle->uploadToBlobHandle, destinationFileName, &uploadCorrelationId, &azureBlobSasUri) != IOTHUB_CLIENT_OK)
        {
            LogError("Failed initializing upload in IoT Hub");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            bool uploadSucceeded;
            IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext = IoTHubClient_LL_UploadToBlob_CreateContext(iotHubClientHandle->uploadToBlobHandle, azureBlobSasUri);

            if (uploadContext == NULL)
            {
                LogError("Failed creating upload to blob context");
                uploadSucceeded = false;
            }
            else
            {
                if (IoTHubClient_LL_UploadToBlob_UploadMultipleBlocks(uploadContext, getDataCallbackEx, context) != IOTHUB_CLIENT_OK)
                {
                    LogError("Failed to upload multiple blocks to Azure blob");
                    uploadSucceeded = false;
                }
                else
                {
                    uploadSucceeded = true;
                }

                IoTHubClient_LL_UploadToBlob_DestroyContext(uploadContext);
            }

            if (IoTHubClient_LL_UploadToBlob_NotifyCompletion(
                    iotHubClientHandle->uploadToBlobHandle, uploadCorrelationId, uploadSucceeded,
                    uploadSucceeded ? 200 : 400, NULL) != IOTHUB_CLIENT_OK)
            {
                LogError("Failed completing upload to blob.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                result = (uploadSucceeded ? IOTHUB_CLIENT_OK : IOTHUB_CLIENT_ERROR); 
            }

            free(uploadCorrelationId);
            free(azureBlobSasUri);
        }
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_InitializeUpload(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, const char* destinationFileName, char** uploadCorrelationId, char** azureBlobSasUri)
{
    IOTHUB_CLIENT_RESULT result;

    if (iotHubClientHandle == NULL)
    {
        LogError("invalid parameter iotHubClientHandle=%p", iotHubClientHandle);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        if (IoTHubClient_LL_UploadToBlob_InitializeUpload(
                iotHubClientHandle->uploadToBlobHandle, destinationFileName, uploadCorrelationId, azureBlobSasUri) != IOTHUB_CLIENT_OK)
        {
            LogError("Failed initializing upload in IoT Hub");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            result = IOTHUB_CLIENT_OK;
        }
    }

    return result;
}

IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE IoTHubClientCore_LL_AzureStorageCreateClient(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, const char* azureBlobSasUri)
{
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE result;
    if (
        (iotHubClientHandle == NULL) ||
        (azureBlobSasUri == NULL)
        )
    {
        LogError("invalid parameters iotHubClientHandle=%p, azureBlobSasUri=%p", iotHubClientHandle, azureBlobSasUri);
        result = NULL;
    }
    else
    {
        result = IoTHubClient_LL_UploadToBlob_CreateContext(iotHubClientHandle->uploadToBlobHandle, azureBlobSasUri);
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_AzureStoragePutBlock(IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE azureStorageClientHandle, uint32_t blockNumber, const uint8_t* dataPtr, size_t dataSize)
{
    IOTHUB_CLIENT_RESULT result;

    if (
        (azureStorageClientHandle == NULL) ||
        (dataPtr == NULL) ||
        (dataSize == 0)
        )
    {
        LogError("invalid parameters azureStorageClientHandle=%p, dataPtr=%p, dataSize=%zu", azureStorageClientHandle, dataPtr, dataSize);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        result = IoTHubClient_LL_UploadToBlob_PutBlock(azureStorageClientHandle, blockNumber, dataPtr, dataSize);
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_AzureStoragePutBlockList(IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE azureStorageClientHandle)
{
    IOTHUB_CLIENT_RESULT result;

    if (azureStorageClientHandle == NULL)
    {
        LogError("invalid parameter azureStorageClientHandle=%p", azureStorageClientHandle);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        result = IoTHubClient_LL_UploadToBlob_PutBlockList(azureStorageClientHandle);
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_NotifyUploadCompletion(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, const char* uploadCorrelationId, bool isSuccess, int responseCode, const char* responseMessage)
{
    IOTHUB_CLIENT_RESULT result;

    if (iotHubClientHandle == NULL)
    {
        LogError("invalid parameter iotHubClientHandle=%p", iotHubClientHandle);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        result = IoTHubClient_LL_UploadToBlob_NotifyCompletion(iotHubClientHandle->uploadToBlobHandle, uploadCorrelationId, isSuccess, responseCode, responseMessage);
    }

    return result;
}

void IoTHubClientCore_LL_AzureStorageDestroyClient(IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE azureStorageClientHandle)
{
    if (azureStorageClientHandle != NULL)
    {
        IoTHubClient_LL_UploadToBlob_DestroyContext(azureStorageClientHandle);
    }
}
#endif // DONT_USE_UPLOADTOBLOB

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_SendEventToOutputAsync(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, const char* outputName, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;

    if ((iotHubClientHandle == NULL) || (outputName == NULL) || (eventMessageHandle == NULL) || ((eventConfirmationCallback == NULL) && (userContextCallback != NULL)))
    {
        LogError("Invalid argument (iotHubClientHandle=%p, outputName=%p, eventMessageHandle=%p)", iotHubClientHandle, outputName, eventMessageHandle);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        if (IoTHubMessage_SetOutputName(eventMessageHandle, outputName) != IOTHUB_MESSAGE_OK)
        {
            LogError("IoTHubMessage_SetOutputName failed");
            result = IOTHUB_CLIENT_ERROR;
        }
        else if ((result = IoTHubClientCore_LL_SendEventAsync(iotHubClientHandle, eventMessageHandle, eventConfirmationCallback, userContextCallback)) != IOTHUB_CLIENT_OK)
        {
            LogError("Call into IoTHubClient_LL_SendEventAsync failed, result=%d", result);
        }
    }

    return result;
}


static IOTHUB_CLIENT_RESULT create_event_handler_callback(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData, const char* inputName, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC callbackSync, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC_EX callbackSyncEx, void* userContextCallback, void* userContextCallbackEx, size_t userContextCallbackExLength)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_ERROR;
    bool add_to_list = false;

    if ((handleData->event_callbacks == NULL) && ((handleData->event_callbacks = singlylinkedlist_create()) == NULL))
    {
        LogError("Could not allocate linked list for callbacks");
        result = IOTHUB_CLIENT_ERROR;
    }
    else
    {
        IOTHUB_EVENT_CALLBACK* event_callback = NULL;
        LIST_ITEM_HANDLE item_handle = singlylinkedlist_find(handleData->event_callbacks, is_event_equal_for_match, (const void*)inputName);
        if (item_handle == NULL)
        {
            event_callback = (IOTHUB_EVENT_CALLBACK*)malloc(sizeof(IOTHUB_EVENT_CALLBACK));
            if (event_callback == NULL)
            {
                LogError("Could not allocate IOTHUB_EVENT_CALLBACK");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                memset(event_callback, 0, sizeof(*event_callback));
                add_to_list = true;
            }
        }
        else
        {
            event_callback = (IOTHUB_EVENT_CALLBACK*)singlylinkedlist_item_get_value(item_handle);
            if (event_callback == NULL)
            {
                LogError("singlylinkedlist_item_get_value failed looking up event callback");
            }
        }

        if (event_callback != NULL)
        {
            if ((inputName != NULL) && (event_callback->inputName == NULL))
            {
                event_callback->inputName = STRING_construct(inputName);
            }

            if ((inputName == NULL) || (event_callback->inputName != NULL))
            {
                event_callback->callbackAsync = callbackSync;
                event_callback->callbackAsyncEx = callbackSyncEx;

                free(event_callback->userContextCallbackEx);
                event_callback->userContextCallbackEx = NULL;

                if (userContextCallbackEx == NULL)
                {
                    event_callback->userContextCallback = userContextCallback;
                }

                if ((userContextCallbackEx != NULL) &&
                    (NULL == (event_callback->userContextCallbackEx = malloc(userContextCallbackExLength))))
                {
                    LogError("Unable to allocate userContextCallback");
                    delete_event(event_callback);
                    result = IOTHUB_CLIENT_ERROR;
                }
                else if ((add_to_list == true) && (NULL == singlylinkedlist_add(handleData->event_callbacks, event_callback)))
                {
                    LogError("Unable to add eventCallback to list");
                    delete_event(event_callback);
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    if (userContextCallbackEx != NULL)
                    {
                        memcpy(event_callback->userContextCallbackEx, userContextCallbackEx, userContextCallbackExLength);
                    }
                    result = IOTHUB_CLIENT_OK;
                }
            }
            else
            {
                delete_event(event_callback);
                result = IOTHUB_CLIENT_ERROR;
            }
        }
    }

    return result;
}

static IOTHUB_CLIENT_RESULT remove_event_unsubscribe_if_needed(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData, const char* inputName)
{
    IOTHUB_CLIENT_RESULT result;

    LIST_ITEM_HANDLE item_handle = singlylinkedlist_find(handleData->event_callbacks, is_event_equal_for_match, (const void*)inputName);
    if (item_handle == NULL)
    {
        LogError("Input name %s was not present", MU_P_OR_NULL(inputName));
        result = IOTHUB_CLIENT_ERROR;
    }
    else
    {
        IOTHUB_EVENT_CALLBACK* event_callback = (IOTHUB_EVENT_CALLBACK*)singlylinkedlist_item_get_value(item_handle);
        if (event_callback == NULL)
        {
            LogError("singlylinkedlist_item_get_value failed");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            delete_event(event_callback);
            if (singlylinkedlist_remove(handleData->event_callbacks, item_handle) != 0)
            {
                LogError("singlylinkedlist_remove failed");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                if (singlylinkedlist_get_head_item(handleData->event_callbacks) == NULL)
                {
                    handleData->IoTHubTransport_Unsubscribe_InputQueue(handleData->deviceHandle);
                }
                result = IOTHUB_CLIENT_OK;
            }
        }
    }

    return result;
}


IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_SetInputMessageCallbackImpl(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, const char* inputName, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC eventHandlerCallback, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC_EX eventHandlerCallbackEx, void *userContextCallback, void *userContextCallbackEx, size_t userContextCallbackExLength)
{
    IOTHUB_CLIENT_RESULT result;

    if (iotHubClientHandle == NULL)
    {
        LogError("Invalid argument - iotHubClientHandle=%p, inputName=%p", iotHubClientHandle, inputName);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)iotHubClientHandle;
        if ((eventHandlerCallback == NULL) && (eventHandlerCallbackEx == NULL))
        {
            result = (IOTHUB_CLIENT_RESULT)remove_event_unsubscribe_if_needed(handleData, inputName);
        }
        else
        {
            bool registered_with_transport_handler = (handleData->event_callbacks != NULL) && (singlylinkedlist_get_head_item(handleData->event_callbacks) != NULL);
            if ((result = (IOTHUB_CLIENT_RESULT)create_event_handler_callback(handleData, inputName, eventHandlerCallback, eventHandlerCallbackEx, userContextCallback, userContextCallbackEx, userContextCallbackExLength)) != IOTHUB_CLIENT_OK)
            {
                LogError("create_event_handler_callback call failed, error = %d", result);
            }
            else if (!registered_with_transport_handler && (handleData->IoTHubTransport_Subscribe_InputQueue(handleData->deviceHandle) != 0))
            {
                LogError("IoTHubTransport_Subscribe_InputQueue failed");
                delete_event_callback_list(handleData);
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                result = IOTHUB_CLIENT_OK;
            }
        }
    }
    return result;

}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_SetInputMessageCallbackEx(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, const char* inputName, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC_EX eventHandlerCallbackEx, void *userContextCallbackEx, size_t userContextCallbackExLength)
{
    return IoTHubClientCore_LL_SetInputMessageCallbackImpl(iotHubClientHandle, inputName, NULL, eventHandlerCallbackEx, NULL, userContextCallbackEx, userContextCallbackExLength);
}

IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_SetInputMessageCallback(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, const char* inputName, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC eventHandlerCallback, void* userContextCallback)
{
    return IoTHubClientCore_LL_SetInputMessageCallbackImpl(iotHubClientHandle, inputName, eventHandlerCallback, NULL, userContextCallback, NULL, 0);
}

int IoTHubClientCore_LL_GetTransportCallbacks(TRANSPORT_CALLBACKS_INFO* transport_cb)
{
    int result;
    if (transport_cb == NULL)
    {
        LogError("Invalid parameter transport callback can not be NULL");
        result = MU_FAILURE;
    }
    else
    {
        transport_cb->send_complete_cb = IoTHubClientCore_LL_SendComplete;
        transport_cb->twin_retrieve_prop_complete_cb = IoTHubClientCore_LL_RetrievePropertyComplete;
        transport_cb->twin_rpt_state_complete_cb = IoTHubClientCore_LL_ReportedStateComplete;
        transport_cb->connection_status_cb = IoTHubClientCore_LL_ConnectionStatusCallBack;
        transport_cb->prod_info_cb = IoTHubClientCore_LL_GetProductInfo;
        transport_cb->msg_input_cb = IoTHubClientCore_LL_MessageCallbackFromInput;
        transport_cb->msg_cb = IoTHubClientCore_LL_MessageCallback;
        transport_cb->method_complete_cb = IoTHubClientCore_LL_DeviceMethodComplete;
        transport_cb->get_model_id_cb = IoTHubClientCore_LL_GetModelId;
        result = 0;
    }
    return result;
}


/* These should be replaced during iothub_client refactor */
IOTHUB_CLIENT_RESULT IoTHubClientCore_LL_GenericMethodInvoke(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, const char* deviceId, const char* moduleId, const char* methodName, const char* methodPayload, unsigned int timeout, int* responseStatus, unsigned char** responsePayload, size_t* responsePayloadSize)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubClientHandle != NULL)
    {
        if (moduleId != NULL)
        {
            result = IoTHubClient_Edge_ModuleMethodInvoke(iotHubClientHandle->methodHandle, deviceId, moduleId, methodName, methodPayload, timeout, responseStatus, responsePayload, responsePayloadSize);
        }
        else
        {
            result = IoTHubClient_Edge_DeviceMethodInvoke(iotHubClientHandle->methodHandle, deviceId, methodName, methodPayload, timeout, responseStatus, responsePayload, responsePayloadSize);

        }
    }
    else
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    return result;
}

/*end*/
