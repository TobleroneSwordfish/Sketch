#include "display.h"

int main(int n, char* args[n])
{
    display *test = newDisplay("Test", 640, 480);
    line(test, 0,0, 100, 100);
    end(test);
}
