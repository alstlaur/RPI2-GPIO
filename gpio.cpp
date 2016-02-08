//
// Created by Alexandre St-Laurent on 1/5/2016.
//

#include "gpio.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

//static const unsigned int baseAddress = 0x20000000; //memory address of the RPI 1
static const unsigned int baseAddress = 0x3f000000; //because the RAM of the PI 2 is 1Gb
static const unsigned int gpio_base = baseAddress + 0x200000;
static const unsigned int GPIO_BLOCK = (4 * 1024);

static bool physicalNumbering = false; // if at false the GPIO Numbering is used else if true the Physical Numbering is used
static int physicalToGPIO[40] = { -1, -1, 2, -1, 3, -1, 4, 14, -1, 15, 17, 18, 27, -1, 22, 23, -1, 24, 10, -1,
                               9, 25, 11, 8, -1, 7, -1, -1, 5, -1, 6, 12, 13, -1, 19, 16, 26, 20, -1, 21 };

volatile unsigned int* mem_ptr;
int mem_fd;

/*
 * ENUMS documentation at https://www.raspberrypi.org/wp-content/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 * Broadcom BCM2835 ARM Datasheet
 * page 90 - 96
 */

//Enum for all the input/output setup registry location 0 to 5
enum GPFSEL
{
    //single increment instead of 4 because it is maped as an int which is 4 bytes
    gpfsel0 = 0x00, //pins 0 to 9
    gpfsel1 = 0x01,//0x04, //pins 10 to 19
    gpfsel2 = 0x02,//0x08, //pins 20 to 29
    gpfsel3 = 0x03,//0x0c, //pins 30 to 39
    gpfsel4 = 0x04,//0x10, //pins 40 to 49
    gpfsel5 = 0x05,//0x14, //pins 50 to 53
};

//must put the right bit at 1 (0 has no effect)
enum GPSET
{
    gpset0 = 0x07,//0x1c, //pins 0 to 31
    gpset1 = 0x08,//0x20, //pins 32 to 53
};

//must put the right bit at 1 (0 has no effect)
enum GPCLR
{
    gpclr0 = 0x0A,//0x28, //pins 0 to 31
    gpclr1 = 0x0B,//0x2c, //pins 32 to 53
};

//Not sure if needed
enum GPLEV
{
    gplev0 = 0x0D,//0x34, //pins 0 to 31
    gplev1 = 0x0E,//0x38, //pins 32 to 53
};

void gpio::setPhysicalNumbering(bool value)
{
    physicalNumbering = value;
}

bool gpio::physicalNumberingOn()
{
    return physicalNumbering;
}

int gpio::init()
{

    //Linux use virtual memory so the file /dev/mem is the way to access the physical memory
    //the first if try to open the file
    //the mmap command map de physical memory passed (gpio_base) to the virtual memory
    //the second if check if the mapping was successful

    if ((mem_fd = open("/dev/mem", O_RDWR)) < 0) // | O_SYNC
    {
        return -1;
    }

    mem_ptr = (unsigned int*)mmap(NULL, GPIO_BLOCK, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, gpio_base);

    if (mem_ptr == MAP_FAILED)
    {
        return -2;
    }

    return 0;
}

int gpio::closeMem()
{
    //remove the volatile
    //Close the mapping and the file
    munmap(const_cast<unsigned int*>(mem_ptr),GPIO_BLOCK);
    close(mem_fd);
    return 0;
}

unsigned int gpio::findAddress(int gpin)
{
    unsigned int mem_register;

    if (gpin <= 9)
    {
        mem_register = gpfsel0;
    }
    else if (gpin <= 19)
    {
        mem_register = gpfsel1;
    }
    else if (gpin <= 29)
    {
        mem_register = gpfsel2;
    }
    else if (gpin <= 39)
    {
        mem_register = gpfsel3;
    }
    else if (gpin <= 49)
    {
        mem_register = gpfsel4;
    }
    else if (gpin <= 53)
    {
        mem_register = gpfsel5;
    }
    else
    {
        mem_register = 0x20;
    }

    return mem_register;
}

int gpio::setInput(int pin)
{
    unsigned int addr;
    unsigned int nmem;
    int gpin;

    //find the right register
    if (physicalNumbering)
    {
        if (pin < 1 && pin > 40) { return 1; } //check if pin exist on the RPI 2

        //convert from physical to gpio numbering
        gpin = physicalToGPIO[pin - 1];

        if (gpin == -1) { return 1; } //nothing to do with this pin (power, ground or must not use)

        addr = findAddress(gpin);
    } else
    {
        if (pin < 2 && pin > 27) { return 1; } //check if pin exist on the RPI 2

        gpin = pin;
        addr = findAddress(pin);
    }

    //create the right 32 bits sequence
    //000 for input 001 for output

    //set the right place to 000
    /*
     * By default all the registers are inputs
     */
    unsigned int start = 7; //start with 0000000...111 32 bits int
    start = start<<((gpin % 10) * 3); //move the 111 to the right position
    start = ~start; //flip all the bits
    nmem = *(mem_ptr + addr) & start; //do a bitwise AND to remove all 1s in the 000 positions
    *(mem_ptr + addr) = nmem; //set the new 32bits in the register space

    return 0;
}

