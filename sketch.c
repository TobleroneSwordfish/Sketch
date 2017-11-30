#include "display.h"
#include <stdio.h>
#include <stdbool.h>
#define width 640
#define height 480

typedef enum {DX, DY, DT, PEN} opcode;

struct Vector2
{
    int x, y;
};
typedef struct Vector2 Vector2;

opcode GetOpcode(unsigned char byte)
{
    //mask the first two bits
    byte = byte & 0xC0;
    return byte;
}

int main (int n, char *args[n])
{
    //last position moved to
    Vector2 last = {0, 0};
    //is the pen writing
    bool write = false;
    //initialize display
    display *disp = newDisplay("Sketch", width, height);
    //start the read
    FILE *in = fopen(args[1], "rb");
    unsigned char byte = fgetch(in);
    while (! feof(in))
    {
        switch (GetOpcode(byte))
        {
            case DX:
            {
                //mask the last 6 bits, and convert to signed
                signed char x = byte & 0x3F;
                if (write)
                {
                    line(disp, last.x, last.y, last.x + x, last.y);
                }
                last.x = x;
            }
            case DY:
            {
                //mask the last 6 bits, and convert to signed
                signed char y = byte & 0x3F;
                if (write)
                {
                    line(disp ,last.x, last.y, last.x, last.y + y);
                }
                last.y = y;
            }
            case DT:
            {
                unsigned char t = byte & 0x3F;
                pause(disp, t);
            }
            case PEN:
            {
                write = !write;
            }
        }
        byte = fgetch(in);
    }
}