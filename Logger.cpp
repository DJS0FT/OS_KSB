#include "Logger.h"
#include <iostream>
#include <ctime>

Logger::Logger() {
    logFile.open("SystemInfoTool.log", std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "�� ������� ������� ���� �����." << std::endl;
    }
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open()) {
        // ��������� �������� �������
        std::time_t now = std::time(nullptr);
        std::tm* ptm = std::localtime(&now);
        char timeStr[32];
        // �������������� �������
        std::strftime(timeStr, 32, "%Y-%m-%d %H:%M:%S", ptm);

        logFile << "[" << timeStr << "] " << message << std::endl;
    }
}
