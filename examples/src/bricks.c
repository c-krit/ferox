/*
    Copyright (c) 2021-2022 jdeokkim

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ferox.h"
#include "raylib.h"

#define TARGET_FPS 60

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

#define SCREEN_WIDTH_IN_METERS  (frNumberPixelsToMeters(SCREEN_WIDTH))
#define SCREEN_HEIGHT_IN_METERS (frNumberPixelsToMeters(SCREEN_HEIGHT))

#define WORLD_RECTANGLE ((Rectangle) { \
    .width = SCREEN_WIDTH_IN_METERS,   \
    .height = SCREEN_HEIGHT_IN_METERS  \
})

#define TEXT_FONT_SIZE    40
#define TEXT_STRING_DATA  "%d brick(s)!"

#define BRICK_MATERIAL    ((frMaterial) { 0.75f, 0.0f, 0.5f, 0.35f })
#define CURSOR_MATERIAL   ((frMaterial) { 2.25f, 0.0f, 0.5f, 0.35f })
#define WALL_MATERIAL     ((frMaterial) { 1.25f, 0.0f, 0.5f, 0.35f })

#define MAX_WALL_COUNT    3

#define BRICK_WIDTH       42.0f
#define BRICK_HEIGHT      28.0f

void InitExample(void);
void DeinitExample(void);
void UpdateExample(void);

const float DELTA_TIME = (1.0f / TARGET_FPS) * 100.0f;

static RenderTexture rtx;

static frWorld *world;

static frBody *walls[MAX_WALL_COUNT];
static frBody *cursor;

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | bricks.c");
    
    InitExample();

    while (!WindowShouldClose())
        UpdateExample();

    DeinitExample();
    
    CloseWindow();

    return 0;
}

void InitExample(void) {
    rtx = LoadRenderTexture(BRICK_WIDTH, BRICK_HEIGHT);

    {
        BeginTextureMode(rtx);

        ClearBackground(BLANK);

        DrawRectangleRec(
            (Rectangle) {
                .width = BRICK_WIDTH,
                .height = BRICK_HEIGHT
            },
            RED
        );

        EndTextureMode();
    }

    SetTextureFilter(rtx.texture, TEXTURE_FILTER_BILINEAR);

    world = frCreateWorld(
        frVec2ScalarMultiply(FR_WORLD_DEFAULT_GRAVITY, 0.00001f),
        WORLD_RECTANGLE
    );

    const frVertices wall1_vertices = {
        .data = {
            frVec2PixelsToMeters((Vector2) { -0.1f * SCREEN_WIDTH, -0.45f * SCREEN_HEIGHT }),
            frVec2PixelsToMeters((Vector2) { -0.1f * SCREEN_WIDTH,  0.45f * SCREEN_HEIGHT }),
            frVec2PixelsToMeters((Vector2) {  0.1f * SCREEN_WIDTH,  0.45f * SCREEN_HEIGHT })
        },
        .count = 3
    };

    const frVertices wall3_vertices = {
        .data = {
            frVec2PixelsToMeters((Vector2) {  0.1f * SCREEN_WIDTH, -0.45f * SCREEN_HEIGHT }),
            frVec2PixelsToMeters((Vector2) { -0.1f * SCREEN_WIDTH,  0.45f * SCREEN_HEIGHT }),
            frVec2PixelsToMeters((Vector2) {  0.1f * SCREEN_WIDTH,  0.45f * SCREEN_HEIGHT })
        },
        .count = 3
    };

    walls[0] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.1f * SCREEN_WIDTH, 0.55f * SCREEN_HEIGHT }),
        frCreatePolygon(WALL_MATERIAL, wall1_vertices)
    );
    
    walls[1] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, SCREEN_HEIGHT - 40.0f }),
        frCreateRectangle(
            WALL_MATERIAL,
            frNumberPixelsToMeters(SCREEN_WIDTH),
            frNumberPixelsToMeters(80.0f)
        )
    );

    walls[2] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.9f * SCREEN_WIDTH, 0.55f * SCREEN_HEIGHT }),
        frCreatePolygon(WALL_MATERIAL, wall3_vertices)
    );

    for (int i = 0; i < MAX_WALL_COUNT; i++)
        frAddToWorld(world, walls[i]);
    
    cursor = frCreateBodyFromShape(
        FR_BODY_KINEMATIC,
        FR_FLAG_NONE,
        FR_STRUCT_ZERO(Vector2),
        frCreateRectangle(
            CURSOR_MATERIAL, 
            frNumberPixelsToMeters(BRICK_WIDTH), 
            frNumberPixelsToMeters(BRICK_HEIGHT)
        )
    );
    
    frAddToWorld(world, cursor);
}

void DeinitExample(void) {
    frReleaseWorld(world);

    UnloadRenderTexture(rtx);
}

void UpdateExample(void) {
    frSetBodyPosition(cursor, frVec2PixelsToMeters(GetMousePosition()));

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        frBody *brick = frCreateBodyFromShape(
            FR_BODY_DYNAMIC,
            FR_FLAG_NONE,
            frVec2PixelsToMeters((Vector2) { GetMouseX(), GetMouseY() + 10.0f }),
            frCreateRectangle(
                CURSOR_MATERIAL,
                frNumberPixelsToMeters(BRICK_WIDTH), 
                frNumberPixelsToMeters(BRICK_HEIGHT)
            )
        );
        
        frAddToWorld(world, brick);
    }
    
    for (int i = 0; i < frGetWorldBodyCount(world); i++) {
        frBody *body = frGetWorldBody(world, i);
        
        if (!frIsInWorldBounds(world, body)) {
            frRemoveFromWorld(world, body);
            frReleaseBody(body);
        }
    }

    frSimulateWorld(world, DELTA_TIME);

    {
        BeginDrawing();
        
        ClearBackground(RAYWHITE);
        
        for (int i = 0; i < MAX_WALL_COUNT; i++)
            frDrawBody(walls[i], BLACK);
        
        for (int i = MAX_WALL_COUNT + 1; i < frGetWorldBodyCount(world); i++) {
            frBody *body = frGetWorldBody(world, i);

            Vector2 position = frGetBodyPosition(body);

            DrawTexturePro(
                rtx.texture,
                (Rectangle) {
                    .width = BRICK_WIDTH,
                    .height = BRICK_HEIGHT,
                },
                (Rectangle) {
                    .x = frNumberMetersToPixels(position.x), 
                    .y = frNumberMetersToPixels(position.y),
                    .width = BRICK_WIDTH,
                    .height = BRICK_HEIGHT
                },
                (Vector2) { 0.5f * BRICK_WIDTH, 0.5f * BRICK_HEIGHT },
                RAD2DEG * frGetBodyRotation(body),
                WHITE
            );
        }
        
        frDrawBody(cursor, Fade(RED, 0.5f));
        
        frDrawSpatialHash(frGetWorldSpatialHash(world), 0.25f, GRAY);
        
        const char *message = TextFormat(
            TEXT_STRING_DATA,
            frGetWorldBodyCount(world) - (MAX_WALL_COUNT + 1)
        );
        
        DrawTextEx(
            GetFontDefault(),
            message,
            (Vector2) { 
                0.5f * (SCREEN_WIDTH - MeasureText(message, TEXT_FONT_SIZE)), 
                0.125f * SCREEN_HEIGHT
            },
            TEXT_FONT_SIZE,
            2.0f, 
            Fade(GRAY, 0.85f)
        );
        
        DrawFPS(8, 8);

        EndDrawing();
    }
}