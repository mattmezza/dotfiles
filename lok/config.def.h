/* lok configuration.
 * `make` copies this to config.h on first build; edit config.h. */

/* user and group to drop privileges to */
static const char *user  = "matteo";
static const char *group = "matteo";

/* background color for each state */
static const char *colorname[NUMCOLS] = {
	[INIT]      = "#000000",  /* idle, nothing typed yet */
	[INPUT]     = "#005577",  /* typing (alternates with INPUT_ALT) */
	[INPUT_ALT] = "#227799",  /* typing (alternates with INPUT) */
	[FAILED]    = "#cc3333",  /* wrong password */
	[CAPS]      = "#cc6600",  /* caps lock is on */
};

/* emoji for each state; set an entry to "" to show none for that state */
static const char *emoji[NUMCOLS] = {
	[INIT]      = "🐒",
	[INPUT]     = "🙈",
	[INPUT_ALT] = "🙉",
	[FAILED]    = "🙊",
	[CAPS]      = "🐵",
};
static const int showemoji = 1;

/* texts; set any to "" to disable it. CLI flags -t/-s/-b override these */
static const char *titletext  = "This computer is locked";
static const char *subtext    = "Type your password to unlock";
static const char *footertext = "%A %d %B  ·  %H:%M:%S"; /* strftime(3) codes are expanded */
static const char *capstext   = "Caps Lock is on";    /* shown instead of subtext while caps lock is on */
static const char *failformat  = "%d failed attempts"; /* shown instead of subtext after a wrong password */
static const char *failformat1 = "1 failed attempt";   /* same, when there is exactly one */

/* fonts, as pango font strings */
static const char *emojifont  = "Noto Color Emoji 96";
static const char *titlefont  = "Sans Bold 32";
static const char *subfont    = "Sans 14";
static const char *footerfont = "Sans 11";

/* text colors */
static const char *textcolor    = "#eeeeee"; /* title */
static const char *dimtextcolor = "#c5c8c9"; /* subtext and footer */

/* layout, in pixels */
static const int textgap      = 24; /* vertical gap between emoji, title and subtext */
static const int bottommargin = 36; /* footer distance from the bottom edge */

/* show the FAILED color when the input is cleared (Esc, Ctrl-U), like slock */
static const int failonclear = 0;

/* turn the monitor off via DPMS after this many seconds; 0 disables */
static const int monitortime = 0;

/* enable live strftime(3) expansion in each text field;
 * CLI flags -T/-S/-B override these at runtime */
static int title_datetime_updated    = 1;
static int subtitle_datetime_updated = 1;
static int footer_datetime_updated   = 1;
