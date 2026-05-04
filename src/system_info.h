#pragma once

#include <string>
#include <windows.h>
#include <pdh.h>
#include <dxgi.h>
#include <d3d11.h>
#include <shlwapi.h>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")

struct SystemInfo {
    float cpuUsage;        // Prozentual (0-100)
    float gpuUsage;        // Prozentual (0-100)
    float gpuMemoryUsed;   // In MB
    float gpuMemoryTotal;  // In MB
    float ramUsed;         // In MB
    float ramTotal;        // In MB
    float ramUsagePercent; // Prozentual (0-100)
    std::string osName;
    std::string cpuName;
    std::string gpuName;
};

class SystemInfoGatherer {
private:
    HQUERY cpuQuery;
    HCOUNTER cpuCounter;
    IDXGIAdapter* dxgiAdapter;
    std::string cachedCpuName;
    std::string cachedGpuName;
    float cachedCpuUsage;
    int cpuUpdateCounter;
    bool gpuNameFetched;
    const int CPU_UPDATE_INTERVAL = 15;  // Update every 15 frames (~500ms at 30fps)
    
public:
    SystemInfoGatherer() : cpuQuery(NULL), cpuCounter(NULL), dxgiAdapter(NULL), cachedCpuUsage(0.0f), cpuUpdateCounter(0), gpuNameFetched(false) {
        initCPUMonitoring();
        initGPUMonitoring();
    }
    
    ~SystemInfoGatherer() {
        if (cpuQuery) PdhCloseQuery(cpuQuery);
        if (dxgiAdapter) dxgiAdapter->Release();
    }
    
    void initCPUMonitoring() {
        PdhOpenQueryW(NULL, NULL, &cpuQuery);
        PdhAddEnglishCounterW(cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuCounter);
        PdhCollectQueryData(cpuQuery);
    }
    
    void initGPUMonitoring() {
        IDXGIFactory* factory = nullptr;
        HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
        if (SUCCEEDED(hr) && factory) {
            factory->EnumAdapters(0, &dxgiAdapter);
            factory->Release();
        }
    }
    
    SystemInfo getSystemInfo() {
        SystemInfo info;
        
        // Update CPU usage every 15 frames (~500ms at 30fps)
        cpuUpdateCounter++;
        if (cpuUpdateCounter >= CPU_UPDATE_INTERVAL) {
            cachedCpuUsage = getCPUUsageRaw();
            cpuUpdateCounter = 0;
        }
        info.cpuUsage = cachedCpuUsage;
        
        info.ramUsed = getRamUsedMB();
        info.ramTotal = getRamTotalMB();
        info.ramUsagePercent = (info.ramUsed / info.ramTotal) * 100.0f;
        getOSInfo(info.osName);
        getCPUName(info.cpuName);
        
        // GPU Monitoring
        info.gpuUsage = getGPUUsage();
        getGPUName(info.gpuName);
        
        return info;
    }
    
private:
    float getCPUUsageRaw() {
        if (!cpuQuery || !cpuCounter) return 0.0f;
        
        PdhCollectQueryData(cpuQuery);
        PDH_FMT_COUNTERVALUE counterVal;
        
        if (SUCCEEDED(PdhGetFormattedCounterValue(cpuCounter, PDH_FMT_DOUBLE, NULL, &counterVal))) {
            float val = static_cast<float>(counterVal.doubleValue);
            return (val < 0.0f) ? 0.0f : (val > 100.0f) ? 100.0f : val;
        }
        return 0.0f;
    }
    
    float getGPUUsage() {
        // GPU usage monitoring would require NVIDIA or AMD specific APIs
        // For now returning 0 - can be extended with DXGI or vendor-specific libraries
        return 0.0f;
    }
    
    float getRamUsedMB() {
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        if (GlobalMemoryStatusEx(&memStatus)) {
            return (memStatus.ullTotalPhys - memStatus.ullAvailPhys) / (1024.0f * 1024.0f);
        }
        return 0.0f;
    }
    
    float getRamTotalMB() {
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        if (GlobalMemoryStatusEx(&memStatus)) {
            return memStatus.ullTotalPhys / (1024.0f * 1024.0f);
        }
        return 0.0f;
    }
    
