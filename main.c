/* Pixel Reaper: a survival run in six screens.

   The arena is one 32x32 map, the size of the hardware's own background, and
   the camera follows the reaper across it. The level is the background and
   nothing here draws the ground; what a kill leaves behind is a cell of it. Every screen
   that is not the run itself is the window layer put at (0, 0), which covers
   the picture without touching it: the arena is still loaded underneath, and
   coming back is one call rather than a reload.

   Every change of screen goes through fade_to, so the swap itself always
   happens on a black screen and nothing is ever seen being built. Nothing
   reads the pad while a fade runs, which is also what stops the button that
   ended one screen from acting on the next.

   The clock is seconds(), and a screen that pauses the run adds the time it
   held to the moment the run started. */

#define SCREEN_TITLE 0
#define SCREEN_PLAY  1
#define SCREEN_CARD  2
#define SCREEN_PAUSE 3
#define SCREEN_OVER  4
#define SCREEN_WIN   5

/* Where a fade is going. Two of these land on the playing screen and are not
   the same arrival: one starts a run and the other picks one back up. */
#define GO_TITLE  0
#define GO_RUN    1
#define GO_RESUME 2
#define GO_CARD   3
#define GO_PAUSE  4
#define GO_OVER   5
#define GO_WIN    6

/* Four steps, since dim has four, and three frames each: a fifth of a second
   in either direction, which reads as a cut with a shape rather than as a
   wait. */
#define FADE_DARKEST 3
#define FADE_FRAMES  3

#define WIN_SECS 600

#define HUD_ROWS 2
#define HP_CELLS 10
#define XP_CELLS 14

#define FULL_TILE  14
#define EMPTY_TILE 15
#define PICK_TILE  16
#define MARK_TILE  17

#define UNIT 4

#define SONG_MENU 0
#define SONG_RUN  1

#define SFX_CHOOSE 6
#define SFX_DEATH  7

int screen_mode;
int run_secs;
int run_from;
int held_at;
int pause_at;

int fade_step;
int fade_way;
int fade_wait;
int fade_goes;

int shown_hp;
int shown_left;
int shown_level;
int shown_xp;

/* The two bars are a division each, and a division is a called routine here.
   These hold what the last one was worked out from, so the sums only happen on
   the frames the numbers behind them actually moved. */
int counted_hp;
int counted_xp;

/* Half the view, worked out when the bar is set rather than twice a frame:
   view_width() cannot change while the run is on. */
int half_wide;
int half_tall;

void hud_bar(int col, int row, int cells, int filled) {
    int i;

    for (i = 0; i < cells; i++)
        window_tile(col + i, row, i < filled ? FULL_TILE : EMPTY_TILE);
}

void hud_frame(void) {
    window_bottom(HUD_ROWS);
    window_clear();

    half_wide = view_width() / 2;
    half_tall = view_height() / 2;

    window_print(11, 0, "T");
    window_print(0, 1, "LV");

    shown_hp = -1;
    shown_left = -1;
    shown_level = -1;
    shown_xp = -1;
    counted_hp = -1;
    counted_xp = -1;
}

/* Written when it changes and not every frame: a window cell keeps what was
   put in it, so a bar that has not moved costs nothing at all. */
void hud_refresh(void) {
    int filled, left;

    if (hero_hp != counted_hp) {
        counted_hp = hero_hp;

        filled = hero_hp <= 0 ? 0 : (hero_hp * HP_CELLS + hero_max - 1) / hero_max;
        if (filled > HP_CELLS) filled = HP_CELLS;

        if (filled != shown_hp) {
            hud_bar(0, 0, HP_CELLS, filled);
            shown_hp = filled;
        }
    }

    left = WIN_SECS - run_secs;
    if (left < 0) left = 0;

    if (left != shown_left) {
        window_print_num(13, 0, left, 3);
        shown_left = left;
    }

    if (hero_level != shown_level) {
        window_print_num(3, 1, hero_level, 2);
        shown_level = hero_level;
    }

    if (hero_xp != counted_xp) {
        counted_xp = hero_xp;

        filled = hero_xp * XP_CELLS / hero_need;
        if (filled > XP_CELLS) filled = XP_CELLS;

        if (filled != shown_xp) {
            hud_bar(6, 1, XP_CELLS, filled);
            shown_xp = filled;
        }
    }
}

