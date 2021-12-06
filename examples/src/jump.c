/*
    Copyright (c) 2021 jdeokkim

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

#define BRICK_MATERIAL  ((frMaterial) { 0.75f, 0.0f, 0.5f, 0.5f })
#define FLOOR_MATERIAL  ((frMaterial) { 1.25f, 0.0f, 0.75f, 0.75f })

#define BRICK_HORIZONTAL_SPEED 0.018f
#define BRICK_VERTICAL_SPEED 0.035f

typedef struct Brick {
    int width;
    int height;
    bool is_jumping;
    bool on_ground;
    frBody *body;
} Brick;

const float DELTA_TIME = (1.0f / TARGET_FPS) * 100;

static void HandleBrickMovement(frWorld *world, Brick *brick);

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | jump.c");
    
    frWorld *world = frCreateWorld(
        frVec2ScalarMultiply(FR_WORLD_DEFAULT_GRAVITY, 0.0001f),
        WORLD_RECTANGLE
    );

    Brick brick = {
        .width = 40,
        .height = 80
    };

    Vector2 floor_vertices[4] = {
        frVec2PixelsToMeters((Vector2) { -0.5f * SCREEN_WIDTH, -60 }),
        frVec2PixelsToMeters((Vector2) { -0.5f * SCREEN_WIDTH, 60 }),
        frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, 60 }),
        frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, -60 })
    };

    Vector2 brick_vertices[4] = {
        frVec2PixelsToMeters((Vector2) { -(brick.width / 2), -(brick.height / 2) }),
        frVec2PixelsToMeters((Vector2) { -(brick.width / 2), (brick.height / 2) }),
        frVec2PixelsToMeters((Vector2) { (brick.width / 2), (brick.height / 2) }),
        frVec2PixelsToMeters((Vector2) { (brick.width / 2), -(brick.height / 2) })
    };

    frBody *floor = frCreateBodyFromShape(
        FR_BODY_STATIC, 
        frVec2PixelsToMeters((Vector2) { SCREEN_WIDTH / 2, SCREEN_HEIGHT - 60 }),
        frCreatePolygon(FLOOR_MATERIAL, floor_vertices, 4)
    );

    brick.body = frCreateBodyFromShape(
        FR_BODY_KINEMATIC,
        frVec2PixelsToMeters((Vector2) { SCREEN_WIDTH / 2, SCREEN_HEIGHT - 300 }),
        frCreatePolygon(BRICK_MATERIAL, brick_vertices, 4)
    );

    frAddToWorld(world, floor);
    frAddToWorld(world, brick.body);

    while (!WindowShouldClose()) {
        BeginDrawing();
        
        ClearBackground(RAYWHITE);

        HandleBrickMovement(world, &brick);

        frDrawBody(floor, BLACK);
        frDrawBody(brick.body, RED);

        frDrawSpatialHash(frGetWorldSpatialHash(world));
        
        frSimulateWorld(world, DELTA_TIME);
        
        Vector2 position = frGetBodyPosition(brick.body);
        Vector2 velocity = frGetBodyVelocity(brick.body);

        /*
        DrawTextEx(
            GetFontDefault(),
            TextFormat(
                "is_jumping: %s\n"
                "on_ground: %s\n"
                "position: (%f, %f)\n"
                "velocity: (%f, %f)",
                brick.is_jumping ? "true" : "false",
                brick.on_ground ? "true" : "false",
                position.x, position.y,
                velocity.x, velocity.y
            ),
            (Vector2) { 8, 8 },
            10,
            2,
            DARKGRAY
        );
        */

        DrawFPS(8, 8);

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

    if (position.x >= SCREEN_WIDTH_IN_METERS - half_brick_width)
        position.x = SCREEN_WIDTH_IN_METERS - half_brick_width;
    
    if (position.y >= SCREEN_HEIGHT_IN_METERS - (half_brick_height + frNumberPixelsToMeters(2 * 60))) {
        position.y = SCREEN_HEIGHT_IN_METERS - (half_brick_height + frNumberPixelsToMeters(2 * 60));
        brick->on_ground = true;
    }

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

    if (!brick->on_ground) {
        velocity = frVec2Add(
            velocity,
            frVec2ScalarMultiply(
                frGetWorldGravity(world), 
                DELTA_TIME
            )
        );
    } else {
        brick->is_jumping = false;
        velocity.y = 0.0f;
    }

    frSetBodyPosition(brick->body, position);
    frSetBodyVelocity(brick->body, velocity);
}