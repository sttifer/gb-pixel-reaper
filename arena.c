/* The arena: the reaper, the horde, the blades it throws and what a kill drops.

   Everything is placed in quarter pixels, so the slowest enemy still moves
   every frame instead of standing still for three of them. A position is
   divided by STEP on the way to the screen and nowhere else. */

#define STEP 4

#define ARENA_TILES 32
#define ARENA_LOW   (12 * STEP)
#define ARENA_HIGH  ((ARENA_TILES * 8 - 12) * STEP)

/* The horde and the blades are what the frame budget is spent on, and the
   cost was measured to be the arithmetic, not the sprites: submitting an
   object is about two scanlines, while stepping the whole horde in 16-bit
   fields was most of the frame. The fields are bytes now and the horde walks
   half per frame, which is what holds eight enemies at full speed. Drops are
   cells of the background and cost nothing per frame, which is why there is
   room for enough of them that the gems of a good fight are all still there to
   be walked over. */
#define ENEMIES 8
#define BLADES  3
#define DROPS   12

#define GEM   1
#define HEART 2
#define COIN  3

#define TILE_BLADE 5
#define TILE_GEM   6
#define TILE_HEART 7
#define TILE_HERO  8
#define TILE_WALK  9
#define TILE_COIN  13

#define BLADE_SPEED 14
#define BLADE_LIFE  34
/* Long enough to walk out of a crowd, short enough that standing in one is
   fatal: this is the whole difference between being surrounded mattering and
   not. */
#define HURT_FRAMES 40

#define TOUCH (6 * STEP)
#define TAKE  (5 * STEP)

#define SFX_SHOT   0
#define SFX_HIT    1
#define SFX_KILL   2
#define SFX_PICKUP 3
#define SFX_LEVEL  4
#define SFX_HURT   5

/* Walker, flyer, brute, in that order. A type is stored as its number plus
   one, so zero is a free slot and no enemy is ever type "none". */
const unsigned char foe_tile[3] = { 10, 11, 12 };

/* Three speeds against the reaper's three: the walker is chaff to weave
   through, the flyer is faster than anyone and has to be shot, and the brute
   keeps exactly the player's pace, so it can be left behind for a while and
   never lost. Every type used to be slower than the player except the flyer,
   and a run ended by walking into the horde rather than by it arriving. */
const unsigned char foe_speed[3] = { 2, 4, 3 };
const unsigned char foe_life[3] = { 2, 1, 6 };
const unsigned char foe_harm[3] = { 1, 1, 2 };
const unsigned char foe_worth[3] = { 1, 1, 3 };

int hero_x;
int hero_y;
int hero_hp;
int hero_max;
int hero_speed;
int hero_face;
int hero_hurt;
int hero_level;
int hero_xp;
int hero_need;
int hero_kills;
int hero_gold;
int level_ups;

int blade_damage;
int blade_period;
int blade_timer;
int blade_count;
int blade_area;
int blade_reach;
int magnet_reach;

/* Positions are 16-bit because the arena runs to 1024 quarter pixels; every
   other field of a mover fits in a byte, and on this CPU that is the whole
   difference between one load and two on every touch of it. */
unsigned char foe_kind[ENEMIES];
int foe_x[ENEMIES];
int foe_y[ENEMIES];
unsigned char foe_hp[ENEMIES];
unsigned char foe_mark[ENEMIES];

int blade_x[BLADES];
int blade_y[BLADES];
int blade_vx[BLADES];
int blade_vy[BLADES];
unsigned char blade_life[BLADES];

/* A drop is a cell of the level, not a sprite: the tile it put there and the
   tile it covered, so collecting one puts the ground back. The level is the
   size of the hardware map, which is what makes every cell of it writable. */
unsigned char drop_kind[DROPS];
unsigned char drop_tx[DROPS];
unsigned char drop_ty[DROPS];
unsigned char drop_back[DROPS];
unsigned char drop_next;

int spawn_timer;
unsigned char walk_frame;

/* The fraction of a step each mover is carrying, in 256ths. A diagonal step is
   0.707 of a straight one and the quarter pixels a position is kept in cannot
   hold that, so what is left over is carried to the next frame instead of
   being thrown away. Rounding it down every frame is what made a diagonal
   slower than a straight line: at speed 4 the step came out 2 where it should
   be 2.83, so walking corner to corner was 29 per cent slow. */
unsigned char hero_run;
unsigned char foe_run[ENEMIES];

