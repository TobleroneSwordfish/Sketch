#include "display.h"
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#define width 640
#define height 480

typedef enum {DX, DY, DT, EXT} opcode;
typedef enum {PEN = 3, CLEAR, KEY, COL} extOpcode;

struct Vector2
{
    int x, y;
};
typedef struct Vector2 Vector2;

struct State
{
    Vector2 last;
    Vector2 current;
    bool write;
    display *disp;
};
typedef struct State State;

void Initialize(State *state, char* windowName)
{
    state->last.x = 0;
    state->last.y = 0;
    state->current = state->last;
    state->write = false;
    state->disp = newDisplay(windowName, width, height);
}

//returns the 2 bit opcode from the start of a byte as an enum
opcode GetOpcode(unsigned char byte)
{
    return byte >> 6;
}

//returns the nth (zero indexed from lsb) bit from a byte 
int GetNthBit(unsigned int byte, int n)
{
    return (byte & ( 1 << n )) >> n;
}

//returns the next power of 2 from a number n, so 1 would return 1, 5 would return 8, 30 would return 32
int GetNextPwr2(int n)
{
    if (n == 0)
    {
        return 0;
    }
    int pwr = 1;
    while(pwr < n)
    {
        pwr *= 2;
    }
    return pwr;
}

//returns the last 6 bits of a byte as a signed integer (well, char but you get the point)
signed char GetSigned6Bit(unsigned char byte)
{
    byte = byte & 0x3F;
    int sign = GetNthBit(byte, 5);
    return (byte & 0x1F) + (sign * -32);
}

//converts the positive representation of a given length (in bits) integer into two's complement binary
//if the number would be positive anyway, then it is simply returned
int ToTwosComplement(int positive, int length)
{
    //check if the number is actually negative
    if (GetNthBit(positive, length - 1) == 1)
    {
        //build a mask of all bits up to the MSB (negative bit)
        int mask = 0;
        for (int i = 0; i < length - 1; i++)
        {
            mask <<= 1;
            mask |= 0x1;
        }
        //printf("Mask: %x\n", mask);
        //apply the mask to get the positive part
        int plus = positive & mask;
        //printf("Positive part: %i\n", plus);
        //weight the MSB correctly and negate it
        int minus = -(pow(2, length - 1));
        //printf("Negative part: %i\n", minus);
        return (minus + plus);
    }
    //default to just returning the number
    return positive;
}

//reads n bytes from a file and treats them as an integer
int ReadNBytes(FILE *in, int n)
{
    int output = 0;
    for (int i = 0; i < n; i++)
    {
        //shift the current number along 8 bits
        output <<= 8;
        //insert the next byte
        unsigned char nextByte = fgetc(in);
        //printf("EXT byte read in: %x\n", nextByte);
        output |= nextByte;
    }
    return output;
}

void MoveX(State *state, int x)
{
    //printf("x current changed from %i to %i\n", state->current.x, state->current.x + x);
    state->current.x += x;
}

void MoveY(State *state, int y)
{
    //printf("y current changed from %i to %i\n", state->current.y, state->current.y + y);
    state->current.y += y;
    if (state->write)
    {
        //printf("Line drawn from %i,%i to %i,%i\n", state->last.x, state->last.y, state->current.x, state->current.y);
        line(state->disp, state->last.x, state->last.y, state->current.x, state->current.y);
    }
    //printf("Last position changed from %i,%i to %i,%i\n", state->last.x, state->last.y, state->current.x, state->current.y);
    state->last.x = state->current.x;
    state->last.y = state->current.y;
}

//this ought to take a time in ms, but seems to instead take a time in cs?
//anyways, this only passes the tests if it's 10 times longer than the value in the file
//don't ask me why, blame Ian
void Wait(State *state, int time)
{
    time *= 10;
    printf("waiting for %ims\n", time);
    pause(state->disp, time);
}

