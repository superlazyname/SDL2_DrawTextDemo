#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

// Types
//---------------------------------------------------------------------------------------------------

typedef struct InitSDLValues_s
{
    SDL_Window*  Window;
    SDL_Renderer* Renderer;
    
} InitSDLValues_t;

typedef struct IntVec2
{
    int X; // X coordinate or column number
    int Y; // Y coordinate or row number
} IntVec2_t;

// Constants
//---------------------------------------------------------------------------------------------------

# define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// change this to change the size of the window
const IntVec2_t cScreenResolution = {640, 480};

// this is more FYI than anything
const int cFPS = 60;

// 1/60 is 0.016666666666666666
const int cFrameDuration_ms = 16;

// the only reason I stopped at the letter 'L' is because I had a very hard time drawing a letter M that was recognizable,
// to be honest I was mostly just interested in drawing '0' through '9'
#define FONT_IMAGE_COUNT 23

// change this to change the amount of spacing between characters in the string
const int cBetweenCharSpacing_px = 2;

// Unfortunately it's not as simple as "Look for letter X in X.png", because symbols like ? are not valid file names in some OSes
const char* cExpectedLetterFileNames[] =
{
    "0.png",
    "1.png",
    "2.png",
    "3.png",
    "4.png",
    "5.png",
    "6.png",
    "7.png",
    "8.png",
    "9.png",
    "A.png",
    "B.png",
    "C.png",
    "D.png",
    "E.png",
    "F.png",
    "G.png",
    "H.png",
    "I.png",
    "J.png",
    "K.png",
    "L.png",
    // ... you can add all the other characters here if you want.
    "QMark.png"
};

const int cQuestionMarkIndex = 22;

// Globals
//---------------------------------------------------------------------------------------------------

// various SDL resources required for rendering.
InitSDLValues_t SDLGlobals;

// The texture to render to, this is copied to the screen
SDL_Texture* RenderTexture = NULL;

// The size of each letter texture
IntVec2_t LetterSizes[FONT_IMAGE_COUNT];

// The texture for each letter
SDL_Texture* LetterTextures[FONT_IMAGE_COUNT];

static_assert(ARRAY_SIZE(cExpectedLetterFileNames) == ARRAY_SIZE(LetterTextures));

// Functions
//---------------------------------------------------------------------------------------------------

IntVec2_t InquireTextureSize(SDL_Texture* texture)
{
    int width, height;

    SDL_QueryTexture(texture, NULL, NULL, &width, &height);

    IntVec2_t size = {width, height};

    return size;
}


SDL_Texture* LoadImage(SDL_Renderer *renderer, const char* path)
{
    SDL_Texture *texture = NULL;

    SDL_Surface* image = IMG_Load(path);

    if (image != NULL) 
    {
        texture = SDL_CreateTextureFromSurface(renderer, image);

        SDL_FreeSurface(image);
        image = NULL;
    } 
    else 
    {
        printf("Image '%s' could not be loaded. SDL Error: %s\n", path, SDL_GetError());
    }

    return texture;

}

// this function expects the images to be in a folder in the same directory as this file
void LoadLetterTextures(const char* folderPath)
{
    printf("Looking for images in folder %s\n", folderPath);
    chdir(folderPath);

    for(int fileIndex = 0; fileIndex < ARRAY_SIZE(LetterTextures); fileIndex++)
    {
        const char* filePath = cExpectedLetterFileNames[fileIndex];
        SDL_Texture* texture = LoadImage(SDLGlobals.Renderer, filePath);
        LetterTextures[fileIndex] = texture;
        LetterSizes[fileIndex] = InquireTextureSize(texture);
    }

    chdir("..");

    printf("Done loading images.\n");
}

void FreeAllLetterTextures(void)
{
    for(int fileIndex = 0; fileIndex < ARRAY_SIZE(LetterTextures); fileIndex++)
    {
        if(LetterTextures[fileIndex] != NULL)
        {
            SDL_DestroyTexture(LetterTextures[fileIndex]);
            LetterTextures[fileIndex] = NULL;
            IntVec2_t zeroSize = {0, 0};
            LetterSizes[fileIndex] = zeroSize;
        }
    }

}

void DrawTexture(SDL_Texture *texture, IntVec2_t textureSize, IntVec2_t screenCoord)
{
    SDL_Rect textureRectangle;
    textureRectangle.x = 0;
    textureRectangle.y = 0;
    textureRectangle.w = textureSize.X;
    textureRectangle.h = textureSize.Y;

    SDL_Rect screenRectangle;
    screenRectangle.x = screenCoord.X;
    screenRectangle.y = screenCoord.Y;

    screenRectangle.w = textureSize.X;
    screenRectangle.h = textureSize.Y;

    SDL_RenderCopy(SDLGlobals.Renderer, texture, &textureRectangle, &screenRectangle);
}

