#include "system_utils.h"
#include "string_utils.h"

#include <cassert>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/sysinfo.h"
#include "sys/times.h"
#include "sys/types.h"
#include "sys/vtimes.h"

clock_t SystemUtils::start = 0;
clock_t SystemUtils::lastCPU = 0;
clock_t SystemUtils::lastSysCPU = 0;
clock_t SystemUtils::lastUserCPU = 0;
int SystemUtils::numProcessors = 0;
bool SystemUtils::clockRunning = false;
bool SystemUtils::CPUMeasurementOfProcessRunning = false;

void SystemUtils::abort(std::string msg) {
    std::cerr << msg << std::endl;
    exit(0);
}

void SystemUtils::warn(std::string msg) {
    std::cerr << msg << std::endl;
}

void SystemUtils::takeTime() {
    if (!clockRunning) {
        clockRunning = true;
        start = clock();
    }
}

double SystemUtils::stopTime() {
    if (!clockRunning) {
        return -1.0;
    }

    clockRunning = false;
    return double(clock() - start) / (double)CLOCKS_PER_SEC;
}

bool SystemUtils::readFile(std::string& file, std::string& res,
                           std::string ignoreSign) {
    std::ifstream ifs(file.c_str());
    if (!ifs) {
        return false;
    }

    std::string tmp;
    std::stringstream ss;
    while (std::getline(ifs, tmp)) {
        if (ignoreSign != "") {
            StringUtils::deleteCommentFromLine(tmp, ignoreSign);
        }

        StringUtils::trim(tmp);

        if (tmp.length() > 0) {
            ss << tmp << std::endl;
        }
    }
    ifs.close();
    res = ss.str();
    return true;
}

// returns the available total virtual memory
long SystemUtils::getTotalVirtualMemory() {
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    long res = memInfo.totalram;

    res += memInfo.totalswap;
    res *= memInfo.mem_unit;
    return res;
}

// returns the virtual memory used (by all processes)
long SystemUtils::getUsedVirtualMemory() {
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    long res = memInfo.totalram - memInfo.freeram;
    res += memInfo.totalswap - memInfo.freeswap;
    res *= memInfo.mem_unit;
    return res;
}

int SystemUtils::parseLine(char* line) {
    int i = (int)strlen(line);
    while (*line < '0' || *line > '9') {
        line++;
    }
    line[i - 3] = '\0';
    i = atoi(line);
    return i;
}

// returns the virtual memory used by this process in KB
int SystemUtils::getVirtualMemoryUsedByThis() {
    FILE* file = fopen("/proc/self/status", "r");
    int res = -1;
    char line[128];

    while (fgets(line, 128, file) != nullptr) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            res = parseLine(line);
            break;
        }
    }
    fclose(file);
    return res;
}

// returns the available total RAM
long SystemUtils::getTotalRAM() {
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    long res = memInfo.totalram;
    res *= memInfo.mem_unit;
    return res;
}

////returns the RAM used (by all processes)
long SystemUtils::getUsedRAM() {
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    long res = memInfo.totalram - memInfo.freeram;
    res *= memInfo.mem_unit;
    return res;
}

// returns the RAM used by this process in KB
int SystemUtils::getRAMUsedByThis() {
    FILE* file = fopen("/proc/self/status", "r");
    int res = -1;
    char line[128];

    while (fgets(line, 128, file) != nullptr) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            res = parseLine(line);
            break;
        }
    }
    fclose(file);
    return res;
}

// inits measurement of CPU usage by this process
void SystemUtils::initCPUMeasurementOfThis() {
    FILE* file;
    struct tms timeSample;
    char line[128];

    lastCPU = times(&timeSample);
    lastSysCPU = timeSample.tms_stime;
    lastUserCPU = timeSample.tms_utime;

    file = fopen("/proc/cpuinfo", "r");
    numProcessors = 0;
    while (fgets(line, 128, file) != nullptr) {
        if (strncmp(line, "processor", 9) == 0) {
            numProcessors++;
        }
    }
    fclose(file);

    CPUMeasurementOfProcessRunning = true;
}

double SystemUtils::getCPUUsageOfThis() {
    if (!CPUMeasurementOfProcessRunning) {
        return -1.0;
    }

    struct tms timeSample;
    clock_t now;
    double res;

    now = times(&timeSample);
    if (now <= lastCPU || timeSample.tms_stime < lastSysCPU ||
        timeSample.tms_utime < lastUserCPU) {
        // Overflow detection. Just skip this value.
        res = -1.0;
    } else {
        res = (double)(timeSample.tms_stime - lastSysCPU) +
              (double)(timeSample.tms_utime - lastUserCPU);
        res /= (double)(now - lastCPU);
        res /= numProcessors;
        res *= 100;
    }
    lastCPU = now;
    lastSysCPU = timeSample.tms_stime;
    lastUserCPU = timeSample.tms_utime;

    return res;
}