/* Which half of the horde moves this frame. An enemy walks every other frame
   with a doubled step, so the horde moves at the same speed while each frame
   pays for half of it: the peak is what the budget is measured against. */
unsigned char foe_phase;

/* A macro rather than a call: this is the innermost thing the loops do, and a
   call with a 16-bit argument is not cheap on this hardware. The argument is
   always a plain variable here, so evaluating it twice costs nothing. */
#define AWAY(value) ((value) < 0 ? -(value) : (value))

/* 0.707 of each speed, in 256ths: the diagonal step that a whole number of
   quarter pixels cannot say. A table and not `speed * 181`, because a 16-bit
   multiply is a called routine on this hardware and this runs for every mover
   on every frame; the index is a speed, doubled for the movers that walk every
   other frame, which nothing here lets past 12. */
const int slant_step[13] = {
    0, 181, 362, 543, 724, 905, 1086, 1267, 1448, 1629, 1810, 1991, 2172
};

/* How far a mover goes this frame, keeping what a diagonal leaves behind.
   Written out at both callers rather than made a function of: it would want
   the carrier by address, and a pointer argument is stack traffic on every
   mover of every frame. A straight step needs none of it and takes the plain
   speed, which is what the else is for. The carrier is a byte and the table
   entry is not, so the sum lives in a 16-bit local for exactly one line. */
#define STEPPED(speed, slanted, carried, out)     if (slanted) {                                    int total = (carried) + slant_step[speed];    (out) = (unsigned char)(total >> 8);          (carried) = (unsigned char)total;         } else {                                          (out) = (speed);                          }

int inside(int value) {
    if (value < ARENA_LOW) return ARENA_LOW;
    if (value > ARENA_HIGH) return ARENA_HIGH;
    return value;
}

void arena_start(void) {
    int i;

    hero_x = (ARENA_TILES * 8 / 2) * STEP;
    hero_y = (ARENA_TILES * 8 / 2) * STEP;
    hero_max = 10;
    hero_hp = hero_max;
    hero_speed = 3;
    hero_face = 0;
    hero_hurt = 0;
    hero_level = 1;
    hero_xp = 0;
    hero_need = 6;
    hero_kills = 0;
    hero_gold = 0;
    level_ups = 0;

    blade_damage = 1;
    blade_period = 34;
    blade_timer = 8;
    blade_count = 1;
    blade_area = 0;
    blade_reach = 90 * STEP;
    magnet_reach = 14 * STEP;

    spawn_timer = 40;
    hero_run = 0;
    drop_next = 0;
    walk_frame = 0;
    foe_phase = 0;

    for (i = 0; i < ENEMIES; i++) {
        foe_kind[i] = 0;
        foe_run[i] = 0;
    }
    for (i = 0; i < BLADES; i++) blade_life[i] = 0;
    for (i = 0; i < DROPS; i++) drop_kind[i] = 0;
}

void hero_step(void) {
    int mx = 0;
    int my = 0;
    unsigned char step;

    if (pad(LEFT)) mx = -1;
    if (pad(RIGHT)) mx = 1;
    if (pad(UP)) my = -1;
    if (pad(DOWN)) my = 1;

    if (mx < 0) hero_face = FLIP_X;
    if (mx > 0) hero_face = 0;

    if (mx == 0 && my == 0) {
        walk_frame = 0;
        return;
    }

    /* Both axes take the same amount, so one carrier answers for the pair. */
    STEPPED(hero_speed, mx != 0 && my != 0, hero_run, step)

    hero_x = inside(hero_x + mx * step);
    hero_y = inside(hero_y + my * step);

    walk_frame++;
}

int drop_tile(int kind) {
    if (kind == GEM) return TILE_GEM;
    if (kind == HEART) return TILE_HEART;
    return TILE_COIN;
}

void drop_lift(int i) {
    set_tile(drop_tx[i], drop_ty[i], drop_back[i]);
    drop_kind[i] = 0;
}

/* A cell already holding a drop is stepped over rather than written twice: the
   second write would record a gem as the ground under it, and collecting it
   would leave a gem nothing can pick up. */
int drop_free(int tx, int ty) {
    int i;

    for (i = 0; i < DROPS; i++)
        if (drop_kind[i] != 0 && drop_tx[i] == tx && drop_ty[i] == ty) return 0;

    return 1;
}

