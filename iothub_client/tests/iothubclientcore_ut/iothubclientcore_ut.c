// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#endif

#include <time.h>
#include <signal.h>

#if defined _MSC_VER
#pragma warning(disable: 4054) /* MSC incorrectly fires this */
#endif

static size_t my_malloc_count;
static void* my_malloc_items[100];


static bool g_fail_my_gballoc_malloc = false;
static void* my_gballoc_malloc(size_t size)
{
    void* result;

    if (g_fail_my_gballoc_malloc)
    {
        result = NULL;
    }
    else
    {
        result = malloc(size);
        my_malloc_items[my_malloc_count++] = result;
    }

    return result;
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

void* my_gballoc_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"

#define IOTHUB_CLIENT_CORE_H

#define ENABLE_MOCKS
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/agenttime.h"
#include "iothub_client_core_ll.h"
#include "internal/iothubtransport.h"

#undef ENABLE_MOCKS

#undef IOTHUB_CLIENT_CORE_H

#include "iothub_client_core.h"

#ifdef __cplusplus
extern "C" {
#endif

    extern VECTOR_HANDLE real_VECTOR_create(size_t elementSize);
    extern VECTOR_HANDLE real_VECTOR_move(VECTOR_HANDLE handle);
    extern void real_VECTOR_destroy(VECTOR_HANDLE handle);
    extern int real_VECTOR_push_back(VECTOR_HANDLE handle, const void* elements, size_t numElements);
    extern void real_VECTOR_erase(VECTOR_HANDLE handle, void* elements, size_t numElements);
    extern void real_VECTOR_clear(VECTOR_HANDLE handle);
    extern void* real_VECTOR_element(VECTOR_HANDLE handle, size_t index);
    extern void* real_VECTOR_front(VECTOR_HANDLE handle);
    extern void* real_VECTOR_back(VECTOR_HANDLE handle);
    extern void* real_VECTOR_find_if(VECTOR_HANDLE handle, PREDICATE_FUNCTION pred, const void* value);
    extern size_t real_VECTOR_size(VECTOR_HANDLE handle);

    extern int real_mallocAndStrcpy_s(char** destination, const char* source);
    extern int real_size_tToString(char* destination, size_t destinationSize, size_t value);

#ifdef __cplusplus
}
#endif

static void* g_userContextCallback;
static const size_t method_calls_repeat = 3;
static void my_test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    (void)result;
    (void)userContextCallback;
    g_userContextCallback = NULL;
}

static int my_DeviceMethodCallback_Impl(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback)
{
    (void)method_name;
    (void)payload;
    (void)size;
    (void)userContextCallback;

    *response = (unsigned char*)malloc(2);
    *resp_size = 2;

    return 200;
}

typedef struct TEST_GETDATA_CALLBACK_ARGS_TAG
{
    IOTHUB_CLIENT_FILE_UPLOAD_RESULT last_result;
    unsigned char const ** last_data;
    size_t* last_size;
    int invokation_counter;
} TEST_GETDATA_CALLBACK_ARGS;

static void my_FileUpload_GetData_Callback(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context)
{
    TEST_GETDATA_CALLBACK_ARGS* saved_args = (TEST_GETDATA_CALLBACK_ARGS*)context;
    saved_args->last_result = result;
    saved_args->last_data = data;
    saved_args->last_size = size;
    saved_args->invokation_counter++;
}

static IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT my_FileUpload_GetData_CallbackEx(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context)
{
    TEST_GETDATA_CALLBACK_ARGS* saved_args = (TEST_GETDATA_CALLBACK_ARGS*)context;
    saved_args->last_result = result;
    saved_args->last_data = data;
    saved_args->last_size = size;
    saved_args->invokation_counter++;
    return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK;
}

#define ENABLE_MOCKS
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/threadapi.h"

MOCKABLE_FUNCTION(, void, test_event_confirmation_callback, IOTHUB_CLIENT_CONFIRMATION_RESULT, result, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, test_event_confirmation_callback2, IOTHUB_CLIENT_CONFIRMATION_RESULT, result, void*, userContextCallback);
MOCKABLE_FUNCTION(, IOTHUBMESSAGE_DISPOSITION_RESULT, test_message_confirmation_callback, IOTHUB_MESSAGE_HANDLE, message, void*, userContextCallback);
MOCKABLE_FUNCTION(, IOTHUBMESSAGE_DISPOSITION_RESULT, test_message_confirmation_callback_ex, IOTHUB_MESSAGE_HANDLE, message, void*, userContextCallback, void*, transportContext);
MOCKABLE_FUNCTION(, void, test_device_twin_callback, DEVICE_TWIN_UPDATE_STATE, update_state, const unsigned char*, payLoad, size_t, size, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, test_connection_status_callback, IOTHUB_CLIENT_CONNECTION_STATUS, result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, test_report_state_callback, int, status_code, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, test_report_state_callback2, int, status_code, void*, userContextCallback);
MOCKABLE_FUNCTION(, int, test_incoming_method_callback, const char*, method_name, const unsigned char*, payload, size_t, size, METHOD_HANDLE, method_id, void*, userContextCallback);
MOCKABLE_FUNCTION(, int, test_method_callback, const char*, method_name, const unsigned char*, payload, size_t, size, unsigned char**, response, size_t*, resp_size, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, test_file_upload_callback, IOTHUB_CLIENT_FILE_UPLOAD_RESULT, result, void*, userContextCallback);
MOCKABLE_FUNCTION(, int, my_DeviceMethodCallback, const char*, method_name, const unsigned char*, payload, size_t, size, unsigned char**, response, size_t*, resp_size, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, test_command_callback, const IOTHUB_CLIENT_COMMAND_REQUEST*, commandRequest, IOTHUB_CLIENT_COMMAND_RESPONSE*, commandResponse, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, test_method_invoke_callback, IOTHUB_CLIENT_RESULT, result, int, responseStatus, unsigned char*, responsePayload, size_t, responsePayloadSize, void*, userContextCallBack);

#undef ENABLE_MOCKS

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RETRY_POLICY, IOTHUB_CLIENT_RETRY_POLICY_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RETRY_POLICY, IOTHUB_CLIENT_RETRY_POLICY_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, IOTHUB_CLIENT_FILE_UPLOAD_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, IOTHUB_CLIENT_FILE_UPLOAD_RESULT_VALUES);

// Overloading operators for Micromock
static TEST_MUTEX_HANDLE test_serialize_mutex;

#ifdef __cplusplus
extern "C" const size_t IoTHubClientCore_ThreadTerminationOffset;
#else
extern const size_t IoTHubClientCore_ThreadTerminationOffset;
#endif

typedef struct LOCK_TEST_INFO_TAG
{
    void* lock_taken;
} LOCK_TEST_INFO;

static LOCK_TEST_INFO g_transport_lock;

static THREAD_START_FUNC g_thread_func;
static void* g_thread_func_arg;
static IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK g_eventConfirmationCallback;
static IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK g_deviceTwinCallback;
static IOTHUB_CLIENT_REPORTED_STATE_CALLBACK g_reportedStateCallback;
static IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK g_connectionStatusCallback;
static IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK g_inboundDeviceCallback;
static IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC g_messageCallback;
static IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC_EX g_messageCallback_ex;

static size_t g_how_thread_loops = 0;
static size_t g_thread_loop_count = 0;

static const IOTHUB_CLIENT_TRANSPORT_PROVIDER TEST_TRANSPORT_PROVIDER = (IOTHUB_CLIENT_TRANSPORT_PROVIDER)0x1110;
static IOTHUB_CLIENT_CORE_LL_HANDLE TEST_IOTHUB_CLIENT_CORE_LL_HANDLE = (IOTHUB_CLIENT_CORE_LL_HANDLE)0x1111;
static SINGLYLINKEDLIST_HANDLE TEST_SLL_HANDLE = (SINGLYLINKEDLIST_HANDLE)0x1114;
static const IOTHUB_CLIENT_CONFIG* TEST_CLIENT_CONFIG = (IOTHUB_CLIENT_CONFIG*)0x1115;
static IOTHUB_MESSAGE_HANDLE TEST_MESSAGE_HANDLE = (IOTHUB_MESSAGE_HANDLE)0x1116;
static THREAD_HANDLE TEST_THREAD_HANDLE = (THREAD_HANDLE)0x1117;
static LIST_ITEM_HANDLE TEST_LIST_HANDLE = (LIST_ITEM_HANDLE)0x1118;
static TRANSPORT_HANDLE TEST_TRANSPORT_HANDLE = (TRANSPORT_HANDLE)0x1119;
static IOTHUB_CLIENT_DEVICE_CONFIG* TEST_CLIENT_DEVICE_CONFIG = (IOTHUB_CLIENT_DEVICE_CONFIG*)0x111A;
static METHOD_HANDLE TEST_METHOD_ID = (METHOD_HANDLE)0x111B;
static STRING_HANDLE TEST_STRING_HANDLE = (STRING_HANDLE)0x111C;
static BUFFER_HANDLE TEST_BUFFER_HANDLE = (BUFFER_HANDLE)0x111D;

static const char* TEST_CONNECTION_STRING = "Test_connection_string";
static const char* TEST_DEVICE_ID = "theidofTheDevice";
static const char* TEST_DEVICE_KEY = "theKeyoftheDevice";
static const char* TEST_IOTHUBNAME = "theNameoftheIotHub";
static const char* TEST_DEVICE_SAS = "theSasOfTheDevice";
static const char* TEST_IOTHUBSUFFIX = "theSuffixoftheIotHubHostname";
#define TEST_METHOD_NAME_VAL "method_name"
#define TEST_COMPONENT_NAME "test_component_name"
static const char* TEST_METHOD_NAME = TEST_METHOD_NAME_VAL;
static const char* TEST_METHOD_NAME_WITH_COMPONENT = TEST_COMPONENT_NAME "*" TEST_METHOD_NAME_VAL;
static const char* TEST_INPUT_NAME = "theInputName";
static const char* TEST_OUTPUT_NAME = "theOutputName";
static const char* TEST_IOTHUB_URI = "iothub_uri";
static const char* TEST_MODULE_ID = "ModuleId";
static const unsigned char* TEST_DEVICE_METHOD_RESPONSE = (const unsigned char*)0x62;
static size_t TEST_DEVICE_RESP_LENGTH = 1;
static void* CALLBACK_CONTEXT = (void*)0x1210;
static void* CALLBACK_CONTEXT2 = (void*)0x1211;
static const time_t TEST_TIME_VALUE = (time_t)123456;

static const char TEST_COMMAND_PAYLOAD[] = "{\"foo\":41,\"bar\":42,\"baz\":\"boo\"}";
static size_t TEST_COMMAND_PAYLOAD_LENGTH = sizeof(TEST_COMMAND_PAYLOAD) - 1;

static char* g_currentTestComponentName;

#define REPORTED_STATE_STATUS_CODE      200

const char *TEST_METHOD_PAYLOAD = "MethodPayload";
const int TEST_INVOKE_TIMEOUT = 1234;

static const size_t command_calls_repeat = 3;
static METHOD_HANDLE TEST_COMMAND_ID = (METHOD_HANDLE)0x111E;

static LOCK_HANDLE my_Lock_Init(void)
{
    LOCK_TEST_INFO* lock_info = (LOCK_TEST_INFO*)my_gballoc_malloc(sizeof(LOCK_TEST_INFO) );
    lock_info->lock_taken = NULL;
    return (LOCK_HANDLE)lock_info;
}

static LOCK_RESULT my_Lock_Deinit(LOCK_HANDLE handle)
{
    LOCK_TEST_INFO* lock_info = (LOCK_TEST_INFO*)handle;
    my_gballoc_free(lock_info);
    return LOCK_OK;
}

static LOCK_RESULT my_Lock(LOCK_HANDLE handle)
{
    LOCK_TEST_INFO* lock_info = (LOCK_TEST_INFO*)handle;
    lock_info->lock_taken = my_gballoc_malloc(1);
    return LOCK_OK;
}

static LOCK_RESULT my_Unlock(LOCK_HANDLE handle)
{
    LOCK_TEST_INFO* lock_info = (LOCK_TEST_INFO*)handle;
    my_gballoc_free(lock_info->lock_taken);
    return LOCK_OK;
}

static LOCK_HANDLE my_IoTHubTransport_GetLock(TRANSPORT_HANDLE transportHandle)
{
    (void)transportHandle;
    return (LOCK_HANDLE)&g_transport_lock;
}

static THREADAPI_RESULT my_ThreadAPI_Join(THREAD_HANDLE threadHandle, int *res)
{
    (void)threadHandle;
    res = 0;
    return THREADAPI_OK;
}

