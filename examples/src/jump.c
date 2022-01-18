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

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define SCREEN_WIDTH_IN_METERS  (frNumberPixelsToMeters(SCREEN_WIDTH))
#define SCREEN_HEIGHT_IN_METERS (frNumberPixelsToMeters(SCREEN_HEIGHT))

#define WORLD_RECTANGLE ((Rectangle) { \
    .width = SCREEN_WIDTH_IN_METERS,   \
    .height = SCREEN_HEIGHT_IN_METERS  \
})

#define EXAMPLE_STRING "PRESS SPACE TO JUMP!"

#define BOX_MATERIAL       ((frMaterial) { 2.25f, 0.0f, 1.25f, 1.0f })
#define BRICK_MATERIAL     ((frMaterial) { 1.85f, 0.0f, 1.0f, 0.85f })
#define PLATFORM_MATERIAL  ((frMaterial) { 2.0f, 0.0f, 1.25f, 1.0f })
#define WALL_MATERIAL      ((frMaterial) { 2.5f, 0.0f, 1.25f, 1.0f })

#define BRICK_HORIZONTAL_SPEED  0.016f
#define BRICK_VERTICAL_SPEED    0.02f

typedef struct Brick {
    int width;
    int height;
    bool is_jumping;
    bool on_ground;
    frBody *body;
} Brick;

const float DELTA_TIME = (1.0f / TARGET_FPS) * 100.0f;

static Brick brick = { .width = 40, .height = 80 };

static void HandleBrickMovement(frWorld *world, Brick *brick);

static void onCollisionPreSolve(frCollision *collision);

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | jump.c");
    
    frWorld *world = frCreateWorld(
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

    frBody *wall1 = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.1f * SCREEN_WIDTH, 0.65f * SCREEN_HEIGHT }),
        frCreatePolygon(WALL_MATERIAL, wall1_vertices)
    );

    frBody *wall2 = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, SCREEN_HEIGHT - 60 }),
        frCreateRectangle(
            WALL_MATERIAL, 
            frNumberPixelsToMeters(SCREEN_WIDTH),
            frNumberPixelsToMeters(120.0f)
        )
    );

    frBody *wall3 = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.9f * SCREEN_WIDTH, 0.65f * SCREEN_HEIGHT }),
        frCreatePolygon(WALL_MATERIAL, wall3_vertices)
    );

    frBody *platform = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, 0.75f * SCREEN_HEIGHT }),
        frCreateRectangle(
            PLATFORM_MATERIAL, 
            frNumberPixelsToMeters(0.5f * SCREEN_WIDTH), 
            frNumberPixelsToMeters(40.0f)
        )
    );

    frBody *box = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, 0.25f * SCREEN_HEIGHT}),
        frCreateRectangle(
            BOX_MATERIAL, 
            frNumberPixelsToMeters(50.0f),
            frNumberPixelsToMeters(40.0f)
        )
    );

    frAddToWorld(world, brick.body);

    frAddToWorld(world, wall1);
    frAddToWorld(world, wall2);
    frAddToWorld(world, wall3);

    frAddToWorld(world, platform);

    frAddToWorld(world, box);

    while (!WindowShouldClose()) {
        HandleBrickMovement(world, &brick);

        frSimulateWorld(world, DELTA_TIME);

        BeginDrawing();
        
        ClearBackground(RAYWHITE);

        frDrawBody(wall1, BLACK);
        frDrawBody(wall2, BLACK);
        frDrawBody(wall3, BLACK);
        frDrawBody(platform, BLACK);

        frDrawBody(box, DARKGRAY);
        frDrawBody(brick.body, RED);

        frDrawSpatialHash(frGetWorldSpatialHash(world));
        
        Vector2 position = frGetBodyPosition(brick.body);
        Vector2 velocity = frGetBodyVelocity(brick.body);

        DrawTextEx(
            GetFontDefault(),
            EXAMPLE_STRING,
            (Vector2) { 
                (SCREEN_WIDTH - MeasureText(EXAMPLE_STRING, 20)) / 2, 
                SCREEN_HEIGHT / 16
            },
            20,
            2, 
            Fade(GRAY, 0.85f)
        );

        DrawFPS(8, 8);

        EndDrawing();
    }

    frReleaseWorld(world);
    
    CloseWindow();

    return 0;
}

static void HandleBrickMovement(frWorld *world, Brick *brick) {
    Vector2 position = frGetBodyPosition(brick->body);
    Vector2 velocity = frGetBodyVelocity(brick->body);

    float half_brick_width = frNumberPixelsToMeters(brick->width) / 2;
    float half_brick_height = frNumberPixelsToMeters(brick->height) / 2;

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

static void onCollisionPreSolve(frCollision *collision) {
    frBody *b1 = collision->_bodies[0];
    frBody *b2 = collision->_bodies[1];

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