int gpio::setOutput(int pin)
{
    unsigned int addr;
    unsigned int nmem;
    int gpin;

    //find the right register
    if (physicalNumbering)
    {
        if (pin < 1 && pin > 40) { return 1; } //check if pin exist on the RPI 2

        //convert from physical to gpio numbering
        gpin = physicalToGPIO[pin - 1];

        if (gpin == -1) { return 1; } //nothing to do with this pin (power, ground or must not use)

        addr = findAddress(gpin);
    } else
    {
        if (pin < 2 && pin > 27) { return 1; } //check if pin exist on the RPI 2

        gpin = pin;
        addr = findAddress(pin);
    }

    //create the right 32 bits sequence
    //000 for input 001 for output
    //set the right place to 001
    unsigned int value = 1; //create a starting 32 bits of 000...001
    value = value<<((gpin % 10) * 3); //move the 1 to the right position
    nmem = *(mem_ptr + addr) | value; //do a bitwise OR on the value in memory and the added value
    *(mem_ptr + addr) = nmem; //set the new 32bits in the register space

    return 0;
}

int gpio::readPin(int pin)
{
    /*
    *   Do not forget if the returned int is 0 it means the circuit is closed (there is an input)
    *   if the int is > 0 the circuit is open
    */

    unsigned int addr;
    unsigned int nmem;
    int gpin;

    //set the gpio pin
    if (physicalNumbering)
    {
        if (pin < 1 && pin > 40) { return 1; } //check if pin exist on the RPI 2

        //convert from physical to gpio numbering
        gpin = physicalToGPIO[pin - 1];

        if (gpin == -1) { return 1; } //nothing to do with this pin (power, ground or must not use)

    } else
    {
        if (pin < 2 && pin > 27) { return 1; } //check if pin exist on the RPI 2
        gpin = pin;
    }

    //find the read register
    if (gpin <= 31)
    {
        addr = gplev0;
    }
    else
    {
        addr = gplev1;
    }

    //create the right 32 bits sequence
    //1 to turn on the pin register
    unsigned int value = 1; //create a starting 32 bits of 000...001
    value = value<<(gpin % 32); //move the 1 to the right position
    nmem = *(mem_ptr + addr) & value; //do a bitwise AND on the value in memory and the added value

    return nmem;
}

int gpio::writePin(int pin)
{
    unsigned int addr;
    int gpin;

    //set the gpio pin
    if (physicalNumbering)
    {
        if (pin < 1 && pin > 40) { return 1; } //check if pin exist on the RPI 2

        //convert from physical to gpio numbering
        gpin = physicalToGPIO[pin - 1];

        if (gpin == -1) { return 1; } //nothing to do with this pin (power, ground or must not use)

    } else
    {
        if (pin < 2 && pin > 27) { return 1; } //check if pin exist on the RPI 2
        gpin = pin;
    }

    //find the write register
    if (gpin <= 31)
    {
        addr = gpset0;
    }
    else
    {
        addr = gpset1;
    }

    //create the right 32 bits sequence
    //1 to turn on the pin register
    unsigned int value = 1; //create a starting 32 bits of 000...001
    value = value<<(gpin % 32); //move the 1 to the right position
    *(mem_ptr + addr) = value; //set the new 32bits in the register space

    return 0;
}

int gpio::clearPin(int pin)
{
    unsigned int addr;
    int gpin;

    //set the gpio pin
    if (physicalNumbering)
    {
        if (pin < 1 && pin > 40) { return 1; } //check if pin exist on the RPI 2

        //convert from physical to gpio numbering
        gpin = physicalToGPIO[pin - 1];

        if (gpin == -1) { return 1; } //nothing to do with this pin (power, ground or must not use)

    } else
    {
        if (pin < 2 && pin > 27) { return 1; } //check if pin exist on the RPI 2
        gpin = pin;
    }

    //find the clear register
    if (gpin <= 31)
    {
        addr = gpclr0;
    }
    else
    {
        addr = gpclr1;
    }

    //create the right 32 bits sequence
    //1 to clear the pin register
    unsigned int value = 1; //create a starting 32 bits of 000...001
    value = value<<(gpin % 32); //move the 1 to the right position
    *(mem_ptr + addr) = value; //set the new 32bits in the register space

    return 0;
}