static THREADAPI_RESULT my_ThreadAPI_Create(THREAD_HANDLE* threadHandle, THREAD_START_FUNC func, void* arg)
{
    *threadHandle = TEST_THREAD_HANDLE;
    g_thread_func = func;
    g_thread_func_arg = arg;
    return THREADAPI_OK;
}

static void my_ThreadAPI_Sleep(unsigned int milliseconds)
{
    (void)milliseconds;
    g_thread_loop_count++;
    if ( (g_how_thread_loops > 0) && (g_how_thread_loops == g_thread_loop_count))
    {
        *(sig_atomic_t*)(((char*)g_thread_func_arg) + IoTHubClientCore_ThreadTerminationOffset) = 1; /*tell the thread to stop*/
    }
}

static IOTHUB_CLIENT_RESULT my_IoTHubClientCore_LL_GetSendStatus(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
{
    (void)iotHubClientHandle;
    *iotHubClientStatus = IOTHUB_CLIENT_SEND_STATUS_IDLE;
    return IOTHUB_CLIENT_OK;
}

static bool g_fail_my_SendEventAsync = false;

static IOTHUB_CLIENT_RESULT my_IoTHubClientCore_LL_SendEventAsync(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback)
{
    (void)iotHubClientHandle;
    (void)eventMessageHandle;
    if (g_fail_my_SendEventAsync)
    {
        return IOTHUB_CLIENT_ERROR;
    }
    else
    {
        g_eventConfirmationCallback = eventConfirmationCallback;
        g_userContextCallback = userContextCallback;
        return IOTHUB_CLIENT_OK;
    }
}

static IOTHUB_CLIENT_RESULT my_IoTHubClientCore_LL_SetDeviceTwinCallback(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback)
{
    (void)iotHubClientHandle;
    g_deviceTwinCallback = deviceTwinCallback;
    g_userContextCallback = userContextCallback;
    return IOTHUB_CLIENT_OK;
}

static IOTHUB_CLIENT_RESULT my_IoTHubClientCore_LL_GetTwinAsync(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback)
{
    (void)iotHubClientHandle;
    g_deviceTwinCallback = deviceTwinCallback;
    g_userContextCallback = userContextCallback;
    return IOTHUB_CLIENT_OK;
}

static IOTHUB_CLIENT_RESULT my_IoTHubClientCore_LL_SendReportedState(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, const unsigned char* reportedState, size_t size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback)
{
    (void)iotHubClientHandle;
    (void)reportedState;
    (void)size;
    g_reportedStateCallback = reportedStateCallback;
    g_userContextCallback = userContextCallback;
    return IOTHUB_CLIENT_OK;
}

static IOTHUB_CLIENT_RESULT my_IoTHubClient_LL_SetConnectionStatusCallback_result;
static IOTHUB_CLIENT_RESULT my_IoTHubClient_LL_SetConnectionStatusCallback(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK connectionStatusCallback, void* userContextCallback)
{
    (void)iotHubClientHandle;
    g_connectionStatusCallback = connectionStatusCallback;
    g_userContextCallback = userContextCallback;
    return my_IoTHubClient_LL_SetConnectionStatusCallback_result;
}

static IOTHUB_CLIENT_RESULT my_IoTHubClientCore_LL_SetDeviceMethodCallback(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, void* userContextCallback)
{
    (void)iotHubClientHandle;
    (void)deviceMethodCallback;
    g_userContextCallback = userContextCallback;
    return IOTHUB_CLIENT_OK;
}

static IOTHUB_CLIENT_RESULT my_IoTHubClientCore_LL_SetDeviceMethodCallback_Ex_result;
static IOTHUB_CLIENT_RESULT my_IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK inboundDeviceMethodCallback, void* userContextCallback)
{
    (void)iotHubClientHandle;
    g_inboundDeviceCallback = inboundDeviceMethodCallback;
    g_userContextCallback = userContextCallback;
    return my_IoTHubClientCore_LL_SetDeviceMethodCallback_Ex_result;
}

static IOTHUB_CLIENT_RESULT my_IoTHubClientCore_LL_GetLastMessageReceiveTime(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, time_t* lastMessageReceiveTime)
{
    (void)iotHubClientHandle;
    *lastMessageReceiveTime = time(NULL);
    return IOTHUB_CLIENT_OK;
}

static IOTHUB_CLIENT_RESULT my_IoTHubClientCore_LL_SetMessageCallback_Ex_result;
static IOTHUB_CLIENT_RESULT my_IoTHubClientCore_LL_SetMessageCallback_Ex(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC_EX messageCallback, void* userContextCallback)
{
    (void)iotHubClientHandle;
    g_messageCallback_ex = messageCallback;
    g_userContextCallback = userContextCallback;
    return my_IoTHubClientCore_LL_SetMessageCallback_Ex_result;
}

static void my_IoTHubClient_LL_Destroy(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle)
{
    (void)iotHubClientHandle;
    if ((g_eventConfirmationCallback != NULL) && (g_userContextCallback)) /*no test ever set the user context to NULL, so we piggyback it*/
    {
        g_eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY, g_userContextCallback);
    }
}

typedef enum METHOD_INVOKE_TEST_TARGET_TAG
{
    METHOD_INVOKE_TEST_TARGET_DEVICE,
    METHOD_INVOKE_TEST_TARGET_MODULE
} METHOD_INVOKE_TEST_TARGET;


static METHOD_INVOKE_TEST_TARGET current_method_invoke_test;

static IOTHUB_CLIENT_RESULT my_IoTHubClientCore_LL_GenericMethodInvoke(IOTHUB_CLIENT_CORE_LL_HANDLE iotHubClientHandle, const char* deviceId, const char* moduleId, const char* methodName, const char* methodPayload, unsigned int timeout, int* responseStatus, unsigned char** responsePayload, size_t* responsePayloadSize)
{
    (void)iotHubClientHandle; (void)deviceId, (void)moduleId, (void)methodName, (void)methodPayload, (void)timeout;

    ASSERT_ARE_EQUAL(char_ptr, deviceId, TEST_DEVICE_ID, "DeviceIDs don't match");

    if (current_method_invoke_test == METHOD_INVOKE_TEST_TARGET_MODULE)
    {
        ASSERT_ARE_EQUAL(char_ptr, moduleId, TEST_MODULE_ID, "ModuleIds don't match");
    }
    else
    {
        ASSERT_IS_NULL(moduleId, "ModuleID should be NULL for device test");
    }

    ASSERT_ARE_EQUAL(char_ptr, methodName, TEST_METHOD_NAME, "Method names match");
    ASSERT_ARE_EQUAL(char_ptr, methodPayload, TEST_METHOD_PAYLOAD, "Method payloads don't match");
    ASSERT_ARE_EQUAL(int, timeout, TEST_INVOKE_TIMEOUT, "Timeouts don't match");

    *responseStatus = REPORTED_STATE_STATUS_CODE;
    *responsePayload = (unsigned char*)TEST_DEVICE_METHOD_RESPONSE;
    *responsePayloadSize = TEST_DEVICE_RESP_LENGTH;

    return IOTHUB_CLIENT_OK;
}

static void test_command_callback_Impl(const IOTHUB_CLIENT_COMMAND_REQUEST* commandRequest, IOTHUB_CLIENT_COMMAND_RESPONSE* commandResponse, void* userContextCallback)
{
    ASSERT_IS_NOT_NULL(commandRequest);
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_COMMAND_REQUEST_STRUCT_VERSION_1, commandRequest->structVersion);
    ASSERT_ARE_EQUAL(char_ptr, TEST_METHOD_NAME, commandRequest->commandName);
    if (g_currentTestComponentName == NULL)
    {
        ASSERT_IS_NULL(commandRequest->componentName);
    }
    else
    {
        ASSERT_ARE_EQUAL(char_ptr, g_currentTestComponentName, commandRequest->componentName);
    }
    ASSERT_ARE_EQUAL(char_ptr, TEST_COMMAND_PAYLOAD, commandRequest->payload);
    ASSERT_ARE_EQUAL(int, TEST_COMMAND_PAYLOAD_LENGTH, commandRequest->payloadLength);
    
    ASSERT_IS_NOT_NULL(commandResponse);
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_COMMAND_RESPONSE_STRUCT_VERSION_1, commandResponse->structVersion);
    ASSERT_IS_NULL(commandResponse->payload);
    ASSERT_ARE_EQUAL(int, 0, commandResponse->payloadLength);
    ASSERT_ARE_EQUAL(int, 500, commandResponse->statusCode);

    ASSERT_ARE_EQUAL(void_ptr, CALLBACK_CONTEXT, userContextCallback);

    commandResponse->payload  = (unsigned char*)my_gballoc_malloc(1);
    commandResponse->payloadLength = 1;
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

BEGIN_TEST_SUITE(iothubclientcore_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    umock_c_init(on_umock_c_error);
    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(VECTOR_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_START_FUNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CORE_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONFIRMATION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(TICK_COUNTER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_TRANSPORT_PROVIDER, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_PROCESS_ITEM_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_STATUS, int);
    REGISTER_UMOCK_ALIAS_TYPE(DEVICE_TWIN_UPDATE_STATE, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, int);
    REGISTER_UMOCK_ALIAS_TYPE(SINGLYLINKEDLIST_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LOCK_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC_EX, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_REPORTED_STATE_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_RETRY_POLICY, int);
    REGISTER_UMOCK_ALIAS_TYPE(LIST_ITEM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(VECTOR_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const VECTOR_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TRANSPORT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CORE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_STATUS, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUBMESSAGE_DISPOSITION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(METHOD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREADAPI_RESULT, int);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_CreateFromConnectionString, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_CreateFromConnectionString, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_Create, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_Create, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_CreateWithTransport, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_CreateWithTransport, NULL);
#ifdef USE_PROV_MODULE
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_CreateFromDeviceAuth, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_CreateFromDeviceAuth, NULL);
#endif
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_SendEventAsync, my_IoTHubClientCore_LL_SendEventAsync);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_SendEventAsync, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_GetSendStatus, my_IoTHubClientCore_LL_GetSendStatus);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_GetSendStatus, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_GetLastMessageReceiveTime, my_IoTHubClientCore_LL_GetLastMessageReceiveTime);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_GetLastMessageReceiveTime, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_SetOption, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_SetOption, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_SetMessageCallback_Ex, my_IoTHubClientCore_LL_SetMessageCallback_Ex);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_SetMessageCallback_Ex, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_SetConnectionStatusCallback, my_IoTHubClient_LL_SetConnectionStatusCallback);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_SetConnectionStatusCallback, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_SetDeviceTwinCallback, my_IoTHubClientCore_LL_SetDeviceTwinCallback);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_SetDeviceTwinCallback, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_GetTwinAsync, my_IoTHubClientCore_LL_GetTwinAsync);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_GetTwinAsync, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_SendReportedState, my_IoTHubClientCore_LL_SendReportedState);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_SendReportedState, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_SetDeviceMethodCallback, my_IoTHubClientCore_LL_SetDeviceMethodCallback);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_SetDeviceMethodCallback, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_SetDeviceMethodCallback_Ex, my_IoTHubClientCore_LL_SetDeviceMethodCallback_Ex);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_SetDeviceMethodCallback_Ex, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_DeviceMethodResponse, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_DeviceMethodResponse, IOTHUB_CLIENT_ERROR);
#ifndef DONT_USE_UPLOADTOBLOB
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_UploadToBlob, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_UploadToBlob, IOTHUB_CLIENT_ERROR);
#endif

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_CreateFromEnvironment, TEST_IOTHUB_CLIENT_CORE_LL_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_CreateFromEnvironment, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(get_time, (time_t)TEST_TIME_VALUE);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_GetRetryPolicy, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_Destroy, my_IoTHubClient_LL_Destroy);
    REGISTER_GLOBAL_MOCK_HOOK(test_event_confirmation_callback, my_test_event_confirmation_callback);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_SendEventToOutputAsync, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_SendEventToOutputAsync, IOTHUB_CLIENT_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_SetInputMessageCallback, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_SetInputMessageCallback, IOTHUB_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_SetInputMessageCallbackEx, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_SetInputMessageCallbackEx, IOTHUB_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetOutputName, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetOutputName, IOTHUB_MESSAGE_ERROR);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_GetRetryPolicy, IOTHUB_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(IoTHubTransport_GetLock, my_IoTHubTransport_GetLock);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubTransport_GetLock, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_GetLLTransport, TEST_TRANSPORT_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubTransport_GetLLTransport, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, real_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_construct, TEST_STRING_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(BUFFER_create, TEST_BUFFER_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(Lock_Init, my_Lock_Init);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Lock_Init, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(Lock_Deinit, my_Lock_Deinit);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Lock_Deinit, LOCK_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(Lock, my_Lock);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Lock, LOCK_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(Unlock, my_Unlock);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Unlock, LOCK_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Sleep, my_ThreadAPI_Sleep);
    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Join, my_ThreadAPI_Join);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ThreadAPI_Join, THREADAPI_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Create, my_ThreadAPI_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ThreadAPI_Create, THREADAPI_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_create, real_VECTOR_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(VECTOR_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_move, real_VECTOR_move);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(VECTOR_move, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_push_back, real_VECTOR_push_back);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(VECTOR_push_back, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_element, real_VECTOR_element);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(VECTOR_element, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_clear, real_VECTOR_clear);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_destroy, real_VECTOR_destroy);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_size, real_VECTOR_size);

    REGISTER_GLOBAL_MOCK_RETURN(singlylinkedlist_create, TEST_SLL_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_create, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(singlylinkedlist_get_head_item, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(singlylinkedlist_add, TEST_LIST_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(singlylinkedlist_add, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubTransport_SignalEndWorkerThread, true);

    REGISTER_GLOBAL_MOCK_HOOK(my_DeviceMethodCallback, my_DeviceMethodCallback_Impl);
    REGISTER_GLOBAL_MOCK_HOOK(test_command_callback, test_command_callback_Impl)

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_SendMessageDisposition, IOTHUB_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_SendMessageDisposition, IOTHUB_CLIENT_ERROR);


    REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_GenericMethodInvoke,  my_IoTHubClientCore_LL_GenericMethodInvoke);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubClientCore_LL_GenericMethodInvoke, IOTHUB_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubClientCore_LL_ParseMethodToCommand, IOTHUB_CLIENT_OK);

}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    TEST_MUTEX_DESTROY(test_serialize_mutex);
}