void run_stats(int row) {
    window_print(3, row, "TIME");
    window_print_num(13, row, run_secs, 3);
    window_print(3, row + 2, "LEVEL");
    window_print_num(13, row + 2, hero_level, 3);
    window_print(3, row + 4, "KILLS");
    window_print_num(13, row + 4, hero_kills, 3);
    window_print(3, row + 6, "GOLD");
    window_print_num(13, row + 6, hero_gold, 3);
}

void show_title(void) {
    screen_mode = SCREEN_TITLE;

    music(SONG_MENU);
    window(0, 0);
    window_clear();

    window_print(3, 4, "PIXEL  REAPER");

    window_print(4, 11, "PRESS START");
    window_print(1, 15, "MOVE AND SURVIVE");
    window_print(1, 16, "THE BLADE SWINGS");

    pad_clear();
}

void show_over(void) {
    screen_mode = SCREEN_OVER;

    music(SONG_MENU);
    window(0, 0);
    window_clear();

    window_print(5, 2, "GAME OVER");
    run_stats(6);
    window_print(2, 16, "A RETRY  B MENU");

    pad_clear();
}

void show_win(void) {
    screen_mode = SCREEN_WIN;

    music(SONG_MENU);
    window(0, 0);
    window_clear();

    window_print(4, 2, "YOU SURVIVED");
    run_stats(6);
    window_print(2, 16, "A AGAIN  B MENU");

    pad_clear();
}

void show_pause(void) {
    screen_mode = SCREEN_PAUSE;
    pause_at = 0;

    window(0, 0);
    window_clear();

    window_print(7, 2, "PAUSED");

    window_print(3, 5, "LIFE");
    window_print_num(13, 5, hero_hp, 3);
    window_print(3, 6, "DAMAGE");
    window_print_num(13, 6, blade_damage, 3);
    window_print(3, 7, "BLADES");
    window_print_num(13, 7, blade_count, 3);
    window_print(3, 8, "LEVEL");
    window_print_num(13, 8, hero_level, 3);

    window_print(3, 12, "CONTINUE");
    window_print(3, 14, "QUIT RUN");

    window_tile(1, 12, PICK_TILE);
    window_tile(1, 14, 0);

    pad_clear();
}

/* Where the screen looks while a run is on. Called on the way in as well as
   every frame, because a fade comes up on the picture several frames before
   update() is allowed to run again: without it the level is shown from its
   own corner and lurches to the player on the first live frame. */
void look(void) {
    camera((hero_x >> 2) - half_wide, (hero_y >> 2) - half_tall);
}

void resume_play(void) {
    run_from += seconds() - held_at;

    screen_mode = SCREEN_PLAY;
    hud_frame();
    hud_refresh();
    look();
    pad_clear();
}

void show_cards(void) {
    screen_mode = SCREEN_CARD;

    cards_deal();
    window(0, 0);
    cards_show();

    pad_clear();
}

void start_run(void) {
    /* The map is put back as it was drawn: the last run left gems and coins
       written into its cells, and set_tile is not undone by anything else. */
    load_bkg(0);
    arena_start();

    run_from = seconds();
    run_secs = 0;

    music(SONG_RUN);
    screen_mode = SCREEN_PLAY;
    hud_frame();
    hud_refresh();
    look();
    pad_clear();
}

/* The one place a screen is put up, so the fade has a single thing to call
   when it reaches black. */
void enter(int where) {
    if (where == GO_TITLE) show_title();
    else if (where == GO_RUN) start_run();
    else if (where == GO_RESUME) resume_play();
    else if (where == GO_CARD) show_cards();
    else if (where == GO_PAUSE) show_pause();
    else if (where == GO_OVER) show_over();
    else show_win();
}