void drop_at(int kind, int x, int y) {
    int tx = x >> 5;
    int ty = y >> 5;

    if (!drop_free(tx, ty)) {
        tx++;
        if (!drop_free(tx, ty)) return;
    }

    if (drop_kind[drop_next] != 0) drop_lift(drop_next);

    drop_kind[drop_next] = kind;
    drop_tx[drop_next] = tx;
    drop_ty[drop_next] = ty;
    drop_back[drop_next] = tile_at(tx, ty);

    set_tile(tx, ty, drop_tile(kind));

    drop_next++;
    if (drop_next >= DROPS) drop_next = 0;
}

void foe_dies(int i) {
    int roll = rnd(100);

    hero_kills++;
    sfx(SFX_KILL);

    drop_at(GEM, foe_x[i], foe_y[i]);

    if (roll < 4) drop_at(HEART, foe_x[i] + TAKE, foe_y[i]);
    else if (roll < 18) drop_at(COIN, foe_x[i] + TAKE, foe_y[i]);

    foe_kind[i] = 0;
}

void hero_hurts(int harm) {
    if (hero_hurt > 0) return;

    hero_hp -= harm;
    hero_hurt = HURT_FRAMES;
    sfx(SFX_HURT);
}

/* The locals are bytes wherever the value fits in one: a 16-bit local widens
   every comparison under it, and this is the loop the frame is spent in. */
void foes_step(void) {
    unsigned char i, type, step;
    int dx, dy;

    foe_phase = foe_phase == 0 ? 1 : 0;

    for (i = foe_phase; i < ENEMIES; i += 2) {
        if (foe_kind[i] == 0) continue;

        type = foe_kind[i] - 1;

        dx = hero_x - foe_x[i];
        dy = hero_y - foe_y[i];

        STEPPED(foe_speed[type] << 1, dx != 0 && dy != 0, foe_run[i], step)

        if (dx > 0) foe_x[i] += step;
        else if (dx < 0) foe_x[i] -= step;

        if (dy > 0) foe_y[i] += step;
        else if (dy < 0) foe_y[i] -= step;

        if (AWAY(dx) < TOUCH && AWAY(dy) < TOUCH) hero_hurts(foe_harm[type]);
    }
}

void blade_at(int tx, int ty) {
    int i, dx, dy, span;

    for (i = 0; i < BLADES; i++) {
        if (blade_life[i] > 0) continue;

        dx = tx - hero_x;
        dy = ty - hero_y;
        span = AWAY(dx) + AWAY(dy);
        if (span < 1) span = 1;

        blade_x[i] = hero_x;
        blade_y[i] = hero_y;
        blade_vx[i] = dx * BLADE_SPEED / span;
        blade_vy[i] = dy * BLADE_SPEED / span;
        blade_life[i] = BLADE_LIFE;
        return;
    }
}

/* One blade per shot, each at the nearest enemy that no earlier blade of this
   swing already took, so an upgrade to the count spreads the horde rather than
   putting every blade through one walker. */
void swing(void) {
    unsigned char shot, i;
    int best, span, near;

    for (i = 0; i < ENEMIES; i++) foe_mark[i] = 0;

    for (shot = 0; shot < blade_count; shot++) {
        best = -1;
        near = 0;

        for (i = 0; i < ENEMIES; i++) {
            if (foe_kind[i] == 0 || foe_mark[i] != 0) continue;

            span = AWAY(foe_x[i] - hero_x) + AWAY(foe_y[i] - hero_y);
            if (span > blade_reach) continue;

            if (best < 0 || span < near) {
                best = i;
                near = span;
            }
        }

        if (best < 0) return;

        foe_mark[best] = 1;
        blade_at(foe_x[best], foe_y[best]);

        if (shot == 0) sfx(SFX_SHOT);
    }
}

void blades_step(void) {
    unsigned char i, e;
    int reach, dx, dy;

    blade_timer--;
    if (blade_timer <= 0) {
        blade_timer = blade_period;
        swing();
    }

    reach = TOUCH + blade_area;

    for (i = 0; i < BLADES; i++) {
        if (blade_life[i] <= 0) continue;

        blade_x[i] += blade_vx[i];
        blade_y[i] += blade_vy[i];
        blade_life[i]--;

        for (e = 0; e < ENEMIES; e++) {
            if (foe_kind[e] == 0) continue;
            dx = foe_x[e] - blade_x[i];
            if (AWAY(dx) >= reach) continue;

            dy = foe_y[e] - blade_y[i];
            if (AWAY(dy) >= reach) continue;

            blade_life[i] = 0;

            /* Compared before subtracting, so the life never goes below zero
               and fits in the byte it is kept in. */
            if (foe_hp[e] <= blade_damage) {
                foe_dies(e);
            } else {
                foe_hp[e] -= blade_damage;
                sfx(SFX_HIT);
            }

            break;
        }
    }
}

