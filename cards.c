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

int card_pick[CARD_SHOWN];
int card_at;

/* An upgrade that has nothing left to give is not offered, so a card is always
   worth taking rather than being a slot the player has to spend. */
int card_ready(int kind) {
    if (kind == 1) return blade_period > 12;
    if (kind == 3) return blade_count < 3;
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

void cards_deal(void) {
    int i, j, tries, kind, taken;

    card_at = 0;

    for (i = 0; i < CARD_SHOWN; i++) {
        card_pick[i] = 0;

        for (tries = 0; tries < 24; tries++) {
            kind = rnd(CARD_KINDS);
            if (!card_ready(kind)) continue;

            taken = 0;
            for (j = 0; j < i; j++)
                if (card_pick[j] == kind) taken = 1;

            if (taken) continue;

            card_pick[i] = kind;
            break;
        }
    }
}

void cards_cursor(void) {
    int i;

    for (i = 0; i < CARD_SHOWN; i++)
        window_tile(0, CARD_ROW + i * CARD_GAP, i == card_at ? CURSOR_TILE : 0);
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
        row = CARD_ROW + i * CARD_GAP;
        window_print(2, row, card_name(card_pick[i]));
        window_print(2, row + 1, card_line(card_pick[i]));
    }

    cards_cursor();

    window_print(2, 17, "A TAKE");

    /* The offer is only on the screen while it can be paid for, so the line
       answers "can I?" before the button does. */
    if (hero_gold >= REROLL_COST) {
        window_print(11, 17, "B ROLL");
        window_print_num(18, 17, REROLL_COST, 1);
    }
}

/* A hand nobody wants, bought off with what the horde dropped. Refused rather
   than partly done when the gold is short: nothing is spent and the cards
   stay as they were. */
void cards_reroll(void) {
    if (hero_gold < REROLL_COST) return;

    hero_gold -= REROLL_COST;

    cards_deal();
    cards_show();
    sfx(SFX_PICK);
}

void cards_move(int delta) {
    card_at += delta;

    if (card_at < 0) card_at = CARD_SHOWN - 1;
    if (card_at >= CARD_SHOWN) card_at = 0;

    sfx(SFX_PICK);
    cards_cursor();
}

void cards_take(void) {
    switch (card_pick[card_at]) {
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
