/* nod - default configuration. copy to config.h and edit. */

/* Position values */
enum { TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT };

/* Appearance */
static const char *font         = "monospace:size=10";
static const int   max_width    = 500;  /* pixels */
static const int   max_lines    = 5;
static const int   padding      = 12;   /* pixels */
static const int   border_width = 2;    /* pixels */
static const int   border_radius = 0;   /* rounded corners via X Shape; 0 = square. set border_width = 0 for clean edges */
static const int   gap_x        = 13;   /* pixels from screen edge */
static const int   gap_y        = 38;   /* pixels from screen edge */

/* Gap between stacked notifications */
static const int   stack_gap    = 8;    /* pixels */

/* Extra vertical gap between summary (bold title) and body */
static const int   title_gap    = 6;    /* pixels */

/* Position: TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT */
static const int   position     = TOP_RIGHT;

/* Maximum visible notifications at once */
static const int   max_visible  = 5;

/* Colors (hex) per urgency: low, normal, critical */
static const char *bg_colors[]     = { "#1d2021", "#1d2021", "#cc241d" };
static const char *fg_colors[]     = { "#928374", "#ebdbb2", "#1d2021" };
static const char *border_colors[] = { "#928374", "#458588", "#cc241d" };

/* Load colors and font from X resources at startup.
 * Falls back to the values above for any resource that is not set.
 *
 * Supported resource names (examples):
 *   nod.font:              Noto Sans:size=10
 *   nod.background.low:    #1d2021
 *   nod.background.normal: #1d2021
 *   nod.background.critical: #cc241d
 *   nod.foreground.low:    #928374
 *   nod.foreground.normal: #ebdbb2
 *   nod.foreground.critical: #1d2021
 *   nod.border.low:        #928374
 *   nod.border.normal:     #458588
 *   nod.border.critical:   #cc241d
 */
static const int use_xresources = 1;

/* Poll interval (ms) for detecting X resource changes at runtime.
 * 0 disables polling; send SIGUSR1 to force a reload. */
static const int xresources_poll_ms = 5000;

/* Timeouts in seconds per urgency: low, normal, critical.
 * 0 = do not auto-dismiss (click to close). */
static const int timeouts[] = { 5, 10, 0 };
