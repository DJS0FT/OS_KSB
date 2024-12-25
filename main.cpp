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

// ========= Определения типов и структур =========
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

// Структура для хранения информации о CPU
struct CPUInfoData {
    std::string manufacturer;
    std::string brand;
    int family;
    int model;
};

// ========= Вспомогательные функции =========
std::string BSTRToString(BSTR bstr) {
    if (!bstr) return "N/A";
    int wslen = ::SysStringLen(bstr);
    if (wslen == 0) return "";

    int len = ::WideCharToMultiByte(CP_UTF8, 0, bstr, wslen, NULL, 0, NULL, NULL);
    std::string result(len, 0);
    ::WideCharToMultiByte(CP_UTF8, 0, bstr, wslen, &result[0], len, NULL, NULL);
    return result;
}

std::string GetWMIPropertyFromObject(IWbemClassObject* pclsObj, const std::wstring& property) {
    VARIANT vtProp;
    VariantInit(&vtProp);

    std::string result = "N/A";
    HRESULT hr = pclsObj->Get(property.c_str(), 0, &vtProp, 0, 0);
    if (SUCCEEDED(hr)) {
        switch (vtProp.vt) {
        case VT_BSTR:
            result = BSTRToString(vtProp.bstrVal);
            break;
        case VT_UI4:
            result = std::to_string(vtProp.ulVal);
            break;
        case VT_I4:
            result = std::to_string(vtProp.lVal);
            break;
        case VT_NULL:
            result = "Не указано";
            break;
        case VT_I8:
            result = std::to_string(static_cast<long long>(vtProp.llVal));
            break;
        case VT_UI8:
            result = std::to_string(static_cast<unsigned long long>(vtProp.ullVal));
            break;
        default:
            result = "Неизвестный тип";
            break;
        }
    }

    VariantClear(&vtProp);
    return result;
}

std::string ParseWMIDate(const std::string& wmiDate) {
    if (wmiDate.size() < 8)
        return wmiDate;

    std::string year = wmiDate.substr(0, 4);
    std::string month = wmiDate.substr(4, 2);
    std::string day = wmiDate.substr(6, 2);

    return year + "." + month + "." + day;
}

// ========= 1) Версия Windows =========
std::string GetWindowsVersion() {
    HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
    if (!hMod)
        return "Не удалось загрузить ntdll.dll";

    RtlGetVersionPtr pRtlGetVersion = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
    if (!pRtlGetVersion)
        return "Не удалось найти RtlGetVersion";

    MY_RTL_OSVERSIONINFOW osver = { 0 };
    osver.dwOSVersionInfoSize = sizeof(osver);

    if (pRtlGetVersion(&osver) == STATUS_SUCCESS) {
        std::wstring wcsd(osver.szCSDVersion);
        std::string csd(wcsd.begin(), wcsd.end());

        std::string osName;
        if (osver.dwMajorVersion == 10) {
            if (osver.dwBuildNumber >= 22000)
                osName = "Windows 11";
            else
                osName = "Windows 10";
        }
        else {
            osName = "Windows " + std::to_string(osver.dwMajorVersion) + "." +
                std::to_string(osver.dwMinorVersion);
        }

        return osName + " (Build " + std::to_string(osver.dwBuildNumber) + ") " + csd;
    }
    return "Не удалось получить версию ОС";
}

// ========= 2) Расширенная информация о Windows =========
std::string GetWindowsFullInfo() {
    std::ostringstream oss;
    oss << GetWindowsVersion() << "\n";

    if (SUCCEEDED(InitializeWMI())) {
        IEnumWbemClassObject* pEnumerator = NULL;
        HRESULT hres = g_pSvc->ExecQuery(
            bstr_t("WQL"),
            bstr_t(L"SELECT OSArchitecture, SystemDrive, RegisteredUser, InstallDate FROM Win32_OperatingSystem"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator
        );

        if (SUCCEEDED(hres)) {
            IWbemClassObject* pclsObj = NULL;
            ULONG uReturn = 0;
            if (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
                std::string arch = GetWMIPropertyFromObject(pclsObj, L"OSArchitecture");
                std::string drive = GetWMIPropertyFromObject(pclsObj, L"SystemDrive");
                std::string user = GetWMIPropertyFromObject(pclsObj, L"RegisteredUser");
                std::string installDateRaw = GetWMIPropertyFromObject(pclsObj, L"InstallDate");
                std::string instDate = ParseWMIDate(installDateRaw);

                oss << "Архитектура ОС: " << arch << "\n"
                    << "Системный диск: " << drive << "\n"
                    << "Зарегистрированный пользователь: " << user << "\n"
                    << "Дата установки ОС: " << instDate << "\n";

                pclsObj->Release();
            }
            pEnumerator->Release();
        }
    }

    return oss.str();
}

// Глобальные переменные для WMI
IWbemServices* g_pSvc = nullptr;
std::mutex g_wmiMutex;

int main()
{
    // Пустой main для первого коммита
    return 0;
}
