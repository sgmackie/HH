#include "handmade.h"

internal void sound_OutputSound(HANDMADE_SOUND_BUFFER *SoundBuffer, int ToneHz)
{
    local float32 tSine;
    int16 Amplitude = 3000;
    int WavePeriod = SoundBuffer->SampleRate / ToneHz;

    int16 *SampleOut = SoundBuffer->Samples;

    for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
        float32 SineValue = sinf(tSine);
        int16 SampleValue = (int16) (SineValue * Amplitude);
        
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f * Pi32 * 1.0 / (float32) WavePeriod;
    }
}

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

internal void handmade_GameUpdate_Render(HANDMADE_INPUT_USER *Input, HANDMADE_OFFSCREEN_BUFFER *Buffer, HANDMADE_SOUND_BUFFER *SoundBuffer)
{
    local int XOffset = 0;
    local int YOffset = 0;
    local int ToneHz = 256;

    HANDMADE_INPUT_CONTROLLER *Input0 = &Input->Controllers[0];

    if(Input0->IsAnalog)
    {

    }
    else
    {

    }

    if(Input0->Down.EndedDown)
    {
        XOffset += 1;
    }

    sound_OutputSound(SoundBuffer, ToneHz);
    render_Gradient(Buffer, XOffset, YOffset);
}