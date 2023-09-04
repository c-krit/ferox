/*
    Copyright (c) 2021-2023 Jaedeok Kim <jdeokkim@protonmail.com>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

/* Includes ============================================================================= */

#include "ferox.h"
#include "raylib.h"

#define FEROX_RAYLIB_IMPLEMENTATION
#include "ferox-raylib.h"

#ifdef PLATFORM_WEB
    #include <emscripten/emscripten.h>
#endif

/* Macros =============================================================================== */

#define TARGET_FPS       60

#define SCREEN_WIDTH     1280
#define SCREEN_HEIGHT    800

#define MAX_WALL_COUNT   4

/* Constants ============================================================================ */

static const frMaterial MATERIAL_BRICK = { .density = 1.25f, .friction = 0.75f };
static const frMaterial MATERIAL_WALL = { .density = 1.5f, .friction = 0.85f };

static const float BRICK_WIDTH = 60.0f, BRICK_HEIGHT = 48.0f;
static const float CELL_SIZE = 4.0f, DELTA_TIME = 1.0f / TARGET_FPS;

/* Private Variables ==================================================================== */

static frWorld *world;

static frBody *cursor, *walls[MAX_WALL_COUNT];

static Rectangle bounds = { .width = SCREEN_WIDTH, .height = SCREEN_HEIGHT };

static RenderTexture2D brickTarget;

/* Private Function Prototypes ========================================================== */

static void InitExample(void);
static void UpdateExample(void);
static void DeinitExample(void);

/* Public Functions ==================================================================== */

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | bricks.c");

    InitExample();

#ifdef PLATFORM_WEB
    emscripten_set_main_loop(UpdateExample, 0, 1);
#else
    // SetTargetFPS(TARGET_FPS);

    while (!WindowShouldClose()) 
        UpdateExample();
#endif

    DeinitExample();

    CloseWindow();

    return 0;
}

/* Private Functions ==================================================================== */

static void InitExample(void) {
    world = frCreateWorld(
        frVector2ScalarMultiply(FR_WORLD_DEFAULT_GRAVITY, 2.5f), 
        CELL_SIZE
    );

    walls[0] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        frVector2PixelsToUnits(
            (frVector2) { 
                .x = -0.05f * SCREEN_WIDTH,
                .y = 0.5f * SCREEN_HEIGHT 
            }
        ),
        frCreateRectangle(
            MATERIAL_WALL,
            frPixelsToUnits(0.1f * SCREEN_WIDTH),
            frPixelsToUnits(1.1f * SCREEN_HEIGHT)
        )
    );

    walls[1] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        frVector2PixelsToUnits(
            (frVector2) {
                .x = 0.5f * SCREEN_WIDTH, 
                .y = 1.05f * SCREEN_HEIGHT 
            }
        ),
        frCreateRectangle(
            MATERIAL_WALL,
            frPixelsToUnits(1.1f * SCREEN_WIDTH),
            frPixelsToUnits(0.1f * SCREEN_HEIGHT)
        )
    );

    walls[2] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        frVector2PixelsToUnits(
            (frVector2) { 
                .x = 1.05f * SCREEN_WIDTH, 
                .y = 0.5f * SCREEN_HEIGHT 
            }
        ),
        frCreateRectangle(
            MATERIAL_WALL,
            frPixelsToUnits(0.1f * SCREEN_WIDTH),
            frPixelsToUnits(1.1f * SCREEN_HEIGHT)
        )
    );

    walls[3] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        frVector2PixelsToUnits(
            (frVector2) {
                .x = 0.5f * SCREEN_WIDTH, 
                .y = -0.05f * SCREEN_HEIGHT 
            }
        ),
        frCreateRectangle(
            MATERIAL_WALL,
            frPixelsToUnits(1.1f * SCREEN_WIDTH),
            frPixelsToUnits(0.1f * SCREEN_HEIGHT)
        )
    );

    for (int i = 0; i < MAX_WALL_COUNT; i++)
        frAddBodyToWorld(world, walls[i]);

    cursor = frCreateBodyFromShape(
        FR_BODY_KINEMATIC,
        FR_API_STRUCT_ZERO(frVector2),
        frCreateRectangle(
            MATERIAL_BRICK, 
            frPixelsToUnits(BRICK_WIDTH), 
            frPixelsToUnits(BRICK_HEIGHT)
        )
    );
    
    frAddBodyToWorld(world, cursor);

    brickTarget = LoadRenderTexture(BRICK_WIDTH, BRICK_HEIGHT);

    {
        BeginTextureMode(brickTarget);

        ClearBackground(BLANK);

        DrawRectangleLinesEx(
            (Rectangle) { 
                .width = BRICK_WIDTH, 
                .height = BRICK_HEIGHT 
            }, 
            2.0f, 
            ColorAlpha(WHITE, 0.95f)
        );

        DrawCircleV(
            (Vector2) {
                .x = 0.5f * BRICK_WIDTH, 
                .y = 0.5f * BRICK_HEIGHT 
            }, 
            2.0f, 
            ColorAlpha(WHITE, 0.95f)
        );

        EndTextureMode();
    }

    SetTextureFilter(brickTarget.texture, TEXTURE_FILTER_BILINEAR);
}

