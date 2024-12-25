#include "SystemInfo.h"
#include "WMIHelper.h"
#include <iostream>
#include <future>
#include <mutex>
#include <sstream>
#include <iomanip>

extern IWbemServices* g_pSvc;
extern std::mutex g_wmiMutex;

// ========= 1) ������ Windows =========
std::string GetWindowsVersion() {
    HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
    if (!hMod)
        return "�� ������� ��������� ntdll.dll";

    RtlGetVersionPtr pRtlGetVersion = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
    if (!pRtlGetVersion)
        return "�� ������� ����� RtlGetVersion";

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
    return "�� ������� �������� ������ ��";
}

// ========= 2) ����������� ���������� � Windows =========
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

                oss << "����������� ��: " << arch << "\n"
                    << "��������� ����: " << drive << "\n"
                    << "������������������ ������������: " << user << "\n"
                    << "���� ��������� ��: " << instDate << "\n";

                pclsObj->Release();
            }
            else {
                oss << "�� ������� �������� ������ �� Win32_OperatingSystem.\n";
            }
            pEnumerator->Release();
        }
        else {
            oss << "�� ������� ��������� WMI-������ ��� Win32_OperatingSystem. ������: " << hres << "\n";
        }
    }
    else {
        oss << "WMI �� ����������������.\n";
    }

    return oss.str();
}

// ========= 3) CPU-���������� =========
CPUInfoData GetDetailedCPUInfo() {
    CPUInfoData info;

    // --- ������������� ---
    {
        int CPUInfoReg[4] = { 0 };
        __cpuid(CPUInfoReg, 0x00000000);

        char CPUString[13];
        memcpy(CPUString, &CPUInfoReg[1], 4);  // EBX
        memcpy(CPUString + 4, &CPUInfoReg[3], 4);  // EDX
        memcpy(CPUString + 8, &CPUInfoReg[2], 4);  // ECX
        CPUString[12] = '\0';

        info.manufacturer = CPUString;
    }

    // --- ������ �������� ---
    {
        int CPUInfoReg[4] = { 0 };
        char CPUBrandString[49];
        memset(CPUBrandString, 0, sizeof(CPUBrandString));

        __cpuid(CPUInfoReg, 0x80000002);
        memcpy(CPUBrandString, CPUInfoReg, sizeof(CPUInfoReg));

        __cpuid(CPUInfoReg, 0x80000003);
        memcpy(CPUBrandString + 16, CPUInfoReg, sizeof(CPUInfoReg));

        __cpuid(CPUInfoReg, 0x80000004);
        memcpy(CPUBrandString + 32, CPUInfoReg, sizeof(CPUInfoReg));

        CPUBrandString[48] = '\0';
        info.brand = std::string(CPUBrandString);
    }

    // --- ��������� � ������ ---
    {
        int CPUInfoReg[4] = { 0 };
        __cpuid(CPUInfoReg, 1);

        int familyID = (CPUInfoReg[0] >> 8) & 0xF;
        int extendedFamilyID = (CPUInfoReg[0] >> 20) & 0xFF;
        int modelID = (CPUInfoReg[0] >> 4) & 0xF;
        int extendedModelID = (CPUInfoReg[0] >> 16) & 0xF;

        int displayFamily = familyID;
        int displayModel = modelID;

        if (familyID == 0xF)
            displayFamily = familyID + extendedFamilyID;

        if (familyID == 0x6 || familyID == 0xF)
            displayModel = modelID + (extendedModelID << 4);

        info.family = displayFamily;
        info.model = displayModel;
    }

    return info;
}