    void getOSInfo(std::string& osName) {
        // Use RtlGetVersion for accurate OS information
        typedef NTSTATUS(WINAPI *fnRtlGetVersion)(LPOSVERSIONINFOEXW);
        HMODULE hMod = GetModuleHandleA("ntdll.dll");
        fnRtlGetVersion pRtlGetVersion = (fnRtlGetVersion)GetProcAddress(hMod, "RtlGetVersion");
        
        if (pRtlGetVersion) {
            OSVERSIONINFOEXW osvi = { 0 };
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            
            if (pRtlGetVersion(&osvi) == 0) {
                char buffer[256];
                // Windows 11 or later
                if (osvi.dwMajorVersion >= 10 && osvi.dwBuildNumber >= 22000) {
                    sprintf_s(buffer, sizeof(buffer), "Windows 11+ (Build %lu)", osvi.dwBuildNumber);
                }
                // Windows 10
                else if (osvi.dwMajorVersion == 10) {
                    sprintf_s(buffer, sizeof(buffer), "Windows 10 (Build %lu)", osvi.dwBuildNumber);
                }
                // Windows 7/8
                else if (osvi.dwMajorVersion == 6) {
                    if (osvi.dwMinorVersion == 3) {
                        sprintf_s(buffer, sizeof(buffer), "Windows 8.1 (Build %lu)", osvi.dwBuildNumber);
                    } else if (osvi.dwMinorVersion == 2) {
                        sprintf_s(buffer, sizeof(buffer), "Windows 8 (Build %lu)", osvi.dwBuildNumber);
                    } else if (osvi.dwMinorVersion == 1) {
                        sprintf_s(buffer, sizeof(buffer), "Windows 7 (Build %lu)", osvi.dwBuildNumber);
                    } else {
                        sprintf_s(buffer, sizeof(buffer), "Windows Vista (Build %lu)", osvi.dwBuildNumber);
                    }
                }
                else {
                    sprintf_s(buffer, sizeof(buffer), "Windows (Build %lu)", osvi.dwBuildNumber);
                }
                osName = buffer;
                return;
            }
        }
        osName = "Unknown OS";
    }
    
    void getGPUName(std::string& gpuName) {
        // Return cached GPU name if already fetched
        if (gpuNameFetched) {
            gpuName = cachedGpuName;
            return;
        }
        
        // Try DXGI first
        if (dxgiAdapter) {
            DXGI_ADAPTER_DESC desc;
            if (SUCCEEDED(dxgiAdapter->GetDesc(&desc))) {
                char buffer[256];
                int result = WideCharToMultiByte(CP_ACP, 0, desc.Description, -1, buffer, sizeof(buffer), NULL, NULL);
                if (result > 0 && strlen(buffer) > 0) {
                    cachedGpuName = buffer;
                    gpuNameFetched = true;
                    gpuName = cachedGpuName;
                    return;
                }
            }
        }
        
        // Fallback 1: Search registry for active ControlSet
        HKEY hKey = NULL;
        char gpuPath[512];
        
        // Find the active ControlSet
        DWORD controlSet = 1;  // Default to ControlSet001
        HKEY hControlSet;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\Select", 0, KEY_READ, &hControlSet) == ERROR_SUCCESS) {
            DWORD size = sizeof(DWORD);
            DWORD csValue = 0;
            if (RegQueryValueExA(hControlSet, "Current", NULL, NULL, (LPBYTE)&csValue, &size) == ERROR_SUCCESS) {
                controlSet = csValue;
            }
            RegCloseKey(hControlSet);
        }
        
        // Try multiple adapter indices
        for (int adapterIdx = 0; adapterIdx < 5; adapterIdx++) {
            sprintf_s(gpuPath, sizeof(gpuPath), 
                "SYSTEM\\ControlSet%03d\\Control\\Class\\{4D36E968-E325-11CE-BFC1-08002BE10318}\\%04d",
                controlSet, adapterIdx);
            
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, gpuPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                char buffer[256] = { 0 };
                DWORD size = sizeof(buffer);
                
                // Try DriverDesc first
                if (RegQueryValueExA(hKey, "DriverDesc", NULL, NULL, (LPBYTE)buffer, &size) == ERROR_SUCCESS && strlen(buffer) > 0) {
                    cachedGpuName = buffer;
                    RegCloseKey(hKey);
                    gpuNameFetched = true;
                    gpuName = cachedGpuName;
                    return;
                }
                
                // Try DeviceDesc as fallback
                size = sizeof(buffer);
                if (RegQueryValueExA(hKey, "DeviceDesc", NULL, NULL, (LPBYTE)buffer, &size) == ERROR_SUCCESS && strlen(buffer) > 0) {
                    // Remove device class prefix if present
                    char* pch = strchr(buffer, ';');
                    if (pch) {
                        pch++;
                        while (*pch == ' ') pch++;
                        cachedGpuName = pch;
                    } else {
                        cachedGpuName = buffer;
                    }
                    RegCloseKey(hKey);
                    gpuNameFetched = true;
                    gpuName = cachedGpuName;
                    return;
                }
                
                RegCloseKey(hKey);
            }
        }
        
        cachedGpuName = "GPU Information Unavailable";
        gpuNameFetched = true;
        gpuName = cachedGpuName;
    }
    
    void getCPUName(std::string& cpuName) {
        if (!cachedCpuName.empty()) {
            cpuName = cachedCpuName;
            return;
        }
        
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            char buffer[256];
            DWORD bufferSize = sizeof(buffer);
            if (RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
                cpuName = buffer;
                cachedCpuName = buffer;
            } else {
                cpuName = "Unknown";
            }
            RegCloseKey(hKey);
        } else {
            cpuName = "Unknown";
        }
    }
};
