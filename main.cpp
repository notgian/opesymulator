#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>

// to make our lives easier
using std::cout;
using std::endl;
using std::cin;
using std::string;

void print_welcome() {
    cout << endl << endl;

    cout << "   ____  ____  _____________  ____  _____  ____    ___  __________  ____ \n"
        << "  / __ \\/ __ \\/ ____/ ___/\\ \\/ /  |/  / / / / /   /   |/_  __/ __ \\/ __ \\\n"
        << " / / / / /_/ / __/  \\__ \\  \\  / /|_/ / / / / /   / /| | / / / / / / /_/ /\n"
        << "/ /_/ / ____/ /___ ___/ /  / / /  / / /_/ / /___/ ___ |/ / / /_/ / _, _/ \n"
        << "\\____/_/   /_____//____/  /_/_/  /_/\\____/_____/_/  |_/_/  \\____/_/ |_|  \n" << endl;

    cout << endl << "Welcome" << endl << "CSOPESY Emulator" << endl;
    cout << "Agsalon - Ercia - Garcia - Ortha" << endl;
    cout << endl;
}

enum class CommandStatus {
    Success =  0,
    ExitRequest = 1,
    UnknownCommand = -1,
};

/**
 * Acts as a dispatcher for each command call
 *
 * TODO: this will likely be replaced by something more structured
 *
 *
 * Quick guide on the return codes:
 *
 */
CommandStatus cmd_dispatch(const string& command) {
    static const std::unordered_map<std::string, std::function<CommandStatus()>> handlers = {
        {"initialize", []() {
            cout << "'initialize' command recognized. Doing something." << endl;
            return CommandStatus::Success;
        }},
        {"screen", []() {
            cout << "'screen' command recognized. Doing something." << endl;
            return CommandStatus::Success;
        }},
        {"scheduler-start", []() {
            cout << "'scheduler-start' command recognized. Doing something." << endl;
            return CommandStatus::Success;
        }},
        {"scheduler-stop", []() {
            cout << "'scheduler-stop' command recognized. Doing something." << endl;
            return CommandStatus::Success;
        }},
        {"report-util", []() {
            cout << "'report-util' command recognized. Doing something." << endl;
            return CommandStatus::Success;
        }},
        {"clear", []() { 
            cout << "\033[2J";
            print_welcome();
            return CommandStatus::Success; 
        }},
        {"exit", []() { return CommandStatus::ExitRequest; }},
        {"help", []() { 
            // TODO: Implement help display
            return CommandStatus::Success; 
        }},
    };

    auto it = handlers.find(command);
    if (it != handlers.end()) {
        return it->second(); // Execute handler
    }

    return CommandStatus::UnknownCommand;
}


int main (int argc, char *argv[]) {
    print_welcome();

    bool running = true;
    string cmdText = "Command";

    while (running) {
        cout << cmdText << "> ";

        string inputCommand;
        if (!getline(cin, inputCommand)) break;
        cout << endl;

        CommandStatus status = cmd_dispatch(inputCommand);
        switch (status) {
            case CommandStatus::UnknownCommand:
                cout << "Command not found: " << inputCommand << endl;
                break;
            case CommandStatus::ExitRequest:
                cout << "EXIT signal received" << endl;
                cout << "bye bye" << endl;
                running = false;
                break;
            // Success. Do nothing (for now)
            case CommandStatus::Success:
                break;
        }

    }
    
    return 0;
}
