#include "ram.h"
#include "program_loader.h"

int main() {
    Y86_64::RAM ram;
    Y86_64::ProgramLoader loader;

    loader.load("test.yo", ram);

    ram.write(0, 42);
    return ram.read(0);
}
