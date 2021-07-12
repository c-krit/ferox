/*
    Copyright (c) 2021 jdeokkim

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

#include "ferox.h"

/* | `dynamics` 모듈 구조체... | */

/* 강체의 상태를 나타내는 구조체. */
typedef enum frBodyState {
    FR_STATE_UNKNOWN = -1,
    FR_STATE_AWAKE,
    FR_STATE_SLEEPING
} frBodyState;

/* 강체의 물리량을 나타내는 구조체. */
typedef struct frMotionData {
    float mass;
    float inverse_mass;
    float inertia;
    float inverse_inertia;
    Vector2 velocity;
    float angular_velocity;
    float gravity_scale;
    Vector2 force;
    float torque;
} frMotionData;

/* 강체를 나타내는 구조체. */
typedef struct frBody {
    frBodyType type;
    // frBodyState state;
    frMaterial material;
    frMotionData motion;
    frTransform tx;
    frShape *shape;
    Rectangle aabb;
} frBody;

/* | `dynamics` 모듈 함수... | */

/* 강체 `b`의 질량을 다시 계산한다. */
static void frResetBodyMass(frBody *b);

/* 종류가 `type`이고 위치가 `p`인 강체를 생성한다. */
frBody *frCreateBody(frBodyType type, Vector2 p) {
    if (type == FR_BODY_UNKNOWN) return NULL;
    
    frBody *result = calloc(1, sizeof(frBody));
    
    result->material = FR_DYNAMICS_DEFAULT_MATERIAL;
    
    frSetBodyType(result, type);
    frSetBodyGravityScale(result, 1.0f);
    frSetBodyPosition(result, p);
    
    return result;
}

/* 종류가 `type`이고 위치가 `p`이며 충돌 처리용 도형이 `shape`인 강체를 생성한다. */
frBody *frCreateBodyFromShape(frBodyType type, Vector2 p, frShape *s) {
    if (type == FR_BODY_UNKNOWN) return NULL;
    
    frBody *result = frCreateBody(type, p);
    frAttachShapeToBody(result, s);
    
    return result;
}

/* 강체 `b`에 할당된 메모리를 해제한다. */
void frReleaseBody(frBody *b) {
    if (b->shape != NULL) frDetachShapeFromBody(b);
    
    free(b);
}

/* 강체 `b에 충돌 처리용 도형 `s`를 추가한다. */
void frAttachShapeToBody(frBody *b, frShape *s) {
    if (b == NULL || s == NULL) return;
    
    b->shape = s;
    b->material = frGetShapeMaterial(s);
    b->aabb = frGetShapeAABB(b->shape, b->tx);
    
    frResetBodyMass(b);
}

/* 강체 `b`에서 충돌 처리용 도형을 제거한다. */ 
void frDetachShapeFromBody(frBody *b) {
    if (b == NULL) return;
    
    b->shape = NULL;
    b->material = FR_DYNAMICS_DEFAULT_MATERIAL;
    b->aabb = FR_STRUCT_ZERO(Rectangle);
    
    frResetBodyMass(b);
}

/* 강체 `b`의 종류를 반환한다. */
frBodyType frGetBodyType(frBody *b) {
    return (b != NULL) ? b->type : FR_BODY_UNKNOWN;
}

/* 강체 `b`의 질량을 반환한다. */
float frGetBodyMass(frBody *b) {
    return (b != NULL) ? b->motion.mass : 0.0f;
}

/* 강체 `b`의 질량의 역수를 반환한다. */
float frGetBodyInverseMass(frBody *b) {
    return (b != NULL) ? b->motion.inverse_mass : 0.0f;
}

/* 강체 `b`의 관성 모멘트를 반환한다. */
float frGetBodyInertia(frBody *b) {
    return (b != NULL) ? b->motion.inertia : 0.0f;
}

/* 강체 `b`의 관성 모멘트의 역수를 반환한다. */
float frGetBodyInverseInertia(frBody *b) {
    return (b != NULL) ? b->motion.inverse_inertia : 0.0f;
}

/* 강체 `b`의 속도를 반환한다. */
Vector2 frGetBodyVelocity(frBody *b) {
    return (b != NULL) ? b->motion.velocity : FR_STRUCT_ZERO(Vector2);
}

/* 강체 `b`의 각속도를 반환한다. */
float frGetBodyAngularVelocity(frBody *b) {
    return (b != NULL) ? b->motion.angular_velocity : 0.0f;
}