// uses ? for unknown characters
int TextureIndexForChar(char ch)
{
    // http://www.asciitable.com/
    if(ch < '0')
    {
        return cQuestionMarkIndex;
    }

    if(ch <='9')
    {
        return (ch - '0');
    }

    if(ch < 'A')
    {
        return cQuestionMarkIndex;
    }

    if(ch < 'M')
    {
        return ((ch - 'A') + 10);
    }

    return cQuestionMarkIndex;

}

void DrawChar(IntVec2_t topLeftCorner, char ch)
{
    int charIndex = TextureIndexForChar(ch);

    assert(charIndex >= 0);
    assert(charIndex < ARRAY_SIZE(LetterTextures));

    SDL_Texture* charTexture = LetterTextures[charIndex];
    IntVec2_t size = LetterSizes[charIndex];

    DrawTexture(charTexture, size, topLeftCorner);
}


// note that only letters you have textures for will actually work,
// don't try and render the string "%$^@#&~@abcd||__ unless you want to provide characters for that
// note that space (' ') also needs its own character
void RenderString(IntVec2_t startPosition, const char* text, int textLength)
{
    if(textLength < 1)
    {
        return;
    }

    int stringCharIndex = 0;

    int xPosition = startPosition.X;;
    for(stringCharIndex = 0; stringCharIndex < textLength; stringCharIndex++)
    {
        IntVec2_t position = {xPosition, startPosition.Y};
        const char ch = text[stringCharIndex];

        DrawChar(position, ch);

        int charID = TextureIndexForChar(ch);

        IntVec2_t size = LetterSizes[charID];

        xPosition += size.X;
        xPosition += cBetweenCharSpacing_px;
        int breakpoint = 1;
    }
}


const InitSDLValues_t InitSDL(IntVec2_t windowSize_px)
{
    InitSDLValues_t sdlInitResult = {NULL, NULL};

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) 
    {
        return sdlInitResult;
    }

    // Init the window
    SDL_Window* window = SDL_CreateWindow("Font Demo!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowSize_px.X, windowSize_px.Y, SDL_WINDOW_SHOWN);
    if (!window) 
    {
        printf("An error occured while trying to create window : %s\n", SDL_GetError());
        return sdlInitResult;
    }

    // Init the renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) 
    {
        printf("An error occured while trying to create renderer : %s\n", SDL_GetError());
        return sdlInitResult;
    }

    sdlInitResult.Window = window;
    sdlInitResult.Renderer = renderer;
    return sdlInitResult;
}

// just checks for a quit signal. You can put more keypresses and mouse button handlers here if you want.
// returns 1 on quit, returns 0 otherwise
int HandleInput()
{
    SDL_Event event = {0};

    while (SDL_PollEvent(&event)) 
    {
        // E.g., from hitting the close window button
        if (event.type == SDL_QUIT) 
        {
            return 1;
        }
    }

    return 0;
}

void Render(void)
{
    // Render the string to the texture
    SDL_SetRenderTarget(SDLGlobals.Renderer, RenderTexture);

    // clear to something that isn't black, because our text is black
    SDL_SetRenderDrawColor(SDLGlobals.Renderer, 255, 200, 200, 255);
    SDL_RenderClear(SDLGlobals.Renderer);

    IntVec2_t stringPosition = {32, 64};

    const char* testString = "A0A0A0BE334C3D0C";
    const int length = strlen(testString);
    RenderString(stringPosition, testString, length);

    SDL_RenderPresent(SDLGlobals.Renderer);

    // now render the render texture to the screen
    SDL_SetRenderTarget(SDLGlobals.Renderer, NULL);

    SDL_Rect fullScreenRectangle = {0};
    fullScreenRectangle.w = cScreenResolution.X;
    fullScreenRectangle.h = cScreenResolution.Y;
    fullScreenRectangle.x = 0;
    fullScreenRectangle.y = 0;

    SDL_RenderCopy(SDLGlobals.Renderer, RenderTexture, &fullScreenRectangle, &fullScreenRectangle);
}

void FrameDelay(unsigned int targetTicks)
{
    // Block at 60 fps

    // ticks is in ms
    unsigned int ticks = SDL_GetTicks();

    if (targetTicks < ticks) 
    {
        return;
    }

    if (targetTicks > ticks + cFrameDuration_ms) 
    {
        SDL_Delay(cFrameDuration_ms);
    } 
    else 
    {
        SDL_Delay(targetTicks - ticks);
    }
}

int main()
{
    // initialization
    SDLGlobals = InitSDL(cScreenResolution);

    RenderTexture = SDL_CreateTexture(SDLGlobals.Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, cScreenResolution.X, cScreenResolution.Y);

    LoadLetterTextures("LetterImages");    

    // main loop
    unsigned int targetTicks = SDL_GetTicks() + cFrameDuration_ms;
    while(1)
    {
        int quitSignal = HandleInput();

        if(quitSignal)
        {
            break;
        }

        Render();
        FrameDelay(targetTicks);
        targetTicks = SDL_GetTicks() + cFrameDuration_ms;
    }

    // after quit: clean up resources

    FreeAllLetterTextures();
    SDL_DestroyTexture(RenderTexture);
    RenderTexture = NULL;

    return 0;
}