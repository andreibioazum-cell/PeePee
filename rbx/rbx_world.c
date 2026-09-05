/* Скользящий кэш чанков + слой правок: движок умеет читать, менять
 * и трассировать блоки, а меши остаются только из открытых граней. */
#include "rbx_internal.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { CACHE_SIDE = WORLD_RADIUS * 2 + 1, CACHE_COUNT = CACHE_SIDE * CACHE_SIDE };
typedef struct { unsigned char x, y, z, face, block; } Face;
typedef struct {
    int cx, cz, valid, ready, min_y, max_y;
    unsigned char blocks[CHUNK_SIZE * CHUNK_SIZE * WORLD_HEIGHT];
    Face *faces;
    int count, capacity;
} Chunk;
typedef struct { int x, y, z; unsigned char block; } Edit;

static Chunk chunks[CACHE_COUNT];
static Edit *edits;
static int edit_count, edit_capacity;
static int center_x = INT_MIN, center_z = INT_MIN;

static int floor_chunk(int x) { return x / CHUNK_SIZE - (x < 0 && x % CHUNK_SIZE != 0); }
static int wrap(int x) { int n = x % CACHE_SIDE; return n < 0 ? n + CACHE_SIDE : n; }
static Chunk *slot(int cx, int cz) { return &chunks[wrap(cz) * CACHE_SIDE + wrap(cx)]; }
static int index3(int x, int y, int z) { return (y * CHUNK_SIZE + z) * CHUNK_SIZE + x; }
static int find_edit(int x, int y, int z) {
    for (int i = edit_count - 1; i >= 0; i--)
        if (edits[i].x == x && edits[i].y == y && edits[i].z == z) return i;
    return -1;
}
static int edited_block(int x, int y, int z, int *block) {
    int i = find_edit(x, y, z);
    if (i < 0) return 0;
    if (block) *block = edits[i].block;
    return 1;
}
static void apply_edits(Chunk *c) {
    int ox = c->cx * CHUNK_SIZE, oz = c->cz * CHUNK_SIZE;
    for (int i = 0; i < edit_count; i++) {
        if (edits[i].y < 0 || edits[i].y >= WORLD_HEIGHT) continue;
        int x = edits[i].x - ox, z = edits[i].z - oz;
        if (x >= 0 && x < CHUNK_SIZE && z >= 0 && z < CHUNK_SIZE)
            c->blocks[index3(x, edits[i].y, z)] = edits[i].block;
    }
}
static void load_blocks(Chunk *c) {
    rbx_terrain_chunk(c->cx, c->cz, c->blocks);
    apply_edits(c);
}

int rbx_world_block(int x, int y, int z) {
    if (y < 0) return BLOCK_STONE;
    if (y >= WORLD_HEIGHT) return BLOCK_AIR;
    int edited;
    if (edited_block(x, y, z, &edited)) return edited;
    int cx = floor_chunk(x), cz = floor_chunk(z);
    Chunk *c = slot(cx, cz);
    if (!c->valid || c->cx != cx || c->cz != cz) return rbx_terrain_block(x, y, z);
    return c->blocks[index3(x - cx * CHUNK_SIZE, y, z - cz * CHUNK_SIZE)];
}
int rbx_world_solid(int x, int y, int z) {
    int b = rbx_world_block(x, y, z);
    return b != BLOCK_AIR && b != BLOCK_WATER;
}

static void add_face(Chunk *c, int x, int y, int z, int face, int block) {
    if (c->count == c->capacity) {
        int cap = c->capacity ? c->capacity * 2 : 512;
        Face *f = realloc(c->faces, (size_t)cap * sizeof(*f));
        if (!f) { ds_runtime_error("Недостаточно памяти для чанка"); return; }
        c->faces = f; c->capacity = cap;
    }
    Face f = {(unsigned char)x, (unsigned char)y, (unsigned char)z, (unsigned char)face, (unsigned char)block};
    c->faces[c->count++] = f;
    if (y < c->min_y) c->min_y = y;
    if (y + 1 > c->max_y) c->max_y = y + 1;
}
static void mesh(Chunk *c) {
    static const int offsets[6][3] = {{0,1,0},{0,-1,0},{0,0,1},{0,0,-1},{1,0,0},{-1,0,0}};
    c->count = 0; c->min_y = WORLD_HEIGHT; c->max_y = 0;
    int ox = c->cx * CHUNK_SIZE, oz = c->cz * CHUNK_SIZE;
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                int b = c->blocks[index3(x, y, z)];
                if (!b) continue;
                for (int face = 0; face < 6; face++) {
                    int neighbor = rbx_world_block(ox + x + offsets[face][0], y + offsets[face][1], oz + z + offsets[face][2]);
                    if (neighbor == BLOCK_AIR || (neighbor == BLOCK_WATER && b != BLOCK_WATER))
                        add_face(c, x, y, z, face, b);
                }
            }
        }
    }
    c->ready = 1;
}

