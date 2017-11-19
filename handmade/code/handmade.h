#if !defined(HANDMADE_H)

//Create struct instead of global variables, means multiple buffers can be made
struct HANDMADE_OFFSCREEN_BUFFER
{
    void *BitmapMemory; //Void * means the format of the pointer can be any type, but will need to be cast later
    int BitmapWidth;
    int BitmapHeight;
    int Pitch;
};

//4 inputs: timing / keyboard input / bitmap buffer / sound buffer 
internal void handmade_GameUpdate_Render(HANDMADE_OFFSCREEN_BUFFER *Buffer, int XOffset, int YOffset);

#define HANDMADE_H
#endif