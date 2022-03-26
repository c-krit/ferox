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

#define SCREEN_CENTER ((Vector2) { 0.5f * SCREEN_WIDTH, 0.5f * SCREEN_HEIGHT })

#define SCREEN_WIDTH_IN_METERS  (frNumberPixelsToMeters(SCREEN_WIDTH))
#define SCREEN_HEIGHT_IN_METERS (frNumberPixelsToMeters(SCREEN_HEIGHT))

#define WORLD_RECTANGLE ((Rectangle) { \
    .width = SCREEN_WIDTH_IN_METERS,   \
    .height = SCREEN_HEIGHT_IN_METERS  \
})

#define BALL_MATERIAL ((frMaterial) { 0.75f, 0.0f, 0.85f, 0.75f })
#define BOX_MATERIAL  ((frMaterial) { 1.75f, 0.0f, 0.75f, 0.5f })

#define BALL_DEFAULT_RADIUS    4.0f

#define TOTAL_WIDTH_IN_BOXES   8
#define TOTAL_HEIGHT_IN_BOXES  8

#define MAX_BOX_COUNT (TOTAL_WIDTH_IN_BOXES * TOTAL_HEIGHT_IN_BOXES)

const Color BACKGROUND_COLOR = { 220, 220, 220, 255 };

const float DELTA_TIME = (1.0f / TARGET_FPS) * 100.0f;

typedef struct Box {
    frBody *body;
    RenderTexture rtx;
} Box;

static Texture2D rl_logo;

static frWorld *world;

static frBody *ball;

static Box boxes[MAX_BOX_COUNT];

static float box_width, box_height;
static float half_box_width, half_box_height;

void InitExample(void);
void DeinitExample(void);
void UpdateExample(void);

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | smash.c");

    InitExample();

    while (!WindowShouldClose())
        UpdateExample();

    DeinitExample();
    
    CloseWindow();

    return 0;
}

void InitExample(void) {
    rl_logo = LoadTexture("../res/images/raylib.png");
    
    world = frCreateWorld(FR_STRUCT_ZERO(Vector2), WORLD_RECTANGLE);

    box_width = rl_logo.width / TOTAL_WIDTH_IN_BOXES;
    box_height = rl_logo.height / TOTAL_HEIGHT_IN_BOXES;

    half_box_width = 0.5f * box_width;
    half_box_height = 0.5f * box_height;

    for (int i = 0; i < MAX_BOX_COUNT; i++) {
        Vector2 offset = {
            (i % TOTAL_WIDTH_IN_BOXES) * box_width,
            (i / TOTAL_WIDTH_IN_BOXES) * box_height
        };

        Vector2 position = {
            0.5f * (SCREEN_WIDTH - rl_logo.width) + offset.x + half_box_width,
            0.5f * (SCREEN_HEIGHT - rl_logo.height) + offset.y + half_box_height
        };

        boxes[i].body = frCreateBodyFromShape(
            FR_BODY_DYNAMIC,
            FR_FLAG_NONE,
            frVec2PixelsToMeters(position),
            frCreateRectangle(
                BOX_MATERIAL,
                frNumberPixelsToMeters(box_width),
                frNumberPixelsToMeters(box_height)
            )
        );

        boxes[i].rtx = LoadRenderTexture(box_width, box_height);

        {
            BeginTextureMode(boxes[i].rtx);

            ClearBackground(BLACK);

            DrawTextureRec(
                rl_logo,
                (Rectangle) {
                    .x = offset.x,
                    .y = offset.y,
                    .width = box_width, 
                    .height = -box_height
                },
                FR_STRUCT_ZERO(Vector2),
                WHITE
            );

            EndTextureMode();
        }

        frAddToWorld(world, boxes[i].body);
    }

    ball = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        FR_FLAG_INFINITE_INERTIA,
        (Vector2) { -BALL_DEFAULT_RADIUS, -BALL_DEFAULT_RADIUS },
        frCreateCircle(BALL_MATERIAL, BALL_DEFAULT_RADIUS)
    );

    frAddToWorld(world, ball);
}

void DeinitExample(void) {
    for (int i = 0; i < MAX_BOX_COUNT; i++)
        UnloadRenderTexture(boxes[i].rtx);

    UnloadTexture(rl_logo);
    
    frReleaseWorld(world);
}

void UpdateExample(void) {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 position = frVec2PixelsToMeters(GetMousePosition());

        Vector2 impulse = frVec2ScalarMultiply(
            frVec2Subtract(
                frVec2PixelsToMeters(SCREEN_CENTER), 
                position
            ),
            0.15f
        );

        frSetBodyVelocity(ball, FR_STRUCT_ZERO(Vector2));
        frSetBodyPosition(ball, position);
        
        frApplyImpulse(ball, impulse);
    }

    for (int i = 0; i < MAX_BOX_COUNT; i++) {
        if (!frIsInWorldBounds(world, boxes[i].body)) {
            frRemoveFromWorld(world, boxes[i].body);
            frReleaseBody(boxes[i].body);

            boxes[i].body = NULL;
        }
    }

    frSimulateWorld(world, DELTA_TIME);

    {
        BeginDrawing();
        
        ClearBackground(BACKGROUND_COLOR);

        for (int i = 0; i < MAX_BOX_COUNT; i++) {
            if (boxes[i].body == NULL) continue;

            Vector2 position = frGetBodyPosition(boxes[i].body);

            DrawTexturePro(
                boxes[i].rtx.texture,
                (Rectangle) {
                    .width = box_width,
                    .height = box_height
                },
                (Rectangle) {
                    .x = frNumberMetersToPixels(position.x), 
                    .y = frNumberMetersToPixels(position.y),
                    .width = box_width,
                    .height = box_height
                },
                (Vector2) { half_box_width, half_box_height },
                RAD2DEG * frGetBodyRotation(boxes[i].body),
                WHITE
            );
        }

        frDrawBodyLines(ball, 2.0f, RED);

        frDrawArrow(
            frVec2PixelsToMeters(GetMousePosition()),
            frVec2PixelsToMeters(SCREEN_CENTER),
            2.0f,
            Fade(RED, 0.75f)
        );

        DrawRing(
            GetMousePosition(),
            frNumberMetersToPixels(BALL_DEFAULT_RADIUS) - 2.0f,
            frNumberMetersToPixels(BALL_DEFAULT_RADIUS),
            0.0f,
            360.0f,
            32,
            Fade(RED, 0.75f)
        );

        frDrawSpatialHash(frGetWorldSpatialHash(world), 0.25f, GRAY);
        
        DrawFPS(8, 8);

        EndDrawing();
    }
}