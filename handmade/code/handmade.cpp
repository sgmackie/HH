#include "handmade.h"

internal void render_Gradient(HANDMADE_OFFSCREEN_BUFFER *Buffer, int XOffset, int YOffset)
{    
    uint8 *Row = (uint8 *) Buffer->BitmapMemory; 

    //Animation loops
    for(int Y = 0; Y < Buffer->BitmapHeight; ++Y)
    {
        uint32 *Pixel = (uint32 *) Row;

        for(int X = 0; X < Buffer->BitmapWidth; ++X)
        {
            //Pixels in memory = 0x xxBBGGRR (Little Endian)
            uint8 Blue = (X + XOffset);
            uint8 Green = (Y + YOffset);

            //Pixel increments by 1 * sizeof(uint32) = 4 bytes
            //xx BB GG RR -> xx RR GG BB from Little Endian
            *Pixel++ = ((Green << 8) | Blue); //Shift and Or RGB values together
        }

        //Pitch moves from one row to the next (in bytes)
        Row += Buffer->Pitch;
    }
}

internal void handmade_GameUpdate_Render(HANDMADE_OFFSCREEN_BUFFER *Buffer, int XOffset, int YOffset)
{
    render_Gradient(Buffer, XOffset, YOffset);
}