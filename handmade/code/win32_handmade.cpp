#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>
#include <stdio.h>

//Static can have three different meanings:
#define global static //Global access to variable
#define internal static //Variable local to source file only
#define local static //Variable persists after stepping out of scope (this should be avoided when possible)

#define Pi32 3.14159265359f

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;
typedef float float32;
typedef double float64;

//Create struct instead of global variables, means multiple buffers can be made
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

//Create own version of XInput/DirectSound functions to avoid calling libraries
//Create macros of function prototypes
#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
#define DIRECTSOUND_CREATE(name) DWORD WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)

//Define type of functions so they can be used as pointers
typedef XINPUT_GET_STATE(win32_xinput_GetState);
typedef XINPUT_SET_STATE(win32_xinput_SetState);
typedef DIRECTSOUND_CREATE(directsound_create);

//When .dlls aren't found on machine, call fake functions so game doesn't crash
XINPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

XINPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

//Global variables, static values initialise to 0 (or false) by default
global win32_xinput_GetState *XInputGetState_ = XInputGetStateStub;
global win32_xinput_SetState *XInputSetState_ = XInputSetStateStub;


global LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global bool32 GlobalRunning; 
global WIN32_OFFSCREEN_BUFFER GlobalBackBuffer;

//Rename to prevent conflicts with headers
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

//Load XInput in the same fashion Windows would
internal void win32_LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");

    if(!XInputLibrary)
    {
        HMODULE XInputLibrary = LoadLibrary("xinput1_3.dll");
    }

    if(!XInputLibrary)
    {
        HMODULE XInputLibrary = LoadLibrary("xinput9_1_0.dll");
    }

    if(XInputLibrary)
    {
        XInputGetState = (win32_xinput_GetState *) GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (win32_xinput_SetState *) GetProcAddress(XInputLibrary, "XInputSetState");
    }
}

//Load and initialise DirectSound (OOP API)
internal void Win32InitDSound(HWND Window, int32 SampleRate, int32 BufferSize)
{
    //Load library
    HMODULE DSoundLibrary = LoadLibrary("dsound.dll");

    if(DSoundLibrary)
    {
        //Get DirectSound Object
        directsound_create *DirectSoundCreate = (directsound_create *) GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND DirectSound;
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SampleRate;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            //Loading .dll add vtable to memory, which has pointers to functions (hence ->)
            if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                //Primary buffer is only to specify sound file fomat for the soundcard
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                
                if(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0))
                {
                    HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);

                    if(SUCCEEDED(Error))
                    {
                        //Primary buffer format set
                        OutputDebugString("Primary Buffer format set\n");
                    }
                }
            }

            //Secondary buffer is for actual playback
            DSBUFFERDESC BufferDescription ={};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
            
            if(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0))
            {
                OutputDebugString("Secondary Buffer created\n");
            }                                                 
        }
    }
}

//Sound test buffer values
struct WIN32_SOUND_OUTPUT
{
    int SampleRate;
    int ToneHz;
    int16 Amplitude;
    uint32 RunningSampleIndex;
    int SquareWave_Period;
    int BytesPerSample;
    int SecondaryBufferSize;
    float32 tSine;
};