static void reset_test_data()
{
    g_thread_func = NULL;
    g_thread_func_arg = NULL;
    g_userContextCallback = NULL;
    g_how_thread_loops = 0;
    g_thread_loop_count = 0;

    g_eventConfirmationCallback = NULL;
    g_deviceTwinCallback = NULL;
    g_reportedStateCallback = NULL;
    g_connectionStatusCallback = NULL;
    g_inboundDeviceCallback = NULL;
    g_messageCallback = NULL;
    g_messageCallback_ex = NULL;

    my_IoTHubClientCore_LL_SetDeviceMethodCallback_Ex_result = IOTHUB_CLIENT_OK;
    my_IoTHubClient_LL_SetConnectionStatusCallback_result = IOTHUB_CLIENT_OK;
    my_IoTHubClientCore_LL_SetMessageCallback_Ex_result = IOTHUB_CLIENT_OK;
    g_fail_my_gballoc_malloc = false;
    my_malloc_count = 0;
    memset(my_malloc_items, 0, sizeof(my_malloc_items));

    g_currentTestComponentName = NULL;
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    TEST_MUTEX_ACQUIRE(test_serialize_mutex);
    umock_c_reset_all_calls();
    reset_test_data();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    reset_test_data();
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

static int should_skip_index(size_t current_index, const size_t skip_array[], size_t length)
{
    int result = 0;
    for (size_t index = 0; index < length; index++)
    {
        if (current_index == skip_array[index])
        {
            result = MU_FAILURE;
            break;
        }
    }
    return result;
}

typedef enum CREATE_IOTHUB_TEST_TYPE_TAG
{
    CREATE_IOTHUB_TEST_CREATE,
    CREATE_IOTHUB_TEST_CREATE_FROM_CONNECTION_STRING,
    CREATE_IOTHUB_TEST_CREATE_FROM_ENVIRONMENT
} CREATE_IOTHUB_TEST_TYPE;

static void setup_create_iothub_instance(CREATE_IOTHUB_TEST_TYPE create_iothub_test_type) // bool use_ll_create)
{
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG) );
    STRICT_EXPECTED_CALL(VECTOR_create(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_create());
    STRICT_EXPECTED_CALL(Lock_Init());

    switch (create_iothub_test_type)
    {
        case CREATE_IOTHUB_TEST_CREATE:
            STRICT_EXPECTED_CALL(IoTHubClientCore_LL_Create(TEST_CLIENT_CONFIG));
            break;

        case CREATE_IOTHUB_TEST_CREATE_FROM_CONNECTION_STRING:
            STRICT_EXPECTED_CALL(IoTHubClientCore_LL_CreateFromConnectionString(TEST_CONNECTION_STRING, TEST_TRANSPORT_PROVIDER));
            break;


        case CREATE_IOTHUB_TEST_CREATE_FROM_ENVIRONMENT:
            STRICT_EXPECTED_CALL(IoTHubClientCore_LL_CreateFromEnvironment(TEST_TRANSPORT_PROVIDER));
            break;

        default:
            ASSERT_FAIL("Unknown enum type");
            break;
    }
}


static void setup_iothubclient_createwithtransport()
{
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG) );
    STRICT_EXPECTED_CALL(VECTOR_create(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_create());

    STRICT_EXPECTED_CALL(IoTHubTransport_GetLock(TEST_TRANSPORT_HANDLE));
    STRICT_EXPECTED_CALL(IoTHubTransport_GetLLTransport(TEST_TRANSPORT_HANDLE));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_CreateWithTransport(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).CallCannotFail();
}

#ifdef USE_PROV_MODULE
static void setup_iothubclient_createwithdeviceauth()
{
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG) );
    STRICT_EXPECTED_CALL(VECTOR_create(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_create());
    STRICT_EXPECTED_CALL(Lock_Init());
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_CreateFromDeviceAuth(TEST_IOTHUB_URI, TEST_DEVICE_ID, TEST_TRANSPORT_PROVIDER));
}
#endif

static void setup_iothubclient_sendeventasync(bool use_threads)
{
    if (use_threads)
    {
        EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SendEventAsync(IGNORED_PTR_ARG, TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).CallCannotFail();
}

static void setup_gargageCollection(void* saved_data, bool can_item_be_collected)
{
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(1));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));

    EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_SLL_HANDLE))
        .SetReturn(TEST_LIST_HANDLE);
    EXPECTED_CALL(singlylinkedlist_item_get_value(TEST_LIST_HANDLE))
        .SetReturn(saved_data);
    EXPECTED_CALL(singlylinkedlist_get_next_item(TEST_LIST_HANDLE))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));

    if (can_item_be_collected)
    {
        EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        EXPECTED_CALL(singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        EXPECTED_CALL(free(IGNORED_PTR_ARG));
        EXPECTED_CALL(free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG));
        EXPECTED_CALL(free(IGNORED_PTR_ARG));
        EXPECTED_CALL(free(IGNORED_PTR_ARG));
        EXPECTED_CALL(free(IGNORED_PTR_ARG));
    }
    else
    {
        STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    }
}


static void setup_IothubClient_Destroy_after_garbage_collection()
{
    EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_SLL_HANDLE));
    STRICT_EXPECTED_CALL(singlylinkedlist_destroy(TEST_SLL_HANDLE));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

#ifndef DONT_USE_UPLOADTOBLOB
static void setup_iothubclient_uploadtoblobasync()
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)); /*this is creating a HTTPWORKER_THREAD_INFO*/
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)); /*this is making a copy of the filename*/
    STRICT_EXPECTED_CALL(Lock_Init());
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)); /*this is creating a UPLOADTOBLOB_SAVED_DATA*/
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG)); /*this is adding HTTPWORKER_THREAD_INFO to the list of HTTPWORKER_THREAD_INFO's to be cleaned*/
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(get_time(IGNORED_NUM_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_UploadToBlob(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1)); /*this is the thread calling into _LL layer*/
    STRICT_EXPECTED_CALL(test_file_upload_callback(FILE_UPLOAD_OK, IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Exit(0));
}

#endif

// Initial time we loop through ScheduleWork, including DoWork and into the always run dispatch_user_callbacks functions.
static void set_expected_calls_first_ScheduleWork_Thread_loop(size_t expected_callbacks_length)
{
    STRICT_EXPECTED_CALL(get_time(IGNORED_NUM_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_DoWork(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_SLL_HANDLE));
    STRICT_EXPECTED_CALL(VECTOR_move(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG)).SetReturn(expected_callbacks_length);
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
}

// Final time we loop through ScheduleWork_Thread, from return of dispatch_user_callbacks/sleep to exiting out.
static void set_expected_calls_final_ScheduleWork_Thread_loop()
{
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(1));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Exit(0));
}

static void set_expected_calls_nocallbacks_Schedule_Thread_loop()
{
    set_expected_calls_first_ScheduleWork_Thread_loop(0);
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();
}

