#include <iostream>
#include <string>
#include <conio.h>
#include <windows.h>
#include "Display.h"
#include "Component.h"
#include "ConsoleScreen.h"
#include "CommandInterpreter.h"
#include "Kernel.h"

int main(int argc, char *argv[]) {
    // Instantiate the kernel with target screen dimensions
    Kernel os(100, 25);
    
    // Start the OS simulation loop
    os.run();

    return 0;
}