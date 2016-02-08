/*
    This is only a main class to test the library
    and to show how to use it you don't need it.
*/

#include <iostream>
#include "gpio.h"
#include <unistd.h>

using namespace std;

int main()
{
    cout << "Started using my library" << endl;

    int error = gpio::init();

    if (error != 0) { return 1; }

    gpio::setPhysicalNumbering(false);

    error = gpio::setOutput(17);
    error = gpio::setOutput(27);
    error = gpio::setOutput(22);

    if (error != 0) { return 1; }

    int second = 1000000;

    gpio::writePin(17);

    usleep(second);

    gpio::writePin(27);
    gpio::clearPin(17);

    usleep(second);

    gpio::writePin(22);
    gpio::clearPin(27);

    usleep(second);

    gpio::clearPin(22);

    gpio::closeMem();

    cout << "Done" << endl;

    cin.get();

    return 0;
}