TEST_FUNCTION(IoTHubClientCore_CreateFromConnectionString_connection_string_NULL_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_CORE_HANDLE result = IoTHubClientCore_CreateFromConnectionString(NULL, TEST_TRANSPORT_PROVIDER);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_CreateFromConnectionString_transport_provider_NULL_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_CORE_HANDLE result = IoTHubClientCore_CreateFromConnectionString(TEST_CONNECTION_STRING, NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_CreateFromConnectionString_succeeds)
{
    // arrange
    setup_create_iothub_instance(CREATE_IOTHUB_TEST_CREATE_FROM_CONNECTION_STRING);

    // act
    IOTHUB_CLIENT_CORE_HANDLE result = IoTHubClientCore_CreateFromConnectionString(TEST_CONNECTION_STRING, TEST_TRANSPORT_PROVIDER);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(result);
}

TEST_FUNCTION(IoTHubClientCore_CreateFromConnectionString_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_create_iothub_instance(CREATE_IOTHUB_TEST_CREATE_FROM_CONNECTION_STRING);

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubClientCore_CreateFromConnectionString failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        IOTHUB_CLIENT_CORE_HANDLE result = IoTHubClientCore_CreateFromConnectionString(TEST_CONNECTION_STRING, TEST_TRANSPORT_PROVIDER);

        // assert
        ASSERT_IS_NULL(result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClientCore_Create_client_config_NULL_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_CORE_HANDLE result = IoTHubClientCore_Create(NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_Create_client_succeed)
{
    // arrange
    setup_create_iothub_instance(CREATE_IOTHUB_TEST_CREATE);

    // act
    IOTHUB_CLIENT_CORE_HANDLE result = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(result);
}

TEST_FUNCTION(IoTHubClientCore_Create_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_create_iothub_instance(CREATE_IOTHUB_TEST_CREATE);

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubClientCore_Create failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        IOTHUB_CLIENT_CORE_HANDLE result = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);

        // assert
        ASSERT_IS_NULL(result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClientCore_CreateWithTransport_transport_handle_NULL_fail)
{
    // arrange
    IOTHUB_CLIENT_CONFIG client_config;
    client_config.deviceId = TEST_DEVICE_ID;
    client_config.deviceKey = TEST_DEVICE_KEY;
    client_config.deviceSasToken = TEST_DEVICE_SAS;
    client_config.protocol = TEST_TRANSPORT_PROVIDER;

    // act
    IOTHUB_CLIENT_CORE_HANDLE result = IoTHubClientCore_CreateWithTransport(NULL, &client_config);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_CreateWithTransport_client_config_NULL_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_CORE_HANDLE result = IoTHubClientCore_CreateWithTransport(TEST_TRANSPORT_HANDLE, NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_CreateWithTransport_succeed)
{
    // arrange
    setup_iothubclient_createwithtransport();

    IOTHUB_CLIENT_CONFIG client_config;
    client_config.deviceId = TEST_DEVICE_ID;
    client_config.deviceKey = TEST_DEVICE_KEY;
    client_config.deviceSasToken = TEST_DEVICE_SAS;
    client_config.protocol = TEST_TRANSPORT_PROVIDER;

    // act
    IOTHUB_CLIENT_CORE_HANDLE result = IoTHubClientCore_CreateWithTransport(TEST_TRANSPORT_HANDLE, &client_config);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(result);
}

TEST_FUNCTION(IoTHubClientCore_CreateWithTransport_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_iothubclient_createwithtransport();

    umock_c_negative_tests_snapshot();

    IOTHUB_CLIENT_CONFIG client_config;
    client_config.deviceId = TEST_DEVICE_ID;
    client_config.deviceKey = TEST_DEVICE_KEY;
    client_config.deviceSasToken = TEST_DEVICE_SAS;
    client_config.protocol = TEST_TRANSPORT_PROVIDER;

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubClientCore_CreateWithTransport failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
            IOTHUB_CLIENT_CORE_HANDLE result = IoTHubClientCore_CreateWithTransport(TEST_TRANSPORT_HANDLE, &client_config);

            // assert
            ASSERT_IS_NULL(result, tmp_msg);
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

#ifdef USE_PROV_MODULE

TEST_FUNCTION(IoTHubClientCore_CreateFromDeviceAuth_iothub_uri_NULL_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_CORE_HANDLE result = IoTHubClientCore_CreateFromDeviceAuth(NULL, TEST_DEVICE_ID, TEST_TRANSPORT_PROVIDER);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_CreateFromDeviceAuth_device_id_NULL_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_CORE_HANDLE result = IoTHubClientCore_CreateFromDeviceAuth(TEST_IOTHUB_URI, NULL, TEST_TRANSPORT_PROVIDER);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_CreateFromDeviceAuth_transport_NULL_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_CORE_HANDLE result = IoTHubClientCore_CreateFromDeviceAuth(TEST_IOTHUB_URI, TEST_DEVICE_ID, NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}


TEST_FUNCTION(IoTHubClientCore_CreateFromDeviceAuth_succeed)
{
    // arrange
    setup_iothubclient_createwithdeviceauth();

    // act
    IOTHUB_CLIENT_CORE_HANDLE result = IoTHubClientCore_CreateFromDeviceAuth(TEST_IOTHUB_URI, TEST_DEVICE_ID, TEST_TRANSPORT_PROVIDER);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(result);
}

TEST_FUNCTION(IoTHubClientCore_CreateFromDeviceAuth_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_iothubclient_createwithdeviceauth();

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubClientCore_CreateFromDeviceAuth failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        IOTHUB_CLIENT_CORE_HANDLE result = IoTHubClientCore_CreateFromDeviceAuth(TEST_IOTHUB_URI, TEST_DEVICE_ID, TEST_TRANSPORT_PROVIDER);

        // assert
        ASSERT_IS_NULL(result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}
#endif


TEST_FUNCTION(IoTHubClientCore_CreateFromEnvironment_succeed)
{
    // arrange
    setup_create_iothub_instance(CREATE_IOTHUB_TEST_CREATE_FROM_ENVIRONMENT);

    // act
    IOTHUB_CLIENT_CORE_HANDLE result = IoTHubClientCore_CreateFromEnvironment(TEST_TRANSPORT_PROVIDER);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(result);
}

TEST_FUNCTION(IoTHubClientCore_CreateFromEnvironment_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_create_iothub_instance(CREATE_IOTHUB_TEST_CREATE_FROM_ENVIRONMENT);

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubClientCore_CreateFromEnvironment failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        IOTHUB_CLIENT_CORE_HANDLE result = IoTHubClientCore_CreateFromEnvironment(TEST_TRANSPORT_PROVIDER);

        // assert
        ASSERT_IS_NULL(result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}



TEST_FUNCTION(IoTHubClientCore_Destroy_iothub_client_handle_NULL_fail)
{
    // arrange

    // act
    IoTHubClientCore_Destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_Destroy_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    // signal threads to end
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // garbage collection
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_SLL_HANDLE));
    STRICT_EXPECTED_CALL(singlylinkedlist_destroy(TEST_SLL_HANDLE));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG) );

    // act
    IoTHubClientCore_Destroy(iothub_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_Destroy_calls_IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    setup_iothubclient_sendeventasync(true);
    (void)IoTHubClientCore_SendEventAsync(iothub_handle, (IOTHUB_MESSAGE_HANDLE)0x42, test_event_confirmation_callback, (void*)0x42);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_SLL_HANDLE));
    STRICT_EXPECTED_CALL(singlylinkedlist_destroy(TEST_SLL_HANDLE));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_Destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG,0));
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY, (void*)0x42));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));


    // act
    IoTHubClientCore_Destroy(iothub_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_Destroy_calls_pending_reported_state_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    const unsigned char* reported_state = (const unsigned char*)0x1234;
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SendReportedState(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, reported_state, 1, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    (void)IoTHubClientCore_SendReportedState(iothub_handle, reported_state, 1, test_report_state_callback, NULL);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_SLL_HANDLE));
    STRICT_EXPECTED_CALL(singlylinkedlist_destroy(TEST_SLL_HANDLE));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG,0));
    STRICT_EXPECTED_CALL(test_report_state_callback(0, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    // act
    g_reportedStateCallback(0, g_userContextCallback); // Simulating a cb from LL with unsent reported state. 
    IoTHubClientCore_Destroy(iothub_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClient_SendEventAsync_handle_NULL_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SendEventAsync(NULL, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_SendEventAsync_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    setup_iothubclient_sendeventasync(true);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SendEventAsync(iothub_handle, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_SendEventAsync_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_iothubclient_sendeventasync(false);

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubClientCore_SendEventAsync failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
            IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SendEventAsync(iothub_handle, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, NULL);

            // assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_SendEventAsync_event_confirm_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SendEventAsync(iothub_handle, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG) );

    // act
    g_eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_OK, g_userContextCallback);

    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(1);
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0));
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_OK, NULL));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    ASSERT_IS_NOT_NULL(g_thread_func);
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_SendEventAsync_event_confirm_callback_two_events_succeed)
{
    // arrange
    void* userContextCallback1;
    void* userContextCallback2;

    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);

    (void)IoTHubClientCore_SendEventAsync(iothub_handle, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, CALLBACK_CONTEXT);
    // We need to save off the reported state context here because next call overwrites the global g_userContextCallback.
    userContextCallback1 = g_userContextCallback;

    
    (void)IoTHubClientCore_SendEventAsync(iothub_handle, TEST_MESSAGE_HANDLE, test_event_confirmation_callback2, CALLBACK_CONTEXT2);
    userContextCallback2 = g_userContextCallback;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG) );

    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG) );

    // act
    g_eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_OK, userContextCallback1);
    g_eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_OK, userContextCallback2);

    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(2);
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0));
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_OK, CALLBACK_CONTEXT));

    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 1));
    STRICT_EXPECTED_CALL(test_event_confirmation_callback2(IOTHUB_CLIENT_CONFIRMATION_OK, CALLBACK_CONTEXT2));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    ASSERT_IS_NOT_NULL(g_thread_func);
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_GetSendStatus_iothub_handle_NULL_fail)
{
    // arrange
    IOTHUB_CLIENT_STATUS iothub_status;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GetSendStatus(NULL, &iothub_status);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_GetSendStatus_lock_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_STATUS iothub_status;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).SetReturn(LOCK_ERROR);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GetSendStatus(iothub_handle, &iothub_status);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_GetSendStatus_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_STATUS iothub_status;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_GetSendStatus(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, &iothub_status));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GetSendStatus(iothub_handle, &iothub_status);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetMessageCallback_client_handle_NULL_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetMessageCallback(NULL, test_message_confirmation_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_SetMessageCallback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetMessageCallback_Ex(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetMessageCallback(iothub_handle, test_message_confirmation_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetMessageCallback_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetMessageCallback_Ex(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (index == 4)
        {
            continue;
        }
        else if (index == 2)
        {
            g_fail_my_gballoc_malloc = true;
        }
        else if (index == 3)
        {
            my_IoTHubClientCore_LL_SetMessageCallback_Ex_result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            g_fail_my_gballoc_malloc = false;
            my_IoTHubClientCore_LL_SetMessageCallback_Ex_result = IOTHUB_CLIENT_OK;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubClientCore_SetMessageCallback failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetMessageCallback(iothub_handle, test_message_confirmation_callback, NULL);

        // assert
        ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClientCore_SetMessageCallback_fails_if_SetMessageCallback_Ex_fails)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetMessageCallback_Ex(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(IOTHUB_CLIENT_ERROR);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetMessageCallback(iothub_handle, test_message_confirmation_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetConnectionStatusCallback_client_handle_NULL_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetConnectionStatusCallback(NULL, test_connection_status_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_SetConnectionStatusCallback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetConnectionStatusCallback(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetConnectionStatusCallback(iothub_handle, test_connection_status_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetConnectionStatusCallback_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetConnectionStatusCallback(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (index == 4)
        {
            continue;
        }
        else if (index == 2)
        {
            g_fail_my_gballoc_malloc = true;
        }
        else if (index == 3)
        {
            my_IoTHubClient_LL_SetConnectionStatusCallback_result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            my_IoTHubClient_LL_SetConnectionStatusCallback_result = IOTHUB_CLIENT_OK;
            g_fail_my_gballoc_malloc = false;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubClient_SetConnectionStatusCallback failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetConnectionStatusCallback(iothub_handle, test_connection_status_callback, NULL);

        // assert
        ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClientCore_SetRetryPolicy_client_handle_fail)
{
    // arrange
    IOTHUB_CLIENT_RETRY_POLICY retry_policy = IOTHUB_CLIENT_RETRY_RANDOM;
    size_t retry_in_seconds = 10;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetRetryPolicy(NULL, retry_policy, retry_in_seconds);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_SetRetryPolicy_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_RETRY_POLICY retry_policy = IOTHUB_CLIENT_RETRY_RANDOM;

    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetRetryPolicy(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, retry_policy, IGNORED_NUM_ARG))
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetRetryPolicy(iothub_handle, retry_policy, IGNORED_NUM_ARG);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_GetRetryPolicy_client_handle_NULL_fail)
{
    // arrange
    IOTHUB_CLIENT_RETRY_POLICY retry_policy;
    size_t retry_in_seconds;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GetRetryPolicy(NULL, &retry_policy, &retry_in_seconds);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_GetRetryPolicy_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_RETRY_POLICY retry_policy;
    size_t retry_in_seconds;

    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_GetRetryPolicy(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, &retry_policy, &retry_in_seconds));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GetRetryPolicy(iothub_handle, &retry_policy, &retry_in_seconds);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_GetRetryPolicy_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_RETRY_POLICY retry_policy;
    size_t retry_in_seconds;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_GetRetryPolicy(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, &retry_policy, &retry_in_seconds));

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubClientCore_CreateFromConnectionString failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GetRetryPolicy(iothub_handle, &retry_policy, &retry_in_seconds);

        // assert
        ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_GetLastMessageReceiveTime_client_handle_NULL_fail)
{
    // arrange
    time_t recv_time;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GetLastMessageReceiveTime(NULL, &recv_time);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_GetLastMessageReceiveTime_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    time_t recv_time;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_GetLastMessageReceiveTime(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, &recv_time));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GetLastMessageReceiveTime(iothub_handle, &recv_time);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_GetLastMessageReceiveTime_failed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    time_t recv_time;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_GetLastMessageReceiveTime(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, &recv_time));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).CallCannotFail();

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubClientCore_GetLastMessageReceiveTime failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
            IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GetLastMessageReceiveTime(iothub_handle, &recv_time);

            // assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetOption_client_handle_NULL_fail)
{
    // arrange
    const char* option_name = "option_name";
    const void* option_value = (void*)0x0123;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetOption(NULL, option_name, option_value);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_SetOption_option_name_NULL_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    const void* option_value = (void*)0x0123;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetOption(iothub_handle, NULL, option_value);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetOption_option_value_NULL_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    const char* option_name = "option_name";

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetOption(iothub_handle, option_name, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetOption_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    const char* option_name = "option_name";
    const void* option_value = (void*)0x0123;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetOption(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, option_name, option_value));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetOption(iothub_handle, option_name, option_value);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetOption_DO_WORK_FREQUENCY_IN_MS_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    const char* option_name = "do_work_freq_ms";
    tickcounter_ms_t tickcounter_value = 100;
    const void* option_value = (void*) &tickcounter_value;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetOption(iothub_handle, option_name, option_value);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetOption_DO_WORK_FREQUENCY_IN_MS_value_limits_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    const char* option_name = "do_work_freq_ms";
    tickcounter_ms_t tickcounter_value_low = 0;
    const void* option_value_low = (void*) &tickcounter_value_low;
    tickcounter_ms_t tickcounter_value_high = 200;
    const void* option_value_high = (void*) &tickcounter_value_high;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result_low = IoTHubClientCore_SetOption(iothub_handle, option_name, option_value_low);
    IOTHUB_CLIENT_RESULT result_high = IoTHubClientCore_SetOption(iothub_handle, option_name, option_value_high);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result_low);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result_high);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetOption_DO_WORK_FREQUENCY_IN_MS_and_MESSAGE_TIMEOUT_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    const char* freq_option = "do_work_freq_ms";
    const char* timeout_option = "messageTimeout";

    tickcounter_ms_t frequency = 30; // arbitrary
    tickcounter_ms_t timeout = 20; // arbitrary; just lower than frequency
    // const void* option_value = (void*) &frequency;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetOption(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, timeout_option, &timeout));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT timeout_result = IoTHubClientCore_SetOption(iothub_handle, timeout_option, &timeout);
    IOTHUB_CLIENT_RESULT freq_result = IoTHubClientCore_SetOption(iothub_handle, freq_option, &frequency);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, timeout_result);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, freq_result);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_DO_WORK_FREQ_IN_MS_success)
{
    
    const char* option_name = "do_work_freq_ms";
    tickcounter_ms_t tickcounter_value = 57;
    const void* option_value = (void*) &tickcounter_value;

    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetOption(iothub_handle, option_name, option_value);

     (void)IoTHubClientCore_SetDeviceMethodCallback(iothub_handle, test_method_callback, CALLBACK_CONTEXT);
     (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, g_userContextCallback);
    umock_c_reset_all_calls();
    g_how_thread_loops = 1;

    STRICT_EXPECTED_CALL(get_time(IGNORED_NUM_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_DoWork(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_SLL_HANDLE));
    STRICT_EXPECTED_CALL(VECTOR_move(IGNORED_PTR_ARG)).SetReturn(NULL);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(57));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Exit(0));

    // act
	ASSERT_IS_NOT_NULL(g_thread_func);
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}