static void UpdateExample(void) {
    const Vector2 mousePosition = GetMousePosition();

    frSetBodyPosition(
        cursor, 
        frVector2PixelsToUnits(
            (frVector2) {
                .x = mousePosition.x,
                .y = mousePosition.y
            }
        )
    );

    {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            frBody *brick = frCreateBodyFromShape(
                FR_BODY_DYNAMIC,
                frVector2PixelsToUnits(
                    (frVector2) {
                        .x = mousePosition.x,
                        .y = mousePosition.y + (1.05f * BRICK_HEIGHT)
                    }
                ),
                frCreateRectangle(
                    MATERIAL_BRICK,
                    frPixelsToUnits(BRICK_WIDTH), 
                    frPixelsToUnits(BRICK_HEIGHT)
                )
            );
            
            frAddBodyToWorld(world, brick);
        }
    }

    frUpdateWorld(world, DELTA_TIME);

    {
        BeginDrawing();
            
        ClearBackground(FR_DRAW_COLOR_MATTEBLACK);

        frDrawGrid(bounds, CELL_SIZE, 0.25f, ColorAlpha(DARKGRAY, 0.75f));

        const int bodyCount = frGetBodyCountForWorld(world);

        for (int i = MAX_WALL_COUNT; i < bodyCount; i++) {
            frBody *body = frGetBodyFromWorld(world, i);

            const frVector2 bodyPosition = frGetBodyPosition(body);

            DrawTexturePro(
                brickTarget.texture,
                (Rectangle) { 
                    .width = BRICK_WIDTH, 
                    .height = BRICK_HEIGHT 
                },
                (Rectangle) {
                    .x = frUnitsToPixels(bodyPosition.x), 
                    .y = frUnitsToPixels(bodyPosition.y),
                    .width = BRICK_WIDTH,
                    .height = BRICK_HEIGHT
                },
                (Vector2) {
                    .x = 0.5f * BRICK_WIDTH, 
                    .y = 0.5f * BRICK_HEIGHT 
                },
                RAD2DEG * frGetBodyAngle(body),
                ColorAlpha(WHITE, 1.0f)
            );
        }

        DrawTexturePro(
            brickTarget.texture,
            (Rectangle) {
                .width = BRICK_WIDTH, 
                .height = BRICK_HEIGHT 
            },
            (Rectangle) {
                .x = mousePosition.x, 
                .y = mousePosition.y,
                .width = BRICK_WIDTH,
                .height = BRICK_HEIGHT
            },
            (Vector2) {
                .x = 0.5f * BRICK_WIDTH, 
                .y = 0.5f * BRICK_HEIGHT 
            },
            0.0f,
            ColorAlpha(WHITE, 0.5f)
        );

        DrawFPS(8, 8);

        EndDrawing();
    }
}

static void DeinitExample(void) {
    frReleaseWorld(world);
}