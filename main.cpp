#include <iostream>
#include <future>
#include <mutex>
#include "WMIHelper.h"
#include "SystemInfo.h"
#include "Logger.h"

// Глобальные переменные для WMI
IWbemServices* g_pSvc = nullptr;
std::mutex g_wmiMutex;

// ========= Точка входа в программу (main) =========
int main()
{
    setlocale(LC_ALL, "Russian");

    // Инициализация COM и WMI
    if (FAILED(InitializeCOMAndSecurity())) {
        std::cerr << "Не удалось инициализировать COM." << std::endl;
        return 1;
    }

    if (FAILED(InitializeWMI())) {
        std::cerr << "Не удалось инициализировать WMI." << std::endl;
        CoUninitialize();
        return 1;
    }

    // Для параллельного выполнения используем std::async
    auto futureWindowsInfo = std::async(std::launch::async, GetWindowsFullInfo);
    auto futureCPUInfo = std::async(std::launch::async, GetCPUInfo);
    auto futureMotherboardInfo = std::async(std::launch::async, GetMotherboardInfo);
    auto futureGPUInfo = std::async(std::launch::async, DetailedGPUInfo);
    auto futureMemoryInfo = std::async(std::launch::async, GetMemoryInfo);
    auto futureDevicesInfo = std::async(std::launch::async, GetDevicesInfo);

    // Получение результатов
    std::string windowsInfo = futureWindowsInfo.get();
    std::string cpuInfo = futureCPUInfo.get();
    std::string motherboardInfo = futureMotherboardInfo.get();
    std::string gpuInfo = futureGPUInfo.get();
    std::string memoryInfo = futureMemoryInfo.get();
    std::string devicesInfo = futureDevicesInfo.get();

    // Освобождение WMI и COM
    if (g_pSvc) {
        g_pSvc->Release();
        g_pSvc = nullptr;
    }
    CoUninitialize();

    // Чтобы вывод было проще читать, выводим разделители
    auto printDelimiter = []() {
        std::cout << "\n=======================================================\n";
        };

    // 1) Базовая версия Windows
    printDelimiter();
    std::cout << "[ Информация об ОС ]" << std::endl;
    std::cout << windowsInfo << std::endl;

    // 2) CPU
    printDelimiter();
    std::cout << "[ Информация о CPU ]" << std::endl;
    std::cout << cpuInfo << std::endl;

    // 3) Материнская плата
    printDelimiter();
    std::cout << "[ Информация о материнской плате ]" << std::endl;
    std::cout << motherboardInfo << std::endl;

    // 4) GPU
    printDelimiter();
    std::cout << "[ Информация о видеокартах ]" << std::endl;
    std::cout << gpuInfo << std::endl;

    // 5) Память
    printDelimiter();
    std::cout << "[ Информация о модулях памяти ]" << std::endl;
    std::cout << memoryInfo << std::endl;

    // 6) Другие PnP-устройства
    printDelimiter();
    std::cout << "[ Информация о PnP-устройствах ]" << std::endl;
    std::cout << devicesInfo << std::endl;

    printDelimiter();
    std::cout << "Нажмите Enter для выхода..." << std::endl;
    std::cin.get();

    return 0;
}