TEST_FUNCTION(IoTHubClientCore_SetOption_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    const char* option_name = "option_name";
    const void* option_value = (void*)0x0123;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetOption(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, option_name, option_value));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).CallCannotFail();

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubClientCore_SetOption failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
            IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetOption(iothub_handle, option_name, option_value);

            // assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetDeviceTwinCallback_client_handle_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetDeviceTwinCallback(NULL, test_device_twin_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_SetDeviceTwinCallback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetDeviceTwinCallback(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetDeviceTwinCallback(iothub_handle, test_device_twin_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetDeviceTwinCallback_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetDeviceTwinCallback(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).CallCannotFail();

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubClientCore_SetDeviceTwinCallback failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
            IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetDeviceTwinCallback(iothub_handle, test_device_twin_callback, NULL);

            // assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetDeviceTwinCallback_device_twin_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetDeviceTwinCallback(iothub_handle, test_device_twin_callback, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1));

    // act
    g_deviceTwinCallback(DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0, g_userContextCallback);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SendReportedState_client_handle_NULL_fail)
{
    // arrange
    const unsigned char* reported_state = (const unsigned char*)0x1234;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SendReportedState(NULL, reported_state, 1, test_report_state_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_SendReportedState_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    const unsigned char* reported_state = (const unsigned char*)0x1234;

    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SendReportedState(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, reported_state, 1, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SendReportedState(iothub_handle, reported_state, 1, test_report_state_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
    my_gballoc_free(g_userContextCallback);
}

TEST_FUNCTION(IoTHubClientCore_SendReportedState_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    const unsigned char* reported_state = (const unsigned char*)0x1234;

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SendReportedState(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, reported_state, 1, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).CallCannotFail();

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubClientCore_SendReportedState failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
            IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SendReportedState(iothub_handle, reported_state, 1, test_report_state_callback, NULL);

            // assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
        }
    }

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClientCore_SendReportedState_report_state_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    const unsigned char* reported_state = (const unsigned char*)0x1234;
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SendReportedState(iothub_handle, reported_state, 1, test_report_state_callback, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG) );

    // act
    g_reportedStateCallback(REPORTED_STATE_STATUS_CODE, g_userContextCallback);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_GetTwinAsync_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);

    umock_c_reset_all_calls();
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_GetTwinAsync(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GetTwinAsync(iothub_handle, test_device_twin_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    // cleanup
    my_gballoc_free(g_userContextCallback);
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_GetTwinAsync_NULL_handle_fail)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GetTwinAsync(NULL, test_device_twin_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_GetTwinAsync_NULL_callback_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);

    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GetTwinAsync(iothub_handle, NULL, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_GetTwinAsync_fail)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_GetTwinAsync(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG)).CallCannotFail();
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();

    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "Failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
            IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GetTwinAsync(iothub_handle, test_device_twin_callback, NULL);

            // assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
        }
    }

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClientCore_GetTwinAsync_startWorkerThread_fail)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);

    umock_c_reset_all_calls();
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();

    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "Failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GetTwinAsync(iothub_handle, test_device_twin_callback, NULL);

        // assert
        ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubClientCore_SetDeviceMethodCallback_iothub_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetDeviceMethodCallback(NULL, test_method_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_SetDeviceMethodCallback_already_subscribed_to_command_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SubscribeToCommands(iothub_handle, test_command_callback, NULL);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    result = IoTHubClientCore_SetDeviceMethodCallback(iothub_handle, test_method_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetDeviceMethodCallback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetDeviceMethodCallback(iothub_handle, test_method_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

/* Tests_*SRS_IOTHUBCLIENT_12_013: [ If acquiring the lock fails, IoTHubClientCore_SetDeviceMethodCallback shall return IOTHUB_CLIENT_ERROR. ]*/
TEST_FUNCTION(IoTHubClientCore_SetDeviceMethodCallback_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    umock_c_reset_all_calls();
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count() - 1;
    for (size_t index = 0; index < count; index++)
    {
        my_IoTHubClientCore_LL_SetDeviceMethodCallback_Ex_result = IOTHUB_CLIENT_OK;

        if (index == 2)
        {
            continue;
        }
        else if (index == 3)
        {
            my_IoTHubClientCore_LL_SetDeviceMethodCallback_Ex_result = IOTHUB_CLIENT_ERROR;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[99];
        sprintf(tmp_msg, "IoTHubClientCore_SetDeviceMethodCallback failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetDeviceMethodCallback(iothub_handle, test_method_callback, NULL);

        // assert
        ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetDeviceMethodCallback_Ex_handle_NULL)
{
    // arrange

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetDeviceMethodCallback_Ex(NULL, test_incoming_method_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_SetDeviceMethodCallback_Ex_already_subscribed_to_command_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SubscribeToCommands(iothub_handle, test_command_callback, NULL);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    result = IoTHubClientCore_SetDeviceMethodCallback_Ex(iothub_handle, test_incoming_method_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetDeviceMethodCallback_Ex_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetDeviceMethodCallback_Ex(iothub_handle, test_incoming_method_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetDeviceMethodCallback_Ex_NonNULL_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetDeviceMethodCallback_Ex(iothub_handle, test_incoming_method_callback, CALLBACK_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_call_inbound_device_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetDeviceMethodCallback_Ex(iothub_handle, test_incoming_method_callback, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    // act
    ASSERT_IS_NOT_NULL(g_inboundDeviceCallback);
    int result = g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, g_userContextCallback);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_call_inbound_device_callback_usercontext_NULL)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetDeviceMethodCallback_Ex(iothub_handle, test_incoming_method_callback, NULL);
    umock_c_reset_all_calls();

    // act
    ASSERT_IS_NOT_NULL(g_inboundDeviceCallback);
    int result = g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetDeviceMethodCallback_Ex_remove_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetDeviceMethodCallback_Ex(iothub_handle, NULL, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SubscribeToCommands_iothub_NULL_handle)
{
    // arrange

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SubscribeToCommands(NULL, test_command_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_SubscribeToCommands_already_subscribed_to_method_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetDeviceMethodCallback(iothub_handle, test_method_callback, NULL);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    result = IoTHubClientCore_SubscribeToCommands(iothub_handle, test_command_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}


TEST_FUNCTION(IoTHubClientCore_SubscribeToCommands_already_subscribed_to_method_ex_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetDeviceMethodCallback_Ex(iothub_handle, test_incoming_method_callback, NULL);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    result = IoTHubClientCore_SubscribeToCommands(iothub_handle, test_command_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SubscribeToCommands_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SubscribeToCommands(iothub_handle, test_command_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SubscribeToCommands_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    umock_c_reset_all_calls();
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count() - 1;
    for (size_t index = 0; index < count; index++)
    {
        my_IoTHubClientCore_LL_SetDeviceMethodCallback_Ex_result = IOTHUB_CLIENT_OK;

        if (index == 2)
        {
            continue;
        }
        else if (index == 3)
        {
            my_IoTHubClientCore_LL_SetDeviceMethodCallback_Ex_result = IOTHUB_CLIENT_ERROR;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[99];
        sprintf(tmp_msg, "IoTHubClientCore_SubscribeToCommands failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SubscribeToCommands(iothub_handle, test_command_callback, NULL);

        // assert
        ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_call_inbound_command_with_component_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    IOTHUB_CLIENT_RESULT clientResult = IoTHubClientCore_SubscribeToCommands(iothub_handle, test_command_callback, NULL);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, clientResult);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    g_currentTestComponentName = TEST_COMPONENT_NAME;

    // act
    ASSERT_IS_NOT_NULL(g_inboundDeviceCallback);
    int result = g_inboundDeviceCallback(TEST_METHOD_NAME_WITH_COMPONENT, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, g_userContextCallback);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_call_inbound_command_no_component_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SubscribeToCommands(iothub_handle, test_command_callback, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    // act
    ASSERT_IS_NOT_NULL(g_inboundDeviceCallback);
    int result = g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, g_userContextCallback);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}


TEST_FUNCTION(IoTHubClient_call_inbound_command_usercontext_NULL)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SubscribeToCommands(iothub_handle, test_command_callback, NULL);
    umock_c_reset_all_calls();

    // act
    ASSERT_IS_NOT_NULL(g_inboundDeviceCallback);
    int result = g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SubscribeToCommands_remove_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetDeviceMethodCallback_Ex(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SubscribeToCommands(iothub_handle, NULL, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_DeviceMethodResponse_handle_NULL_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_DeviceMethodResponse(NULL, TEST_METHOD_ID, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, REPORTED_STATE_STATUS_CODE);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_DeviceMethodResponse_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_DeviceMethodResponse(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, TEST_METHOD_ID, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, REPORTED_STATE_STATUS_CODE));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_DeviceMethodResponse(iothub_handle, TEST_METHOD_ID, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, REPORTED_STATE_STATUS_CODE);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_DeviceMethodResponse_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_DeviceMethodResponse(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, TEST_METHOD_ID, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, REPORTED_STATE_STATUS_CODE));

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubClientCore_DeviceMethodResponse failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        IOTHUB_CLIENT_RESULT result = IoTHubClientCore_DeviceMethodResponse(iothub_handle, TEST_METHOD_ID, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, REPORTED_STATE_STATUS_CODE);

        // assert
        ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClientCore_Destroy(iothub_handle);
}

#ifndef DONT_USE_UPLOADTOBLOB
TEST_FUNCTION(IoTHubClientCore_UploadToBlobAsync_with_NULL_iotHubClientHandle_fails)
{
    //arrange
    IOTHUB_CLIENT_RESULT result;

    //act
    result = IoTHubClientCore_UploadToBlobAsync(NULL, "a", (const unsigned char*)"b", 1, NULL, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClientCore_UploadToBlobAsync_with_NULL_destinationFileName_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_UploadToBlobAsync(iothub_handle, NULL, (const unsigned char*)"b", 1, NULL, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_UploadToBlobAsync_with_NULL_source_and_size_1_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_UploadToBlobAsync(iothub_handle, "someFileName.txt", NULL, 1, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_UploadToBlobAsync_succeeds)
{
    //arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    setup_iothubclient_uploadtoblobasync();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_UploadToBlobAsync(iothub_handle, "someFileName.txt", (const unsigned char*)"a", 1, test_file_upload_callback, (void*)1);

    g_thread_func(g_thread_func_arg); /*this is the thread uploading function*/

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_SLL_HANDLE))
        .SetReturn(TEST_LIST_HANDLE);

    setup_gargageCollection(my_malloc_items[2], true);
    setup_IothubClient_Destroy_after_garbage_collection();

    IoTHubClientCore_Destroy(iothub_handle);
}

static void set_expected_calls_for_freeUploadToBlobThreadInfo()
{
    STRICT_EXPECTED_CALL(Lock_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_allocateUploadToBlob()
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock_Init());
}

TEST_FUNCTION(IoTHubClientCore_UploadMultipleBlocksToBlobAsync_fails_when_handle_is_NULL)
{
    ///arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = NULL;
    TEST_GETDATA_CALLBACK_ARGS context = { 0 };
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_UploadMultipleBlocksToBlobAsync(iothub_handle, "someFileName.txt", my_FileUpload_GetData_Callback, NULL, &context);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_UploadMultipleBlocksToBlobAsync_fails_when_destinationFileName_is_NULL)
{
    ///arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    TEST_GETDATA_CALLBACK_ARGS context = { 0 };
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_UploadMultipleBlocksToBlobAsync(iothub_handle, NULL, my_FileUpload_GetData_Callback, NULL, &context);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_UploadMultipleBlocksToBlobAsync_fails_when_getDataCallback_is_NULL)
{
    ///arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    int context = 1;
    umock_c_reset_all_calls();

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_UploadMultipleBlocksToBlobAsync(iothub_handle, "someFileName.txt", NULL, NULL, &context);

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

static void IoTHubClientCore_UploadMultipleBlocksToBlobAsync_succeeds_Impl(bool exCall)
{
    ///arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    TEST_GETDATA_CALLBACK_ARGS context =  { 0 };
    umock_c_reset_all_calls();

    set_expected_calls_for_allocateUploadToBlob();
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG)); /*this is adding HTTPWORKER_THREAD_INFO to the list of HTTPWORKER_THREAD_INFO's to be cleaned*/
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(get_time(IGNORED_NUM_ARG)).CallCannotFail();

    /* thread uploading function */
    if (exCall)
    {
        STRICT_EXPECTED_CALL(IoTHubClientCore_LL_UploadMultipleBlocksToBlobEx(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    else
    {
        STRICT_EXPECTED_CALL(IoTHubClientCore_LL_UploadMultipleBlocksToBlob(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Exit(0));

    ///act
    IOTHUB_CLIENT_RESULT result;
    if (exCall)
    {
        result = IoTHubClientCore_UploadMultipleBlocksToBlobAsync(iothub_handle, "someFileName.txt", NULL, my_FileUpload_GetData_CallbackEx, &context);
    }
    else
    {
        result = IoTHubClientCore_UploadMultipleBlocksToBlobAsync(iothub_handle, "someFileName.txt", my_FileUpload_GetData_Callback, NULL, &context);
    }
    g_thread_func(g_thread_func_arg); /*this is the thread uploading function*/

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_SLL_HANDLE))
        .SetReturn(TEST_LIST_HANDLE);

    setup_gargageCollection(my_malloc_items[2], true);
    setup_IothubClient_Destroy_after_garbage_collection();

    IoTHubClientCore_Destroy(iothub_handle);

}

TEST_FUNCTION(IoTHubClientCore_UploadMultipleBlocksToBlobAsync_succeeds)
{
    IoTHubClientCore_UploadMultipleBlocksToBlobAsync_succeeds_Impl(false);
}

TEST_FUNCTION(IoTHubClientCore_UploadMultipleBlocksToBlobAsyncEx_succeeds)
{
    IoTHubClientCore_UploadMultipleBlocksToBlobAsync_succeeds_Impl(true);
}


static void IoTHubClientCore_UploadMultipleBlocksToBlobAsync_fails_when_ThreadAPI_Create_fails_Impl(bool exCall)
{
    ///arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    TEST_GETDATA_CALLBACK_ARGS context = { 0 };
    umock_c_reset_all_calls();

    set_expected_calls_for_allocateUploadToBlob();
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG)); /*this is adding HTTPWORKER_THREAD_INFO to the list of HTTPWORKER_THREAD_INFO's to be cleaned*/

    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(THREADAPI_ERROR);
    STRICT_EXPECTED_CALL(singlylinkedlist_remove(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    set_expected_calls_for_freeUploadToBlobThreadInfo();

    ///act
    IOTHUB_CLIENT_RESULT result;

    if (exCall)
    {
        result = IoTHubClientCore_UploadMultipleBlocksToBlobAsync(iothub_handle, "someFileName.txt", NULL, my_FileUpload_GetData_CallbackEx, &context);
    }
    else
    {
        result = IoTHubClientCore_UploadMultipleBlocksToBlobAsync(iothub_handle, "someFileName.txt", my_FileUpload_GetData_Callback, NULL, &context);
    }

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}


TEST_FUNCTION(IoTHubClientCore_UploadMultipleBlocksToBlobAsync_fails_when_ThreadAPI_Create_fails)
{
    IoTHubClientCore_UploadMultipleBlocksToBlobAsync_fails_when_ThreadAPI_Create_fails_Impl(false);
}

TEST_FUNCTION(IoTHubClientCore_UploadMultipleBlocksToBlobAsyncEx_fails_when_ThreadAPI_Create_fails)
{
    IoTHubClientCore_UploadMultipleBlocksToBlobAsync_fails_when_ThreadAPI_Create_fails_Impl(true);
}

static void IoTHubClientCore_UploadMultipleBlocksToBlobAsync_fails_when_strcpy_fails_Impl(bool exCall)
{
    ///arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    TEST_GETDATA_CALLBACK_ARGS context = { 0 };
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(1);
    set_expected_calls_for_freeUploadToBlobThreadInfo();

    ///act
    IOTHUB_CLIENT_RESULT result;

    if (exCall)
    {
        result = IoTHubClientCore_UploadMultipleBlocksToBlobAsync(iothub_handle, "someFileName.txt", NULL, my_FileUpload_GetData_CallbackEx, &context);
    }
    else
    {
        result = IoTHubClientCore_UploadMultipleBlocksToBlobAsync(iothub_handle, "someFileName.txt", my_FileUpload_GetData_Callback, NULL, &context);
    }

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}


TEST_FUNCTION(IoTHubClientCore_UploadMultipleBlocksToBlobAsync_fails_when_strcpy_fails)
{
    IoTHubClientCore_UploadMultipleBlocksToBlobAsync_fails_when_strcpy_fails_Impl(false);
}

TEST_FUNCTION(IoTHubClientCore_UploadMultipleBlocksToBlobAsyncEx_fails_when_strcpy_fails)
{
    IoTHubClientCore_UploadMultipleBlocksToBlobAsync_fails_when_strcpy_fails_Impl(true);
}

static void IoTHubClientCore_UploadMultipleBlocksToBlobAsync_fails_when_malloc_fails_Impl(bool exCall)
{
    ///arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    TEST_GETDATA_CALLBACK_ARGS context = { 0 };
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .SetReturn(NULL);

    ///act
    IOTHUB_CLIENT_RESULT result;

    if (exCall)
    {
        result = IoTHubClientCore_UploadMultipleBlocksToBlobAsync(iothub_handle, "someFileName.txt", NULL, my_FileUpload_GetData_CallbackEx, &context);
    }
    else
    {
        result = IoTHubClientCore_UploadMultipleBlocksToBlobAsync(iothub_handle, "someFileName.txt", my_FileUpload_GetData_Callback, NULL, &context);
    }

    ///assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}


TEST_FUNCTION(IoTHubClientCore_UploadMultipleBlocksToBlobAsync_fails_when_malloc_fails)
{
    IoTHubClientCore_UploadMultipleBlocksToBlobAsync_fails_when_malloc_fails_Impl(false);
}

TEST_FUNCTION(IoTHubClientCore_UploadMultipleBlocksToBlobAsyncEx_fails_when_malloc_fails)
{
    IoTHubClientCore_UploadMultipleBlocksToBlobAsync_fails_when_malloc_fails_Impl(true);
}

TEST_FUNCTION(IoTHubClientCore_UploadMultipleBlocksToBlobAsync_callback_invoked_on_error_return_if_not_already)
{
    ///arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    TEST_GETDATA_CALLBACK_ARGS context = { 0 };
    umock_c_reset_all_calls();

    set_expected_calls_for_allocateUploadToBlob();
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG)); /*this is adding HTTPWORKER_THREAD_INFO to the list of HTTPWORKER_THREAD_INFO's to be cleaned*/
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(get_time(IGNORED_NUM_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_UploadMultipleBlocksToBlob(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(IOTHUB_CLIENT_ERROR);

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Exit(0));

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_UploadMultipleBlocksToBlobAsync(iothub_handle, "someFileName.txt", my_FileUpload_GetData_Callback, NULL, &context);
    g_thread_func(g_thread_func_arg); /*this is the thread uploading function*/

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 1, context.invokation_counter);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, FILE_UPLOAD_ERROR, context.last_result);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    ///cleanup
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_SLL_HANDLE))
        .SetReturn(TEST_LIST_HANDLE);

    setup_gargageCollection(my_malloc_items[2], true);
    setup_IothubClient_Destroy_after_garbage_collection();

    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_UploadMultipleBlocksToBlobAsync_callbackEx_invoked_on_error_return_if_not_already)
{
    ///arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    TEST_GETDATA_CALLBACK_ARGS context = { 0 };
    umock_c_reset_all_calls();

    set_expected_calls_for_allocateUploadToBlob();
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG)); /*this is adding HTTPWORKER_THREAD_INFO to the list of HTTPWORKER_THREAD_INFO's to be cleaned*/
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(get_time(IGNORED_NUM_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_UploadMultipleBlocksToBlobEx(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(IOTHUB_CLIENT_ERROR);

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Exit(0));

    ///act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_UploadMultipleBlocksToBlobAsync(iothub_handle, "someFileName.txt", NULL, my_FileUpload_GetData_CallbackEx, &context);
    g_thread_func(g_thread_func_arg); /*this is the thread uploading function*/

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 1, context.invokation_counter);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, FILE_UPLOAD_ERROR, context.last_result);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

    ///cleanup
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_SLL_HANDLE))
        .SetReturn(TEST_LIST_HANDLE);

    setup_gargageCollection(my_malloc_items[2], true);
    setup_IothubClient_Destroy_after_garbage_collection();

    IoTHubClientCore_Destroy(iothub_handle);
}
#endif

/* SYNC DEVICE METHOD */
TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_method_callback_VECTOR_move_FAILS_fail)
{
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetDeviceMethodCallback(iothub_handle, test_method_callback, CALLBACK_CONTEXT);
    (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, g_userContextCallback);
    umock_c_reset_all_calls();
    g_how_thread_loops = 1;

    STRICT_EXPECTED_CALL(get_time(IGNORED_NUM_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_DoWork(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE));
    STRICT_EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_SLL_HANDLE));
    STRICT_EXPECTED_CALL(VECTOR_move(IGNORED_PTR_ARG)).SetReturn(NULL);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(1));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Exit(0));

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_method_callback_STRING_construct_FAILS_fail)
{
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetDeviceMethodCallback(iothub_handle, test_method_callback, CALLBACK_CONTEXT);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG)).SetReturn(NULL);
    (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, g_userContextCallback);
    g_how_thread_loops = 1;

    set_expected_calls_nocallbacks_Schedule_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_method_callback_BUFFER_create_FAILS_fail)
{
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetDeviceMethodCallback(iothub_handle, test_method_callback, CALLBACK_CONTEXT);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(NULL);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, g_userContextCallback);
    g_how_thread_loops = 1;

    set_expected_calls_nocallbacks_Schedule_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_method_callback_VECTOR_push_back_FAILS_fail)
{
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetDeviceMethodCallback(iothub_handle, test_method_callback, CALLBACK_CONTEXT);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1)).SetReturn(MU_FAILURE);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));

    (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, g_userContextCallback);
    g_how_thread_loops = 1;

    set_expected_calls_nocallbacks_Schedule_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_method_callback_NULL_CONTEXT_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetDeviceMethodCallback(iothub_handle, test_method_callback, CALLBACK_CONTEXT);
    (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, NULL);
    umock_c_reset_all_calls();
    g_how_thread_loops = 1;

    set_expected_calls_nocallbacks_Schedule_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_method_callback_DMR_Fails)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetDeviceMethodCallback(iothub_handle, my_DeviceMethodCallback, CALLBACK_CONTEXT);
    (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, g_userContextCallback);
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(1);
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(my_DeviceMethodCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG, IGNORED_NUM_ARG, CALLBACK_CONTEXT));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_DeviceMethodResponse(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG)).SetReturn(IOTHUB_CLIENT_ERROR);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_method_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetDeviceMethodCallback(iothub_handle, my_DeviceMethodCallback, CALLBACK_CONTEXT);
    (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, g_userContextCallback);
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(1);
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(my_DeviceMethodCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG, IGNORED_NUM_ARG, CALLBACK_CONTEXT));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_DeviceMethodResponse(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_repeated_method_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetDeviceMethodCallback_Ex(iothub_handle, test_incoming_method_callback, CALLBACK_CONTEXT);
    for (size_t ii = 0; ii < method_calls_repeat; ++ii)
    {
        (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, g_userContextCallback);
    }
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(method_calls_repeat);

    for (size_t ii = 0; ii < method_calls_repeat; ++ii)
    {
        STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, ii));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(test_incoming_method_callback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 0, TEST_METHOD_ID, CALLBACK_CONTEXT));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

