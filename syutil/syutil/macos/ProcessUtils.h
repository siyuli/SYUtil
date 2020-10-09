#pragma once

#import <AppKit/AppKit.h>
#include <string>
#include <memory>
#include <mutex>
#include <map>
#include <functional>
#include <vector>

class TaskHandle;
class MontiorHandle;

typedef void* HANDLE;

class ProcessUtils
{
public:
    using Argument = std::string;
    using Arguments = std::vector<Argument>;

    ProcessUtils();
    ~ProcessUtils();
    bool launchProcess(int argc, const char *argv[], const char *executableName, const char *monitorName, uint32_t& pid);
    int terminateProcess(const unsigned int processId);
    void monitorLaunchedProcess(const char *monitorName, std::function<void()> processDiedCallBack);
    bool checkLaunchedProcessRunning(const char *monitorName  );
private:

    std::shared_ptr<MontiorHandle> mMontiorHandle;

    std::map<std::string, std::shared_ptr<TaskHandle>> tasksMap;
    std::mutex mDataAccessMutex;
};
