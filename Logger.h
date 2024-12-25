#pragma once
#include <string>
#include <fstream>
#include <mutex>

class Logger {
public:
    static Logger& getInstance();
    void log(const std::string& message);
private:
    Logger(); // Конструктор по умолчанию
    std::ofstream logFile;
    std::mutex logMutex;
};