static Chunk *loaded_chunk(int cx, int cz) {
    Chunk *c = slot(cx, cz);
    return c->valid && c->cx == cx && c->cz == cz ? c : NULL;
}
static void rebuild_chunks(Chunk **list, int count) {
    for (int i = 0; i < count; i++) if (list[i]) { load_blocks(list[i]); list[i]->ready = 0; }
    for (int i = 0; i < count; i++) if (list[i]) mesh(list[i]);
}
static void collect_chunk(Chunk **list, int *count, int cx, int cz) {
    Chunk *c = loaded_chunk(cx, cz);
    if (!c) return;
    for (int i = 0; i < *count; i++) if (list[i] == c) return;
    list[(*count)++] = c;
}
static void rebuild_near_block(int x, int z) {
    int cx = floor_chunk(x), cz = floor_chunk(z), count = 0;
    Chunk *list[5] = {0};
    collect_chunk(list, &count, cx, cz);
    collect_chunk(list, &count, cx + 1, cz);
    collect_chunk(list, &count, cx - 1, cz);
    collect_chunk(list, &count, cx, cz + 1);
    collect_chunk(list, &count, cx, cz - 1);
    rebuild_chunks(list, count);
}

int rbx_world_set_block(int x, int y, int z, int block) {
    if (block < 0 || block >= BLOCK_COUNT || y < 0 || y >= WORLD_HEIGHT) return 0;
    if (rbx_world_block(x, y, z) == block) return 1;
    int terrain = rbx_terrain_block(x, y, z);
    int i = find_edit(x, y, z);
    if (block == terrain) {
        if (i >= 0) edits[i] = edits[--edit_count];
    } else if (i >= 0) {
        edits[i].block = (unsigned char)block;
    } else {
        if (edit_count == edit_capacity) {
            int cap = edit_capacity ? edit_capacity * 2 : 64;
            Edit *next = realloc(edits, (size_t)cap * sizeof(*next));
            if (!next) return 0;
            edits = next;
            edit_capacity = cap;
        }
        edits[edit_count++] = (Edit){x, y, z, (unsigned char)block};
    }
    rebuild_near_block(x, z);
    return 1;
}
void rbx_world_clear_edits(void) {
    edit_count = 0;
    Chunk *list[CACHE_COUNT];
    int count = 0;
    for (int i = 0; i < CACHE_COUNT; i++) if (chunks[i].valid) list[count++] = &chunks[i];
    rebuild_chunks(list, count);
}
int rbx_world_edit_count(void) { return edit_count; }

