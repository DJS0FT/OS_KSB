#include <windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <intrin.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <thread>
#include <future>
#include <mutex>

#pragma comment(lib, "wbemuuid.lib")

// ========= ����������� ����� � �������� =========
typedef LONG NTSTATUS;
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

typedef struct _MY_RTL_OSVERSIONINFOW {
    ULONG dwOSVersionInfoSize;
    ULONG dwMajorVersion;
    ULONG dwMinorVersion;
    ULONG dwBuildNumber;
    ULONG dwPlatformId;
    WCHAR szCSDVersion[128];
} MY_RTL_OSVERSIONINFOW, * PMY_RTL_OSVERSIONINFOW;

typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PMY_RTL_OSVERSIONINFOW);

// ��������� ��� �������� ���������� � CPU
struct CPUInfoData {
    std::string manufacturer;
    std::string brand;
    int family;
    int model;
};

// ���������� ���������� ��� WMI
IWbemServices* g_pSvc = nullptr;
std::mutex g_wmiMutex;

int main()
{
    // ������ main ��� ������� �������
    return 0;
}
