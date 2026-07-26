/*******************************************************************************************
 *
 *   raylib game template
 *
 *
 *   Code licensed under an unmodified zlib/libpng license, which is an
 *OSI-certified, BSD-like license that allows static linking with closed source
 *software
 *
 *   Copyright (c) 2021-2026 Ramon Santamaria (@raysan5)
 *
 ********************************************************************************************/

#include "raylib.h"
#include "screens.h" // NOTE: Declares global (extern) variables and screens functions

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h> // Emscripten library
#endif

#include <math.h>
#include <stdio.h>  // Required for: printf()
#include <stdlib.h> // Required for:
#include <string.h> // Required for:
#include <time.h>

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
// Simple log system to avoid printf() calls if required
// NOTE: Avoiding those calls, also avoids const strings memory usage
#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...)
#endif

// My game defines
#define BOARD_SIZE 8
#define TILE_SIZE 42
#define TIlE_TYPES 5
#define SCORE_FONT_SIZE 32

//----------------------------------------------------------------------------------
// Shared Variables Definition (global)
// NOTE: Those variables are shared between modules through screens.h
//----------------------------------------------------------------------------------
GameScreen currentScreen = LOGO;
Font font = {0};
Music music = {0};
Sound fxCoin = {0};

//----------------------------------------------------------------------------------
// Global Variables Definition (local to this module)
//----------------------------------------------------------------------------------
static const int screenWidth = 800;
static const int screenHeight = 450;

// Required variables to manage screen transitions (fade-in, fade-out)
static float transAlpha = 0.0f;
static bool onTransition = false;
static bool transFadeOut = false;
static int transFromScreen = -1;
static GameScreen transToScreen = UNKNOWN;

// My game variables
static const char tile_chars[TILE_SIZE] = {'#', '@', '$', '%', '&'};
static char board[BOARD_SIZE][BOARD_SIZE];
static char matched[BOARD_SIZE][BOARD_SIZE] = {0};

static Vector2 grid_origin;
static int score = 200;
static Vector2 selected_tile = {-1, -1};

static Texture2D background;
static Font score_font;
//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
/*
static void ChangeToScreen(int screen
); // Change to screen, no transition effect

static void TransitionToScreen(int screen); // Request transition to next screen
static void UpdateTransition(void);         // Update transition effect
static void DrawTransition(void
); // Draw transition effect (full-screen rectangle)

static void UpdateDrawFrame(void); // Update and draw one frame
*/

// My functions

static char random_tile();
static bool find_matches();
static void init_board();

