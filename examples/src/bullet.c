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
#include "fr_vec2.h"

#include "raylib.h"

#define TARGET_FPS 60

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

#define SCREEN_WIDTH_IN_METERS  (frNumberPixelsToMeters(SCREEN_WIDTH))
#define SCREEN_HEIGHT_IN_METERS (frNumberPixelsToMeters(SCREEN_HEIGHT))

#define WORLD_RECTANGLE ((Rectangle) {       \
    .y = -0.5f * SCREEN_HEIGHT_IN_METERS,    \
    .width = SCREEN_WIDTH_IN_METERS,         \
    .height = 1.5f * SCREEN_HEIGHT_IN_METERS \
})

#define SEMO_MATERIAL   ((frMaterial) { 2.0f, 0.0f, 0.75f, 0.5f })
#define BULLET_MATERIAL ((frMaterial) { 1.0f, 0.0f, 0.75f, 0.5f })
#define ENEMY_MATERIAL  ((frMaterial) { 1.0f, 0.0f, 0.5f, 0.25f })

#define SEMO_HORIZONTAL_SPEED      0.026f
#define ENEMY_IMPULSE_MULTIPLIER   0.0005f

#define MAX_ENEMY_COUNT 72

static const float DELTA_TIME = (1.0f / TARGET_FPS) * 100.0f;
static const int SEMO_DATA = 0, BULLET_DATA = 1, ENEMY_DATA = 2;

static int enemy_count = 0;

static frVertices semo_vertices, bullet_vertices;

static void DrawCustomCursor(Vector2 position);

static void HandleSemoBullets(frWorld *world, frBody *semo);
static void HandleSemoMovement(frWorld *world, frBody *semo);

static void onCollisionPreSolve(frCollision *collision);

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | bullet.c");
    
    frWorld *world = frCreateWorld(FR_STRUCT_ZERO(Vector2), WORLD_RECTANGLE);

    frSetWorldCollisionHandler(
        world,
        (frCollisionHandler) { 
            .pre_solve = onCollisionPreSolve
        }
    );

    semo_vertices = (frVertices) {
        .data = {
            frVec2PixelsToMeters((Vector2) {  0.0f, -18.0f }),
            frVec2PixelsToMeters((Vector2) { -16.0f, 18.0f }),
            frVec2PixelsToMeters((Vector2) {  16.0f, 18.0f })
        },
        .count = 3
    };

    bullet_vertices = (frVertices) {
        .data = {
            frVec2PixelsToMeters((Vector2) {  0.0f, -6.0f }),
            frVec2PixelsToMeters((Vector2) { -3.0f,  6.0f }),
            frVec2PixelsToMeters((Vector2) {  3.0f,  6.0f })
        },
        .count = 3
    };

    frBody *semo = frCreateBodyFromShape(
        FR_BODY_KINEMATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, 0.85f * SCREEN_HEIGHT }),
        frCreatePolygon(SEMO_MATERIAL, semo_vertices)
    );

    frSetBodyUserData(semo, (void *) &SEMO_DATA);
    
    frAddToWorld(world, semo);

    HideCursor();

    frRaycastHit hits[MAX_ENEMY_COUNT];

    while (!WindowShouldClose()) {
        if (enemy_count < MAX_ENEMY_COUNT) {
            for (int i = 0; i < MAX_ENEMY_COUNT - enemy_count; i++) {
                Vector2 position = FR_STRUCT_ZERO(Vector2);
                
                position.x = GetRandomValue(0.05f * SCREEN_WIDTH, 0.95f * SCREEN_WIDTH);
                position.y = GetRandomValue(-0.25f * SCREEN_HEIGHT, -0.05f * SCREEN_HEIGHT);
                
                frBody *enemy = frCreateBodyFromShape(
                    FR_BODY_DYNAMIC,
                    FR_FLAG_INFINITE_INERTIA,
                    frVec2PixelsToMeters(position),
                    frCreateCircle(ENEMY_MATERIAL, 0.5f * GetRandomValue(3, 5))
                );
                
                frSetBodyUserData(enemy, (void *) &ENEMY_DATA);
                
                frAddToWorld(world, enemy);

                enemy_count++;
            }
        }

        for (int i = 0; i < frGetWorldBodyCount(world); i++) {
            frBody *body = frGetWorldBody(world, i);
            
            if (!frIsInWorldBounds(world, body)) {
                int *user_data = (int *) frGetBodyUserData(body);

                if (*user_data == ENEMY_DATA) 
                    enemy_count--;

                frRemoveFromWorld(world, body);
                frReleaseBody(body);
            }
        }

        frSimulateWorld(world, DELTA_TIME);

        HandleSemoBullets(world, semo);
        HandleSemoMovement(world, semo);

        BeginDrawing();
        
        ClearBackground(RAYWHITE);

        for (int i = 0; i < frGetWorldBodyCount(world); i++) {
            frBody *body = frGetWorldBody(world, i);
            
            int *user_data = (int *) frGetBodyUserData(body);
            
            if (*user_data == SEMO_DATA) {
                frDrawBody(body, DARKGRAY);
            } else if (*user_data == BULLET_DATA) {
                frDrawBody(body, RED);
            } else {
                Vector2 direction = frVec2Normalize(
                    frVec2Subtract(
                        frGetBodyPosition(semo), 
                        frGetBodyPosition(body)
                    )
                );

                frApplyImpulse(body, frVec2ScalarMultiply(direction, ENEMY_IMPULSE_MULTIPLIER));

                frDrawArrow(
                    frGetBodyPosition(body),
                    frVec2Add(
                        frGetBodyPosition(body),
                        frVec2ScalarMultiply(
                            direction,
                            2.0f * frGetCircleRadius(frGetBodyShape(body))
                        )
                    ),
                    2.0f,
                    DARKGRAY
                );

                frDrawBodyLines(body, 2.0f, BLACK);
            }
        }

        DrawCustomCursor(GetMousePosition());
        
        frDrawSpatialHash(frGetWorldSpatialHash(world));
        
        DrawFPS(8, 8);

        EndDrawing();
    }

    frReleaseWorld(world);
    
    CloseWindow();

    return 0;
}

