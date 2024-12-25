#include "WMIHelper.h"
#include <sstream>
#include <iostream>
#include <mutex>

extern IWbemServices* g_pSvc;
extern std::mutex g_wmiMutex;

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

HRESULT InitializeCOMAndSecurity() {
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        return hres;
    }
    hres = CoInitializeSecurity(
        NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL
    );
    if (FAILED(hres) && hres != RPC_E_TOO_LATE) {
        CoUninitialize();
        return hres;
    }
    return S_OK;
}

HRESULT InitializeWMI() {
    std::lock_guard<std::mutex> lock(g_wmiMutex);
    if (g_pSvc != nullptr) {
        Logger::getInstance().log("WMI уже инициализировано.");
        return S_OK; // Уже инициализировано
    }

    IWbemLocator* pLoc = NULL;
    HRESULT hres = CoCreateInstance(
        CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc
    );
    if (FAILED(hres)) {
        Logger::getInstance().log("Не удалось создать экземпляр IWbemLocator. Ошибка: " + std::to_string(hres));
        return hres;
    }

    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL, NULL, 0, NULL, 0, 0,
        &g_pSvc
    );
    if (FAILED(hres)) {
        Logger::getInstance().log("Не удалось подключиться к WMI серверу. Ошибка: " + std::to_string(hres));
        pLoc->Release();
        return hres;
    }
    pLoc->Release();

    hres = CoSetProxyBlanket(
        g_pSvc,
        RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE
    );

    if (FAILED(hres)) {
        Logger::getInstance().log("Не удалось установить прокси blanket. Ошибка: " + std::to_string(hres));
        g_pSvc->Release();
        g_pSvc = nullptr;
    }
    else {
        Logger::getInstance().log("WMI успешно инициализировано.");
    }

    return hres;
}
