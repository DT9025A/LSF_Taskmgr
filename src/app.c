#include "app.h"
#include "config.h"
#include "views.h"
#include "args.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

static const char *COLOR_NAMES[C_MAX] = {
    "#000000", "#ffffff", "#cc0000", "#00aa00", "#0000cc",
    "#aa00cc", "#777777", "#2a2a3f", "#f5e6a8"
};

/* --------------------------------------------------------------- */
static void draw_text(App *a, int x, int y, const char *s, int color) {
    XSetForeground(a->dpy, a->gc, a->pixels[color]);
    char *buf = xstrdup(s);
    if (buf) {
        for (char *p = buf; *p; p++)
            if ((unsigned char)*p < 32 || (unsigned char)*p >= 127) *p = '?';
        XDrawString(a->dpy, a->win, a->gc, x, y + a->ascent, buf, (int)strlen(buf));
        free(buf);
    }
}

/* ---------------------------------------------------------------
 *  Content
 * --------------------------------------------------------------- */
static void render_content(App *a) {
    int start_y = 44;
    int line_h  = a->line_h;

    if (a->mode == MODE_INFO) {
        XSetForeground(a->dpy, a->gc, a->pixels[C_SEL]);
        XFillRectangle(a->dpy, a->win, a->gc, 0, 36, a->width, 26);
        char buf[600];
        snprintf(buf, sizeof(buf), "Job IDs: %s_", a->input);
        draw_text(a, 8, 40, buf, C_BLACK);
        start_y = 72;
    }
    a->content_y0 = start_y;

    int max_lines = (a->height - 24 - start_y) / line_h;
    if (max_lines < 1) max_lines = 1;
    a->visible_lines = max_lines;

    int total      = a->lines.count;
    int max_scroll = total - max_lines;
    if (max_scroll < 0) max_scroll = 0;
    if (a->scroll > max_scroll) a->scroll = max_scroll;
    if (a->scroll < 0)          a->scroll = 0;

/*
    for (int i = 0; i < max_lines && i + a->scroll < total; i++) {
        int idx = i + a->scroll;
        Line *l = &a->lines.items[idx];
        int y = start_y + i * line_h;

        if (idx == a->selected_line) {
            XSetForeground(a->dpy, a->gc, a->pixels[C_SEL]);
            XFillRectangle(a->dpy, a->win, a->gc, 0, y, a->width, line_h);
            draw_text(a, 8, y, l->text, C_BLACK);
        } else {
            draw_text(a, 8, y, l->text, l->color);
        }
    }
*/

    for (int i = 0; i < max_lines && i + a->scroll < total; i++) {
        int idx = i + a->scroll;
        Line *l = &a->lines.items[idx];
        int y = start_y + i * line_h;

        int is_sel = (idx == a->selected_line);
        if (is_sel) {
            XSetForeground(a->dpy, a->gc, a->pixels[C_SEL]);
            XFillRectangle(a->dpy, a->win, a->gc, 0, y, a->width, line_h);
        }

        int x = 8;
        for (int s = 0; s < l->nsegs; s++) {
            Segment *sg = &l->segs[s];
            /* selected row: force black text on the highlight */
            draw_text(a, x, y, sg->text, is_sel ? C_BLACK : sg->color);
            /* advance x using the actual pixel width of this segment */
            x += XTextWidth(a->font, sg->text, (int)strlen(sg->text));
        }
    }
}

/* ---------------------------------------------------------------
 *  Scrollbar (drawn on top of content)
 * --------------------------------------------------------------- */
