#ifndef SYSTEMUTILS_H
#define SYSTEMUTILS_H

#include <string>

class SystemUtils {
public:
    static void abort(std::string msg);
    static void warn(std::string msg);

    static void takeTime();
    static double stopTime();

    static bool readFile(std::string& file, std::string& res,
                         std::string ignoreSign = "");

    static long getTotalVirtualMemory();
    static long getUsedVirtualMemory();
    static int getVirtualMemoryUsedByThis();

    static long getTotalRAM();
    static long getUsedRAM();
    static int getRAMUsedByThis();

    static void initCPUMeasurementOfThis();
    static double getCPUUsageOfThis();

protected:
    static clock_t start;
    static bool clockRunning;
    static bool CPUMeasurementRunning;
    static bool CPUMeasurementOfProcessRunning;

    static clock_t lastCPU;
    static clock_t lastSysCPU;
    static clock_t lastUserCPU;
    static int numProcessors;

    static int parseLine(char* line);

private:
    SystemUtils() {}
};

#endif
