#include <Windows.h>

#include <stdio.h>
#include <stdint.h>

#include "stb_image.h"

#define WINDOW_CLASS_NAME "GameWindowClass"
#define WINDOW_CAPTION "Game Window"

#define SCREEN_W 420
#define SCREEN_H 240

#define FPS 60

struct GameBitmap
{
    int w;
    int h;
    int channels;
    BITMAPINFO info;
    uint32_t * data;
};

struct GameRect
{
    int x;
    int y;
    int w;
    int h;
};


struct
{
    BITMAPINFO info;
    void * data;
} graphicsBuffer;

union Pixel32
{
    struct
    {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t a;
    };

    uint32_t hexValue;
};


struct
{
    int x;
    int y;
    int spd;
} player;


struct
{
    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    float delta;
    float accumulator;
    float updatesPerSec;
} timer;

bool gRunning;

// test bmp
GameBitmap mehworm;
GameBitmap fethyr;

/////////////////////////////////////
// Function declarations

void InitGraphicsBuffer(void);
void ClearScreen(void);
void PlotPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b);
void LoadBitmapFromPNG(GameBitmap * gameBitmap, const char * filepath);
void BlitBitmap(GameBitmap * gameBitmap, int x, int y, GameRect * srcRect);

//
/////////////////////////////////////


LRESULT CALLBACK WindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CLOSE:
            gRunning = false;
            DestroyWindow(window);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcA(window, msg, wParam, lParam);
    }
    return 0;
}