static void render_scrollbar(App *a) {
    int total   = a->lines.count;
    int visible = a->visible_lines;

    if (visible <= 0 || total <= visible) { a->sb_visible = 0; return; }

    a->sb_width = SCROLLBAR_WIDTH;
    a->sb_x     = a->width - a->sb_width - 2;

    a->sb_track_y0 = a->content_y0;
    a->sb_track_y1 = a->height - 24 - 4;
    int track_h = a->sb_track_y1 - a->sb_track_y0;
    if (track_h <= 0) { a->sb_visible = 0; return; }

    int max_scroll = total - visible;
    int thumb_h = (int)((long)track_h * visible / total);
    if (thumb_h < SCROLLBAR_MIN_THUMB) thumb_h = SCROLLBAR_MIN_THUMB;
    if (thumb_h > track_h) thumb_h = track_h;

    int thumb_y = a->sb_track_y0;
    if (max_scroll > 0)
        thumb_y += (int)((long)(track_h - thumb_h) * a->scroll / max_scroll);

    a->sb_thumb_y = thumb_y;
    a->sb_thumb_h = thumb_h;
    a->sb_visible = 1;

    /* track */
    XSetForeground(a->dpy, a->gc, a->pixels[C_GRAY]);
    XFillRectangle(a->dpy, a->win, a->gc,
                   a->sb_x, a->sb_track_y0, a->sb_width, track_h);

    /* thumb */
    int thumb_col = a->sb_dragging ? C_BLUE : C_BAR;
    XSetForeground(a->dpy, a->gc, a->pixels[thumb_col]);
    XFillRectangle(a->dpy, a->win, a->gc,
                   a->sb_x, thumb_y, a->sb_width, thumb_h);
}

/* ---------------------------------------------------------------
 *  Full frame
 * --------------------------------------------------------------- */
static void render(App *a) {
    XClearWindow(a->dpy, a->win);

    XSetForeground(a->dpy, a->gc, a->pixels[C_BAR]);
    XFillRectangle(a->dpy, a->win, a->gc, 0, 0, a->width, 36);

    const char *tab_names[MODE_COUNT] = {
        "1 Jobs", "2 Nodes", "3 Resources", "4 Job Info"
    };
    int x = 8;
    a->tab_y = 4;
    a->tab_h = 28;
    for (int i = 0; i < MODE_COUNT; i++) {
        int w = (int)strlen(tab_names[i]) * 8 + 18;
        a->tab_x[i] = x;
        a->tab_w[i] = w;
        if (a->mode == i) {
            XSetForeground(a->dpy, a->gc, a->pixels[C_WHITE]);
            XFillRectangle(a->dpy, a->win, a->gc, x, a->tab_y, w, a->tab_h);
            draw_text(a, x + 8, 8, tab_names[i], C_BAR);
        } else {
            draw_text(a, x + 8, 8, tab_names[i], C_WHITE);
        }
        x += w + 4;
    }

    if (a->collect_time[0]) {
        char tbuf[64];
        snprintf(tbuf, sizeof(tbuf), "Data: %s", a->collect_time);
        int tw = XTextWidth(a->font, tbuf, (int)strlen(tbuf));
        draw_text(a, a->width - tw - 12, 8, tbuf, C_WHITE);
    }

    render_content(a);
    render_scrollbar(a);

    /* status bar */
    XSetForeground(a->dpy, a->gc, a->pixels[C_BAR]);
    XFillRectangle(a->dpy, a->win, a->gc, 0, a->height - 24, a->width, 24);

    char sbuf[256];
    const char *hint;
    time_t now = time(NULL);

    
    if (a->pending_kill_job && now - a->pending_kill_time < KILL_CONFIRM_SECS) {
        if (strcmp(a->pending_kill_job, "*ALL*") == 0) {
            snprintf(sbuf, sizeof(sbuf),
                     "Press SHIFT+K again to CONFIRM killing ALL jobs");
        } else {
            snprintf(sbuf, sizeof(sbuf),
                     "Press k again to CONFIRM kill of job %s",
                     a->pending_kill_job);
        }
        hint = sbuf;
    } else if (a->status_msg[0] && now - a->status_msg_time < KILL_CONFIRM_SECS) {
        hint = a->status_msg;
    } else if (a->selected_line >= 0 && a->selected_line < a->lines.count &&
               a->lines.items[a->selected_line].job_id) {
        snprintf(sbuf, sizeof(sbuf), "Selected job %s  (k=kill, l=locate)",
                 a->lines.items[a->selected_line].job_id);
        hint = sbuf;
    } else if (a->mode == MODE_INFO) {
        hint = "Type job IDs, Enter to query, Esc to clear";
    } else {
        static char hintbuf[256];
        snprintf(hintbuf, sizeof(hintbuf),
                 "1-4 switch, r refresh, q quit, "
                 "click row to select, k kill, l locate"
                 "%s",
                 (appConfig.kill_config & KILL_ALL_ENABLE) ? ", SHIFT+K kill all" : "");
        hint = hintbuf;
    }
    draw_text(a, 8, a->height - 20, hint, C_WHITE);

    XFlush(a->dpy);
}