//----------------------------------------------------------------------------------
// Program main entry point
//----------------------------------------------------------------------------------
int main(void) {
    InitWindow(screenWidth, screenHeight, "Raylib 2D ASCII MATCH");
    InitAudioDevice(); // Initialize audio device
    srand(time(NULL));

    background = LoadTexture("src/resources/bg_sound_800_450_n.png");
    score_font =
        LoadFontEx("src/resources/04b03.ttf", SCORE_FONT_SIZE, NULL, 0);
    init_board();

    Vector2 mouse = {0, 0};

    while (!WindowShouldClose()) {
        mouse = GetMousePosition();

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            int x = (mouse.x - grid_origin.x) / TILE_SIZE;
            int y = (mouse.y - grid_origin.y) / TILE_SIZE;

            if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) {
                selected_tile = (Vector2){x, y};
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        DrawTexturePro(
            background,
            (Rectangle){0, 0, background.width, background.height},
            (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()},
            (Vector2){0, 0},
            0.0f,
            WHITE
        );

        DrawTextEx(
            score_font,
            TextFormat("SCORE: %d", score),
            (Vector2){20, 20},
            SCORE_FONT_SIZE,
            1.0f,
            YELLOW
        );

        for (int y = 0; y < BOARD_SIZE; y++) {
            for (int x = 0; x < BOARD_SIZE; x++) {
                Rectangle rect = {
                    .x = grid_origin.x + x * TILE_SIZE,
                    .y = grid_origin.y + y * TILE_SIZE,
                    .height = TILE_SIZE,
                    .width = TILE_SIZE
                };

                Rectangle selected = {
                    .x = grid_origin.x + selected_tile.x * TILE_SIZE,
                    .y = grid_origin.y + selected_tile.y * TILE_SIZE,
                    .height = TILE_SIZE,
                    .width = TILE_SIZE
                };

                // Draw the rectangle for each of the board pieces
                DrawRectangleRec(rect, DARKGRAY);

                Vector2 pos = {
                    .x = rect.x + 12,
                    .y = rect.y + 8,
                };

                // Draw a character with different colors for each symbol into
                // the rectangle
                // SYMBOLS => '#', '@', '$', '%', '&'
                switch (board[y][x]) {
                case '#':
                    DrawTextEx(
                        GetFontDefault(),
                        TextFormat("%c", board[y][x]),
                        pos,
                        20,
                        1,
                        BLUE
                    );
                    break;
                case '@':
                    DrawTextEx(
                        GetFontDefault(),
                        TextFormat("%c", board[y][x]),
                        pos,
                        20,
                        1,
                        GREEN
                    );
                    break;
                case '$':
                    DrawTextEx(
                        GetFontDefault(),
                        TextFormat("%c", board[y][x]),
                        pos,
                        20,
                        1,
                        RED
                    );
                    break;
                case '%':
                    DrawTextEx(
                        GetFontDefault(),
                        TextFormat("%c", board[y][x]),
                        pos,
                        20,
                        1,
                        GOLD
                    );
                    break;
                case '&':
                    DrawTextEx(
                        GetFontDefault(),
                        TextFormat("%c", board[y][x]),
                        pos,
                        20,
                        1,
                        PURPLE
                    );
                    break;

                default:
                    break;
                }

                // Draw the selected tile
                if (selected_tile.x >= 0) {
                    DrawRectangleLinesEx(selected, 2, YELLOW);
                }
            }
        }

        EndDrawing();
    }

    /*
    // Load global data (assets that must be available in all screens, i.e.
    // font)
    font = LoadFont("resources/mecha.png");
    // music = LoadMusicStream("resources/ambient.ogg"); // TODO: Load music
    fxCoin = LoadSound("resources/coin.wav");

    SetMusicVolume(music, 1.0f);
    PlayMusicStream(music);

    // Setup and init first screen
    currentScreen = LOGO;
    InitLogoScreen();

    // clang-format off
    #if defined(PLATFORM_WEB)
        emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
    #else
        SetTargetFPS(60); // Set our game to run at 60 frames-per-second
        //--------------------------------------------------------------------------------------

        // Main game loop
        while (!WindowShouldClose()) // Detect window close button or ESC
    key
        {
            UpdateDrawFrame();
        }
    #endif
    // clang-format on

    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Unload current screen data before closing
    switch (currentScreen) {
    case LOGO:
        UnloadLogoScreen();
        break;
    case TITLE:
        UnloadTitleScreen();
        break;
    case OPTIONS:
        UnloadOptionsScreen();
        break;
    case GAMEPLAY:
        UnloadGameplayScreen();
        break;
    case ENDING:
        UnloadEndingScreen();
        break;
    default:
        break;
    }

    // Unload global data loaded
    UnloadFont(font);
    UnloadMusicStream(music);
    UnloadSound(fxCoin);
    */

    // Unload data before closing
    UnloadTexture(background);
    UnloadFont(score_font);

    // Closing devices before exit
    CloseAudioDevice(); // Close audio context
    CloseWindow();      // Close window and OpenGL context

    return 0;
}

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
/*
// Change to next screen, no transition
static void ChangeToScreen(int screen) {
    // Unload current screen
    switch (currentScreen) {
    case LOGO:
        UnloadLogoScreen();
        break;
    case TITLE:
        UnloadTitleScreen();
        break;
    case OPTIONS:
        UnloadOptionsScreen();
        break;
    case GAMEPLAY:
        UnloadGameplayScreen();
        break;
    case ENDING:
        UnloadEndingScreen();
        break;
    default:
        break;
    }

    // Init next screen
    switch (screen) {
    case LOGO:
        InitLogoScreen();
        break;
    case TITLE:
        InitTitleScreen();
        break;
    case OPTIONS:
        InitOptionsScreen();
    case GAMEPLAY:
        InitGameplayScreen();
        break;
    case ENDING:
        InitEndingScreen();
        break;
    default:
        break;
    }

    currentScreen = screen;
}

// Request transition to next screen
static void TransitionToScreen(int screen) {
    onTransition = true;
    transFadeOut = false;
    transFromScreen = currentScreen;
    transToScreen = screen;
    transAlpha = 0.0f;
}

// Update transition effect (fade-in, fade-out)
static void UpdateTransition(void) {
    if (!transFadeOut) {
        transAlpha += 0.05f;

        // NOTE: Due to float internal representation, condition jumps on 1.0f
        // instead of 1.05f For that reason we compare against 1.01f, to avoid
        // last frame loading stop
        if (transAlpha > 1.01f) {
            transAlpha = 1.0f;

            // Unload current screen
            switch (transFromScreen) {
            case LOGO:
                UnloadLogoScreen();
                break;
            case TITLE:
                UnloadTitleScreen();
                break;
            case OPTIONS:
                UnloadOptionsScreen();
                break;
            case GAMEPLAY:
                UnloadGameplayScreen();
                break;
            case ENDING:
                UnloadEndingScreen();
                break;
            default:
                break;
            }

            // Load next screen
            switch (transToScreen) {
            case LOGO:
                InitLogoScreen();
                break;
            case TITLE:
                InitTitleScreen();
                break;
            case OPTIONS:
                InitOptionsScreen();
                break;
            case GAMEPLAY:
                InitGameplayScreen();
                break;
            case ENDING:
                InitEndingScreen();
                break;
            default:
                break;
            }

            currentScreen = transToScreen;

            // Activate fade out effect to next loaded screen
            transFadeOut = true;
        }
    } else // Transition fade out logic
    {
        transAlpha -= 0.02f;

        if (transAlpha < -0.01f) {
            transAlpha = 0.0f;
            transFadeOut = false;
            onTransition = false;
            transFromScreen = -1;
            transToScreen = UNKNOWN;
        }
    }
}

// Draw transition effect (full-screen rectangle)
static void DrawTransition(void) {
    DrawRectangle(
        0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, transAlpha)
    );
}

// Update and draw game frame
static void UpdateDrawFrame(void) {
    // Update
    //----------------------------------------------------------------------------------
    // UpdateMusicStream(music);       // NOTE: Music keeps playing between
    // screens

    if (!onTransition) {
        switch (currentScreen) {
        case LOGO: {
            UpdateLogoScreen();

            if (FinishLogoScreen()) TransitionToScreen(TITLE);

        } break;
        case TITLE: {
            UpdateTitleScreen();

            if (FinishTitleScreen() == 1)
                TransitionToScreen(OPTIONS);
            else if (FinishTitleScreen() == 2)
                TransitionToScreen(GAMEPLAY);

        } break;
        case OPTIONS: {
            UpdateOptionsScreen();

            if (FinishOptionsScreen()) TransitionToScreen(TITLE);

        } break;
        case GAMEPLAY: {
            UpdateGameplayScreen();

            if (FinishGameplayScreen() == 1) TransitionToScreen(ENDING);
            // else if (FinishGameplayScreen() == 2) TransitionToScreen(TITLE);

        } break;
        case ENDING: {
            UpdateEndingScreen();

            if (FinishEndingScreen() == 1) TransitionToScreen(TITLE);

        } break;
        default:
            break;
        }
    } else
        UpdateTransition(); // Update transition (fade-in, fade-out)
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(RAYWHITE);

    switch (currentScreen) {
    case LOGO:
        DrawLogoScreen();
        break;
    case TITLE:
        DrawTitleScreen();
        break;
    case OPTIONS:
        DrawOptionsScreen();
        break;
    case GAMEPLAY:
        DrawGameplayScreen();
        break;
    case ENDING:
        DrawEndingScreen();
        break;
    default:
        break;
    }

    // Draw full screen rectangle in front of everything
    if (onTransition) DrawTransition();

    // DrawFPS(10, 10);

    EndDrawing();
    //----------------------------------------------------------------------------------
}
*/
// My game functions definitions

static char random_tile() { return tile_chars[rand() % TIlE_TYPES]; }

static bool find_matches() { bool found = false; }

static void init_board() {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            board[y][x] = random_tile();
        }
    }

    int grid_width = BOARD_SIZE * TILE_SIZE;
    int grid_height = BOARD_SIZE * TILE_SIZE;

    grid_origin = (Vector2){
        .x = (GetScreenWidth() - grid_width) / 2,
        .y = (GetScreenHeight() - grid_height) / 2,
    };
}