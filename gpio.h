//
// Created by Alexandre St-Laurent on 1/5/2016.
//

#ifndef MATHTEST_GPIO_H
#define MATHTEST_GPIO_H


class gpio {

private:
    static unsigned int findAddress(int gpin);

public:
    static void setPhysicalNumbering(bool value);
    static bool physicalNumberingOn();

    static int init();
    static int closeMem();

    static int setInput(int pin);
    static int setOutput(int pin);

    static int readPin(int pin);
    static int writePin(int pin);

    static int clearPin(int pin);
};


#endif //MATHTEST_GPIO_H
