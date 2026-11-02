#include "Kernel.h"
#include "PrintCommand.h"
#include "ProcessScreen.h"
#include "Config.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <random>
#include <conio.h>
#include <windows.h>

namespace {

constexpr const char* REPORT_FILE_PATH = "csopesy-log.txt";

// Strips leading and trailing spaces from a command argument
std::string trimSpaces(const std::string& text) {
    size_t begin = text.find_first_not_of(' ');
    if (begin == std::string::npos) return "";
    size_t end = text.find_last_not_of(' ');
    return text.substr(begin, end - begin + 1);
}

}

Kernel::Kernel(int screenWidth, int screenHeight)
    : mainDisplay_(screenWidth, screenHeight),
      consoleScreen_(0, 0, screenWidth, screenHeight, "Command> "),
      screenWidth_(screenWidth),
      screenHeight_(screenHeight),
      activeScreen_(nullptr),
      running_(true) {
    
    printWelcome();
    consoleScreen_.print("Type 'help' to see the available commands.");

    // Wire up the command submission callback to the interpreter
    consoleScreen_.setOnCommandSubmitted([this](const std::string& commandText) {
        CommandResult result = interpreter_.executeCommand(commandText);

        // Print outputs back to the console screen
        consoleScreen_.printLines(result.output);

        // Handle system level command statuses
        switch (result.status) {
            case CommandStatus::EXIT_REQUESTED:
                running_ = false;
                break;
            case CommandStatus::CLEAR_HISTORY:
                consoleScreen_.clearHistory();
                break;
            case CommandStatus::SUCCESS:
            case CommandStatus::NOT_FOUND:
            case CommandStatus::CMD_ERROR:
                break;
        }
    });
}

void Kernel::onInitialization() {
    // Initialize the cpu and scheduler
    cpu_ = std::make_unique<CPU>(emulatorConfig_.getNumCpu(), emulatorConfig_.getDelaysPerExec());
    scheduler_ = std::make_unique<Scheduler>(emulatorConfig_.getScheduler(), emulatorConfig_.getQuantumCycles());

    // A process that executes SLEEP leaves its core and waits in the scheduler's sleep list
    cpu_->setOnSleep([this](std::shared_ptr<ProcessControlBlock> pcb) {
        scheduler_->addSleepingProcess(pcb);
    });

    // Round robin: a process whose quantum expired goes to the back of the ready queue
    cpu_->setOnPreempt([this](std::shared_ptr<ProcessControlBlock> pcb) {
        scheduler_->addProcess(pcb);
    });

    // Register commands which only become available on initialization

    // 'screen -ls' prints the CPU utilization and the running / finished process lists
    interpreter_.registerCommand("screen -ls", [this](const std::string& args) {
        return CommandResult{CommandStatus::SUCCESS, buildUtilizationReport()};
    });

    // 'screen -s' creates a new process
    interpreter_.registerCommand("screen -s", [this](const std::string& args) {
        std::string procName = trimSpaces(args);
        if (procName.empty()) {
            return CommandResult{CommandStatus::CMD_ERROR, {"Usage: screen -s <process_name>"}};
        }
        if (findProcess(procName)) {
            return CommandResult{CommandStatus::CMD_ERROR, {"Process " + procName + " already exists."}};
        }

        std::shared_ptr<ProcessScreen> process = createProcess(procName);
        generateInstructions(*process);
        registerProcess(process);
        scheduler_->addProcess(process);

        return CommandResult{CommandStatus::SUCCESS, {"Process '" + procName + "' created!"}};
    });

    // 'screen -r' reattaches to a process that has not finished yet
    interpreter_.registerCommand("screen -r", [this](const std::string& args) {
        std::string procName = trimSpaces(args);
        if (procName.empty()) {
            return CommandResult{CommandStatus::CMD_ERROR, {"Usage: screen -r <process_name>"}};
        }

        std::shared_ptr<ProcessScreen> process = findProcess(procName);
        if (!process || process->getState() == ProcessState::TERMINATED) {
            return CommandResult{CommandStatus::CMD_ERROR, {"Process " + procName + " not found."}};
        }

        attachScreen(process);
        return CommandResult{CommandStatus::SUCCESS, {}};
    });

    interpreter_.registerCommand("screen", [this](const std::string& args) {
        return CommandResult{CommandStatus::CMD_ERROR, {"Usage: screen -ls OR screen -s <process_name> OR screen -r <process_name>"}};
    }, "  screen         - lists processes (-ls), creates (-s <name>) or reattaches (-r <name>)" );

    // 'report-util' saves the same report as 'screen -ls' into csopesy-log.txt
    interpreter_.registerCommand("report-util", [this](const std::string& args) {
        std::ofstream reportFile(REPORT_FILE_PATH);
        if (!reportFile.is_open()) {
            return CommandResult{CommandStatus::CMD_ERROR, {"Report Error: could not write to " + std::string(REPORT_FILE_PATH)}};
        }

        for (const std::string& line : buildUtilizationReport()) {
            reportFile << line << "\n";
        }
        reportFile.close();

        // Show the full path so the file is easy to find
        char fullPath[MAX_PATH];
        std::string reportPath = GetFullPathNameA(REPORT_FILE_PATH, MAX_PATH, fullPath, nullptr) > 0
            ? fullPath
            : REPORT_FILE_PATH;

        return CommandResult{CommandStatus::SUCCESS, {"Report generated at " + reportPath + "!"}};
    }, "  report-util    - saves the CPU utilization report to csopesy-log.txt" );

    // Multi-threaded execution: 1 scheduler thread + 1 worker thread per core.
    // Everything is queued now (first-come-first-serve order = screen_01 first).
    for (const auto& pcb : allProcesses_) {
        scheduler_->addProcess(pcb);
    }

    cpu_->start();
    scheduler_->start(*cpu_);
}