int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmd, int cmdShow)
{
    WNDCLASSEXA windowClass;
    HWND window;
    MSG msg;

    windowClass.cbSize = sizeof(WNDCLASSEXA);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    windowClass.hCursor = LoadCursorA(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = WINDOW_CLASS_NAME;
    windowClass.hIconSm = LoadIconA(NULL, IDI_APPLICATION);

    if (RegisterClassExA(&windowClass) == 0)
    {
        MessageBoxA(NULL, "Window class registration failed", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    window = CreateWindowExA(
        0,
        windowClass.lpszClassName,
        WINDOW_CAPTION,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        SCREEN_W * 2,
        SCREEN_H * 2,
        NULL,
        NULL,
        windowClass.hInstance,
        NULL
    );

    QueryPerformanceFrequency(&timer.freq);
    timer.updatesPerSec = 1.0f / FPS;
    timer.delta = 0.0f;
    timer.accumulator = 0.0f;

    InitGraphicsBuffer();
    player.x = 32;
    player.y = 32;
    player.spd = 4;

    LoadBitmapFromPNG(&mehworm, "mehworm_beta.png");
    LoadBitmapFromPNG(&fethyr, "fethyr.png");

    gRunning = true;

    QueryPerformanceCounter(&timer.end);
    timeBeginPeriod(1);
    while (gRunning)
    {
        QueryPerformanceCounter(&timer.start);
        timer.delta = (float)(timer.start.QuadPart - timer.end.QuadPart) / timer.freq.QuadPart;
        timer.end = timer.start;
        timer.accumulator += timer.delta;

        // Poll messages
        if (PeekMessageA(&msg, window, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        while(timer.accumulator >= timer.updatesPerSec)
        {
            // input
            if (GetAsyncKeyState('A'))
            {
                player.x -= player.spd;
            }
            else if (GetAsyncKeyState('D'))
            {
                player.x += player.spd;
            }

            if (GetAsyncKeyState('W'))
            {
                player.y -= player.spd;
            }
            else if (GetAsyncKeyState('S'))
            {
                player.y += player.spd;
            }

            // step

            timer.accumulator -= timer.updatesPerSec;

            char debugBuffer[256];
            sprintf_s(debugBuffer, "UpdatesPerSec: %.2f Delta: %.2f Accumulator: %.2f\n", timer.updatesPerSec, timer.delta, timer.accumulator);
            OutputDebugStringA(debugBuffer);
        }

        

        

        // draw
        HDC dc; 
        RECT windowRect;
        int windowWidth;
        int windowHeight;

        dc = GetDC(window);
        GetWindowRect(window, &windowRect);
        windowWidth = windowRect.right - windowRect.left;
        windowHeight = windowRect.bottom - windowRect.top;

        ClearScreen();

        // Draw mehworm
        BlitBitmap(&mehworm, player.x, player.y, NULL);

        // Draw Fethyr
        GameRect fethyrRect = { 96, 96 * 4, 96, 96 };
        BlitBitmap(&fethyr, 40, 40, &fethyrRect);
        
        StretchDIBits(dc, 0, 0, windowWidth, windowHeight, 0, 0, SCREEN_W, SCREEN_H, graphicsBuffer.data, &graphicsBuffer.info, DIB_RGB_COLORS, SRCCOPY);
        ReleaseDC(window, dc);

        Sleep(1);
        
    }
    timeEndPeriod(1);
    return 0;
}



void InitGraphicsBuffer(void)
{
    graphicsBuffer.info.bmiHeader.biSize = sizeof(graphicsBuffer.info.bmiHeader);
    graphicsBuffer.info.bmiHeader.biWidth = SCREEN_W;
    graphicsBuffer.info.bmiHeader.biHeight = -SCREEN_H;
    graphicsBuffer.info.bmiHeader.biPlanes = 1;
    graphicsBuffer.info.bmiHeader.biBitCount = 32;
    graphicsBuffer.info.bmiHeader.biCompression = BI_RGB;
    graphicsBuffer.info.bmiHeader.biSizeImage = 0;
    graphicsBuffer.info.bmiHeader.biXPelsPerMeter = 0;
    graphicsBuffer.info.bmiHeader.biYPelsPerMeter = 0;
    graphicsBuffer.info.bmiHeader.biClrUsed = 0;
    graphicsBuffer.info.bmiHeader.biClrImportant = 0;


    graphicsBuffer.data = malloc(sizeof(uint32_t) * SCREEN_W * SCREEN_H);
}


void ClearScreen(void)
{
    //uint32_t clearColor = 0xFFC0C0C0;
    Pixel32 clearColor;
    clearColor.r = 64;
    clearColor.g = 128;
    clearColor.b = 64;
    clearColor.a = 255;


    for (int y = 0; y < SCREEN_H; y++)
    {
        for (int x = 0; x < SCREEN_W; x++)
        {
            ((uint32_t*)graphicsBuffer.data)[y * SCREEN_W + x] = clearColor.hexValue;
        }
    }
}


void PlotPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b)
{
    Pixel32 p;
    
    if (x < 0 || x > SCREEN_W - 1 || y < 0 || y > SCREEN_H - 1)
        return;

    p.r = r;
    p.g = g;
    p.b = b;
    p.a = 255;

    ((uint32_t*)graphicsBuffer.data)[y * SCREEN_W + x] = p.hexValue;
}


void LoadBitmapFromPNG(GameBitmap * gameBitmap, const char * filepath)
{
    int w, h;
    int channels;
    uint8_t * dataLoaded;
    
    dataLoaded = stbi_load(filepath, &w, &h, &channels, 4);
    gameBitmap->data = (uint32_t*)malloc(sizeof(uint32_t) * w * h);

    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            Pixel32 p;
            uint8_t temp;

            p.hexValue = (uint32_t)((uint32_t*)dataLoaded)[y * w + x];
            temp = p.r;
            p.r = p.b;
            p.b = temp;

            gameBitmap->data[y * w + x] = p.hexValue;
        }
    }

    gameBitmap->w = w;
    gameBitmap->h = h;
    gameBitmap->channels = channels;

    gameBitmap->info.bmiHeader.biSize = sizeof(gameBitmap->info.bmiHeader);
    gameBitmap->info.bmiHeader.biHeight = h;
    gameBitmap->info.bmiHeader.biWidth = w;
    gameBitmap->info.bmiHeader.biPlanes = 1;
    gameBitmap->info.bmiHeader.biCompression = BI_RGB;
    gameBitmap->info.bmiHeader.biSizeImage = 0;
    gameBitmap->info.bmiHeader.biXPelsPerMeter = 0;
    gameBitmap->info.bmiHeader.biYPelsPerMeter = 0;
    gameBitmap->info.bmiHeader.biClrUsed = 0;
    gameBitmap->info.bmiHeader.biClrImportant = 0;


    stbi_image_free(dataLoaded);
}


void BlitBitmap(GameBitmap * gameBitmap, int x, int y, GameRect * srcRect)
{
    Pixel32 bmpPixel;
    GameRect srcUse = { 0, 0, gameBitmap->w, gameBitmap->h };

    if (srcRect != NULL)
    {
        srcUse = *srcRect;
    }

    for (int dy = y; dy < y + srcUse.h; dy++)
    {
        for (int dx = x; dx < x + srcUse.w; dx++)
        {
            int bmpXOffset = dx - x + srcUse.x;
            int bmpYOffset = dy - y + srcUse.y;

            bmpPixel.hexValue = gameBitmap->data[bmpYOffset * gameBitmap->w + bmpXOffset];
            if (bmpPixel.a > 0)
            {
                PlotPixel(dx, dy, bmpPixel.r, bmpPixel.g, bmpPixel.b);
            }
        }
    }
}