/* ASYNC DEVICE METHOD */
TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_incoming_method_callback_STRING_construct_FAILS_fail)
{
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetDeviceMethodCallback_Ex(iothub_handle, test_incoming_method_callback, CALLBACK_CONTEXT);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG)).SetReturn(NULL);
    (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, g_userContextCallback);
    g_how_thread_loops = 1;

    set_expected_calls_nocallbacks_Schedule_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_incoming_method_callback_BUFFER_create_FAILS_fail)
{
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetDeviceMethodCallback_Ex(iothub_handle, test_incoming_method_callback, CALLBACK_CONTEXT);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(NULL);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, g_userContextCallback);
    g_how_thread_loops = 1;

    set_expected_calls_nocallbacks_Schedule_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}


TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_incoming_method_callback_VECTOR_push_back_FAILS_fail)
{
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetDeviceMethodCallback_Ex(iothub_handle, test_incoming_method_callback, CALLBACK_CONTEXT);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1)).SetReturn(MU_FAILURE);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));

    (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, g_userContextCallback);
    g_how_thread_loops = 1;

    set_expected_calls_nocallbacks_Schedule_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_incoming_method_callback_NULL_CONTEXT_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetDeviceMethodCallback_Ex(iothub_handle, test_incoming_method_callback, CALLBACK_CONTEXT);
    (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, NULL);
    umock_c_reset_all_calls();
    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(1);
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_incoming_method_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetDeviceMethodCallback_Ex(iothub_handle, test_incoming_method_callback, CALLBACK_CONTEXT);
    (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, g_userContextCallback);
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(1);
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(test_incoming_method_callback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 0, TEST_METHOD_ID, CALLBACK_CONTEXT));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_repeated_incoming_method_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetDeviceMethodCallback_Ex(iothub_handle, test_incoming_method_callback, CALLBACK_CONTEXT);
    for (size_t ii = 0; ii < method_calls_repeat; ++ii)
    {
        (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_METHOD_ID, g_userContextCallback);
    }
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(method_calls_repeat);

    for (size_t ii = 0; ii < method_calls_repeat; ++ii)
    {
        STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, ii));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(test_incoming_method_callback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 0, TEST_METHOD_ID, CALLBACK_CONTEXT));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_repeated_command_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SubscribeToCommands(iothub_handle, test_command_callback, CALLBACK_CONTEXT);
    for (size_t ii = 0; ii < command_calls_repeat; ++ii)
    {
        (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_COMMAND_ID, g_userContextCallback);
    }
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(command_calls_repeat);

    for (size_t ii = 0; ii < command_calls_repeat; ++ii)
    {
        STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, ii));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)).SetReturn((unsigned char*)TEST_COMMAND_PAYLOAD);
        STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG)).SetReturn(TEST_COMMAND_PAYLOAD_LENGTH);
        STRICT_EXPECTED_CALL(IoTHubClientCore_LL_ParseMethodToCommand(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_command_name(&TEST_METHOD_NAME, sizeof(const char *));
        STRICT_EXPECTED_CALL(test_command_callback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(IoTHubClientCore_LL_DeviceMethodResponse(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_incoming_command_callback_STRING_construct_FAILS_fail)
{
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SubscribeToCommands(iothub_handle, test_command_callback, CALLBACK_CONTEXT);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG)).SetReturn(NULL);
    (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_COMMAND_ID, g_userContextCallback);
    g_how_thread_loops = 1;

    set_expected_calls_nocallbacks_Schedule_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_incoming_command_callback_BUFFER_create_FAILS_fail)
{
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SubscribeToCommands(iothub_handle, test_command_callback, CALLBACK_CONTEXT);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(NULL);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_COMMAND_ID, g_userContextCallback);
    g_how_thread_loops = 1;

    set_expected_calls_nocallbacks_Schedule_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_incoming_command_callback_VECTOR_push_back_FAILS_fail)
{
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SubscribeToCommands(iothub_handle, test_command_callback, CALLBACK_CONTEXT);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1)).SetReturn(MU_FAILURE);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));

    (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_COMMAND_ID, g_userContextCallback);
    g_how_thread_loops = 1;

    set_expected_calls_nocallbacks_Schedule_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_incoming_command_callback_NULL_CONTEXT_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SubscribeToCommands(iothub_handle, test_command_callback, CALLBACK_CONTEXT);
    (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_COMMAND_ID, NULL);
    umock_c_reset_all_calls();
    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(1);
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_incoming_command_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SubscribeToCommands(iothub_handle, test_command_callback, CALLBACK_CONTEXT);
    (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_COMMAND_ID, g_userContextCallback);
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(1);
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)).SetReturn((unsigned char*)TEST_COMMAND_PAYLOAD);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG)).SetReturn(TEST_COMMAND_PAYLOAD_LENGTH);
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_ParseMethodToCommand(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_command_name(&TEST_METHOD_NAME, sizeof(const char *));
    STRICT_EXPECTED_CALL(test_command_callback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_DeviceMethodResponse(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));   
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_incoming_command_with_component_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SubscribeToCommands(iothub_handle, test_command_callback, CALLBACK_CONTEXT);
    (void)g_inboundDeviceCallback(TEST_METHOD_NAME_WITH_COMPONENT, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_COMMAND_ID, g_userContextCallback);
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    // Allocate the memory for the test here since during the callback in g_thread_func() we're testing, it'll be free'd automatically.
    char* componentName = my_gballoc_malloc(strlen(TEST_COMPONENT_NAME) + 1);
    ASSERT_IS_NOT_NULL(componentName);

    g_currentTestComponentName = TEST_COMPONENT_NAME;
    strcpy(componentName, TEST_COMPONENT_NAME);

    set_expected_calls_first_ScheduleWork_Thread_loop(1);
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)).SetReturn((unsigned char*)TEST_COMMAND_PAYLOAD);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG)).SetReturn(TEST_COMMAND_PAYLOAD_LENGTH);
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_ParseMethodToCommand(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_component_name(&componentName, sizeof(char *))
        .CopyOutArgumentBuffer_command_name(&TEST_METHOD_NAME, sizeof(const char *));
    STRICT_EXPECTED_CALL(test_command_callback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_DeviceMethodResponse(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));   
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}


TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_repeated_incoming_command_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SubscribeToCommands(iothub_handle, test_command_callback, CALLBACK_CONTEXT);
    for (size_t ii = 0; ii < command_calls_repeat; ++ii)
    {
        (void)g_inboundDeviceCallback(TEST_METHOD_NAME, TEST_DEVICE_METHOD_RESPONSE, TEST_DEVICE_RESP_LENGTH, TEST_COMMAND_ID, g_userContextCallback);
    }
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(command_calls_repeat);

    for (size_t ii = 0; ii < command_calls_repeat; ++ii)
    {
        STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, ii));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)).SetReturn((unsigned char*)TEST_COMMAND_PAYLOAD);
        STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG)).SetReturn(TEST_COMMAND_PAYLOAD_LENGTH);
        STRICT_EXPECTED_CALL(IoTHubClientCore_LL_ParseMethodToCommand(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer_command_name(&TEST_METHOD_NAME, sizeof(const char *));
        STRICT_EXPECTED_CALL(test_command_callback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(IoTHubClientCore_LL_DeviceMethodResponse(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

/* Test_SRS_IOTHUBCLIENT_07_002: [ IoTHubClientCore_SetDeviceTwinCallback shall allocate a IOTHUB_QUEUE_CONTEXT object to be sent to the IoTHubClientCore_LL_SetDeviceTwinCallback function as a user context. ] */
TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_device_twin_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetDeviceTwinCallback(iothub_handle, test_device_twin_callback, NULL);
    g_deviceTwinCallback(DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0, g_userContextCallback);
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(1);
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0));
    STRICT_EXPECTED_CALL(test_device_twin_callback(DEVICE_TWIN_UPDATE_COMPLETE, NULL, 0, NULL));

    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    ASSERT_IS_NOT_NULL(g_thread_func);
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_event_confirm_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SendEventAsync(iothub_handle, TEST_MESSAGE_HANDLE, test_event_confirmation_callback, NULL);
    g_eventConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_OK, g_userContextCallback);
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(1);

    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0));
    STRICT_EXPECTED_CALL(test_event_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_OK, NULL));

    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    ASSERT_IS_NOT_NULL(g_thread_func);
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_reported_state_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    const unsigned char* reported_state = (const unsigned char*)0x1234;
    (void)IoTHubClientCore_SendReportedState(iothub_handle, reported_state, 1, test_report_state_callback, NULL);
    g_reportedStateCallback(REPORTED_STATE_STATUS_CODE, g_userContextCallback);
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(1);
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0));
    STRICT_EXPECTED_CALL(test_report_state_callback(REPORTED_STATE_STATUS_CODE, NULL));

    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    ASSERT_IS_NOT_NULL(g_thread_func);
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

// Make sure that if different callback functions are specified, they're called correctly
TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_reported_state_two_callbacks_succeed)
{
    // arrange
    void* userContextCallback1;
    void* userContextCallback2;
    
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    const unsigned char* reported_state = (const unsigned char*)0x1234;
    (void)IoTHubClientCore_SendReportedState(iothub_handle, reported_state, 1, test_report_state_callback, CALLBACK_CONTEXT);
    // We need to save off the reported state context here because next call overwrites the global g_userContextCallback.
    userContextCallback1 = g_userContextCallback;
    (void)IoTHubClientCore_SendReportedState(iothub_handle, reported_state, 1, test_report_state_callback2, CALLBACK_CONTEXT2);
    userContextCallback2 = g_userContextCallback;

    g_reportedStateCallback(REPORTED_STATE_STATUS_CODE, userContextCallback1);
    g_reportedStateCallback(REPORTED_STATE_STATUS_CODE, userContextCallback2);
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(2);
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0));
    STRICT_EXPECTED_CALL(test_report_state_callback(REPORTED_STATE_STATUS_CODE, CALLBACK_CONTEXT));

    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 1));
    STRICT_EXPECTED_CALL(test_report_state_callback2(REPORTED_STATE_STATUS_CODE, CALLBACK_CONTEXT2));

    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    ASSERT_IS_NOT_NULL(g_thread_func);
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}


TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_message_callback_LOCK_Fails)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetMessageCallback(iothub_handle, test_message_confirmation_callback, NULL);
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromString("Hello World");
    g_messageCallback_ex(messageHandle, g_userContextCallback);
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(1);
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0));
    STRICT_EXPECTED_CALL(test_message_confirmation_callback(NULL, NULL));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG)).SetReturn(LOCK_ERROR);
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    ASSERT_IS_NOT_NULL(g_thread_func);
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubMessage_Destroy(messageHandle);
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_message_callback_SMD_Fails)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetMessageCallback(iothub_handle, test_message_confirmation_callback, NULL);
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromString("Hello World");
    g_messageCallback_ex(messageHandle, g_userContextCallback);
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(1);
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0));
    STRICT_EXPECTED_CALL(test_message_confirmation_callback(NULL, NULL));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SendMessageDisposition(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IOTHUBMESSAGE_ACCEPTED)).SetReturn(IOTHUB_CLIENT_ERROR);
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();

    // act
    ASSERT_IS_NOT_NULL(g_thread_func);
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubMessage_Destroy(messageHandle);
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_ScheduleWork_Thread_message_callback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    (void)IoTHubClientCore_SetMessageCallback(iothub_handle, test_message_confirmation_callback, NULL);
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromString("Hello World");
    g_messageCallback_ex(messageHandle, g_userContextCallback);
    umock_c_reset_all_calls();

    g_how_thread_loops = 1;

    set_expected_calls_first_ScheduleWork_Thread_loop(1);
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0));
    STRICT_EXPECTED_CALL(test_message_confirmation_callback(NULL, NULL));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SendMessageDisposition(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IOTHUBMESSAGE_ACCEPTED));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    set_expected_calls_final_ScheduleWork_Thread_loop();


    // act
    ASSERT_IS_NOT_NULL(g_thread_func);
    g_thread_func(g_thread_func_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubMessage_Destroy(messageHandle);
    IoTHubClientCore_Destroy(iothub_handle);
}


