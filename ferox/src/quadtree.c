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

/* | `quadtree` 모듈 구조체... | */

/* 쿼드 트리의 값을 나타내는 구조체. */
typedef struct frQuadtreeValue {
    int index;
    Rectangle aabb;
} frQuadtreeValue;

/* 쿼드 트리를 나타내는 구조체. */
typedef struct frQuadtree {
    int depth;
    Rectangle bounds;
    frQuadtreeValue *values;
    frQuadtreeValue *temp_values;
    struct frQuadtree *children[4];
} frQuadtree;

/* | `quadtree` 모듈 함수... | */

/* 경계 범위가 `bounds`인 쿼드 트리 구조체의 메모리 주소를 반환한다. */
frQuadtree *frCreateQuadtree(int depth, Rectangle bounds) {
    frQuadtree *result = calloc(1, sizeof(frQuadtree));
    
    result->depth = depth;
    result->bounds = bounds;
    
    frCreateArray(result->values, FR_QUADTREE_MAX_LEAF_COUNT);
    frCreateArray(result->temp_values, FR_QUADTREE_MAX_LEAF_COUNT);
    
    return result;
}

/* 쿼드 트리 `tree`에 할당된 메모리를 해제한다. */
void frReleaseQuadtree(frQuadtree *tree) {
    if (tree == NULL) return;
    
    frClearQuadtree(tree);
    
    frReleaseArray(tree->values);
    frReleaseArray(tree->temp_values);
    
    free(tree);
}

/* 쿼드 트리 `tree`에 새로운 값을 추가한다. */
void frAddToQuadtree(frQuadtree *tree, int index, Rectangle aabb) {
    if (tree == NULL) return;
    
    frQuadtreeValue value = { index, aabb };
    
    if (!frIsQuadtreeLeaf(tree)) {
        int child_index = frGetQuadtreeIndex(tree, value.aabb);
        
        if (child_index != -1) frAddToQuadtree(tree->children[child_index], index, aabb);
        else frAddToArray(tree->values, value);
    } else {
        if (tree->depth < FR_QUADTREE_MAX_DEPTH
            && frGetArrayLength(tree->values) >= FR_QUADTREE_MAX_LEAF_COUNT) {
            frSplitQuadtree(tree);
            frAddToQuadtree(tree, index, aabb);
        } else {
            frAddToArray(tree->values, value);
        }
    }
}

/* 쿼드 트리 `tree`의 모든 노드를 제거한다. */
void frClearQuadtree(frQuadtree *tree) {
    if (tree == NULL) return;
    
    frClearArray(tree->values);
    
    if (tree->depth > 0) 
        frReleaseArray(tree->values);
    
    if (!frIsQuadtreeLeaf(tree)) {
        for (int i = 0; i < 4; i++) {
            frClearQuadtree(tree->children[i]);
            
            free(tree->children[i]);
            tree->children[i] = NULL;
        }
    }
}

/* 쿼드 트리 `tree`가 잎 노드인지 확인한다. */
bool frIsQuadtreeLeaf(frQuadtree *tree) {
    return (tree != NULL && tree->children[0] == NULL);
}

/* 쿼드 트리 `tree`의 경계 범위를 반환한다. */
Rectangle frGetQuadtreeBounds(frQuadtree *tree) {
    return (tree != NULL) ? tree->bounds : (Rectangle) { 0 };
}

/* 쿼드 트리 `tree`에서 인덱스가 `index`인 자식 노드의 메모리 주소를 반환한다. */
frQuadtree *frGetQuadtreeChild(frQuadtree *tree, int index) {
    return (tree != NULL && !frIsQuadtreeLeaf(tree)) ? tree->children[index] : NULL;
}

/* 쿼드 트리 `tree`의 깊이를 반환한다. */
int frGetQuadtreeDepth(frQuadtree *tree) {
    return (tree != NULL) ? tree->depth : -1;
}

/* 쿼드 트리 `tree`에서 경계 범위 `bounds`가 포함된 자식 노드의 인덱스를 반환한다. */
int frGetQuadtreeIndex(frQuadtree *tree, Rectangle bounds) {
    int result = -1;
    
    int sub_width = tree->bounds.width / 2;
    int sub_height = tree->bounds.height / 2;
    
    Vector2 origin = (Vector2) { tree->bounds.x + sub_width, tree->bounds.y + sub_height };
    
    if (bounds.x + bounds.width < origin.x) {
        if (bounds.y + bounds.height < origin.y) result = 1;
        else if (bounds.y > origin.y) result = 2;
    } else if (bounds.x > origin.x) {
        if (bounds.y + bounds.height < origin.y) result = 0;
        else if (bounds.y > origin.y) result = 3;
    }
    
    return result;
}

/* 쿼드 트리 `tree`에서 `bounds`와 경계 범위가 겹치는 모든 도형의 인덱스를 반환한다. */
void frQueryQuadtree(frQuadtree *tree, Rectangle bounds, int **result) {
    if (tree == NULL || tree->values == NULL || result == NULL) return;
    
    for (int i = 0; i < frGetArrayLength(tree->values); i++)
        if (CheckCollisionRecs(bounds, tree->values[i].aabb))
            frAddToArray(*result, tree->values[i].index);
    
    if (frIsQuadtreeLeaf(tree)) return;
        
    for (int i = 0; i < 4; i++)
        if (CheckCollisionRecs(bounds, tree->children[i]->bounds))
            frQueryQuadtree(tree->children[i], bounds, result);
}

/* 쿼드 트리 `tree`를 4등분하고, `tree`에 저장된 모든 값을 자식 노드로 분배한다. */
void frSplitQuadtree(frQuadtree *tree) {
    if (tree == NULL || !frIsQuadtreeLeaf(tree)) return;
    
    int sub_width = tree->bounds.width / 2;
    int sub_height = tree->bounds.height / 2;
    
    tree->children[0] = frCreateQuadtree(
        tree->depth + 1, 
        (Rectangle) {
            tree->bounds.x + sub_width,
            tree->bounds.y,
            sub_width,
            sub_height
        }
    );
    
    tree->children[1] = frCreateQuadtree(
        tree->depth + 1, 
        (Rectangle) {
            tree->bounds.x,
            tree->bounds.y,
            sub_width,
            sub_height
        }
    );
    
    tree->children[2] = frCreateQuadtree(
        tree->depth + 1, 
        (Rectangle) {
            tree->bounds.x,
            tree->bounds.y + sub_height,
            sub_width,
            sub_height
        }
    );
    
    tree->children[3] = frCreateQuadtree(
        tree->depth + 1, 
        (Rectangle) {
            tree->bounds.x + sub_width,
            tree->bounds.y + sub_height,
            sub_width,
            sub_height
        }
    );
    
    for (int i = 0; i < frGetArrayLength(tree->values); i++) {
        int child_index = frGetQuadtreeIndex(tree, tree->values[i].aabb);
        
        if (tree->children[child_index] == NULL) continue;
        
        if (child_index != -1) frAddToArray(tree->children[child_index]->values, tree->values[i]);
        else frAddToArray(tree->temp_values, tree->values[i]);
    }
    
    for (int i = 0; i < frGetArrayLength(tree->temp_values); i++)
        tree->values[i] = tree->temp_values[i];
    
    frClearArray(tree->temp_values);
}