void rbx_world_update(float x, float z) {
    int cx = (int)floorf(x / CHUNK_SIZE), cz = (int)floorf(z / CHUNK_SIZE);
    if (cx == center_x && cz == center_z) return;
    center_x = cx; center_z = cz;
    for (int dz = -WORLD_RADIUS; dz <= WORLD_RADIUS; dz++) {
        for (int dx = -WORLD_RADIUS; dx <= WORLD_RADIUS; dx++) {
            Chunk *c = slot(cx + dx, cz + dz);
            if (c->valid && c->cx == cx + dx && c->cz == cz + dz) continue;
            c->cx = cx + dx; c->cz = cz + dz; c->valid = 1; c->ready = 0;
            load_blocks(c);
        }
    }
    for (int i = 0; i < CACHE_COUNT; i++) if (!chunks[i].ready) mesh(&chunks[i]);
}
void rbx_world_build(uint32_t seed) {
    rbx_terrain_seed(seed);
    edit_count = 0;
    center_x = center_z = INT_MIN;
    for (int i = 0; i < CACHE_COUNT; i++) chunks[i].valid = chunks[i].ready = 0;
    rbx_world_update(8.5f, 8.5f);
}
void rbx_world_draw(void) {
    for (int ring = 0; ring <= WORLD_RADIUS; ring++) {
        for (int dz = -ring; dz <= ring; dz++) {
            for (int dx = -ring; dx <= ring; dx++) {
                if (abs(dx) != ring && abs(dz) != ring) continue;
                Chunk *c = slot(center_x + dx, center_z + dz);
                if (!c->ready || !c->count) continue;
                float ox = c->cx * CHUNK_SIZE, oz = c->cz * CHUNK_SIZE;
                float hy = (c->max_y - c->min_y) * .5f;
                if (!rbx3d_visible(ox + 8, c->min_y + hy, oz + 8, 8, hy, 8)) continue;
                for (int i = 0; i < c->count; i++) {
                    const Face *f = &c->faces[i];
                    rbx3d_block_face(ox + f->x, f->y, oz + f->z, f->face, f->block);
                }
            }
        }
    }
}
void rbx_world_stats(int *loaded, int *faces) {
    int n = 0, f = 0;
    for (int i = 0; i < CACHE_COUNT; i++) { n += chunks[i].valid; f += chunks[i].count; }
    if (loaded) *loaded = n;
    if (faces) *faces = f;
}

static void fill_hit(RbxRaycastHit *hit, int x, int y, int z, int face, int block,
                     float distance, float ox, float oy, float oz, float dx, float dy, float dz) {
    if (!hit) return;
    hit->hit = 1; hit->x = x; hit->y = y; hit->z = z; hit->face = face; hit->block = block;
    hit->distance = distance;
    hit->hit_x = ox + dx * distance;
    hit->hit_y = oy + dy * distance;
    hit->hit_z = oz + dz * distance;
}
int rbx_world_raycast(float ox, float oy, float oz, float dx, float dy, float dz,
                      float max_distance, RbxRaycastHit *hit) {
    if (hit) memset(hit, 0, sizeof(*hit));
    if (!isfinite(ox + oy + oz + dx + dy + dz + max_distance) || max_distance <= 0.0f) return 0;
    float len = sqrtf(dx * dx + dy * dy + dz * dz);
    if (len < 1e-6f) return 0;
    dx /= len; dy /= len; dz /= len;
    int x = (int)floorf(ox), y = (int)floorf(oy), z = (int)floorf(oz);
    int block = rbx_world_block(x, y, z);
    if (block != BLOCK_AIR) { fill_hit(hit, x, y, z, -1, block, 0, ox, oy, oz, dx, dy, dz); return 1; }
    int sx = dx > 0 ? 1 : dx < 0 ? -1 : 0;
    int sy = dy > 0 ? 1 : dy < 0 ? -1 : 0;
    int sz = dz > 0 ? 1 : dz < 0 ? -1 : 0;
    float tx = sx > 0 ? (x + 1 - ox) / dx : sx < 0 ? (ox - x) / -dx : INFINITY;
    float ty = sy > 0 ? (y + 1 - oy) / dy : sy < 0 ? (oy - y) / -dy : INFINITY;
    float tz = sz > 0 ? (z + 1 - oz) / dz : sz < 0 ? (oz - z) / -dz : INFINITY;
    float ddx = sx ? 1.0f / fabsf(dx) : INFINITY;
    float ddy = sy ? 1.0f / fabsf(dy) : INFINITY;
    float ddz = sz ? 1.0f / fabsf(dz) : INFINITY;
    for (int guard = 0; guard < 4096; guard++) {
        float dist;
        int face;
        if (tx <= ty && tx <= tz) { x += sx; dist = tx; tx += ddx; face = sx > 0 ? 5 : 4; }
        else if (ty <= tz) { y += sy; dist = ty; ty += ddy; face = sy > 0 ? 1 : 0; }
        else { z += sz; dist = tz; tz += ddz; face = sz > 0 ? 3 : 2; }
        if (dist > max_distance) return 0;
        block = rbx_world_block(x, y, z);
        if (block != BLOCK_AIR) { fill_hit(hit, x, y, z, face, block, dist, ox, oy, oz, dx, dy, dz); return 1; }
    }
    return 0;
}