internal void win32_FillSoundBuffer(WIN32_SOUND_OUTPUT *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;                  
    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
    {
        DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
        int16 *SampleOut = (int16 *) Region1;

        for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {
            float32 SineValue = sinf(SoundOutput->tSine);
            int16 SampleValue = (int16) (SineValue * SoundOutput->Amplitude);
            
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;

            SoundOutput->tSine += 2.0f * Pi32 * 1.0 / (float32) SoundOutput->SquareWave_Period;
            ++SoundOutput->RunningSampleIndex;
        }
    
        DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
        SampleOut = (int16 *) Region2;
       
        for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {
            float32 SineValue = sinf(SoundOutput->tSine);
            int16 SampleValue = (int16) (SineValue * SoundOutput->Amplitude);
            
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;

            SoundOutput->tSine += 2.0f * Pi32 * 1.0 / (float32) SoundOutput->SquareWave_Period;
            ++SoundOutput->RunningSampleIndex;
        }
    
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

//Replaces GetClientRect calls
internal WIN32_WINDOW_DIMENSIONS win32_GetWindowDimensions(HWND Window)
{
    WIN32_WINDOW_DIMENSIONS WindowDimensions;

    RECT ClientRect; //Rect structure for window dimensions
    GetClientRect(Window, &ClientRect); //Function to get current window dimensions in a RECT format
    
    WindowDimensions.Width = ClientRect.right - ClientRect.left;
    WindowDimensions.Height = ClientRect.bottom - ClientRect.top;

    return WindowDimensions;
}


internal void render_Gradient(WIN32_OFFSCREEN_BUFFER *Buffer, int XOffset, int YOffset)
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

//Device Independant Bitmap, function for writing into bitmaps for Windows to display through it's graphic library GDI
internal void win32_ResizeDIBSection(WIN32_OFFSCREEN_BUFFER *Buffer, int Width, int Height)
{
    //If there's anything in the bitmap memory, then free first so it can be written again
    if(Buffer->BitmapMemory)
    {
        VirtualFree(Buffer->BitmapMemory, 0, MEM_RELEASE);
    }

    //Pass function inputs to buffer dimensions
    Buffer->BitmapWidth = Width;
    Buffer->BitmapHeight = Height;
    int BytesPerPixel = 4;

    Buffer->BitmapInfo.bmiHeader.biSize = sizeof(Buffer->BitmapInfo.bmiHeader);
    Buffer->BitmapInfo.bmiHeader.biWidth = Buffer->BitmapWidth;
    Buffer->BitmapInfo.bmiHeader.biHeight = -Buffer->BitmapHeight; //BiHeight being negative is clue to Windows to treat bitmap as top-down, not bottom-up. First 3 bytes of the image is are the RGB values for the top left pixel, not bottom left
    Buffer->BitmapInfo.bmiHeader.biPlanes = 1;
    Buffer->BitmapInfo.bmiHeader.biBitCount = 32; //8 bits per colour channel, 8 bits of pad = 32 bit to align with DWORD (4 byte boundary)
    Buffer->BitmapInfo.bmiHeader.biCompression = BI_RGB; //No compresion (JPG, PNG..) for faster blt to the screen
    
    int BitmapMemory_Size = (Buffer->BitmapWidth * Buffer->BitmapHeight) * BytesPerPixel;
    
    //Virutal alloc uses whole memory pages, returns void *
    Buffer->BitmapMemory = VirtualAlloc(0, BitmapMemory_Size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);  

    Buffer->Pitch = Buffer->BitmapWidth * BytesPerPixel;
}

//Primary function for passing infomation to the game's window
internal void win32_DisplayBuffer_Window(WIN32_OFFSCREEN_BUFFER *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight)
{
    //Copy rectangle from one buffer to another
    StretchDIBits(DeviceContext, 0, 0, WindowWidth, WindowHeight, 0, 0, Buffer->BitmapWidth, Buffer->BitmapHeight, Buffer->BitmapMemory, &Buffer->BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

//Callback function as Windows is free to pass this function when it pleases                                      
LRESULT CALLBACK win32_MainWindow_Callback(HWND Window, UINT UserMessage, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch (UserMessage)
    {
        case WM_SIZE:
        {   
            OutputDebugStringA("WM_SIZE\n");
            break;
        }

        case WM_DESTROY:
        {
            GlobalRunning = false;
            OutputDebugStringA("WM_DESTROY\n");
            break;
        }

        case WM_CLOSE:
        {
            GlobalRunning = false;
            OutputDebugStringA("WM_CLOSE\n");
            break;
        }

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
            break;
        }

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            //Virtual Keycode, keys without direct ANSI mappings
            uint32 VKCode = WParam;

            //Check if key was down, if bit == 30 then it's true, otherwise false
            bool32 WasDown = ((LParam & (1 << 30)) != 0);
            //Check if being held down
            bool32 IsDown = ((LParam & (1 << 31)) == 0);

            if(WasDown != IsDown)
            {
                if(VKCode == 'W')
                {
                    OutputDebugStringA("W\n");
                }
                else if(VKCode == 'A')
                {
                    OutputDebugStringA("A\n");
                }
                else if(VKCode == 'S')
                {
                    OutputDebugStringA("S\n");
                }
                else if(VKCode == 'D')
                {
                    OutputDebugStringA("D\n");
                }
                else if(VKCode == 'Q')
                {
                    OutputDebugStringA("Q\n");
                }
                else if(VKCode == 'E')
                {
                    OutputDebugStringA("E\n");
                }
                else if(VKCode == VK_UP)
                {
                    OutputDebugStringA("Up\n");
                }
                else if(VKCode == VK_DOWN)
                {
                    OutputDebugStringA("Down\n");
                }
                else if(VKCode == VK_LEFT)
                {
                    OutputDebugStringA("Left\n");
                }
                else if(VKCode == VK_RIGHT)
                {
                    OutputDebugStringA("Right\n");
                }
                else if(VKCode == VK_SPACE)
                {
                    OutputDebugStringA("Space\n");
                }
                else if(VKCode == VK_ESCAPE)
                {
                    OutputDebugStringA("Escape\n");
                }
            }

            bool32 AltKeyWasDown = ((LParam & (1 << 29)) != 0);

            if((VKCode == VK_F4) && AltKeyWasDown)
            {
                GlobalRunning = false;
            }
            
            break;
        }

        case WM_PAINT: //Call paint functions to draw to the window
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);

            //Subtract top from bottom and right from left for height and width values of RECT(angle) structure
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;

            WIN32_WINDOW_DIMENSIONS WindowDimensions = win32_GetWindowDimensions(Window);

            //Call function to update window
            win32_DisplayBuffer_Window(&GlobalBackBuffer, DeviceContext, WindowDimensions.Width, WindowDimensions.Height);

            EndPaint(Window, &Paint);
        }

        default:
        {
            //OutputDebugStringA("default\n");
            Result = DefWindowProc(Window, UserMessage, WParam, LParam); //Default window procedure
            break;
        }
    }

    return Result;
}  