//think about what the word toggle means...
void ToggleWrite(State *state)
{
    state->write = !state->write;
    //printf(state->write ? "Pen down\n" : "Pen up\n");
}

//runs an operation from extended format, or non-extended where EXT is interpreted as PEN
void RunOperation(State *state, extOpcode code, int operand)
{
    switch(code)
    {
        case DX:
            MoveX(state, operand);
            break;
        case DY:
            MoveY(state, operand);
            break;
        case DT:
            Wait(state, operand);
            break;
        case PEN:
            ToggleWrite(state);
            break;
        case CLEAR:
            clear(state->disp);
            break;
        case KEY:
            key(state->disp);
            break;
        case COL:
            //printf("Setting colour to %x\n", operand);
            colour(state->disp, operand);
            break;
    }
}

//handle the EXT opcode
void ReadExtendedOperation(State *state, unsigned char currentByte, FILE *in)
{
    //next two bits are the extra number of bytes the extension takes up
    //we mask out the EXT opcode, then shift right by 4 to nuke the next extended opcode
    unsigned char length = GetNextPwr2((currentByte & 0x30) >> 4);
    printf("Length of operand: %i\n", length);
    //last four bits are the extended opcode
    opcode nextOp = currentByte & 0x0F;
    //the multi-byte operand
    int operand = ReadNBytes(in, length);
    printf("Initial EXT operand: %i\n", operand);
    //ensure that the number is treated as two's complement
    operand = ToTwosComplement(operand, length * 8);
    printf("Fixed EXT operand: %i\n", operand);
    RunOperation(state, nextOp, operand);
}

//testing

void TestGetOpcode()
{
    assert(GetOpcode(0x80) == 2);
    assert(GetOpcode(0x40) == 1);
}

void TestNthBit()
{
    assert(GetNthBit(0x40, 6) == 1);
    assert(GetNthBit(0xDD, 1) == 0);
    assert(GetNthBit(0xDD, 7) == 1);
}

void TestNextPwr2()
{
    assert(GetNextPwr2(1) == 1);
    assert(GetNextPwr2(3) == 4);
    assert(GetNextPwr2(50) == 64);
}

void TestTwosComplement()
{
    //101
    //printf("Two's complement version of 101: %i\n", ToTwosComplement(0x5, 3));
    assert(ToTwosComplement(0x5, 3) == -3);
    //1011
    assert(ToTwosComplement(0xB, 4) == -5);
    //11001110
    assert(ToTwosComplement(0xCE, 8) == -50);
    //0110
    assert(ToTwosComplement(0x6, 4) == 6);
    //FF74
    printf("Two's complement version of FF74: %i\n", ToTwosComplement(0xFF74, 16));
    assert(ToTwosComplement(0xFF74, 16) == -140);
}

//some basic testing
void RunTests()
{
    TestGetOpcode();
    TestNthBit();
    TestNextPwr2();
    TestTwosComplement();
    printf("Tests complete\n");
}

void DisplayFile(char *filename)
{
    //initialize display
    State state;
    Initialize(&state, filename);
    //start the read
    FILE *in = fopen(filename, "rb");
    unsigned char byte = fgetc(in);
    while (! feof(in))
    {
        //printf("Byte read in: %i\n", byte);
        //printf("Opcode: %i\n", GetOpcode(byte));
        switch (GetOpcode(byte))
        {
            case DX:
            {
                MoveX(&state, GetSigned6Bit(byte));
                break;
            }
            case DY:
            {
                MoveY(&state, GetSigned6Bit(byte));
                break;
            }
            case DT:
            {
                Wait(&state, byte & 0x3F);
                break;
            }
            case EXT:
            {
                ReadExtendedOperation(&state, byte, in);
                break;
            }
        }
        byte = fgetc(in);
    }
    //key(state.disp);
    end(state.disp);
}

int main (int n, char *args[n])
{
    if (n == 2)
    {
        DisplayFile(args[1]);
    }
    else
    {
        RunTests();
    }
}
