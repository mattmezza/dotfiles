/* mwm configuration — copied to config.h on first build, then edit freely.
 * Recompile (make) and restart mwm to apply changes (suckless style). */

/* ---- appearance ------------------------------------------------------ */
static unsigned int borderpx         = 3;    /* client border width in pixels   */
static unsigned int gappx            = 7;   /* gap between/around windows      */
static const int roundcorners        = 0;    /* round client corners via X Shape;
                                                set 0 if your compositor already
                                                rounds corners (picom etc.)     */
static int cornerradius              = 3;   /* client corner radius (px); needs roundcorners=1.
                                                mwm rounds via X Shape (no compositor needed) —
                                                set to 0 only if a compositor does the rounding */
static const int resizehints         = 0;    /* 1 = honour client size hints when tiling */
static const int focusfollowsmouse   = 1;    /* 1 = focus the window under the pointer   */
static const int warpcursor          = 1;    /* 1 = warp pointer to a monitor on CLI focus/move
                                                (keeps CLI focus authoritative with focusfollowsmouse) */
static const unsigned int movemod    = Mod4Mask; /* hold + drag a window: Btn1 move, Btn3 resize
                                                (Mod4=Super, Mod1=Alt). Auto-floats the window. */
static const unsigned int snap       = 8;    /* edge snap distance in px when mouse-moving */

/* fonts (first one drives the bar height; "mwm.font" in xrdb overrides #0) */
static char *fonts[] = { "monospace:size=10" };

/* ---- layouts --------------------------------------------------------- */
/* 0 = cols "|||"  (equal full-height columns)
 * 1 = stack "||=" (one master column + stacked rows)
 * 2 = float "><>" */
static const char *layoutsym[] = { "|||", "||=", "><>" };
static const int defaultlayout  = 1;
static const unsigned int nmaster = 1;       /* clients in the master area (stack layout) */
static float mfact                = 0.50f;   /* master area width fraction               */

/* ---- bar ------------------------------------------------------------- */
static const int topbar       = 1;    /* 1 = bar on top, 0 = bottom         */
static const int showbar      = 1;    /* 1 = show bar at startup            */
static const int barheight    = 0;    /* 0 = auto (font height + 2*padding) */
static int barvertpad         = 3;    /* vertical padding inside the bar    */
static int barpadx            = 10;   /* horizontal text padding in pills   */
static int barmargin          = 7;   /* horizontal gap: screen left/right edge to the pills */
static int barvmargin         = 5;    /* vertical gap: top/bottom screen edge to the bar      */
static int barseggap          = 8;    /* gap between left/center/right pills */
static int segradius          = 3;    /* pill corner radius (0 = square)    */
static const int titlealign   = 0;    /* center pill (title) justify: 0=left, 1=center, 2=right */

/* ---- system tray ----------------------------------------------------- */
static const int showsystray            = 1;  /* embed an XEmbed system tray        */
static const int showtraybar            = 0;  /* 1 = show the tray at startup
                                                 (needs showsystray; toggle live with
                                                 `mwmc traybar toggle`)             */
static const int systraymon             = 0;  /* monitor index whose bar hosts it   */
static const unsigned int systrayspacing = 4; /* gap between tray icons             */
static const int systrayvertpad         = 4;  /* vertical inset of icons in the bar */

/* ---- colors ---------------------------------------------------------- *
 * Each scheme is { foreground, background, border }.
 * The background doubles as the pill / highlight fill; its alpha controls
 * translucency. Set a background alpha to 0 for a "no background" segment.
 * Real translucency needs a compositor (e.g. picom); rounded corners do not. */