static void DrawCustomCursor(Vector2 position) {
    DrawLineEx(
        frVec2Add(position, (Vector2) { .x = -8.0f }),
        frVec2Add(position, (Vector2) { .x = 8.0f }),
        2,
        LIME
    );
    
    DrawLineEx(
        frVec2Add(position, (Vector2) { .y = -8.0f }),
        frVec2Add(position, (Vector2) { .y = 8.0f }),
        2,
        LIME
    );
}

static void HandleSemoBullets(frWorld *world, frBody *semo) {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 direction = frVec2Subtract(
            frVec2PixelsToMeters(GetMousePosition()), 
            frGetBodyPosition(semo)
        );

        frBody *bullet = frCreateBodyFromShape(
            FR_BODY_DYNAMIC,
            FR_FLAG_INFINITE_INERTIA,
            frGetWorldPoint(semo, semo_vertices.data[0]),
            frCreatePolygon(BULLET_MATERIAL, bullet_vertices)
        );
        
        frSetBodyRotation(bullet, frVec2Angle((Vector2) { .y = -1.0f }, direction));
        frSetBodyUserData(bullet, (void *) &BULLET_DATA);
        
        frApplyImpulse(bullet, frVec2ScalarMultiply(frVec2Normalize(direction), 0.006f));
        
        frAddToWorld(world, bullet);
    }
}

static void HandleSemoMovement(frWorld *world, frBody *semo) {
    Vector2 direction = frVec2Subtract(
        frVec2PixelsToMeters(GetMousePosition()), 
        frGetBodyPosition(semo)
    );

    float rotation = frVec2Angle((Vector2) { .y = -1.0f }, direction);

    Vector2 position = frGetBodyPosition(semo);
    Vector2 velocity = frGetBodyVelocity(semo);

    Rectangle aabb = frGetBodyAABB(semo);

    if (position.x <= 0.5f * aabb.width) 
        position.x = 0.5f * aabb.width;
    else if (position.x >= SCREEN_WIDTH_IN_METERS - 0.5f * aabb.width)
        position.x = SCREEN_WIDTH_IN_METERS - 0.5f * aabb.width;
    
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_D)) {
        if (IsKeyDown(KEY_A)) velocity.x = -SEMO_HORIZONTAL_SPEED;
        else velocity.x = SEMO_HORIZONTAL_SPEED;
    } else {
        velocity.x = 0.0f;
    }
    
    frSetBodyPosition(semo, position);
    frSetBodyRotation(semo, rotation);
    frSetBodyVelocity(semo, velocity);
}

static void onCollisionPreSolve(frCollision *collision) {
    frBody *b1 = collision->cache.bodies[0];
    frBody *b2 = collision->cache.bodies[1];
            
    int *data1 = (int *) frGetBodyUserData(b1);
    int *data2 = (int *) frGetBodyUserData(b2);
    
    if ((*data1 == BULLET_DATA && *data2 == ENEMY_DATA)
       || (*data1 == ENEMY_DATA && *data2 == BULLET_DATA)) {
        frBody *bullet = NULL, *enemy = NULL;

        if (*data1 == BULLET_DATA) {
            bullet = b1;
            enemy = b2;
        } else {
            bullet = b2;
            enemy = b1;
        }

        collision->check = false;
        
        frSetBodyPosition(
            enemy,
            (Vector2) { 
                -SCREEN_WIDTH_IN_METERS, 
                -SCREEN_HEIGHT_IN_METERS 
            }
        );
    }
}