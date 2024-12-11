#include <iostream>
#include <windows.h>
#include <chrono>
#include <iomanip>
#include <sstream>

HANDLE openSerialPort(LPCSTR portName, DWORD baudRate) {
    HANDLE hSerial = CreateFile(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening serial port" << std::endl;
        return INVALID_HANDLE_VALUE;
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error getting COM state" << std::endl;
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    dcbSerialParams.BaudRate = baudRate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error setting COM state" << std::endl;
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    return hSerial;
}

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time);
    std::ostringstream oss;
    oss << "EST: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void processBufferAndWrite(HANDLE hFile, const char* buffer, DWORD bytesRead) {
    std::istringstream iss(std::string(buffer, bytesRead));
    std::string line;
    DWORD dwBytesWritten = 0;

    while (std::getline(iss, line)) {
        std::ostringstream oss;
        oss << getCurrentTimestamp() << " " << line << "\r\n";
        std::string output = oss.str();
        WriteFile(hFile, output.c_str(), static_cast<DWORD>(output.size()), &dwBytesWritten, NULL);
    }
}

void readDataAndWriteToFile(HANDLE hSerial, const char* filePath) {
    DWORD bytesRead;
    char buffer[2048];
    HANDLE hFile = CreateFile(filePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening file for writing" << std::endl;
        return;
    }

    auto startTime = std::chrono::steady_clock::now();
    while (true) {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::minutes>(currentTime - startTime);
        if (elapsedTime.count() >= 1) break;

        ReadFile(hSerial, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';  // Null-terminate the buffer
            processBufferAndWrite(hFile, buffer, bytesRead);
        }
    }

    CloseHandle(hFile);
}

int main() {
    LPCSTR portName = "COM3";
    DWORD baudRate = CBR_9600;

    HANDLE hSerial = openSerialPort(portName, baudRate);
    if (hSerial == INVALID_HANDLE_VALUE) {
        return 1;
    }

    readDataAndWriteToFile(hSerial, "output.txt");
    CloseHandle(hSerial);

    return 0;
}