static char *colors[][3] = {
	/*                 fg          bg(=pill)   border    */
	[SchemeNorm]   = { "#c0caf5", "#1a1b26", "#3b4261" }, /* left pill + normal tags */
	[SchemeSel]    = { "#1a1b26", "#7aa2f7", "#7aa2f7" }, /* selected tag + focused border */
	[SchemeUrg]    = { "#1a1b26", "#f7768e", "#f7768e" }, /* urgent tag */
	[SchemeTitle]  = { "#c0caf5", "#1a1b26", "#000000" }, /* center title pill */
	[SchemeStatus] = { "#c0caf5", "#1a1b26", "#000000" }, /* right status pill */
	[SchemeBar]    = { "#c0caf5", "#16161e", "#000000" }, /* (unused: bar is shaped to its pills, so the gaps between pills show the wallpaper — there is no bar-wide background) */
};
static unsigned int alphas[][3] = {
	/*                 fg      bg      border */
	[SchemeNorm]   = { OPAQUE, 0xc0,   OPAQUE },
	[SchemeSel]    = { OPAQUE, 0xe0,   OPAQUE },
	[SchemeUrg]    = { OPAQUE, 0xe0,   OPAQUE },
	[SchemeTitle]  = { OPAQUE, 0xa0,   OPAQUE },
	[SchemeStatus] = { OPAQUE, 0xa0,   OPAQUE },
	[SchemeBar]    = { OPAQUE, 0x00,   OPAQUE }, /* (unused: see SchemeBar note above — the bar
	                                               is shaped to its pills, so there is no bar-wide
	                                               background to tint) */
};

/* ---- tags ------------------------------------------------------------ *
 * One global pool, shared across monitors (max 100). These are the NAMES of
 * the first N tags; tags beyond this list still exist and show their number.
 * Names may be words, not just digits, e.g.:
 *     static const char *tags[] = { "web", "code", "term", "chat", "media" };
 * Address a tag by name (mwmc focus tag code) OR by 1-based index
 * (mwmc focus tag 2 -> the 2nd tag), so numeric sxhkd bindings keep working. */
static const char *tags[] = { "w1", "w2", "w3", "w4", "m1", "m2", "p4", "p3", "p2", "p1" };

/* ---- window rules ---------------------------------------------------- *
 * Match by class / instance (WM_CLASS) and/or title substring, and/or window
 * type. class/instance/title matching is a CASE-INSENSITIVE substring, so
 * "pinentry" matches res_class "Pinentry-gtk". A NULL match field matches
 * anything. `tag` is a tag NAME or 1-based
 * number (NULL/"" = keep current tag). `wintype` matches _NET_WM_WINDOW_TYPE_*
 * — write it as WTYPE "DIALOG" etc. Later matching rules win per field. */
#define WTYPE "_NET_WM_WINDOW_TYPE_"
static const Rule rules[] = {
    /* class                instance  title                         tag    wintype          isfloating */
	{ "pavucontrol",        NULL,     NULL,                         NULL,  NULL,            1 },
	{ "arandr",             NULL,     NULL,                         NULL,  NULL,            1 },
	{ "st-256color",        NULL,     "termfloat",                  NULL,  NULL,            1 },
	{ "st-256color",        NULL,     "calcfloat",                  NULL,  NULL,            1 },
	{ NULL,                 NULL,     "is sharing your screen",     NULL,  NULL,            1 },
	{ "chromium-personal",  NULL,     NULL,                         "p1",  NULL,            0 },
	{ "chromium-work",      NULL,     NULL,                         "w1",  NULL,            0 },
	{ "Pinentry",           NULL,     NULL,                         NULL,  NULL,            1 },
	/* catch-all: float any dialog/utility/toolbar/splash from any app */
	{ NULL,                 NULL,     NULL,                         NULL,  WTYPE "DIALOG",  1 },
	{ NULL,                 NULL,     NULL,                         NULL,  WTYPE "UTILITY", 1 },
	{ NULL,                 NULL,     NULL,                         NULL,  WTYPE "TOOLBAR", 1 },
	{ NULL,                 NULL,     NULL,                         NULL,  WTYPE "SPLASH",  1 },
};

