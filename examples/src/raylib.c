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

#define TARGET_FPS             60

#define SCREEN_WIDTH           1280
#define SCREEN_HEIGHT          800

#define WORLD_CELL_SIZE        2.8f

#define LOGO_WIDTH_IN_PIECES   42
#define LOGO_HEIGHT_IN_PIECES  42

/* Typedefs ============================================================================= */

typedef struct _Piece {
    frBody *body;
    frVector2 offset;
} Piece;

/* Constants ============================================================================ */

static const float DELTA_TIME = 1.0f / TARGET_FPS;

/* Private Variables ==================================================================== */

static frWorld *world;
static frBody *ground;

static Rectangle bounds = { .width = SCREEN_WIDTH, .height = SCREEN_HEIGHT };

static Texture2D raylibTexture;

static Piece pieces[LOGO_WIDTH_IN_PIECES * LOGO_HEIGHT_IN_PIECES];

static float pieceWidth, pieceHeight;
static float halfPieceWidth, halfPieceHeight;

/* Private Function Prototypes ========================================================== */

static void InitExample(void);
static void UpdateExample(void);
static void DeinitExample(void);

/* Public Functions ===================================================================== */

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | raylib.c");

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
    world = frCreateWorld(FR_WORLD_DEFAULT_GRAVITY, WORLD_CELL_SIZE);

    raylibTexture = LoadTexture("../res/images/raylib.png");

    if (IsTextureReady(raylibTexture)) {
        pieceWidth = raylibTexture.width / LOGO_WIDTH_IN_PIECES;
        pieceHeight = raylibTexture.height / LOGO_HEIGHT_IN_PIECES;

        halfPieceWidth = 0.5f * pieceWidth, halfPieceHeight = 0.5f * pieceHeight;

        frShape *pieceShape = frCreateRectangle(
            (frMaterial) {
                .density = 0.75f,
                .friction = 0.5f,
                .restitution = 0.0f
            },
            frPixelsToUnits(pieceWidth),
            frPixelsToUnits(pieceHeight)
        );

        const frVector2 origin = {
            0.5f * (SCREEN_WIDTH - raylibTexture.width),
            0.35f * (SCREEN_HEIGHT - raylibTexture.height)
        };

        for (int i = 0; i < LOGO_WIDTH_IN_PIECES * LOGO_HEIGHT_IN_PIECES; i++) {
            pieces[i].offset.x = (i % LOGO_WIDTH_IN_PIECES) * pieceWidth;
            pieces[i].offset.y = (i / LOGO_WIDTH_IN_PIECES) * pieceHeight;

            const frVector2 position = {
                (origin.x + pieces[i].offset.x) + halfPieceWidth,
                (origin.y + pieces[i].offset.y) + halfPieceHeight
            };

            pieces[i].body = frCreateBodyFromShape(
                FR_BODY_DYNAMIC,
                frVector2PixelsToUnits(position),
                pieceShape
            );

            frAddBodyToWorld(world, pieces[i].body);
        }

        const float groundWidth = 0.5f * SCREEN_WIDTH;
        const float groundHeight = 0.05f * SCREEN_HEIGHT;

        const float halfGroundHeight = 0.5f * groundHeight;

        ground = frCreateBodyFromShape(
            FR_BODY_STATIC,
            frVector2PixelsToUnits(
                (frVector2) {
                    .x = 0.5f * SCREEN_WIDTH,
                    .y = ((origin.y + raylibTexture.height) + halfGroundHeight) + 28.0f
                }
            ),
            frCreateRectangle(
                (frMaterial) {
                    .density = 1.5f,
                    .friction = 0.75f,
                    .restitution = 0.0f
                },
                frPixelsToUnits(groundWidth),
                frPixelsToUnits(groundHeight)
            )
        );

        frAddBodyToWorld(world, ground);
    }
}

static void UpdateExample(void) {
    frUpdateWorld(world, DELTA_TIME);

    {
        BeginDrawing();
            
        ClearBackground(FR_DRAW_COLOR_MATTEBLACK);

        frDrawGrid(bounds, WORLD_CELL_SIZE, 0.25f, ColorAlpha(DARKGRAY, 0.75f));

        for (int i = 0; i < LOGO_WIDTH_IN_PIECES * LOGO_HEIGHT_IN_PIECES; i++) {
            const frVector2 bodyPosition = frGetBodyPosition(pieces[i].body);

            DrawTexturePro(
                raylibTexture,
                (Rectangle) {
                    .x = pieces[i].offset.x,
                    .y = pieces[i].offset.y,
                    .width = pieceWidth,
                    .height = pieceHeight
                },
                (Rectangle) {
                    .x = frUnitsToPixels(bodyPosition.x), 
                    .y = frUnitsToPixels(bodyPosition.y),
                    .width = pieceWidth,
                    .height = pieceHeight
                },
                (Vector2) {
                    .x = halfPieceWidth,
                    .y = halfPieceHeight
                },
                RAD2DEG * frGetBodyAngle(pieces[i].body),
                WHITE
            );
        }

        frDrawBodyLines(ground, 1.0f, WHITE);

        DrawFPS(8, 8);

        EndDrawing();
    }
}

static void DeinitExample(void) {
    UnloadTexture(raylibTexture);

    frReleaseShape(frGetBodyShape(pieces[0].body));
    frReleaseShape(frGetBodyShape(ground));

    frReleaseWorld(world);
}