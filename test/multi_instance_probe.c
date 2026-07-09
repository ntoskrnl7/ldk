#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#include "multi_instance_shared.h"

static int
fail (
    const char* message
    )
{
    printf("[LdkMultiProbe] FAIL %s GetLastError=%lu\n", message, GetLastError());
    return 1;
}

static int
send_request (
    HANDLE device,
    unsigned long command,
    uint64_t tls_value,
    unsigned long last_error_value,
    PLDK_MULTI_INSTANCE_RESPONSE response
    )
{
    LDK_MULTI_INSTANCE_REQUEST request;
    DWORD returned;

    request.Command = command;
    request.TlsValue = tls_value;
    request.LastErrorValue = last_error_value;
    returned = 0;

    if (!DeviceIoControl(device,
                         LDK_MULTI_INSTANCE_IOCTL,
                         &request,
                         sizeof(request),
                         response,
                         sizeof(*response),
                         &returned,
                         NULL)) {
        return fail("DeviceIoControl");
    }

    if (returned != sizeof(*response)) {
        printf("[LdkMultiProbe] FAIL unexpected returned byte count %lu\n", returned);
        return 1;
    }

    return 0;
}

static int
expect_response (
    const char* label,
    const LDK_MULTI_INSTANCE_RESPONSE* response,
    unsigned long instance_id,
    uint64_t tls_value,
    unsigned long last_error_value
    )
{
    if (response->InstanceId != instance_id ||
        response->TlsValue != tls_value ||
        response->LastErrorValue != last_error_value ||
        response->Teb == 0 ||
        response->Peb == 0 ||
        response->Thread == 0) {
        printf("[LdkMultiProbe] FAIL %s Instance=%lu Tls=0x%llx LastError=%lu Teb=0x%llx Peb=0x%llx Thread=0x%llx\n",
               label,
               response->InstanceId,
               (unsigned long long)response->TlsValue,
               response->LastErrorValue,
               (unsigned long long)response->Teb,
               (unsigned long long)response->Peb,
               (unsigned long long)response->Thread);
        return 1;
    }

    return 0;
}

int
main (
    void
    )
{
    HANDLE device_a;
    HANDLE device_b;
    LDK_MULTI_INSTANCE_RESPONSE a1;
    LDK_MULTI_INSTANCE_RESPONSE b1;
    LDK_MULTI_INSTANCE_RESPONSE a2;
    LDK_MULTI_INSTANCE_RESPONSE b2;
    const uint64_t a_value = 0xA11DA11DA11DA11Dull;
    const uint64_t b_value = 0xB22DB22DB22DB22Dull;
    const unsigned long a_error = 0xA11D;
    const unsigned long b_error = 0xB22D;

    device_a = CreateFileW(L"\\\\.\\LdkMultiA",
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
    if (device_a == INVALID_HANDLE_VALUE) {
        return fail("CreateFileW(LdkMultiA)");
    }

    device_b = CreateFileW(L"\\\\.\\LdkMultiB",
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
    if (device_b == INVALID_HANDLE_VALUE) {
        CloseHandle(device_a);
        return fail("CreateFileW(LdkMultiB)");
    }

    if (send_request(device_a, LDK_MULTI_INSTANCE_SET_QUERY, a_value, a_error, &a1) ||
        send_request(device_b, LDK_MULTI_INSTANCE_SET_QUERY, b_value, b_error, &b1) ||
        send_request(device_a, LDK_MULTI_INSTANCE_QUERY, 0, 0, &a2) ||
        send_request(device_b, LDK_MULTI_INSTANCE_QUERY, 0, 0, &b2)) {
        CloseHandle(device_b);
        CloseHandle(device_a);
        return 1;
    }

    if (expect_response("A initial", &a1, 1, a_value, a_error) ||
        expect_response("B initial", &b1, 2, b_value, b_error) ||
        expect_response("A after B", &a2, 1, a_value, a_error) ||
        expect_response("B after A", &b2, 2, b_value, b_error)) {
        CloseHandle(device_b);
        CloseHandle(device_a);
        return 1;
    }

    if (a1.Thread != b1.Thread ||
        a1.Thread != a2.Thread ||
        b1.Thread != b2.Thread) {
        printf("[LdkMultiProbe] FAIL calls did not stay on one user thread A1=0x%llx B1=0x%llx A2=0x%llx B2=0x%llx\n",
               (unsigned long long)a1.Thread,
               (unsigned long long)b1.Thread,
               (unsigned long long)a2.Thread,
               (unsigned long long)b2.Thread);
        CloseHandle(device_b);
        CloseHandle(device_a);
        return 1;
    }

    if (a1.Teb != a2.Teb ||
        b1.Teb != b2.Teb ||
        a1.Teb == b1.Teb ||
        a1.Peb != a2.Peb ||
        b1.Peb != b2.Peb ||
        a1.Peb == b1.Peb) {
        printf("[LdkMultiProbe] FAIL instance state mixed A_TEB=0x%llx/0x%llx B_TEB=0x%llx/0x%llx A_PEB=0x%llx/0x%llx B_PEB=0x%llx/0x%llx\n",
               (unsigned long long)a1.Teb,
               (unsigned long long)a2.Teb,
               (unsigned long long)b1.Teb,
               (unsigned long long)b2.Teb,
               (unsigned long long)a1.Peb,
               (unsigned long long)a2.Peb,
               (unsigned long long)b1.Peb,
               (unsigned long long)b2.Peb);
        CloseHandle(device_b);
        CloseHandle(device_a);
        return 1;
    }

    printf("[LdkMultiProbe] PASS A_TEB=0x%llx B_TEB=0x%llx A_PEB=0x%llx B_PEB=0x%llx Thread=0x%llx\n",
           (unsigned long long)a1.Teb,
           (unsigned long long)b1.Teb,
           (unsigned long long)a1.Peb,
           (unsigned long long)b1.Peb,
           (unsigned long long)a1.Thread);

    CloseHandle(device_b);
    CloseHandle(device_a);
    return 0;
}
