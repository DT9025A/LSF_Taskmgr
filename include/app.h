#ifndef LSF_APP_H
#define LSF_APP_H
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include "util.h"


enum { MODE_JOBS, MODE_NODE, MODE_RSRC, MODE_INFO, MODE_COUNT };

typedef struct {
    Display *dpy;
    int      screen;
    Window   win;
    GC       gc;
    XFontStruct *font;
    Atom     wm_delete;

    int      width, height;
    int      running;
    int      mode;
    LineArray lines;
    int      scroll;
    time_t   last_collect;
    int      collect_pending;

    char     input[512];
    char    *info_ids;                 /* only used by MODE_INFO */

    int      tab_x[MODE_COUNT], tab_y, tab_w[MODE_COUNT], tab_h;
    unsigned long pixels[C_MAX];
    int      ascent;
    int      line_h;
    char     collect_time[32];

    /* --- new --- */
    int      content_y0;      /* y of first content row (set each render) */
    int      visible_lines;   /* rows currently visible          (set each render) */

    /* selection */
    int      selected_line;   /* index into lines, -1 = none */

    /* pending kill confirmation */
    char    *pending_kill_job;
    time_t   pending_kill_time;

    /* transient status message (bottom bar) */
    char     status_msg[256];
    time_t   status_msg_time;

    /* scrollbar geometry (recomputed each render) */
    int      sb_x, sb_width;
    int      sb_track_y0, sb_track_y1;
    int      sb_thumb_y, sb_thumb_h;
    int      sb_visible;
    int      sb_dragging;
    int      sb_drag_offset;   /* px offset between mouse and thumb top */
} App;

int app_run(void);

#endif
