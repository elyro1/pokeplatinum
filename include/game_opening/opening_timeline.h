#ifndef POKEPLATINUM_GAME_OPENING_OPENING_TIMELINE_H
#define POKEPLATINUM_GAME_OPENING_OPENING_TIMELINE_H

// Frame numbers (30 per second) of the opening cutscene's global frame counter.
// The cutscene is split into three consecutive scenes: the logo scene, the
// main scene and the closing scene; each one only starts once the previous
// one has finished, but all of them share a single running frame counter.

#define OPENING_END_FRAME 2430

// Logo scene
#define LOGO_FRAME_FIRST_SPARKLES  180
#define LOGO_FRAME_SECOND_SPARKLES 285
#define LOGO_FRAME_FADE_OUT_FIRST  115
#define LOGO_FRAME_SHOW_SECOND     265
#define LOGO_FRAME_FADE_TO_BLACK   490

// Main scene: intro part
#define MAIN_FRAME_FADE_IN_OVERLAY   640
#define MAIN_FRAME_START_PARTICLES   700
#define MAIN_FRAME_FLASH_OUT         785
#define MAIN_FRAME_FLASH_IN          (MAIN_FRAME_FLASH_OUT + 5)
#define MAIN_FRAME_PREPARE_LANDSCAPE 944
#define MAIN_FRAME_SHOW_SUB_OBJ      945
#define MAIN_FRAME_START_LANDSCAPE   975

// Main scene: landscape fly-over
#define LANDSCAPE_FRAME_FADE_OUT_1    1185
#define LANDSCAPE_FRAME_CUT_1         (LANDSCAPE_FRAME_FADE_OUT_1 + 6)
#define LANDSCAPE_FRAME_FADE_OUT_2    1395
#define LANDSCAPE_FRAME_CUT_2         (LANDSCAPE_FRAME_FADE_OUT_2 + 6)
#define LANDSCAPE_FRAME_SLIDE_IN_1    (LANDSCAPE_FRAME_FADE_OUT_2 + 15)
#define LANDSCAPE_FRAME_SLIDE_IN_2    (LANDSCAPE_FRAME_SLIDE_IN_1 + 90)
#define LANDSCAPE_FRAME_SLIDE_OUT     1575
#define LANDSCAPE_FRAME_FLOCK_1       1445
#define LANDSCAPE_FRAME_FLOCK_2       1490
#define LANDSCAPE_FRAME_FLOCK_3       1510
#define LANDSCAPE_FRAME_ZOOM_OUT      1560
#define LANDSCAPE_FRAME_FADE_TO_WHITE 1576
#define LANDSCAPE_FRAME_END           1600

// Main scene: scrolling backgrounds part
#define SCROLL_FRAME_END            1830
#define SCROLL_FRAME_FADE_OUT       1845
#define SCROLL_FRAME_SPIN_START     1860
#define SCROLL_FRAME_SLIDE_OUT      1920
#define SCROLL_FRAME_SLIDE_OUT_2    (SCROLL_FRAME_SLIDE_OUT + 8)
#define SCROLL_FRAME_SLIDE_OUT_3    (SCROLL_FRAME_SLIDE_OUT + 14)
#define SCROLL_FRAME_SLIDE_OUT_4    (SCROLL_FRAME_SLIDE_OUT + 20)
#define SCROLL_FRAME_STARTERS_START 2010
#define SCROLL_FRAME_STARTER_2      2130
#define SCROLL_FRAME_FINAL_SLIDE_IN 2215

// Closing scene
#define CLOSING_FRAME_FADE_IN_WHITE 2205
#define CLOSING_FRAME_FADE_OUT      2420

#endif // POKEPLATINUM_GAME_OPENING_OPENING_TIMELINE_H