//Starting point
//Entry point for any Windows application
int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int CommandShow)
{
    LARGE_INTEGER PerfomanceCounterFrequency_Result;
    QueryPerformanceFrequency(&PerfomanceCounterFrequency_Result);
    int64 PerfomanceCounterFrequency = PerfomanceCounterFrequency_Result.QuadPart;

    win32_LoadXInput(); //Load XInput dll
    WNDCLASS WindowClass = {}; //Initialise everything in struct to 0

    win32_ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; //Create unique device context for this window
    WindowClass.lpfnWndProc = win32_MainWindow_Callback; //Call the window process
    WindowClass.hInstance = Instance; //Handle instance passed from WinMain
    WindowClass.lpszClassName = "HandmadeHeroWindowClass"; //Name of Window class
    //WindowClass.hIcon;

    if(RegisterClass(&WindowClass))
    {
        HWND Window = CreateWindowEx(0, WindowClass.lpszClassName, "Handmade Hero", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0); 

        if(Window) //Process message queue
        {
            //Specified CS_OWNDC so get one device context and use it forever
            HDC DeviceContext = GetDC(Window);

            int XOffset = 0;
            int YOffset = 0;
            
            WIN32_SOUND_OUTPUT SoundOutput = {};

            SoundOutput.SampleRate = 48000;
            SoundOutput.ToneHz = 256;
            SoundOutput.Amplitude = 2000;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.SquareWave_Period = SoundOutput.SampleRate / SoundOutput.ToneHz;
            SoundOutput.BytesPerSample = sizeof(int16) * 2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SampleRate * SoundOutput.BytesPerSample;

            Win32InitDSound(Window, SoundOutput.SampleRate, SoundOutput.SecondaryBufferSize);
            win32_FillSoundBuffer(&SoundOutput, 0, SoundOutput.SecondaryBufferSize);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            
            bool32 SoundIsPlaying = false;
            GlobalRunning = true;

            LARGE_INTEGER LastCounter;
            QueryPerformanceCounter(&LastCounter);

            int64 LastCycleCount = __rdtsc();

            //Loop while program is running / until negative result from GetMessage
            while(GlobalRunning)
            {
                MSG Messages;

                //PeekMessage keeps processing message queue without blocking when there are no messages available
                while(PeekMessage(&Messages, 0, 0, 0, PM_REMOVE))
                {
                    if(Messages.message == WM_QUIT)
                    {
                        GlobalRunning = false;
                    }

                    TranslateMessage(&Messages);
                    DispatchMessage(&Messages);
                }

                //Loop through all connected controllers
                for(DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
                {
                    XINPUT_STATE ControllerState;

                    if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                    {
                        //Controller is plugged in

                        //Button layouts
                        XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                        bool32 PadUp = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool32 PadDown = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool32 PadLeft = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool32 PadRight = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool32 PadStart = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool32 PadBack = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool32 PadLeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool32 PadRightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool32 PadButtonA = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        bool32 PadButtonB = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        bool32 PadButtonX = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        bool32 PadButtonY = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                        int16 PadStickX = Pad->sThumbLX;
                        int16 PadStickY = Pad->sThumbLY;
                    }

                    else
                    {
                        //Controller unavailable
                    }
                }

                //Rendering
                render_Gradient(&GlobalBackBuffer, XOffset, YOffset);

                //DirectSound square wave test tone

                DWORD PlayCursor;
                DWORD WriteCursor;
                if(!SoundIsPlaying && SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                {
                    DWORD ByteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % (SoundOutput.SecondaryBufferSize));
                    DWORD BytesToWrite;

                    if(ByteToLock > PlayCursor)
                    {
                        BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
                        BytesToWrite += PlayCursor;
                    }

                    else
                    {
                        BytesToWrite = PlayCursor - ByteToLock;
                    }
                    
                    win32_FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite);
                }

                ++XOffset;
                YOffset += 2;


                WIN32_WINDOW_DIMENSIONS WindowDimensions = win32_GetWindowDimensions(Window);
                win32_DisplayBuffer_Window(&GlobalBackBuffer, DeviceContext, WindowDimensions.Width, WindowDimensions.Height);
                
                ReleaseDC(Window, DeviceContext);

                LARGE_INTEGER EndCounter;
                QueryPerformanceCounter(&EndCounter);

                int64 EndCycleCount = __rdtsc();

                int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
                int64 CyclesElapsed = EndCycleCount - LastCycleCount;
                float32 MSPerFrame = (float32) (((1000.0f * (float32) CounterElapsed) / (float32) PerfomanceCounterFrequency));
                float32 FramesPerSecond = (float32) PerfomanceCounterFrequency / (float32) CounterElapsed;
                float32 MegaHzCyclesPerFrame = (float32) (CyclesElapsed / (1000.0f * 1000.0f));

                char MSPerFrame_Buffer[256];
                sprintf(MSPerFrame_Buffer, "%0.2f ms/frame\t %0.2f FPS\t %0.2f cycles(MHz)/frame\n", MSPerFrame, FramesPerSecond, MegaHzCyclesPerFrame);
                OutputDebugString(MSPerFrame_Buffer);

                LastCounter = EndCounter;
                LastCycleCount = EndCycleCount;
            }
        }

        else
        {
            //Add logging
        } 
    }

    else
    {
    
    }

    return 0;
}