/* ---------------------------------------------------------------
 *  Collection (keeps selection across refresh via job_id remap)
 * --------------------------------------------------------------- */
static void collect(App *a) {
    time_t now = time(NULL);
    struct tm tm_now; localtime_r(&now, &tm_now);
    strftime(a->collect_time, sizeof(a->collect_time),
             "%Y-%m-%d %H:%M:%S", &tm_now);
    a->last_collect = now;

    /* remember selection by job_id */
    char *prev_job = NULL;
    if (a->selected_line >= 0 && a->selected_line < a->lines.count) {
        const char *jid = a->lines.items[a->selected_line].job_id;
        if (jid) prev_job = xstrdup(jid);
    }

    la_free(&a->lines);
    la_init(&a->lines);
    a->selected_line = -1;
    a->sb_dragging   = 0;

    switch (a->mode) {
        case MODE_JOBS: collect_jobs(&a->lines);                 break;
        case MODE_NODE: collect_nodes(&a->lines);                break;
        case MODE_RSRC: collect_rsrc(&a->lines);                 break;
        case MODE_INFO: collect_info(&a->lines, a->info_ids);    break;
    }

    if (prev_job) {
        for (int i = 0; i < a->lines.count; i++) {
            if (a->lines.items[i].job_id &&
                strcmp(a->lines.items[i].job_id, prev_job) == 0) {
                a->selected_line = i;
                break;
            }
        }
        free(prev_job);
    }
}

/* --------------------------------------------------------------- */
static void clear_pending_kill(App *a) {
    free(a->pending_kill_job);
    a->pending_kill_job = NULL;
}

static void set_mode(App *a, int m) {
    if (a->mode == m) { a->collect_pending = 1; return; }
    a->mode = m;
    a->scroll = 0;
    a->selected_line = -1;
    clear_pending_kill(a);
    a->collect_pending = 1;
}

/* ---------------------------------------------------------------
 *  Key handling
 * --------------------------------------------------------------- */
static void do_kill(App *a) {

    if (!appConfig.kill_config) {
        snprintf(a->status_msg, sizeof(a->status_msg),
                 "Kill function disabled. Restart with -k to enable.");
        a->status_msg_time = time(NULL);
        return;
    }

    if (a->selected_line < 0 || a->selected_line >= a->lines.count) {
        snprintf(a->status_msg, sizeof(a->status_msg), "No row selected.");
        a->status_msg_time = time(NULL);
        return;
    }
    Line *l = &a->lines.items[a->selected_line];
    if (!l->job_id) {
        snprintf(a->status_msg, sizeof(a->status_msg),
                 "Selected line is not a job.");
        a->status_msg_time = time(NULL);
        return;
    }
    
    time_t now = time(NULL);

    if (a->pending_kill_job &&
        strcmp(a->pending_kill_job, l->job_id) == 0 &&
        now - a->pending_kill_time < KILL_CONFIRM_SECS) {
        /* confirmed -> run bkill */
        if (appConfig.kill_config) {
            char cmd[160];
            snprintf(cmd, sizeof(cmd), "bkill %s", l->job_id);
            printf ("Kill command execute: \"%s\"\n", cmd);
            char *out = run_cmd(cmd);
            snprintf(a->status_msg, sizeof(a->status_msg),
                     "bkill %s -> %s",
                     l->job_id, (out && *out) ? out : "sent");
            printf ("-> %s\n", out);
            free(out);
        }
        clear_pending_kill(a);
        a->status_msg_time = time(NULL);
        a->selected_line   = -1;
        a->collect_pending = 1;
    } else {
        /* first press -> ask for confirmation */
        clear_pending_kill(a);
        a->pending_kill_job  = xstrdup(l->job_id);
        a->pending_kill_time = now;
        a->status_msg[0] = 0;
    }
}