/* ---- tag -> monitor rules -------------------------------------------- *
 * Default owner monitor for a tag when that monitor (by index) appears.
 * A monitor with no matching rule starts owning NO tags (blank) until you
 * `mwmc focus tag <name>` (which claims it) or `mwmc move tag <mon>`.
 * The first entry bootstraps a workspace on monitor 0; remove it for a
 * fully blank start. */
static const TagRule tagrules[] = {
	/* tag(index)  mon(index) */
	{ 0, 0 },
	/* { 1, 1 }, */
};

/* ---- Xresources styling ---------------------------------------------- *
 * Override the style variables above from xrdb / ~/.Xresources, WITHOUT
 * recompiling. Resources are namespaced "mwm.*"; load order is mwm startup,
 * so run `xrdb -merge ~/.Xresources` before launching mwm. Example:
 *
 *     mwm.font:       JetBrains Mono:size=11
 *     mwm.normbg:     #1a1b26
 *     mwm.selbg:      #7aa2f7
 *     mwm.statusbgalpha: 0xa0
 *     mwm.gappx:      12
 *     mwm.cornerradius: 8
 *
 * Colors are #rrggbb strings; alphas accept 0xNN or decimal; ints decimal or
 * 0x..; mfact is a float. Anything not present in xrdb keeps its value above. */
static const ResourcePref resources[] = {
	{ "font",            STRING_RES,  &fonts[0] },
	{ "normfg",          STRING_RES,  &colors[SchemeNorm][ColFg] },
	{ "normbg",          STRING_RES,  &colors[SchemeNorm][ColBg] },
	{ "normborder",      STRING_RES,  &colors[SchemeNorm][ColBorder] },
	{ "selfg",           STRING_RES,  &colors[SchemeSel][ColFg] },
	{ "selbg",           STRING_RES,  &colors[SchemeSel][ColBg] },
	{ "selborder",       STRING_RES,  &colors[SchemeSel][ColBorder] },
	{ "urgfg",           STRING_RES,  &colors[SchemeUrg][ColFg] },
	{ "urgbg",           STRING_RES,  &colors[SchemeUrg][ColBg] },
	{ "urgborder",       STRING_RES,  &colors[SchemeUrg][ColBorder] },
	{ "titlefg",         STRING_RES,  &colors[SchemeTitle][ColFg] },
	{ "titlebg",         STRING_RES,  &colors[SchemeTitle][ColBg] },
	{ "statusfg",        STRING_RES,  &colors[SchemeStatus][ColFg] },
	{ "statusbg",        STRING_RES,  &colors[SchemeStatus][ColBg] },
	{ "barbg",           STRING_RES,  &colors[SchemeBar][ColBg] },
	{ "normbgalpha",     INTEGER_RES, &alphas[SchemeNorm][ColBg] },
	{ "selbgalpha",      INTEGER_RES, &alphas[SchemeSel][ColBg] },
	{ "urgbgalpha",      INTEGER_RES, &alphas[SchemeUrg][ColBg] },
	{ "titlebgalpha",    INTEGER_RES, &alphas[SchemeTitle][ColBg] },
	{ "statusbgalpha",   INTEGER_RES, &alphas[SchemeStatus][ColBg] },
	{ "barbgalpha",      INTEGER_RES, &alphas[SchemeBar][ColBg] },
	{ "borderpx",        INTEGER_RES, &borderpx },
	{ "gappx",           INTEGER_RES, &gappx },
	{ "cornerradius",    INTEGER_RES, &cornerradius },
	{ "segradius",       INTEGER_RES, &segradius },
	{ "barpadx",         INTEGER_RES, &barpadx },
	{ "barvertpad",      INTEGER_RES, &barvertpad },
	{ "barmargin",       INTEGER_RES, &barmargin },
	{ "barvmargin",      INTEGER_RES, &barvmargin },
	{ "barseggap",       INTEGER_RES, &barseggap },
	{ "mfact",           FLOAT_RES,   &mfact },
};