void Kernel::run() {
    clearScreen();
    allProcesses_.clear();

    // Register Initialize command
    interpreter_.registerCommand("initialize", [this](const std::string& args) {
        // Relative to the working directory (the project folder when run through VSCode or build.bat)
        std::string configPath = Config::DEFAULT_PATH;
        std::vector<std::string> message;
        bool initSuccess = false;

        ConfigLoadStatus configStatus = emulatorConfig_.load(configPath);

        switch (configStatus) {
            case ConfigLoadStatus::Reinitialization:
                message.push_back("Initialization Error: config already initialized.");
                break;
            case ConfigLoadStatus::FileNotFound:
                message.push_back("Initialization Error: config " + configPath + " not found.");
                break;
            case ConfigLoadStatus::InvalidValue:
                message.push_back("Initialization Error: Attribute(s) with invalid value(s)");
                break;
            case ConfigLoadStatus::MissingAttribute:
                message.push_back("Initialization Error: missing attribute(s)");
                break;
            case ConfigLoadStatus::Success:
                message.push_back("Emulator successfully initialized.");
                initSuccess = true;
                break;

        }

        if (!initSuccess) {
            return CommandResult{CommandStatus::CMD_ERROR, message};
        }

        onInitialization();
        return CommandResult{CommandStatus::SUCCESS, message};
    }, "  initialize     - initializes the emulator configuration" );

    // ---------------------------------------------------------------------

    // Main (UI) thread: keyboard input + rendering only
    while (running_) {
        std::shared_ptr<ConsoleScreen> active = activeScreen_;
        ConsoleScreen* currentScreen = active 
            ? static_cast<ConsoleScreen*>(active.get())
            : static_cast<ConsoleScreen*>(&consoleScreen_);

        if (_kbhit()) {
            int intercepted = _getch();

            switch (intercepted) {
                case '\r': // Enter
                    currentScreen->handleEnter();
                    break;
                case '\b': // Backspace
                case 127:  // DEL
                    currentScreen->handleBackspace();
                    break;
                case 0:
                case 224:
                    if (_kbhit()) {
                        currentScreen->handleSpecial(_getch());
                    }
                    break;
                default:
                    if (intercepted >= 32 && intercepted < 127) {
                        currentScreen->handleKeyPress(static_cast<char>(intercepted));
                    }
                    break;
            }
        }

        // Update display buffer and render
        mainDisplay_.update(*currentScreen);
        mainDisplay_.draw();
        mainDisplay_.clearBuffer();

        // Brief pause to prevent 100% CPU usage
        Sleep(10);
    }

    // Shut the scheduler thread and the core worker threads down cleanly
    if (scheduler_) scheduler_->stop();
    if (cpu_) cpu_->stop();
}

std::shared_ptr<ProcessScreen> Kernel::createProcess(const std::string& name) {
    auto pcb = std::make_shared<ProcessScreen>(nextPid_++, name, 0, 0, screenWidth_, screenHeight_);

    pcb->setOnExit([this]() {
        activeScreen_ = nullptr; // Returns control back to consoleScreen_
        consoleScreen_.print("Returned to main console.");
    });

    return pcb;
}