/* 강체 `b`의 중력 가속률을 반환한다. */
float frGetBodyGravityScale(frBody *b) {
    return (b != NULL) ? b->motion.gravity_scale : 0.0f;
}

/* 강체 `b`의 위치와 회전 각도 (단위: rad.)를 반환한다. */
frTransform frGetBodyTransform(frBody *b) {
    return (b != NULL) ? b->tx : FR_STRUCT_ZERO(frTransform);
}

/* 강체 `b`의 위치를 반환한다. */
Vector2 frGetBodyPosition(frBody *b) {
    return (b != NULL) ? b->tx.position : FR_STRUCT_ZERO(Vector2);
}

/* 강체 `b`의 회전 각도 (단위: rad.)를 반환한다. */
float frGetBodyRotation(frBody *b) {
    return (b != NULL) ? b->tx.rotation : 0.0f;
}

/* 강체 `b`의 충돌 처리용 도형을 반환한다. */
frShape *frGetBodyShape(frBody *b) {
    return (b != NULL) ? b->shape : NULL;
}

/* 강체 `b`의 AABB를 반환한다. */
Rectangle frGetBodyAABB(frBody *b) {
    return (b != NULL && b->shape != NULL) ? b->aabb : FR_STRUCT_ZERO(Rectangle);
}

/* 세계 기준 좌표 `p`를 강체 `b`를 기준으로 한 좌표로 변환한다. */
Vector2 frGetLocalPoint(frBody *b, Vector2 p) {
    return frVec2Transform(p, (frTransform) { frVec2Negate(b->tx.position), -(b->tx.rotation)});
}

/* 강체 `b`를 기준으로 한 좌표 `p`를 세계 기준 좌표로 변환한다. */
Vector2 frGetWorldPoint(frBody *b, Vector2 p) {
    return frVec2Transform(p, b->tx);
}

/* 강체 `b`의 중력 가속률을 `gravity_scale`로 설정한다. */
void frSetBodyGravityScale(frBody *b, float gravity_scale) {
    if (b == NULL || b->type == FR_BODY_STATIC || b->type == FR_BODY_KINEMATIC) return;
    
    b->motion.gravity_scale = gravity_scale;
}

/* 강체 `b`의 위치를 `p`로 설정한다. */
void frSetBodyPosition(frBody *b, Vector2 p) {
    if (b == NULL) return;
    
    b->tx.position = p;
    b->aabb = frGetShapeAABB(b->shape, b->tx);
}

/* 강체 `b`의 회전 각도 (단위: rad.)를 `rotation`으로 설정한다. */
void frSetBodyRotation(frBody *b, float rotation) {
    if (b == NULL) return;
    
    b->tx.rotation = frNormalizeAngle(rotation, PI);
    b->aabb = frGetShapeAABB(b->shape, b->tx);
}

/* 강체 `b`의 위치와 회전 각도를 `tx`의 값으로 설정한다. */ 
void frSetBodyTransform(frBody *b, frTransform tx) {
    if (b == NULL) return;
    
    b->tx.position = tx.position;
    b->tx.rotation = frNormalizeAngle(tx.rotation, PI);
    
    b->aabb = frGetShapeAABB(b->shape, b->tx);
}

/* 강체 `b`의 종류를 `type`으로 설정한다. */
void frSetBodyType(frBody *b, frBodyType type) {
    if (b == NULL || b->type == FR_BODY_UNKNOWN || b->type == type) 
        return;
    
    b->type = type;
    
    frResetBodyMass(b);
}

/* 강체 `b`의 속도를 `v`로 설정한다. */
void frSetBodyVelocity(frBody *b, Vector2 v) {
    if (b == NULL || b->motion.inverse_mass > 0.0f) return;
    
    b->motion.velocity = v;
}

/* 강체 `b`의 각속도를 `a`로 설정한다. */
void frSetBodyAngularVelocity(frBody *b, double a) {
    if (b == NULL || b->motion.inverse_mass > 0.0f) return;
    
    b->motion.angular_velocity = a;
}

/* 강체 `b`에 중력 가속도 `gravity`를 적용한다. */
void frApplyGravity(frBody *b, Vector2 gravity) {
    if (b == NULL || b->motion.inverse_mass <= 0.0f) return;
    
    b->motion.force = frVec2Add(
        b->motion.force,
        frVec2ScalarMultiply(
            frVec2ScalarMultiply(
                gravity,
                b->motion.gravity_scale
            ),
            b->motion.mass
        )
    );
}

