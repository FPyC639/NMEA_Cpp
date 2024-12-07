#include <iostream>
#include <windows.h>
#include <chrono>
#include <stdlib.h>
#include <iomanip>

HANDLE openSerialPort(LPCSTR portName, DWORD baudRate) {
    HANDLE hSerial = CreateFile(portName,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL);
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
    if(!SetCommState(hSerial, &dcbSerialParams)){
        std::cerr << "Error setting COM state" << std::endl;
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    return hSerial;
}

std::string serial_TimePoint(const std::chrono::time_point<std::chrono::_V2::system_clock>& time, const std::string& format){
    _putenv("TZ=EST");
    tzset();

    std::time_t temp = std::chrono::_V2::system_clock::to_time_t(time);
    std::tm tm = *std::localtime(&temp);
    std::stringstream ss;
    ss << std::put_time( &tm, format.c_str());
    return ss.str();

}


void readDataAndWriteToFile(HANDLE hSerial, const char* filePath) {
    DWORD bytesRead;
    char buffer[1024];
    DWORD dwBytesWritten = 0;
    HANDLE hFile;

    hFile = CreateFile(filePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening file for writing" << std::endl;
        return;
    }

    auto startTime = std::chrono::steady_clock::now();
    while (true) {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::minutes>(currentTime - startTime);
        if (elapsedTime.count() >= 2) {
            break;
        }
        std::chrono::time_point<std::chrono::_V2::system_clock> input = std::chrono::system_clock::now();
        ReadFile(hSerial, buffer, sizeof(buffer), &bytesRead, NULL);
        if (bytesRead > 0) {
            std::string timestamp = serial_TimePoint(input, "EST: %Y-%m-%d %H:%M:%S\n");
            DWORD timestampLength = static_cast<DWORD>(timestamp.size());
            WriteFile(hFile, timestamp.c_str(),timestampLength,&dwBytesWritten, NULL);
            WriteFile(hFile, buffer, bytesRead, &dwBytesWritten, NULL);
        }
    }
    CloseHandle(hFile);
}

int main() {
    LPCSTR portName = "COM3";
    DWORD baudRate = CBR_9600; // Adjust as needed

    HANDLE hSerial = openSerialPort(portName, baudRate);
    if (hSerial == INVALID_HANDLE_VALUE) {
        return 1; // Opening serial port failed
    }

    readDataAndWriteToFile(hSerial, "output.txt");

    CloseHandle(hSerial);
    return 0;
}
