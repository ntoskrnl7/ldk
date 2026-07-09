#pragma once

#define LDK_MULTI_INSTANCE_IOCTL \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LDK_MULTI_INSTANCE_SET_QUERY 1u
#define LDK_MULTI_INSTANCE_QUERY 2u

typedef struct _LDK_MULTI_INSTANCE_REQUEST {
    unsigned long Command;
    unsigned __int64 TlsValue;
    unsigned long LastErrorValue;
} LDK_MULTI_INSTANCE_REQUEST, *PLDK_MULTI_INSTANCE_REQUEST;

typedef struct _LDK_MULTI_INSTANCE_RESPONSE {
    unsigned long InstanceId;
    unsigned long TlsIndex;
    unsigned __int64 Teb;
    unsigned __int64 Peb;
    unsigned __int64 Thread;
    unsigned __int64 TlsValue;
    unsigned long LastErrorValue;
} LDK_MULTI_INSTANCE_RESPONSE, *PLDK_MULTI_INSTANCE_RESPONSE;
