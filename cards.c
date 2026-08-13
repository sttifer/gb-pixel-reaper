/* The level up cards: what is offered, what it does, and the screen it is
   chosen on. The names and the lines under them are constants, so they live in
   the file that prints them. */

#define CARD_KINDS 7
#define CARD_SHOWN 3
#define CARD_ROW   5
#define CARD_GAP   4
#define CARD_UNIT  4

#define CURSOR_TILE 16
#define GOLD_TILE   13

/* What a fresh hand of cards costs. Gold falls where an enemy died, which is
   the middle of the horde, so this is the one thing in the run that pays for
   walking towards danger rather than away from it. */
#define REROLL_COST 3

#define SFX_PICK 6

/* A slot holds its kind plus one, so zero is an empty slot and no card is ever
   kind "none": the same shape the arena keeps its enemies in. A hand late in a
   run can be short, and a slot that says nothing has to be tellable from the
   first card of the deck. */
int card_pick[CARD_SHOWN];
int card_count;
int card_at;

/* An upgrade that has nothing left to give is not offered, so a card is always
   worth taking rather than being a slot the player has to spend.
   TWIN BLADES stops at BLADES, the number of blades that can be in flight,
   and not at a number written here as well: a swing only throws what there is
   a slot for, so a cap of its own lets the count climb past what ever flies
   and the card promises a blade the player never gets. */
int card_ready(int kind) {
    if (kind == 1) return blade_period > 12;
    if (kind == 3) return blade_count < BLADES;
    if (kind == 5) return hero_speed < 5;
    if (kind == 6) return magnet_reach < 40 * CARD_UNIT;
    return 1;
}

const char *card_name(int kind) {
    switch (kind) {
        case 0: return "SHARP EDGE";
        case 1: return "QUICK HANDS";
        case 2: return "WIDE ARC";
        case 3: return "TWIN BLADES";
        case 4: return "IRON HEART";
        case 5: return "LIGHT BOOTS";
        default: return "SOUL PULL";
    }
}

const char *card_line(int kind) {
    switch (kind) {
        case 0: return "BLADE HITS HARDER";
        case 1: return "SWING MORE OFTEN";
        case 2: return "BIGGER REACH";
        case 3: return "ONE MORE BLADE";
        case 4: return "MORE MAX LIFE";
        case 5: return "MOVE FASTER";
        default: return "PICK UP FROM AFAR";
    }
}

/* Drawn from the kinds that still have something to give, without replacement,
   rather than rolled until a fresh one comes up: near the end of a run almost
   everything is capped, and rolling could run out of tries and fall back on a
   card that was already maxed. The hand is short instead, and an empty slot is
   an empty slot. */
void cards_deal(void) {
    int ready[CARD_KINDS];
    int i, kind, pick, left;

    card_at = 0;
    card_count = 0;
    left = 0;

    for (kind = 0; kind < CARD_KINDS; kind++)
        if (card_ready(kind)) ready[left++] = kind;

    for (i = 0; i < CARD_SHOWN; i++) {
        if (left == 0) {
            card_pick[i] = 0;
            continue;
        }

        pick = rnd(left);
        card_pick[i] = ready[pick] + 1;
        card_count++;

        /* The hole is filled by the last one, so the rest stay drawable
           without shifting the whole list down. */
        left--;
        ready[pick] = ready[left];
    }
}

void cards_cursor(void) {
    int i;

    for (i = 0; i < CARD_SHOWN; i++)
        window_tile(
            0,
            CARD_ROW + i * CARD_GAP,
            i == card_at && card_pick[i] != 0 ? CURSOR_TILE : 0
        );
}

void cards_show(void) {
    int i, row;

    window_clear();
    window_print(6, 1, "LEVEL UP");
    window_print(3, 2, "LEVEL");
    window_print_num(9, 2, hero_level, 2);

    window_tile(15, 2, GOLD_TILE);
    window_print_num(16, 2, hero_gold, 3);

    for (i = 0; i < CARD_SHOWN; i++) {
        if (card_pick[i] == 0) continue;

        row = CARD_ROW + i * CARD_GAP;
        window_print(2, row, card_name(card_pick[i] - 1));
        window_print(2, row + 1, card_line(card_pick[i] - 1));
    }

    cards_cursor();

    /* A level with nothing left to offer says so, rather than showing a card
       the player takes believing it still gives something. Three of the seven
       kinds have no cap, so a hand is always full as the run is balanced now;
       this is what stops the deal above from having to assume that. */
    if (card_count == 0) window_print(4, CARD_ROW + 2, "NOTHING LEFT");

    window_print(2, 17, card_count == 0 ? "A GO ON" : "A TAKE");

    /* The offer is only on the screen while it can be paid for, so the line
       answers "can I?" before the button does. A fresh hand of nothing is not
       worth gold either. */
    if (card_count > 0 && hero_gold >= REROLL_COST) {
        window_print(11, 17, "B ROLL");
        window_print_num(18, 17, REROLL_COST, 1);
    }
}

/* A hand nobody wants, bought off with what the horde dropped. Refused rather
   than partly done when the gold is short: nothing is spent and the cards
   stay as they were. */
void cards_reroll(void) {
    if (card_count == 0) return;
    if (hero_gold < REROLL_COST) return;

    hero_gold -= REROLL_COST;

    cards_deal();
    cards_show();
    sfx(SFX_PICK);
}

/* The cursor moves inside the hand that was dealt, not inside the three rows:
   a short hand fills the slots from the top, so the filled ones are the first
   card_count of them. */
void cards_move(int delta) {
    if (card_count <= 1) return;

    card_at += delta;

    if (card_at < 0) card_at = card_count - 1;
    if (card_at >= card_count) card_at = 0;

    sfx(SFX_PICK);
    cards_cursor();
}

void cards_take(void) {
    if (card_pick[card_at] == 0) return;

    switch (card_pick[card_at] - 1) {
        case 0:
            blade_damage++;
            break;
        case 1:
            blade_period -= 6;
            if (blade_period < 12) blade_period = 12;
            break;
        case 2:
            blade_area += CARD_UNIT;
            blade_reach += 10 * CARD_UNIT;
            break;
        case 3:
            blade_count++;
            break;
        case 4:
            hero_max += 3;
            hero_hp += 3;
            break;
        case 5:
            hero_speed++;
            break;
        default:
            magnet_reach += 5 * CARD_UNIT;
            break;
    }

    sfx(SFX_PICK);
}
