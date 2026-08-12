# Pixel Reaper

A survival auto battler for the Game Boy (DMG), built as a dotmatrix project. A run goes
from the title screen to game over or victory in 10 minutes, with card progression that
only lasts inside the run.

## Open and run

The project is an editor folder: open in
dotmatrix and hit run. Without the editor, you can build the ROM directly:

```bash
npx tsx scripts/build-rom.mts ../pixel-reaper/main.c ../pixel-reaper/pixel-reaper.gb
```

(from the dotmatrix folder; the ROM comes out at 32 KB.)

## Controls

| Button | Action |
| --- | --- |
| D-pad | move in 8 directions |
| A | confirm (title, card, pause, end of run) |
| B | back to the menu at the end of a run, leave the pause screen |
| START | pause, and start the run from the title |

The attack is automatic: the scythe flies on its own towards the nearest enemy.

## Files

| File | What it is |
| --- | --- |
| `main.c` | state machine for the six screens, HUD, run clock |
| `arena.c` | hero, horde, blades, drops, spawning and drawing |
| `cards.c` | the seven cards: name, text and what each one changes |
| `tiles.txt` | 8x8 art, deliberately rough, for you to draw over |
| `map.txt` | the arena, 32x32 tiles, walled at the edge |
| `map-tiles.txt` | tiles belonging to the map itself (empty here) |
| `font.txt` | the text tiles the HUD and menus print with |
| `palettes.txt` | the four DMG shades the editor draws with |
| `sfx.txt` | 8 game effects plus 4 used only by the music |
| `music.txt` | two songs: menu (song 0) and run (song 1) |

Tiles: 0 empty, 1-3 floor, 4 wall, 5 blade, 6 XP gem, 7 heal, 8-9 hero (standing and
walking), 10-12 enemies (common, fast, tank), 13 gold, 14-15 bar full and empty, 16
cursor, 17 skull.

## Decisions worth knowing before you touch anything

**The frame budget is sprites.** Measured in the emulator, each `obj()` costs around
3300 cycles: 10 sprites in a frame already push the end of `draw()` to line 93 of 154.
That is why the pools are small (`ENEMIES` and `BLADES` in `arena.c`, plus the hero) and
the rest of the game fits in what is left. Raising `ENEMIES` is the first thing that
drops the frame rate: with the arena full, 5 enemies measured about 6% of frames lost,
6 about 10%, and 8 goes past 20%.

**What a kill leaves behind lands in the background, not in a sprite.** Gems, hearts and
gold are map cells written with `set_tile`, and the original tile comes back when the
item is picked up. This only works because the level is exactly the size of the hardware
map (32x32): in a larger arena, a cell outside the camera would not be written. It is
also why `start_run` calls `load_bkg(0)`: the map comes back as it was drawn, otherwise
the next run starts with the previous run's gems on the floor.

**The magnet became reach.** Since a drop is a fixed cell, the SOUL PULL card grows the
pickup radius instead of pulling the item in; it is the same upgrade seen from the other
side, and it costs no movement at all.

**Positions are in quarter pixels.** A slow enemy walks 1/4 of a pixel per frame instead
of standing still for three frames out of every four. The division by 4 only happens at
draw time.

**The screens that are not the run are the window layer at (0, 0).** The arena stays
loaded behind it, so going back is one call and not a reload. Sprites are only submitted
in the play state, so nothing shows up on top of the menus.

## Quick knobs

| Where | What |
| --- | --- |
| `arena.c` `ENEMIES` / `BLADES` | size of the horde and of the blades in flight (frame cost) |
| `arena.c` `spawning()` | spawn pace and when each type joins (25 s, 150 s, 300 s) |
| `arena.c` `foe_life` / `foe_speed` / `foe_harm` | stats for the three types |
| `main.c` `WIN_SECS` | run length (600 s) |
| `cards.c` `cards_take()` | what each card does, and `card_ready()` the caps |
| `cards.c` `REROLL_COST` | the price of a fresh hand |
| `main.c` `FADE_FRAMES` | how many frames each of the four fade steps lasts |

## Out of scope

No permanent progression, no shop, no multiple characters or maps, no bosses and no
achievements, as the GDD asked. GBC support is for later too: the project is set to
`color: off` and the art was made for the four DMG shades.