void fade_to(int where) {
    fade_goes = where;
    fade_way = 1;
    fade_wait = FADE_FRAMES;

    /* The screen a run is coming back from is held from here rather than from
       the arrival, or the darkening counts against the clock. */
    if (where == GO_CARD || where == GO_PAUSE) held_at = seconds();
}

void fade_frame(void) {
    fade_wait--;
    if (fade_wait > 0) return;

    fade_wait = FADE_FRAMES;
    fade_step += fade_way;

    /* Black goes up before the screen is built, not after it. Putting the
       screen up first left the level being written under the last step of the
       fade instead of under black: load_bkg_hidden spreads a level over many
       blank periods, so the room came in a few rows at a time from the top,
       in plain sight, while the palette still had two shades left in it. */
    if (fade_step >= FADE_DARKEST) {
        fade_step = FADE_DARKEST;
        dim(FADE_DARKEST);

        fade_way = -1;
        enter(fade_goes);
        return;
    }

    if (fade_step <= 0) {
        fade_step = 0;
        fade_way = 0;
        load_colors();
        return;
    }

    dim(fade_step);
}

void end_run(void) {
    music_stop();
    sfx(SFX_DEATH);
    fade_to(GO_OVER);
}

void play_frame(void) {
    run_secs = seconds() - run_from;

    arena_update();
    look();

    if (hero_hp <= 0) {
        end_run();
        return;
    }

    if (run_secs >= WIN_SECS) {
        fade_to(GO_WIN);
        return;
    }

    if (level_ups > 0) {
        level_ups--;
        fade_to(GO_CARD);
        return;
    }

    hud_refresh();
}

void pause_frame(void) {
    if (pad_hit(UP) || pad_hit(DOWN)) {
        pause_at = pause_at == 0 ? 1 : 0;
        window_tile(1, 12, pause_at == 0 ? PICK_TILE : 0);
        window_tile(1, 14, pause_at == 1 ? PICK_TILE : 0);
        sfx(SFX_CHOOSE);
    }

    if (pad_hit(START) || pad_hit(B)) {
        fade_to(GO_RESUME);
        return;
    }

    if (pad_hit(A)) {
        fade_to(pause_at == 0 ? GO_RESUME : GO_TITLE);
    }
}

void init(void) {
    load_bkg(0);
    show_title();

    /* The first screen fades up like every other one: the title is built
       behind black rather than appearing whole. */
    fade_step = FADE_DARKEST;
    fade_way = -1;
    fade_wait = FADE_FRAMES;
    dim(FADE_DARKEST);
}

void update(void) {
    /* A fade owns the frame: nothing moves and no button is read, so the press
       that ended one screen cannot act on the next. */
    if (fade_way != 0) {
        fade_frame();
        return;
    }

    if (screen_mode == SCREEN_PLAY) {
        if (pad_hit(START)) {
            fade_to(GO_PAUSE);
            return;
        }

        play_frame();
        return;
    }

    if (screen_mode == SCREEN_CARD) {
        if (pad_hit(UP)) cards_move(-1);
        if (pad_hit(DOWN)) cards_move(1);
        if (pad_hit(B)) cards_reroll();

        if (pad_hit(A)) {
            cards_take();
            fade_to(GO_RESUME);
        }

        return;
    }

    if (screen_mode == SCREEN_PAUSE) {
        pause_frame();
        return;
    }

    if (screen_mode == SCREEN_TITLE) {
        if (pad_hit(START) || pad_hit(A)) fade_to(GO_RUN);
        return;
    }

    /* Game over and victory share their keys: A runs again, B goes back. */
    if (pad_hit(A)) fade_to(GO_RUN);
    else if (pad_hit(B) || pad_hit(START)) fade_to(GO_TITLE);
}

void draw(void) {
    if (screen_mode == SCREEN_PLAY) arena_draw();
}
