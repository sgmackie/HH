struct WIN32_OFFSCREEN_BUFFER
{
    BITMAPINFO BitmapInfo;
    void *BitmapMemory; //Void * means the format of the pointer can be any type, but will need to be cast later
    int BitmapWidth;
    int BitmapHeight;
    int Pitch;
};

//Instead of calling GetClientRect all the time, create struct for window dimensions and store them
struct WIN32_WINDOW_DIMENSIONS
{
    int Width;
    int Height;
};

//Sound test buffer values
struct WIN32_SOUND_OUTPUT
{
    int SampleRate;
    uint32 RunningSampleIndex;
    int BytesPerSample;
    int SecondaryBufferSize;
    float32 tSine;
    int LatencySampleCount;
};