static void do_kill_all(App *a) {

    if (!(appConfig.kill_config & KILL_ALL_ENABLE)) {
        snprintf(a->status_msg, sizeof(a->status_msg),
                 "Kill-all function disabled. Restart with -ka to enable.");
        a->status_msg_time = time(NULL);
        return;
    }

    time_t now = time(NULL);
    if (a->pending_kill_job &&
        strcmp(a->pending_kill_job, "*ALL*") == 0 &&
        now - a->pending_kill_time < KILL_CONFIRM_SECS) {
        /* confirmed -> run bkill 0 (kill all jobs owned by this user) */
        char *out = run_cmd("bkill 0 2>&1");
        printf ("Kill-all command execute: \"bkill 0 2>&1\"\n");
        snprintf(a->status_msg, sizeof(a->status_msg),
                 "bkill 0 -> All jobs will be killed.");
        free(out);
        clear_pending_kill(a);
        a->status_msg_time = time(NULL);
        a->selected_line   = -1;
        a->collect_pending = 1;
    } else {
        clear_pending_kill(a);
        a->pending_kill_job  = xstrdup("*ALL*");
        a->pending_kill_time = now;
        a->status_msg[0] = 0;
    }
}

static void do_locate(App *a) {
    if (a->selected_line < 0 || a->selected_line >= a->lines.count) {
        snprintf(a->status_msg, sizeof(a->status_msg), "No row selected.");
        a->status_msg_time = time(NULL);
        return;
    }
    Line *l = &a->lines.items[a->selected_line];
    if (!l->job_id) {
        snprintf(a->status_msg, sizeof(a->status_msg),
                 "Selected line is not a job.");
        a->status_msg_time = time(NULL);
        return;
    }
    /* jump to Job Info tab and pre-fill the job id */
    free(a->info_ids);
    a->info_ids = xstrdup(l->job_id);
    a->mode = MODE_INFO;
    a->scroll = 0;
    a->selected_line = -1;
    clear_pending_kill(a);
    a->collect_pending = 1;
}

static void on_key(App *a, XKeyEvent *e) {
    char buf[16]; KeySym ks;
    int n = XLookupString(e, buf, sizeof(buf) - 1, &ks, NULL);
    buf[n] = 0;

    if (ks == XK_Escape) {
        a->input[0] = 0;
        clear_pending_kill(a);
        a->collect_pending = 1;
        return;
    }

    if (a->mode == MODE_INFO) {
        /* text-entry mode: no shortcut keys */
        if (ks == XK_Return || ks == XK_KP_Enter) {
            free(a->info_ids);
            a->info_ids = xstrdup(a->input);
            a->input[0] = 0;
            a->scroll = 0;
            a->collect_pending = 1;
            return;
        }
        if (ks == XK_BackSpace) {
            int len = (int)strlen(a->input);
            if (len > 0) a->input[len - 1] = 0;
            a->collect_pending = 1;
            return;
        }
        if (n > 0 && (unsigned char)buf[0] >= 32 && (unsigned char)buf[0] < 127) {
            int len = (int)strlen(a->input);
            if (len < (int)sizeof(a->input) - 2) {
                a->input[len]     = buf[0];
                a->input[len + 1] = 0;
            }
            a->collect_pending = 1;
            return;
        }
        if (ks == XK_Up)   { if (a->scroll > 0) a->scroll--; render(a); return; }
        if (ks == XK_Down) { a->scroll++; render(a); return; }
        return;
    }

    /* navigation */
    if (ks == XK_Up || ks == XK_Down) {
        if (a->selected_line >= 0) {
            int d = (ks == XK_Up) ? -1 : 1;
            int nl = a->selected_line + d;
            if (nl >= 0 && nl < a->lines.count) {
                a->selected_line = nl;
                /* keep selection in view */
                if (nl < a->scroll) a->scroll = nl;
                if (nl >= a->scroll + a->visible_lines)
                    a->scroll = nl - a->visible_lines + 1;
            }
        } else {
            if (ks == XK_Up) { if (a->scroll > 0) a->scroll--; }
            else             { a->scroll++; }
        }
        clear_pending_kill(a);
        render(a);
        return;
    }
    if (ks == XK_Page_Up) {
        a->scroll -= 20; if (a->scroll < 0) a->scroll = 0;
        clear_pending_kill(a); render(a); return;
    }
    if (ks == XK_Page_Down) {
        a->scroll += 20; clear_pending_kill(a); render(a); return;
    }

    /* single-char commands */
    if (n >= 1) {
        switch (buf[0]) {
        case '1': set_mode(a, MODE_JOBS); return;
        case '2': set_mode(a, MODE_NODE); return;
        case '3': set_mode(a, MODE_RSRC); return;
        case '4': set_mode(a, MODE_INFO); return;
        case 'r': case 'R':
            clear_pending_kill(a);
            a->collect_pending = 1; return;
        case 'q': case 'Q': a->running = 0; return;
        case 'k': do_kill(a);     render(a); return;
        case 'K': do_kill_all(a); render(a); return;
        case 'l': case 'L': do_locate(a); render(a); return;
        }
    }
}