/* 강체 `b`에 충격량 `impulse`를 적용한다. */
void frApplyImpulse(frBody *b, Vector2 impulse) {
    if (b == NULL || b->motion.inverse_mass <= 0.0f) return;
    
    b->motion.velocity = frVec2Add(
        b->motion.velocity,
        frVec2ScalarMultiply(
            impulse,
            b->motion.inverse_mass
        )
    );
}

/* 강체 `b` 위의 점 `point`에 각운동량 `impulse`를 적용한다. */
void frApplyTorqueImpulse(frBody *b, Vector2 point, Vector2 impulse) {
    if (b == NULL || b->motion.inverse_inertia <= 0.0f) return;
    
    b->motion.angular_velocity += b->motion.inverse_inertia 
        * frVec2CrossProduct(point, impulse);
}

/* 강체 `b`에 작용하는 모든 힘을 제거한다. */
void frClearBodyForces(frBody *b) {
    if (b == NULL) return;
    
    b->motion.force = FR_STRUCT_ZERO(Vector2);
    b->motion.torque = 0.0f;
}

/* 강체 `b1`과 `b2`의 위치를 적절하게 보정한다. */
void frCorrectBodyPositions(frBody *b1, frBody *b2, frCollision collision) {
    if (b1 == NULL || b2 == NULL) return;
    
    // 두 강체의 질량이 0에 수렴하면 위치를 보정하지 않는다.
    if (b1->motion.inverse_mass + b2->motion.inverse_mass <= 0.0f) {
        b1->motion.velocity = FR_STRUCT_ZERO(Vector2);
        b2->motion.velocity = FR_STRUCT_ZERO(Vector2);
        
        return;
    }
    
    float max_depth = _FR_MAX(collision.depths[0], collision.depths[1]);
    
    // 충돌 방향은 무조건 `b1`에서 `b2`로 향한다.
    Vector2 correction = frVec2ScalarMultiply(
        collision.direction,
        FR_DYNAMICS_CORRECTION_DEPTH_SCALE * (
            _FR_MAX(0.0f, max_depth - FR_DYNAMICS_CORRECTION_DEPTH_THRESHOLD) 
            / (b1->motion.inverse_mass + b2->motion.inverse_mass)
        )
    );
    
    frSetBodyPosition(
        b1, 
        frVec2Subtract(
            b1->tx.position, 
            frVec2ScalarMultiply(correction, b1->motion.inverse_mass)
        )
    );
    frSetBodyPosition(
        b2, 
        frVec2Add(
            b2->tx.position, 
            frVec2ScalarMultiply(correction, b2->motion.inverse_mass)
        )
    );
}

/* 단위 시간 `dt` 이후의 강체 `b`의 위치를 계산한다. */
void frIntegrateForBodyPosition(frBody *b, double dt) {
    if (b == NULL || b->type == FR_BODY_STATIC) return;
    
    frSetBodyPosition(b, frVec2Add(b->tx.position, frVec2ScalarMultiply(b->motion.velocity, dt)));
    frSetBodyRotation(b, b->tx.rotation + (b->motion.angular_velocity * dt));
}

/* 단위 시간 `dt` 이후의 강체 `b`의 속도와 각속도를 계산한다. */
void frIntegrateForBodyVelocities(frBody *b, double dt) {
    if (b == NULL || b->motion.inverse_mass <= 0.0f) return;
    
    b->motion.velocity = frVec2Add(
        b->motion.velocity,
        frVec2ScalarMultiply(
            frVec2ScalarMultiply(b->motion.force, b->motion.inverse_mass),
            (dt / 2.0f)
        )
    );
    
    b->motion.angular_velocity += (b->motion.torque * b->motion.inverse_inertia) * (dt / 2.0f);
}

