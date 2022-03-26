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

#define TEXT_FONT_SIZE    30
#define TEXT_STRING_DATA  "PRESS SPACE TO JUMP!"

#define BOX_MATERIAL       ((frMaterial) { 2.25f, 0.0f, 1.25f, 1.0f })
#define BRICK_MATERIAL     ((frMaterial) { 1.85f, 0.0f, 1.0f, 0.85f })
#define PLATFORM_MATERIAL  ((frMaterial) { 2.0f, 0.0f, 1.25f, 1.0f })
#define WALL_MATERIAL      ((frMaterial) { 2.5f, 0.0f, 1.25f, 1.0f })

#define MAX_WALL_COUNT          3

#define BRICK_HORIZONTAL_SPEED  0.016f
#define BRICK_VERTICAL_SPEED    0.02f

typedef struct Brick {
    float width;
    float height;
    bool is_jumping;
    bool on_ground;
    frBody *body;
} Brick;

const float DELTA_TIME = (1.0f / TARGET_FPS) * 100.0f;

static frWorld *world;

static frBody *walls[MAX_WALL_COUNT];
static frBody *platform, *box;

static Brick brick = { .width = 40.0f, .height = 80.0f };

void InitExample(void);
void DeinitExample(void);
void UpdateExample(void);

void HandleBrickMovement(frWorld *world, Brick *brick);
void onCollisionPreSolve(frCollision *collision);

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | jump.c");
    
    InitExample();

    while (!WindowShouldClose())
        UpdateExample();

    DeinitExample();
    
    CloseWindow();

    return 0;
}

void InitExample(void) {
    world = frCreateWorld(
        frVec2ScalarMultiply(FR_WORLD_DEFAULT_GRAVITY, 0.00001f),
        WORLD_RECTANGLE
    );

    frSetWorldCollisionHandler(
        world,
        (frCollisionHandler) {
            .pre_solve = onCollisionPreSolve
        }
    );

    frVertices wall1_vertices = {
        .data = {
            frVec2PixelsToMeters((Vector2) { -0.1f * SCREEN_WIDTH, -0.35f * SCREEN_HEIGHT }),
            frVec2PixelsToMeters((Vector2) { -0.1f * SCREEN_WIDTH,  0.35f * SCREEN_HEIGHT }),
            frVec2PixelsToMeters((Vector2) {  0.1f * SCREEN_WIDTH,  0.35f * SCREEN_HEIGHT })
        },
        .count = 3
    };

    frVertices wall3_vertices = {
        .data = {
            frVec2PixelsToMeters((Vector2) {  0.1f * SCREEN_WIDTH, -0.35f * SCREEN_HEIGHT }),
            frVec2PixelsToMeters((Vector2) { -0.1f * SCREEN_WIDTH,  0.35f * SCREEN_HEIGHT }),
            frVec2PixelsToMeters((Vector2) {  0.1f * SCREEN_WIDTH,  0.35f * SCREEN_HEIGHT })
        },
        .count = 3
    };

    brick.body = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        FR_FLAG_INFINITE_INERTIA,
        frVec2PixelsToMeters((Vector2) { 0.35f * SCREEN_WIDTH, SCREEN_HEIGHT / 3 }),
        frCreateRectangle(
            BRICK_MATERIAL, 
            frNumberPixelsToMeters(brick.width), 
            frNumberPixelsToMeters(brick.height)
        )
    );

    frSetBodyUserData(brick.body, (void *) &brick);

    frAddToWorld(world, brick.body);

    walls[0] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.1f * SCREEN_WIDTH, 0.65f * SCREEN_HEIGHT }),
        frCreatePolygon(WALL_MATERIAL, wall1_vertices)
    );

    walls[1] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, SCREEN_HEIGHT - 60.0f }),
        frCreateRectangle(
            WALL_MATERIAL, 
            frNumberPixelsToMeters(SCREEN_WIDTH),
            frNumberPixelsToMeters(120.0f)
        )
    );

    walls[2] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.9f * SCREEN_WIDTH, 0.65f * SCREEN_HEIGHT }),
        frCreatePolygon(WALL_MATERIAL, wall3_vertices)
    );

    for (int i = 0; i < MAX_WALL_COUNT; i++)
        frAddToWorld(world, walls[i]);

    platform = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, 0.75f * SCREEN_HEIGHT }),
        frCreateRectangle(
            PLATFORM_MATERIAL, 
            frNumberPixelsToMeters(0.5f * SCREEN_WIDTH), 
            frNumberPixelsToMeters(40.0f)
        )
    );

    frAddToWorld(world, platform);

    box = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, 0.25f * SCREEN_HEIGHT}),
        frCreateRectangle(
            BOX_MATERIAL, 
            frNumberPixelsToMeters(50.0f),
            frNumberPixelsToMeters(40.0f)
        )
    );

    frAddToWorld(world, box);
}

