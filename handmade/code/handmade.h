#if !defined(HANDMADE_H)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array[0])))

//Create struct instead of global variables, means multiple buffers can be made
struct HANDMADE_OFFSCREEN_BUFFER
{
    void *BitmapMemory; //Void * means the format of the pointer can be any type, but will need to be cast later
    int BitmapWidth;
    int BitmapHeight;
    int Pitch;
};

struct HANDMADE_SOUND_BUFFER
{
    int SampleRate;
    int SampleCount;
    int16 *Samples;
};

struct HANDMADE_INPUT_CONTROLLER_BUTTON_STATE
{
    int HalfTransitionCount;
    bool32 EndedDown;
};

struct HANDMADE_INPUT_CONTROLLER
{
    bool32 IsAnalog;

    float32 StartX;
    float32 StartY;

    float32 MinX;
    float32 MinY;

    float32 MaxX;
    float32 MaxY;

    float32 EndX;
    float32 EndY;

    union
    {
        HANDMADE_INPUT_CONTROLLER_BUTTON_STATE Buttons[5];

        struct
        {
            HANDMADE_INPUT_CONTROLLER_BUTTON_STATE Up;
            HANDMADE_INPUT_CONTROLLER_BUTTON_STATE Down;
            HANDMADE_INPUT_CONTROLLER_BUTTON_STATE Left;
            HANDMADE_INPUT_CONTROLLER_BUTTON_STATE Right;
            HANDMADE_INPUT_CONTROLLER_BUTTON_STATE LeftShoulder;
            HANDMADE_INPUT_CONTROLLER_BUTTON_STATE RightShoulder;
        };
    };
};

struct HANDMADE_INPUT_USER
{
    HANDMADE_INPUT_CONTROLLER Controllers[4];
};

//Prototypes

internal void sound_OutputSound(HANDMADE_SOUND_BUFFER *SoundBuffer, int ToneHz);

internal void render_Gradient(HANDMADE_OFFSCREEN_BUFFER *Buffer, int XOffset, int YOffset);

//4 inputs: timing / keyboard input / bitmap buffer / sound buffer 
internal void handmade_GameUpdate_Render(HANDMADE_INPUT_USER *Input, HANDMADE_OFFSCREEN_BUFFER *Buffer, HANDMADE_SOUND_BUFFER *SoundBuffer);

#define HANDMADE_H
#endif