/* ---------------------------------------------------------------
 *  Mouse
 * --------------------------------------------------------------- */
static int y_to_line(App *a, int y) {
    if (a->visible_lines <= 0) return -1;
    if (y < a->content_y0) return -1;
    int rel = (y - a->content_y0) / a->line_h;
    int idx = a->scroll + rel;
    if (idx < 0 || idx >= a->lines.count) return -1;
    return idx;
}

static void on_click(App *a, XButtonEvent *e) {
    int x = e->x, y = e->y;

    /* wheel */
    if (e->button == Button4) {
        a->scroll -= 3; if (a->scroll < 0) a->scroll = 0;
        render(a); return;
    }
    if (e->button == Button5) { a->scroll += 3; render(a); return; }

    if (e->button != Button1) return;

    /* tab bar */
    for (int i = 0; i < MODE_COUNT; i++) {
        if (x >= a->tab_x[i] && x <= a->tab_x[i] + a->tab_w[i] &&
            y >= a->tab_y && y <= a->tab_y + a->tab_h) {
            set_mode(a, i);
            return;
        }
    }

    /* scrollbar */
    if (a->sb_visible && x >= a->sb_x && x < a->sb_x + a->sb_width &&
        y >= a->sb_track_y0 && y < a->sb_track_y1) {
        if (y >= a->sb_thumb_y && y < a->sb_thumb_y + a->sb_thumb_h) {
            a->sb_dragging = 1;
            a->sb_drag_offset = y - a->sb_thumb_y;
        } else if (y < a->sb_thumb_y) {
            a->scroll -= a->visible_lines;
            if (a->scroll < 0) a->scroll = 0;
        } else {
            a->scroll += a->visible_lines;
        }
        render(a);
        return;
    }

    /* content area: select the row */
    if (y >= a->content_y0) {
        int idx = y_to_line(a, y);
        if (idx >= 0 && a->lines.items[idx].job_id) {
            a->selected_line = idx;
        } else {
            a->selected_line = -1;
        }
        clear_pending_kill(a);
        render(a);
    }
}

static void on_motion(App *a, XMotionEvent *e) {
    if (!a->sb_dragging || !a->sb_visible) return;

    int track_h = a->sb_track_y1 - a->sb_track_y0;
    int max_scroll = a->lines.count - a->visible_lines;
    if (max_scroll < 0) max_scroll = 0;
    if (max_scroll == 0 || track_h <= a->sb_thumb_h) return;

    int new_top = e->y - a->sb_drag_offset;
    int lo = a->sb_track_y0;
    int hi = a->sb_track_y1 - a->sb_thumb_h;
    if (new_top < lo) new_top = lo;
    if (new_top > hi) new_top = hi;

    a->scroll = (int)((long)(new_top - a->sb_track_y0) * max_scroll /
                      (track_h - a->sb_thumb_h));
    render(a);
}

static void on_release(App *a, XButtonEvent *e) {
    if (e->button == Button1) {
        a->sb_dragging = 0;
        render(a);
    }
}

/* ---------------------------------------------------------------
 *  Main loop
 * --------------------------------------------------------------- */