void DeinitExample(void) {
    frReleaseWorld(world);
}

void UpdateExample(void) {
    frSimulateWorld(world, DELTA_TIME);

    HandleBrickMovement(world, &brick);

    {
        BeginDrawing();

        ClearBackground(RAYWHITE);

        for (int i = 0; i < MAX_WALL_COUNT; i++)
            frDrawBody(walls[i], BLACK);

        frDrawBody(platform, BLACK);

        frDrawBody(box, DARKGRAY);
        frDrawBody(brick.body, RED);

        frDrawSpatialHash(frGetWorldSpatialHash(world), 0.25f, GRAY);

        Vector2 position = frGetBodyPosition(brick.body);
        Vector2 velocity = frGetBodyVelocity(brick.body);

        DrawTextEx(
            GetFontDefault(),
            TEXT_STRING_DATA,
            (Vector2) { 
                0.5f * (SCREEN_WIDTH - MeasureText(TEXT_STRING_DATA, TEXT_FONT_SIZE)),
                0.125f * SCREEN_HEIGHT
            },
            (float) TEXT_FONT_SIZE,
            2.0f, 
            Fade(GRAY, 0.85f)
        );

        DrawFPS(8, 8);

        EndDrawing();
    }
}

void HandleBrickMovement(frWorld *world, Brick *brick) {
    Vector2 position = frGetBodyPosition(brick->body);
    Vector2 velocity = frGetBodyVelocity(brick->body);

    const float half_brick_width = 0.5f * frNumberPixelsToMeters(brick->width);
    const float half_brick_height = 0.5f * frNumberPixelsToMeters(brick->height);

    if (position.x <= half_brick_width) 
        position.x = half_brick_width;
    else if (position.x >= SCREEN_WIDTH_IN_METERS - half_brick_width)
        position.x = SCREEN_WIDTH_IN_METERS - half_brick_width;

    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT)) {
        if (IsKeyDown(KEY_LEFT)) velocity.x = -BRICK_HORIZONTAL_SPEED;
        else velocity.x = BRICK_HORIZONTAL_SPEED;
    } else {
        velocity.x = 0.0f;
    }

    if (IsKeyDown(KEY_SPACE) && !brick->is_jumping) {
        brick->is_jumping = true;
        brick->on_ground = false;

        velocity.y = -BRICK_VERTICAL_SPEED;
    }

    if (brick->on_ground) brick->is_jumping = false;

    frSetBodyPosition(brick->body, position);
    frSetBodyVelocity(brick->body, velocity);
}

void onCollisionPreSolve(frCollision *collision) {
    frBody *b1 = collision->cache.bodies[0];
    frBody *b2 = collision->cache.bodies[1];

    Brick *data1 = frGetBodyUserData(b1);
    Brick *data2 = frGetBodyUserData(b2);

    frBody *brick_b = NULL;

    if (data1 != NULL && data2 == NULL) brick_b = b1;
    else if (data1 == NULL && data2 != NULL) brick_b = b2;

    if (brick_b == NULL) return;

    Brick *brick = frGetBodyUserData(brick_b);

    if (collision->check && collision->direction.y > 0.0f) brick->on_ground = true;
    else brick->on_ground = false;
}