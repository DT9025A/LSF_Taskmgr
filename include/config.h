#ifndef LSF_CONFIG_H
#define LSF_CONFIG_H

#define TRUE   1
#define FALSE  0

/* ---------------------------------------------------------------
 *  User-configurable settings.
 *  Edit this file and rebuild to apply.  No runtime GUI.
 * --------------------------------------------------------------- */

/* Monospace font used for every drawn string.
 * List candidates with:  xlsfonts | grep -i mono
 * If MONO_FONT cannot be loaded, MONO_FONT_FALLBACK is tried.   */
#define MONO_FONT           "-b&h-lucidatypewriter-bold-r-normal-sans-14-140-75-75-m-90-iso8859-1"
#define MONO_FONT_FALLBACK  "fixed"

/* Extra vertical pixels added to font ascent for line height.   */
#define LINE_SPACING        3

/* Auto-refresh interval in seconds.  0 disables auto-refresh
 * (tab switching always forces an immediate refresh).           */
#define AUTO_REFRESH_JOBS   5
#define AUTO_REFRESH_NODE   5
#define AUTO_REFRESH_RSRC   5
#define AUTO_REFRESH_INFO   0

/* Initial window size (px).                                     */
#define WIN_DEFAULT_W       880
#define WIN_DEFAULT_H       800

/* Scrollbar */
#define SCROLLBAR_WIDTH      14   /* px */
#define SCROLLBAR_MIN_THUMB  20   /* px, minimum thumb height */

/* Confirmation window for bkill (seconds) */
#define KILL_CONFIRM_SECS     5
// use -DKILL_ENABLE
#define KILL_ENABLE           FALSE

#define CPU_USAGE_THRESHOLD   40

#define WINDOW_TITLE               "LSF Task Manager"
#define APPLICATION_VERSION_MAJOR  1
#define APPLICATION_VERSION_MINOR  22

#endif