std::string GetCPUInfo() {
    CPUInfoData cpu = GetDetailedCPUInfo();

    std::string manufacturer;
    if (cpu.manufacturer.find("GenuineIntel") != std::string::npos)
        manufacturer = "Intel";
    else if (cpu.manufacturer.find("AuthenticAMD") != std::string::npos)
        manufacturer = "AMD";
    else
        manufacturer = "����������� �������������";

    std::ostringstream info;
    info << "�������������: " << manufacturer << "\n"
        << "������ ��������: " << cpu.brand << "\n";

    // ������ �������� "�������������" �������� ���������
    if (manufacturer == "Intel") {
        if (cpu.family == 6) {
            info << "���������: Intel Core (Family 6)\n";
        }
        else if (cpu.family == 15) {
            info << "���������: Intel NetBurst (Family 15)\n";
        }
        else {
            info << "���������: Family " << cpu.family << "\n";
        }
    }
    else {
        info << "���������: " << cpu.family << "\n";
    }

    info << "������: " << cpu.model << "\n";

    // --- �������������� CPU-���� ����� WMI ---
    std::lock_guard<std::mutex> lock(g_wmiMutex);
    if (g_pSvc) {
        IEnumWbemClassObject* pEnumerator = NULL;
        HRESULT hres = g_pSvc->ExecQuery(
            bstr_t("WQL"),
            bstr_t(L"SELECT NumberOfCores,NumberOfLogicalProcessors,MaxClockSpeed,L2CacheSize,L3CacheSize FROM Win32_Processor"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator
        );

        if (SUCCEEDED(hres)) {
            IWbemClassObject* pclsObj = NULL;
            ULONG uReturn = 0;

            if (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
                std::string cores = GetWMIPropertyFromObject(pclsObj, L"NumberOfCores");
                std::string logical = GetWMIPropertyFromObject(pclsObj, L"NumberOfLogicalProcessors");
                std::string maxClock = GetWMIPropertyFromObject(pclsObj, L"MaxClockSpeed");
                std::string l2cache = GetWMIPropertyFromObject(pclsObj, L"L2CacheSize");
                std::string l3cache = GetWMIPropertyFromObject(pclsObj, L"L3CacheSize");

                info << "����: " << cores << "\n"
                    << "���������� �����������: " << logical << "\n"
                    << "����. �������: " << maxClock << " ���\n"
                    << "L2 ���: " << l2cache << " ��\n"
                    << "L3 ���: " << l3cache << " ��\n";

                pclsObj->Release();
            }
            pEnumerator->Release();
        }
    }

    return info.str();
}

// ========= 4) ����������� ����� =========
std::string GetMotherboardInfo() {
    std::ostringstream oss;

    std::lock_guard<std::mutex> lock(g_wmiMutex);
    if (g_pSvc) {
        IEnumWbemClassObject* pEnumerator = NULL;
        HRESULT hres = g_pSvc->ExecQuery(
            bstr_t("WQL"),
            bstr_t(L"SELECT Manufacturer,Product,SerialNumber,Version FROM Win32_BaseBoard"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator
        );
        if (SUCCEEDED(hres)) {
            IWbemClassObject* pclsObj = NULL;
            ULONG uReturn = 0;

            if (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
                std::string man = GetWMIPropertyFromObject(pclsObj, L"Manufacturer");
                std::string product = GetWMIPropertyFromObject(pclsObj, L"Product");
                std::string serial = GetWMIPropertyFromObject(pclsObj, L"SerialNumber");
                std::string version = GetWMIPropertyFromObject(pclsObj, L"Version");

                oss << "�������������: " << man << "\n"
                    << "�������: " << product << "\n"
                    << "������: " << version << "\n"
                    << "�������� �����: " << serial << "\n";

                pclsObj->Release();
            }
            pEnumerator->Release();
        }
    }

    return oss.str();
}

// ========= 5) GPU =========
std::string DetailedGPUInfo() {
    std::ostringstream gpuDetails;

    std::lock_guard<std::mutex> lock(g_wmiMutex);
    if (g_pSvc) {
        IEnumWbemClassObject* pEnumerator = NULL;
        HRESULT hres = g_pSvc->ExecQuery(
            bstr_t("WQL"),
            bstr_t(L"SELECT * FROM Win32_VideoController"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator
        );

        if (SUCCEEDED(hres)) {
            IWbemClassObject* pclsObj = NULL;
            ULONG uReturn = 0;

            while (pEnumerator) {
                HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (0 == uReturn) break;

                gpuDetails << "=== ���������� � ���������� ===\n";
                gpuDetails << "��������: "
                    << GetWMIPropertyFromObject(pclsObj, L"Name") << "\n";
                gpuDetails << "��������: "
                    << GetWMIPropertyFromObject(pclsObj, L"Description") << "\n";
                gpuDetails << "�������������: "
                    << GetWMIPropertyFromObject(pclsObj, L"AdapterCompatibility") << "\n";

                std::string driverVersion = GetWMIPropertyFromObject(pclsObj, L"DriverVersion");
                gpuDetails << "������ ��������: " << driverVersion << "\n";

                std::string driverDateRaw = GetWMIPropertyFromObject(pclsObj, L"DriverDate");
                std::string driverDate = ParseWMIDate(driverDateRaw);
                gpuDetails << "���� ��������: " << driverDate << "\n";

                gpuDetails << "��������������: "
                    << GetWMIPropertyFromObject(pclsObj, L"VideoProcessor") << "\n";
                gpuDetails << "����� �����������: "
                    << GetWMIPropertyFromObject(pclsObj, L"VideoModeDescription") << "\n";
                gpuDetails << "----------------------------------------\n";

                pclsObj->Release();
            }

            pEnumerator->Release();
        }
    }

    return gpuDetails.str();
}

// ========= 6) ����������� ������ =========
std::string GetMemoryInfo() {
    std::ostringstream oss;
    std::lock_guard<std::mutex> lock(g_wmiMutex);
    if (g_pSvc) {
        IEnumWbemClassObject* pEnumerator = NULL;
        HRESULT hres = g_pSvc->ExecQuery(
            bstr_t("WQL"),
            bstr_t(L"SELECT Manufacturer,PartNumber,Capacity,Speed FROM Win32_PhysicalMemory"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator
        );
        if (SUCCEEDED(hres)) {
            IWbemClassObject* pclsObj = NULL;
            ULONG uReturn = 0;

            bool anyMemory = false;
            while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
                anyMemory = true;
                std::string man = GetWMIPropertyFromObject(pclsObj, L"Manufacturer");
                std::string part = GetWMIPropertyFromObject(pclsObj, L"PartNumber");
                std::string capStr = GetWMIPropertyFromObject(pclsObj, L"Capacity");
                std::string speedStr = GetWMIPropertyFromObject(pclsObj, L"Speed");

                long long capacity = 0;
                try {
                    capacity = std::stoll(capStr);
                }
                catch (...) {}

                double capacityGB = capacity / (1024.0 * 1024.0 * 1024.0);

                oss << "=== ������ ������ ===\n";
                oss << "�������������: " << man << "\n"
                    << "PartNumber: " << part << "\n";
                if (capacityGB > 0.1) {
                    oss << "�����: " << std::fixed << std::setprecision(2)
                        << capacityGB << " ��\n";
                }
                else {
                    oss << "�����: " << capStr << " ����\n";
                }
                oss << "�������: " << speedStr << " ���\n\n";

                pclsObj->Release();
            }

            if (!anyMemory) {
                oss << "�� ������� �������� ���������� � ������.\n";
            }
            pEnumerator->Release();
        }
    }
    return oss.str();
}

// ������ ����������������� WMI-������� ��� GetDevicesInfo
std::string GetDevicesInfo() {
    std::ostringstream oss;
    std::lock_guard<std::mutex> lock(g_wmiMutex);
    if (g_pSvc) {
        IEnumWbemClassObject* pEnumerator = NULL;
        HRESULT hres = g_pSvc->ExecQuery(
            bstr_t("WQL"),
            bstr_t(L"SELECT Name, Manufacturer, Description, DeviceID FROM Win32_PnPEntity WHERE PNPDeviceID IS NOT NULL"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator
        );
        if (SUCCEEDED(hres)) {
            IWbemClassObject* pclsObj = NULL;
            ULONG uReturn = 0;
            bool anyDev = false;
            while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
                anyDev = true;
                std::string name = GetWMIPropertyFromObject(pclsObj, L"Name");
                std::string man = GetWMIPropertyFromObject(pclsObj, L"Manufacturer");
                std::string desc = GetWMIPropertyFromObject(pclsObj, L"Description");
                std::string devID = GetWMIPropertyFromObject(pclsObj, L"DeviceID");

                // ���������� ��������� �� ����������� ��������� (��������, ���������� �������� ���������)
                if (desc.find("USB") != std::string::npos || desc.find("Bluetooth") != std::string::npos) {
                    oss << "=== ���������� ===\n";
                    oss << "���: " << name << "\n"
                        << "�������������: " << man << "\n"
                        << "��������: " << desc << "\n"
                        << "DeviceID: " << devID << "\n\n";
                }

                pclsObj->Release();
            }

            if (!anyDev) {
                oss << "���������� �� �������.\n";
            }
            pEnumerator->Release();
        }
    }
    return oss.str();
}