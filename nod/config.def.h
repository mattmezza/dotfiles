/* nod - default configuration. copy to config.h and edit. */

/* Position values */
enum { TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT };

/* Appearance */
static const char *font         = "monospace:size=10";
static const int   max_width    = 300;  /* pixels */
static const int   max_lines    = 5;
static const int   padding      = 15;   /* pixels */
static const int   border_width = 2;    /* pixels */
static const int   gap_x        = 10;   /* pixels from screen edge */
static const int   gap_y        = 10;   /* pixels from screen edge */

/* Gap between stacked notifications */
static const int   stack_gap    = 8;    /* pixels */

/* Extra vertical gap between summary (bold title) and body */
static const int   title_gap    = 6;    /* pixels */

/* Position: TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT */
static const int   position     = TOP_RIGHT;

/* Maximum visible notifications at once */
static const int   max_visible  = 3;

/* Colors (hex) per urgency: low, normal, critical */
static const char *bg_colors[]     = { "#1d2021", "#1d2021", "#cc241d" };
static const char *fg_colors[]     = { "#928374", "#ebdbb2", "#1d2021" };
static const char *border_colors[] = { "#928374", "#458588", "#cc241d" };

/* Timeouts in seconds per urgency: low, normal, critical.
 * 0 = do not auto-dismiss (click to close). */
static const int timeouts[] = { 5, 10, 0 };