int app_run(void) {
    App a; 
    char title[64];
    
    memset(&a, 0, sizeof(a));
    a.mode = MODE_JOBS;
    a.running = 1;
    a.width  = WIN_DEFAULT_W;
    a.height = WIN_DEFAULT_H;
    a.selected_line = -1;
    a.content_y0 = 44;
    la_init(&a.lines);

    a.dpy = XOpenDisplay(NULL);
    if (!a.dpy) { fprintf(stderr, "Cannot open X display.\n"); return 1; }
    a.screen = DefaultScreen(a.dpy);

    a.win = XCreateSimpleWindow(a.dpy, RootWindow(a.dpy, a.screen),
                                50, 50, (unsigned)a.width, (unsigned)a.height, 0,
                                BlackPixel(a.dpy, a.screen),
                                WhitePixel(a.dpy, a.screen));
    
    snprintf (title, sizeof(title), "%s v%d.%d%s", WINDOW_TITLE, 
        APPLICATION_VERSION_MAJOR, APPLICATION_VERSION_MINOR, appConfig.kill_config ? ((appConfig.kill_config & KILL_ENABLE) ? " [+Kill]" : " [+KillAll]") : "");
    
    XStoreName(a.dpy, a.win, title);
    XSelectInput(a.dpy, a.win,
                 ExposureMask | KeyPressMask |
                 ButtonPressMask | ButtonReleaseMask |
                 PointerMotionMask | StructureNotifyMask);

    a.wm_delete = XInternAtom(a.dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(a.dpy, a.win, &a.wm_delete, 1);

    a.gc = XCreateGC(a.dpy, a.win, 0, NULL);
    a.font = XLoadQueryFont(a.dpy, appConfig.font);
    if (!a.font) {
        fprintf(stderr, "Warning: font '%s' not found, fallback '%s'.\n",
                MONO_FONT, MONO_FONT_FALLBACK);
        a.font = XLoadQueryFont(a.dpy, MONO_FONT_FALLBACK);
    }
    if (!a.font) { fprintf(stderr, "Error: no usable font.\n"); return 1; }
    XSetFont(a.dpy, a.gc, a.font->fid);
    a.ascent = a.font->ascent;
    a.line_h = a.ascent + LINE_SPACING;
    if (a.line_h < 12) a.line_h = 12;

    Colormap cmap = DefaultColormap(a.dpy, a.screen);
    XColor col;
    for (int i = 0; i < C_MAX; i++) {
        if (XParseColor(a.dpy, cmap, COLOR_NAMES[i], &col) &&
            XAllocColor(a.dpy, cmap, &col)) a.pixels[i] = col.pixel;
        else a.pixels[i] = (i == C_BLACK) ? BlackPixel(a.dpy, a.screen)
                                          : WhitePixel(a.dpy, a.screen);
    }

    XMapWindow(a.dpy, a.win);
    XFlush(a.dpy);

    a.collect_pending = 1;
    a.last_collect = 0;

    int xfd = ConnectionNumber(a.dpy);
    while (a.running) {
        while (XPending(a.dpy)) {
            XEvent ev; XNextEvent(a.dpy, &ev);
            switch (ev.type) {
            case Expose:
                if (ev.xexpose.count == 0) render(&a);
                break;
            case ConfigureNotify:
                if (ev.xconfigure.width != a.width ||
                    ev.xconfigure.height != a.height) {
                    a.width  = ev.xconfigure.width;
                    a.height = ev.xconfigure.height;
                    render(&a);
                }
                break;
            case KeyPress:      on_key(&a, &ev.xkey);           break;
            case ButtonPress:   on_click(&a, &ev.xbutton);      break;
            case ButtonRelease: on_release(&a, &ev.xbutton);    break;
            case MotionNotify:  on_motion(&a, &ev.xmotion);     break;
            case ClientMessage:
                if ((Atom)ev.xclient.data.l[0] == a.wm_delete) a.running = 0;
                break;
            }
        }
        if (!a.running) break;

        time_t now = time(NULL);
        int auto_iv = 0;
        switch (a.mode) {
            case MODE_JOBS: auto_iv = appConfig.jobs_referesh_interval; break;
            case MODE_NODE: auto_iv = appConfig.node_referesh_interval; break;
            case MODE_RSRC: auto_iv = appConfig.rsrc_referesh_interval; break;
            case MODE_INFO: auto_iv = AUTO_REFRESH_INFO; break;
        }
        int do_refresh = a.collect_pending
                      || (auto_iv > 0 && !a.sb_dragging &&
                          now - a.last_collect >= auto_iv);
        if (do_refresh) { a.collect_pending = 0; collect(&a); render(&a); }

        fd_set fds; FD_ZERO(&fds); FD_SET(xfd, &fds);
        struct timeval tv; tv.tv_sec = 0; tv.tv_usec = 100000;
        select(xfd + 1, &fds, NULL, NULL, &tv);
    }

    la_free(&a.lines);
    free(a.info_ids);
    clear_pending_kill(&a);
    XFreeFont(a.dpy, a.font);
    XFreeGC(a.dpy, a.gc);
    XDestroyWindow(a.dpy, a.win);
    XCloseDisplay(a.dpy);
    return 0;
}

