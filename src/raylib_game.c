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
#define TILE_SIZE 60
#define TIlE_TYPES 5
#define SCORE_FONT_SIZE 32
#define TILE_FONT_SIZE 40
#define MAX_SCORE_POPUPS 32

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
static const int screenWidth = 1280;
static const int screenHeight = 720;

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

static int score = 0;
static float score_scale = 1.0f;
static float score_scale_velocity = 0.0f;
static bool score_animating = false;

static Vector2 selected_tile = {-1, -1};
static float fall_offset[BOARD_SIZE][BOARD_SIZE] = {0.0f};
static float fall_speed = 1.0f;
static float match_delay_time = 0.0f;
static const float MATCH_DELAY_DURATION = 0.2f;

typedef enum { STATE_IDLE, STATE_ANIMATING, STATE_MATCH_DELAY } TileState;

TileState tile_state;

typedef struct {
    Vector2 position;
    int amount;
    float lifetime;
    float alpha;
    bool active;
} ScorePopUp;

ScorePopUp score_popups[MAX_SCORE_POPUPS] = {0};

static Texture2D background;
static Font score_font;
static Music background_music;
static Sound match_sound;

// My structs

// My functions

static char random_tile();
static add_score_popup(int x, int y, int amount, Vector2 grid_origin);
static bool find_matches();
static void resolve_matches();
static void init_board();
static void draw_text_on_tile(char symbol, Vector2 position, Color color);
static void swap_tiles(int x1, int y1, int x2, int y2);
static bool are_tiles_adjacent(Vector2 a, Vector2 b);

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
    background_music = LoadMusicStream("src/resources/bgm_old.mp3");
    match_sound = LoadSound("src/resources/match_old.mp3");

    PlayMusicStream(background_music);
    init_board();

    Vector2 mouse = {0, 0};

    while (!WindowShouldClose()) {
        UpdateMusicStream(background_music);
        mouse = GetMousePosition();

        if (tile_state == STATE_IDLE &&
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            int x = (mouse.x - grid_origin.x) / TILE_SIZE;
            int y = (mouse.y - grid_origin.y) / TILE_SIZE;

            if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) {
                Vector2 current_tile = (Vector2){x, y};

                if (selected_tile.x < 0) {
                    selected_tile = current_tile;
                } else {
                    if (are_tiles_adjacent(selected_tile, current_tile)) {
                        swap_tiles(
                            selected_tile.x,
                            selected_tile.y,
                            current_tile.x,
                            current_tile.y
                        );
                        if (find_matches()) {
                            resolve_matches();
                            selected_tile = (Vector2){-1, -1};
                        } else {
                            swap_tiles(
                                selected_tile.x,
                                selected_tile.y,
                                current_tile.x,
                                current_tile.y
                            );
                        }
                    } else if (selected_tile.x == current_tile.x &&
                               selected_tile.y == current_tile.y) {
                        selected_tile = (Vector2){-1, -1};
                    } else {
                        selected_tile = current_tile;
                    }
                }
            }
        }

        if (tile_state == STATE_ANIMATING) {
            bool still_animating = false;

            // Fall animation speed
            for (int y = 0; y < BOARD_SIZE; y++) {
                for (int x = 0; x < BOARD_SIZE; x++) {
                    if (fall_offset[y][x] > 0) {
                        fall_offset[y][x] -= fall_speed;
                        if (fall_offset[y][x] < 0) {
                            fall_offset[y][x] = 0;
                        } else {
                            still_animating = true;
                        }
                    }
                }
            }

            if (!still_animating) {
                tile_state = STATE_MATCH_DELAY;
                match_delay_time = MATCH_DELAY_DURATION;
            }
        }

        if (tile_state == STATE_MATCH_DELAY) {
            match_delay_time -= GetFrameTime();
            if (match_delay_time <= 0) {
                if (find_matches())
                    resolve_matches();
                else
                    tile_state = STATE_IDLE;
            }
        }

        // update score popups array
        for (int i = 0; i < MAX_SCORE_POPUPS; i++) {
            if (score_popups[i].active) {
                score_popups[i].lifetime -= GetFrameTime();
                score_popups[i].position.y -= 30 * GetFrameTime();
                score_popups[i].alpha = score_popups[i].lifetime;

                if (score_popups[i].lifetime <= 0) {
                    score_popups[i].active = false;
                }
            }
        }

        // score animation
        if (score_animating) {
            score_scale += score_scale_velocity * GetFrameTime();

            if (score_scale <= 1.0) {
                score_scale = 1.0;
                score_animating = false;
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
            SCORE_FONT_SIZE * score_scale,
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
                DrawRectangleRec(
                    rect, matched[y][x] ? WHITE : Fade(DARKGRAY, 0.85)
                );
                DrawRectangleLinesEx(rect, 1, Fade(WHITE, 0.65));

                Vector2 pos = {
                    .x = rect.x + 18,
                    .y = rect.y + 10 - fall_offset[y][x],
                };

                if (board[y][x] != ' ') {
                    // Draw a character with different colors for each symbol
                    // into the rectangle SYMBOLS => '#', '@', '$', '%', '&'
                    switch (board[y][x]) {
                    case '#':
                        draw_text_on_tile(board[y][x], pos, BLUE);
                        break;
                    case '@':
                        draw_text_on_tile(board[y][x], pos, GREEN);
                        break;
                    case '$':
                        draw_text_on_tile(board[y][x], pos, RED);
                        break;
                    case '%':
                        draw_text_on_tile(board[y][x], pos, GOLD);
                        break;
                    case '&':
                        draw_text_on_tile(board[y][x], pos, PURPLE);
                        break;
                    default:
                        break;
                    }
                }

                // Draw the selected tile
                if (selected_tile.x >= 0) {
                    DrawRectangleLinesEx(selected, 2, YELLOW);
                }
            }
        }

        // draw score popups
        for (int i = 0; i < MAX_SCORE_POPUPS; i++) {
            if (score_popups[i].active) {
                Color c = Fade(YELLOW, score_popups[i].alpha);
                DrawText(
                    TextFormat("+%d", score_popups[i].amount),
                    score_popups[i].position.x,
                    score_popups[i].position.y,
                    TILE_FONT_SIZE,
                    c
                );
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
    StopMusicStream(background_music);
    UnloadMusicStream(background_music);
    UnloadSound(match_sound);
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

    if (find_matches())
        resolve_matches();
    else
        tile_state = STATE_IDLE;
}

static void draw_text_on_tile(char symbol, Vector2 position, Color color) {
    DrawTextEx(
        GetFontDefault(),
        TextFormat("%c", symbol),
        position,
        TILE_FONT_SIZE,
        1,
        color
    );
}

static char random_tile() { return tile_chars[rand() % TIlE_TYPES]; }

static add_score_popup(int x, int y, int amount, Vector2 grid_origin) {
    for (int i = 0; i < MAX_SCORE_POPUPS; i++) {
        if (!score_popups[i].active) {
            score_popups[i].position = (Vector2){
                grid_origin.x + x * TILE_SIZE + TILE_SIZE / 2,
                grid_origin.y + y * TILE_SIZE + TILE_SIZE / 2,
            };
            score_popups[i].amount = amount;
            score_popups[i].lifetime = 1.0f;
            score_popups[i].alpha = 1.0f;
            score_popups[i].active = true;
            break;
        }
    }
}

static bool find_matches() {
    bool found = false;

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            matched[y][x] = false;
        }
    }

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE - 2; x++) {
            char t = board[y][x];

            if (t == board[y][x + 1] && t == board[y][x + 2]) {
                matched[y][x] = true;
                matched[y][x + 1] = true;
                matched[y][x + 2] = true;

                score += 10;
                found = true;
                PlaySound(match_sound);

                score_animating = true;
                score_scale = 2.0f;
                score_scale_velocity = -2.5f;

                add_score_popup(x, y, 10, grid_origin);
            }
        }
    }

    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE - 2; y++) {
            char t = board[y][x];

            if (t == board[y + 1][x] && t == board[y + 2][x]) {
                matched[y][x] = true;
                matched[y + 1][x] = true;
                matched[y + 2][x] = true;

                score += 10;
                found = true;
                PlaySound(match_sound);

                score_animating = true;
                score_scale = 2.0f;
                score_scale_velocity = -2.5f;

                add_score_popup(x, y, 10, grid_origin);
            }
        }
    }

    return found;
}

static void resolve_matches() {
    for (int x = 0; x < BOARD_SIZE; x++) {
        int write_y = BOARD_SIZE - 1;
        for (int y = BOARD_SIZE - 1; y >= 0; y--) {
            if (!matched[y][x]) {
                if (y != write_y) {
                    board[write_y][x] = board[y][x];
                    fall_offset[write_y][x] = (write_y - y) * TILE_SIZE;
                    board[y][x] = ' ';
                }
                write_y--;
            }
        }

        while (write_y >= 0) {
            board[write_y][x] = random_tile();
            fall_offset[write_y][x] = (write_y + 1) * TILE_SIZE;
            write_y--;
        }
    }

    tile_state = STATE_ANIMATING;
}

static void swap_tiles(int x1, int y1, int x2, int y2) {
    char temp = board[y1][x1];
    board[y1][x1] = board[y2][x2];
    board[y2][x2] = temp;
}

static bool are_tiles_adjacent(Vector2 a, Vector2 b) {
    return (abs((int) a.x - (int) b.x) + abs((int) a.y - (int) b.y)) == 1;
}