void Kernel::registerProcess(const std::shared_ptr<ProcessScreen>& process) {
    std::lock_guard<std::mutex> lock(processesMutex_);
    allProcesses_.push_back(process);
}

// Gives the process a random number of instructions between min-ins and max-ins.
// For now every instruction is the default PRINT from the specs; scheduler-start
// can replace this with randomized instruction types.
void Kernel::generateInstructions(ProcessScreen& process) {
    static std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<std::uint64_t> countDist(emulatorConfig_.getMinIns(), emulatorConfig_.getMaxIns());

    std::uint64_t instructionCount = countDist(rng);
    std::string message = "Hello world from " + process.getName() + "!";

    for (std::uint64_t i = 0; i < instructionCount; ++i) {
        process.addCommand(std::make_shared<PrintCommand>(process.getPID(), message));
    }
}

std::shared_ptr<ProcessScreen> Kernel::findProcess(const std::string& name) {
    std::lock_guard<std::mutex> lock(processesMutex_);
    for (const auto& pcb : allProcesses_) {
        if (pcb->getName() == name) {
            return std::dynamic_pointer_cast<ProcessScreen>(pcb);
        }
    }
    return nullptr;
}

void Kernel::attachScreen(const std::shared_ptr<ProcessScreen>& process) {
    activeScreen_ = process;
}

std::vector<std::string> Kernel::buildUtilizationReport() {
    const std::string separator(38, '-');
    char buf[160];
    std::vector<std::string> out;

    std::vector<RunningEntry> running = cpu_->getRunningProcesses();
    int coresTotal = cpu_->getNumCores();
    int coresUsed = static_cast<int>(running.size());
    int utilization = coresTotal > 0 ? (coresUsed * 100) / coresTotal : 0;

    out.push_back("CPU utilization: " + std::to_string(utilization) + "%");
    out.push_back("Cores used: " + std::to_string(coresUsed));
    out.push_back("Cores available: " + std::to_string(coresTotal - coresUsed));
    out.push_back("");
    out.push_back(separator);

    // ---- Running processes (one per busy core) ----
    out.push_back("Running processes:");
    for (const auto& entry : running) {
        const auto& p = entry.process;
        std::snprintf(buf, sizeof(buf), "%-12s (%s)    Core: %d   %d / %d",
                p->getName().c_str(), p->getCreationTime().c_str(), entry.coreId,
                p->getProgramCounter(), p->getTotalCommands());
        out.push_back(buf);
    }

    out.push_back("");

    // ---- Finished processes ----
    std::vector<std::shared_ptr<ProcessControlBlock>> processes;
    {
        std::lock_guard<std::mutex> lock(processesMutex_);
        processes = allProcesses_;
    }

    out.push_back("Finished processes:");
    for (const auto& p : processes) {
        if (p->getState() != ProcessState::TERMINATED) continue;
        std::snprintf(buf, sizeof(buf), "%-12s (%s)    Finished   %d / %d",
                p->getName().c_str(), p->getCreationTime().c_str(),
                p->getProgramCounter(), p->getTotalCommands());
        out.push_back(buf);
    }

    out.push_back(separator);

    return out;
}

void Kernel::printWelcome() {
    consoleScreen_.print("    ____  ____  _____________   ____  _____   ____    ___  __________  ____ ");
    consoleScreen_.print("   / __ \\/ __ \\/ ____/ ___/\\ \\/ /  |/  / / / / /   /   |/_  __/ __ \\/ __ \\");
    consoleScreen_.print("  / / / / /_/ / __/  \\__ \\  \\  / /|_/ / / / / /   / /| | / / / / / / /_/ /");
    consoleScreen_.print(" / /_/ / ____/ /___ ___/ /  / / /  / / /_/ / /___/ ___ |/ / / /_/ / _, _/ ");
    consoleScreen_.print(" \\____/_/   /_____//____/  /_/_/  /_/\\____/_____/_/  |_/_/  \\____/_/ |_| ");
    consoleScreen_.print("");
    consoleScreen_.print("Welcome");
    consoleScreen_.print("CSOPESY Emulator");
    consoleScreen_.print("Agsalon - Ercia - Garcia - Ortha");
    consoleScreen_.print("");
}

void Kernel::clearScreen() {
    std::cout << "\033[2J\033[H";
}
