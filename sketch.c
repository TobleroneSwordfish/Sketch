#include "display.h"
#include <stdio.h>
#include <stdbool.h>
#define width 640
#define height 480

typedef enum {DX, DY, DT, PEN} opcode;

struct State
{
    int x;
    int y;
    bool write;
    display *disp;
};
typedef struct State State;

opcode GetOpcode(unsigned char byte)
{
    return byte >> 6;
}

int GetNthBit(unsigned char byte, int n)
{
    return (byte & ( 1 << n )) >> n;
}

signed char GetSigned6Bit(unsigned char byte)
{
    byte = byte & 0x3F;
    int sign = GetNthBit(byte, 5);
    return (byte & 0x1F) - - (sign * -32);
}

void MoveX(State *state, unsigned char byte)
{
    signed char x = GetSigned6Bit(byte);
    printf("x: %i\n", x);
    if (state->write)
    {
        line(state->disp, state->x, state->y, state->x + x, state->y);
    }
    state->x += x;
}

void MoveY(State *state, unsigned char byte)
{
    signed char y = GetSigned6Bit(byte);
    printf("y: %i\n", y);
    if (state->write)
    {
        line(state->disp, state->x, state->y, state->x, state->y + y);
    }
    state->y += y;
}

void Wait(State *state, unsigned char byte)
{
    unsigned char t = byte & 0x3F;
    printf("t: %i\n", t);
    pause(state->disp, t);
}

void ToggleWrite(State *state)
{
    state->write = !state->write;
}

int main (int n, char *args[n])
{
    //initialize display
    display *disp = newDisplay("Sketch", width, height);
    State state = {0,0, false, disp};
    //start the read
    FILE *in = fopen(args[1], "rb");
    unsigned char byte = fgetc(in);
    //printf("DX: %i, DY: %i, DT: %i, PEN: %i\n", DX, DY, DT, PEN);
    while (! feof(in))
    {
        printf("Byte read in: %i\n", byte);
        printf("Opcode: %i\n", GetOpcode(byte));
        switch (GetOpcode(byte))
        {
            case DX:
            {
                MoveX(&state, byte);
                break;
            }
            case DY:
            {
                MoveY(&state, byte);
                break;
            }
            case DT:
            {
                Wait(&state, byte);
                break;
            }
            case PEN:
            {
                ToggleWrite(&state);
                break;
            }
        }
        byte = fgetc(in);
    }
    key(disp);
    //end(disp);
}