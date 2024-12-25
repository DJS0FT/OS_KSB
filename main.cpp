#include <iostream>
#include <future>
#include <mutex>
#include "WMIHelper.h"
#include "SystemInfo.h"
#include "Logger.h"

// ���������� ���������� ��� WMI
IWbemServices* g_pSvc = nullptr;
std::mutex g_wmiMutex;

// ========= ����� ����� � ��������� (main) =========
int main()
{
    setlocale(LC_ALL, "Russian");

    // ������������� COM � WMI
    if (FAILED(InitializeCOMAndSecurity())) {
        std::cerr << "�� ������� ���������������� COM." << std::endl;
        return 1;
    }

    if (FAILED(InitializeWMI())) {
        std::cerr << "�� ������� ���������������� WMI." << std::endl;
        CoUninitialize();
        return 1;
    }

    // ��� ������������� ���������� ���������� std::async
    auto futureWindowsInfo = std::async(std::launch::async, GetWindowsFullInfo);
    auto futureCPUInfo = std::async(std::launch::async, GetCPUInfo);
    auto futureMotherboardInfo = std::async(std::launch::async, GetMotherboardInfo);
    auto futureGPUInfo = std::async(std::launch::async, DetailedGPUInfo);
    auto futureMemoryInfo = std::async(std::launch::async, GetMemoryInfo);
    auto futureDevicesInfo = std::async(std::launch::async, GetDevicesInfo);

    // ��������� �����������
    std::string windowsInfo = futureWindowsInfo.get();
    std::string cpuInfo = futureCPUInfo.get();
    std::string motherboardInfo = futureMotherboardInfo.get();
    std::string gpuInfo = futureGPUInfo.get();
    std::string memoryInfo = futureMemoryInfo.get();
    std::string devicesInfo = futureDevicesInfo.get();

    // ������������ WMI � COM
    if (g_pSvc) {
        g_pSvc->Release();
        g_pSvc = nullptr;
    }
    CoUninitialize();

    // ����� ����� ���� ����� ������, ������� �����������
    auto printDelimiter = []() {
        std::cout << "\n=======================================================\n";
        };

    // 1) ������� ������ Windows
    printDelimiter();
    std::cout << "[ ���������� �� �� ]" << std::endl;
    std::cout << windowsInfo << std::endl;

    // 2) CPU
    printDelimiter();
    std::cout << "[ ���������� � CPU ]" << std::endl;
    std::cout << cpuInfo << std::endl;

    // 3) ����������� �����
    printDelimiter();
    std::cout << "[ ���������� � ����������� ����� ]" << std::endl;
    std::cout << motherboardInfo << std::endl;

    // 4) GPU
    printDelimiter();
    std::cout << "[ ���������� � ����������� ]" << std::endl;
    std::cout << gpuInfo << std::endl;

    // 5) ������
    printDelimiter();
    std::cout << "[ ���������� � ������� ������ ]" << std::endl;
    std::cout << memoryInfo << std::endl;

    // 6) ������ PnP-����������
    printDelimiter();
    std::cout << "[ ���������� � PnP-����������� ]" << std::endl;
    std::cout << devicesInfo << std::endl;

    printDelimiter();
    std::cout << "������� Enter ��� ������..." << std::endl;
    std::cin.get();

    return 0;
}
