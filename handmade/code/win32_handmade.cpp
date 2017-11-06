#include <windows.h>
#include <stdint.h>
#include <xinput.h>

//Static can have three different meanings:
#define global static //Global access to variable
#define internal static //Variable local to source file only
#define local static //Variable persists after stepping out of scope (this should be avoided when possible)

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

//Create struct instead of global varibles, means multiple buffers can be made
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

//Create own version of XInput functions so don't have to call xinput.lib
//Create macros of function prototypes
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)

//Define type of functions so they can be used as pointers
typedef X_INPUT_GET_STATE(win32_xinput_GetState);
typedef X_INPUT_SET_STATE(win32_xinput_SetState);

X_INPUT_GET_STATE(XInputGetStateStub)
{
    return 0;
}

X_INPUT_SET_STATE(XInputSetStateStub)
{
    return 0;
}

global win32_xinput_GetState *XInputGetState_ = XInputGetStateStub;
global win32_xinput_SetState *XInputSetState_ = XInputSetStateStub;

//Rename to prevent conflicts with XInput header
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

//Load XInput in the same fashion Windows would
internal void win32_LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibrary("xinput1_3.dll");

    if(XInputLibrary)
    {
        XInputGetState = (win32_xinput_GetState *) GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (win32_xinput_SetState *) GetProcAddress(XInputLibrary, "XInputSetState");
    }
}

//Static values initialise to 0 (or false) by default
global bool GlobalRunning; 
global WIN32_OFFSCREEN_BUFFER GlobalBackBuffer;

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
    Buffer->BitmapMemory = VirtualAlloc(0, BitmapMemory_Size, MEM_COMMIT, PAGE_READWRITE);  

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
            bool WasDown = ((LParam & (1 << 30)) != 0);
            //Check if being held down
            bool IsDown = ((LParam & (1 << 31)) == 0);

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
            int XOffset = 0;
            int YOffset = 0;
            
            GlobalRunning = true;

            while(GlobalRunning) //Loop while program is running / until negative result from GetMessage
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

                        bool PadUp = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool PadDown = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool PadLeft = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool PadRight = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool PadStart = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool PadBack = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool PadLeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool PadRightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool PadButtonA = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        bool PadButtonB = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        bool PadButtonX = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        bool PadButtonY = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                        int16 PadStickX = Pad->sThumbLX;
                        int16 PadStickY = Pad->sThumbLY;
                    }

                    else
                    {
                        //Controller unavailable
                    }
                }

                render_Gradient(&GlobalBackBuffer, XOffset, YOffset);

                //CHECK WHERE TO PUT THIS 
                HDC DeviceContext = GetDC(Window);

                WIN32_WINDOW_DIMENSIONS WindowDimensions = win32_GetWindowDimensions(Window);
                win32_DisplayBuffer_Window(&GlobalBackBuffer, DeviceContext, WindowDimensions.Width, WindowDimensions.Height);
                
                ReleaseDC(Window, DeviceContext);

                ++XOffset;
                YOffset += 2;
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