void gain_xp(int amount) {
    hero_xp += amount;

    while (hero_xp >= hero_need) {
        hero_xp -= hero_need;
        hero_level++;
        hero_need = 3 + hero_level * 3;
        level_ups++;
        sfx(SFX_LEVEL);
    }
}

/* A drop stays where it fell and the reaper's reach is what grows, which is
   the same upgrade seen from the other side and costs no movement at all. */
void drops_step(void) {
    unsigned char i;
    int dx, dy;

    for (i = 0; i < DROPS; i++) {
        if (drop_kind[i] == 0) continue;

        dx = hero_x - (drop_tx[i] << 5) - 16;
        dy = hero_y - (drop_ty[i] << 5) - 16;

        if (AWAY(dx) >= magnet_reach || AWAY(dy) >= magnet_reach) continue;

        if (drop_kind[i] == GEM) {
            gain_xp(1);
        } else if (drop_kind[i] == HEART) {
            hero_hp += 2;
            if (hero_hp > hero_max) hero_hp = hero_max;
            sfx(SFX_PICKUP);
        } else {
            hero_gold++;
            sfx(SFX_PICKUP);
        }

        drop_lift(i);
    }
}

/* The horde thickens with the clock: sooner between spawns, tougher types
   unlocked, and every enemy carrying a little more life the longer the run
   has gone on. */
void spawning(void) {
    int i, free_slot, type, angle, far;

    spawn_timer--;
    if (spawn_timer > 0) return;

    spawn_timer = 52 - run_secs / 6;
    if (spawn_timer < 5) spawn_timer = 5;

    free_slot = -1;
    for (i = 0; i < ENEMIES; i++) {
        if (foe_kind[i] == 0) {
            free_slot = i;
            break;
        }
    }

    if (free_slot < 0) return;

    /* Walkers, then flyers, then brutes, and late in the run the brutes come
       often enough to be the shape of the fight rather than a rare one. */
    if (run_secs < 25) type = 0;
    else if (run_secs < 150) type = rnd(2);
    else if (run_secs < 300) type = rnd(3);
    else type = rnd(5) >> 1;

    angle = rnd(TURN);
    far = 96 + rnd(24);

    foe_kind[free_slot] = type + 1;
    foe_run[free_slot] = 0;
    foe_x[free_slot] = inside(hero_x + ((far * cosine(angle)) >> 8) * STEP);
    foe_y[free_slot] = inside(hero_y + ((far * sine(angle)) >> 8) * STEP);
    foe_hp[free_slot] = foe_life[type] + run_secs / 75 * foe_worth[type];
}

void arena_update(void) {
    hero_step();
    foes_step();
    blades_step();

    /* Every other frame, on the beat the smaller half of the horde walked:
       a drop picked up one frame late is invisible, the cost is not. */
    if (foe_phase == 0) drops_step();

    spawning();

    if (hero_hurt > 0) hero_hurt--;
}

void arena_draw(void) {
    unsigned char i;
    int left, top, right, bottom, x, y;

    obj_origin(camera_x(), camera_y() - view_top());

    left = camera_x() - 8;
    top = camera_y() - 8;
    right = camera_x() + view_width();
    bottom = camera_y() + view_height() - 8;

    for (i = 0; i < ENEMIES; i++) {
        if (foe_kind[i] == 0) continue;

        x = (foe_x[i] >> 2) - 4;
        y = (foe_y[i] >> 2) - 4;
        if (x < left || x > right || y < top || y > bottom) continue;

        obj(foe_tile[foe_kind[i] - 1], x, y);
    }

    for (i = 0; i < BLADES; i++) {
        if (blade_life[i] <= 0) continue;

        x = (blade_x[i] >> 2) - 4;
        y = (blade_y[i] >> 2) - 4;
        if (x < left || x > right || y < top || y > bottom) continue;

        obj(TILE_BLADE, x, y);
    }

    /* A hit blinks the reaper for as long as the hurt lasts, which is also how
       long the player is safe: the two are the same counter on purpose. */
    if (hero_hurt == 0 || (hero_hurt & 4) != 0) {
        obj_flip(
            (walk_frame & 8) != 0 ? TILE_WALK : TILE_HERO,
            (hero_x >> 2) - 4,
            (hero_y >> 2) - 4,
            hero_face
        );
    }
}