/* 강체 `b1`과 `b2` 사이의 충돌을 해결한다. */
void frResolveCollision(frBody *b1, frBody *b2, frCollision collision) {
    if (b1 == NULL || b2 == NULL || !collision.check) return;
    
    for (int i = 0; i < collision.count; i++) {
        Vector2 r1 = frVec2Subtract(collision.points[i], frGetBodyPosition(b1));
        Vector2 r2 = frVec2Subtract(collision.points[i], frGetBodyPosition(b2));
        
        Vector2 r1_normal = frVec2LeftNormal(r1);
        Vector2 r2_normal = frVec2LeftNormal(r2);
        
        // `b1`이 측정한 `b2`의 속도 (`b1`에 대한 `b2`의 상대 속도)를 계산한다.
        Vector2 relative_velocity = frVec2Subtract(
            frVec2Add(
                b2->motion.velocity,
                frVec2ScalarMultiply(r2_normal, b2->motion.angular_velocity)
            ),
            frVec2Add(
                b1->motion.velocity, 
                frVec2ScalarMultiply(r1_normal, b1->motion.angular_velocity)
            )
        );
        
        float relative_normal_velocity = frVec2DotProduct(relative_velocity, collision.direction);
        
        // 두 강체가 서로 충돌하는 방향으로 진행하고 있지 않으면 계산을 종료한다.
        if (relative_normal_velocity > 0.0f) return;
        
        float r1_normal_dot = frVec2DotProduct(r1_normal, collision.direction);
        float r2_normal_dot = frVec2DotProduct(r2_normal, collision.direction);
        
        float epsilon = _FR_MAX(0.0f, _FR_MIN(b1->material.restitution, b2->material.restitution));
        
        float inverse_mass_sum = (b1->motion.inverse_mass + b2->motion.inverse_mass)
            + b1->motion.inverse_inertia * (r1_normal_dot * r1_normal_dot)
            + b2->motion.inverse_inertia * (r2_normal_dot * r2_normal_dot);
            
        float impulse_magnitude = (-(1.0f + epsilon) * relative_normal_velocity) 
            / (collision.count * inverse_mass_sum);

        Vector2 impulse = frVec2ScalarMultiply(collision.direction, impulse_magnitude);

        frApplyImpulse(b1, frVec2Negate(impulse));
        frApplyTorqueImpulse(b1, r1, frVec2Negate(impulse));
        
        frApplyImpulse(b2, impulse);
        frApplyTorqueImpulse(b2, r2, impulse);
        
        // 마찰력 적용을 위해 상대 속도를 다시 계산한다.
        relative_velocity = frVec2Subtract(
            frVec2Add(
                b2->motion.velocity,
                frVec2ScalarMultiply(r2_normal, b2->motion.angular_velocity)
            ),
            frVec2Add(
                b1->motion.velocity, 
                frVec2ScalarMultiply(r1_normal, b1->motion.angular_velocity)
            )
        );
        
        relative_normal_velocity = frVec2DotProduct(relative_velocity, collision.direction);
        
        Vector2 tangent = frVec2Normalize(
            frVec2Subtract(
                relative_velocity,
                frVec2ScalarMultiply(collision.direction, relative_normal_velocity)
            )
        );
        
        float friction_magnitude = -frVec2DotProduct(relative_velocity, tangent)
            / (collision.count * inverse_mass_sum);
        
        float static_coefficient = b1->material.static_friction * b2->material.static_friction;
        float dynamic_coefficient = b1->material.dynamic_friction * b2->material.dynamic_friction;
        
        Vector2 frictional_impulse = (fabs(friction_magnitude) < impulse_magnitude * static_coefficient)
            ? frVec2ScalarMultiply(tangent, friction_magnitude)
            : frVec2ScalarMultiply(tangent, -impulse_magnitude * dynamic_coefficient);
        
        frApplyImpulse(b1, frVec2Negate(frictional_impulse));
        frApplyTorqueImpulse(b1, r1, frVec2Negate(frictional_impulse));
        
        frApplyImpulse(b2, frictional_impulse);
        frApplyTorqueImpulse(b2, r2, frictional_impulse);
    }
}

/* 강체 `b`의 질량을 다시 계산한다. */
static void frResetBodyMass(frBody *b) {
    if (b == NULL) return;
    
    b->motion.mass = 0.0f;
    b->motion.inertia = 0.0f;
    
    if (b->type == FR_BODY_STATIC) {
        b->motion.velocity = FR_STRUCT_ZERO(Vector2);
        b->motion.angular_velocity = 0.0f;
    } else if (b->type == FR_BODY_DYNAMIC) {
        if (b->shape != NULL) {
            b->motion.mass = frGetShapeMass(b->shape);
            b->motion.inertia = frGetShapeInertia(b->shape);
        }
    }
    
    if (b->motion.mass == 0.0f) b->motion.inverse_mass = 0.0f;
    else b->motion.inverse_mass = 1.0f / b->motion.mass;
    
    if (b->motion.inertia == 0.0f) b->motion.inverse_inertia = 0.0f;
    else b->motion.inverse_inertia = 1.0f / b->motion.inertia;
}