TEST_FUNCTION(IoTHubClientCore_SendEventToOutputAsync_iothub_client_handle_NULL_fail)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SendEventToOutputAsync(NULL, TEST_MESSAGE_HANDLE, TEST_OUTPUT_NAME, test_event_confirmation_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClientCore_SendEventToOutputAsync_message_handle_NULL_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SendEventToOutputAsync(iothub_handle, NULL, TEST_OUTPUT_NAME, test_event_confirmation_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SendEventToOutputAsync_output_name_NULL_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SendEventToOutputAsync(iothub_handle, TEST_MESSAGE_HANDLE, NULL, test_event_confirmation_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_SendEventToOutputAsync_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_SetOutputName(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    setup_iothubclient_sendeventasync(true);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SendEventToOutputAsync(iothub_handle, TEST_MESSAGE_HANDLE, TEST_OUTPUT_NAME, test_event_confirmation_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SendEventToOutputAsync_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(IoTHubMessage_SetOutputName(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    setup_iothubclient_sendeventasync(true);

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        // reset mock failure for common paths.
        g_fail_my_gballoc_malloc = false;
        g_fail_my_SendEventAsync = false;

        if (index == 5) // Unlock
        {
            continue;
        }
        else if (index == 3) // malloc
        {
            // Force malloc call to fail
            g_fail_my_gballoc_malloc = true;
        }
        else if (index == 4) // IoTHubClientCore_LL_SendEventAsync
        {
            g_fail_my_SendEventAsync = true;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubClientCore_SendEventToOutputAsync failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SendEventToOutputAsync(iothub_handle, TEST_MESSAGE_HANDLE, TEST_OUTPUT_NAME, test_event_confirmation_callback, NULL);

        // assert
        ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClientCore_Destroy(iothub_handle);
}


static void setup_IoTHubClientCore_SetInputMessageCallback(bool use_threads)
{
    if (use_threads)
    {
        STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SetInputMessageCallbackEx(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
}

TEST_FUNCTION(IoTHubClientCore_SetInputMessageCallback_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    setup_IoTHubClientCore_SetInputMessageCallback(true);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetInputMessageCallback(iothub_handle, TEST_INPUT_NAME, test_message_confirmation_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SetInputMessageCallback_iothub_client_handle_NULL_fail)
{
    // arrange

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetInputMessageCallback(NULL, TEST_INPUT_NAME, test_message_confirmation_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClientCore_SetInputMessageCallback_iothub_inputName_NULL_succeeds)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    setup_IoTHubClientCore_SetInputMessageCallback(true);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetInputMessageCallback(iothub_handle, NULL, test_message_confirmation_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClient_SetInputMessageCallback_fail)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_IoTHubClientCore_SetInputMessageCallback(true);

    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < 2; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubClientCore_SetInputMessageCallback failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetInputMessageCallback(iothub_handle, TEST_INPUT_NAME, test_message_confirmation_callback, NULL);

        // assert
        ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    //
    // We need to de-init and then re-init the test framework due to how IoTHubClient_LL_SetInputMessageCallback is implemented,
    // namely its ThreadAPI_Create is not going to called after initial success.
    //
    umock_c_negative_tests_deinit();
    umock_c_reset_all_calls();


    negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_IoTHubClientCore_SetInputMessageCallback(false);

    umock_c_negative_tests_snapshot();

    count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        if (index == 2) // Unlock
        {
            continue;
        }

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubClientCore_SetInputMessageCallback failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);
        IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SetInputMessageCallback(iothub_handle, TEST_INPUT_NAME, test_message_confirmation_callback, NULL);

        // assert
        ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClientCore_Destroy(iothub_handle);
}


typedef enum METHOD_INVOKE_TEST_THREAD_TAG
{
    METHOD_INVOKE_TEST_MOCK_CREATE_THREAD,
    METHOD_INVOKE_TEST_SKIP_CREATE_THREAD
} METHOD_INVOKE_TEST_THREAD;


static void set_expected_calls_for_IotHubClientCore_GenericMethodInvoke(METHOD_INVOKE_TEST_THREAD testThreadType, METHOD_INVOKE_TEST_TARGET testTarget)
{
    current_method_invoke_test = testTarget;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)); /*this is creating a HTTPWORKER_THREAD_INFO*/
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_DEVICE_ID));
    if (testTarget == METHOD_INVOKE_TEST_TARGET_MODULE)
    {
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_MODULE_ID));
    }
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_METHOD_NAME));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_METHOD_PAYLOAD));

    STRICT_EXPECTED_CALL(Lock_Init());

    if (testThreadType == METHOD_INVOKE_TEST_MOCK_CREATE_THREAD)
    {
        EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_add(IGNORED_PTR_ARG, IGNORED_PTR_ARG)); /*this is adding HTTPWORKER_THREAD_INFO to the list of HTTPWORKER_THREAD_INFO's to be cleaned*/
    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
}

static void set_expected_calls_For_MethodInvokeThread()
{
    int responseStatus = 200;
    int responseSize = 1221;

    // We need to explicitly allocate this here because the core always frees a real pointer here.
    unsigned char* responseData = (unsigned char* )my_gballoc_malloc(1);
    ASSERT_IS_NOT_NULL(responseData, "failed allocating responseData");

    STRICT_EXPECTED_CALL(get_time(IGNORED_NUM_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_GenericMethodInvoke(TEST_IOTHUB_CLIENT_CORE_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG,
                                                                 IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                            .CopyOutArgumentBuffer(7, &responseStatus, sizeof(responseStatus))
                            .CopyOutArgumentBuffer(8, &responseData, sizeof(responseData))
                            .CopyOutArgumentBuffer(9, &responseSize, sizeof(responseSize))
                            ;
    STRICT_EXPECTED_CALL(test_method_invoke_callback(IOTHUB_CLIENT_OK, responseStatus, responseData, responseSize, CALLBACK_CONTEXT));

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Exit(0));
}

static void IoTHubClientCore_GenericMethodInvoke_Impl(METHOD_INVOKE_TEST_TARGET testTarget)
{
    //arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    set_expected_calls_for_IotHubClientCore_GenericMethodInvoke(METHOD_INVOKE_TEST_MOCK_CREATE_THREAD, testTarget);
    set_expected_calls_For_MethodInvokeThread();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GenericMethodInvoke(iothub_handle, TEST_DEVICE_ID,
                                                                       (testTarget==METHOD_INVOKE_TEST_TARGET_MODULE) ?  TEST_MODULE_ID : NULL,
                                                                       TEST_METHOD_NAME,  TEST_METHOD_PAYLOAD, TEST_INVOKE_TIMEOUT,
                                                                       test_method_invoke_callback, CALLBACK_CONTEXT);

    g_thread_func(g_thread_func_arg); /*this is the thread invoking module function, captured during the CreateThread mock*/

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

    EXPECTED_CALL(ThreadAPI_Join(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(singlylinkedlist_get_head_item(TEST_SLL_HANDLE))
        .SetReturn(TEST_LIST_HANDLE);

    setup_gargageCollection(my_malloc_items[3], true);
    setup_IothubClient_Destroy_after_garbage_collection();

    IoTHubClientCore_Destroy(iothub_handle);

}

TEST_FUNCTION(IoTHubClientCore_GenericMethodInvoke_on_Device_succeeds)
{
    IoTHubClientCore_GenericMethodInvoke_Impl(METHOD_INVOKE_TEST_TARGET_DEVICE);
}

TEST_FUNCTION(IoTHubClientCore_GenericMethodInvoke_on_Module_succeeds)
{
    IoTHubClientCore_GenericMethodInvoke_Impl(METHOD_INVOKE_TEST_TARGET_MODULE);
}

TEST_FUNCTION(IoTHubClientCore_GenericMethodInvoke_NULL_handle_fails)
{
    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GenericMethodInvoke(NULL, TEST_DEVICE_ID, TEST_MODULE_ID,
                                                                       TEST_METHOD_NAME,  TEST_METHOD_PAYLOAD, TEST_INVOKE_TIMEOUT,
                                                                       test_method_invoke_callback, CALLBACK_CONTEXT);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
}

TEST_FUNCTION(IoTHubClientCore_GenericMethodInvoke_NULL_device_id_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GenericMethodInvoke(iothub_handle, NULL, TEST_MODULE_ID,
                                                                       TEST_METHOD_NAME,  TEST_METHOD_PAYLOAD, TEST_INVOKE_TIMEOUT,
                                                                       test_method_invoke_callback, CALLBACK_CONTEXT);

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_GenericMethodInvoke_NULL_method_name_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GenericMethodInvoke(iothub_handle, TEST_DEVICE_ID, TEST_MODULE_ID,
                                                                       NULL,  TEST_METHOD_PAYLOAD, TEST_INVOKE_TIMEOUT,
                                                                       test_method_invoke_callback, CALLBACK_CONTEXT);

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    IoTHubClientCore_Destroy(iothub_handle);
}


TEST_FUNCTION(IoTHubClientCore_GenericMethodInvoke_NULL_method_payload_fails)
{
    //arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GenericMethodInvoke(iothub_handle, TEST_DEVICE_ID, TEST_MODULE_ID,
                                                                       TEST_METHOD_NAME,  NULL, TEST_INVOKE_TIMEOUT,
                                                                       test_method_invoke_callback, CALLBACK_CONTEXT);

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_GenericMethodInvoke_fail)
{
    //arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_IotHubClientCore_GenericMethodInvoke(METHOD_INVOKE_TEST_MOCK_CREATE_THREAD, METHOD_INVOKE_TEST_TARGET_MODULE);
    printf("Expected:: %s\n", umockcallrecorder_get_expected_calls(umock_c_get_call_recorder()));
    umock_c_negative_tests_snapshot();

    // act
    size_t count = 7; // stop after ThreadAPI_Create() for first run
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GenericMethodInvoke(iothub_handle, TEST_DEVICE_ID, TEST_MODULE_ID,
                                                                           TEST_METHOD_NAME,  TEST_METHOD_PAYLOAD, TEST_INVOKE_TIMEOUT,
                                                                           test_method_invoke_callback, CALLBACK_CONTEXT);

        char tmp_msg[128];
        sprintf(tmp_msg, "IoTHubClientCore_GenericMethodInvoke failure in test %lu/%lu in run 1", (unsigned long)index, (unsigned long)count);

        // assert
        ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
    }

    //
    // We need to de-init and then re-init the test framework due to how IoTHubClient_LL_SetInputMessageCallback is implemented,
    // namely its ThreadAPI_Create is not going to called after initial success.
    //
    umock_c_negative_tests_deinit();
    umock_c_reset_all_calls();

    negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_IotHubClientCore_GenericMethodInvoke(METHOD_INVOKE_TEST_SKIP_CREATE_THREAD, METHOD_INVOKE_TEST_TARGET_MODULE);
    printf("Expected:: %s\n", umockcallrecorder_get_expected_calls(umock_c_get_call_recorder()));
    umock_c_negative_tests_snapshot();

    // act
    count = umock_c_negative_tests_call_count() - 1;
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            IOTHUB_CLIENT_RESULT result = IoTHubClientCore_GenericMethodInvoke(iothub_handle, TEST_DEVICE_ID, TEST_MODULE_ID,
                                                                               TEST_METHOD_NAME,  TEST_METHOD_PAYLOAD, TEST_INVOKE_TIMEOUT,
                                                                               test_method_invoke_callback, CALLBACK_CONTEXT);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubClientCore_GenericMethodInvoke failure in test %lu/%lu in run 2", (unsigned long)index, (unsigned long)count);

            // assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, tmp_msg);
        }
    }

    umock_c_negative_tests_deinit();
    umock_c_reset_all_calls();

    // cleanup
    umock_c_reset_all_calls();
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SendMessageDisposition_succeed)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);

    IOTHUBMESSAGE_DISPOSITION_RESULT disposition_option[4];
    disposition_option[0] = IOTHUBMESSAGE_ACCEPTED;
    disposition_option[1] = IOTHUBMESSAGE_REJECTED;
    disposition_option[2] = IOTHUBMESSAGE_ABANDONED;
    disposition_option[3] = IOTHUBMESSAGE_ASYNC_ACK;
    
    for (int index = 0; index < 4; index++)
    {
        umock_c_reset_all_calls();
        if (index == 0)
        {
            STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        }
        STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SendMessageDisposition(IGNORED_PTR_ARG, TEST_MESSAGE_HANDLE, disposition_option[index]));    
        STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG));

        // act
        IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SendMessageDisposition(iothub_handle, TEST_MESSAGE_HANDLE, disposition_option[index]);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    }

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SendMessageDisposition_negative_test_fails)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);

    umock_c_reset_all_calls();
    // STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Lock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubClientCore_LL_SendMessageDisposition(IGNORED_PTR_ARG, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED));    
    STRICT_EXPECTED_CALL(Unlock(IGNORED_PTR_ARG))
        .CallCannotFail();
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "IoTHubClientCore_SendMessageDisposition failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

            IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SendMessageDisposition(iothub_handle, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED);

            // assert
            ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result, tmp_msg);
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
    IoTHubClientCore_Destroy(iothub_handle);
}

TEST_FUNCTION(IoTHubClientCore_SendMessageDisposition_NULL_handle_fails)
{
    // arrange
    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SendMessageDisposition(NULL, TEST_MESSAGE_HANDLE, IOTHUBMESSAGE_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    // cleanup
}

TEST_FUNCTION(IoTHubClientCore_SendMessageDisposition_NULL_message_fails)
{
    // arrange
    IOTHUB_CLIENT_CORE_HANDLE iothub_handle = IoTHubClientCore_Create(TEST_CLIENT_CONFIG);

    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClientCore_SendMessageDisposition(iothub_handle, NULL, IOTHUBMESSAGE_ACCEPTED);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    // cleanup
    IoTHubClientCore_Destroy(iothub_handle);
}

END_TEST_SUITE(iothubclientcore_ut)
