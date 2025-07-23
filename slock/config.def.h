/* user and group to drop privileges to */
static const char *user  = "matteo";
static const char *group = "matteo";

static const char *colorname[NUMCOLS] = {
	[INIT] =   "black",     /* after initialization */
	[INPUT] =  "#005577",   /* during input */
	[INPUT_ALT] = "#227799", /* during input, second color */
	[FAILED] = "#CC3333",   /* wrong password */
};

/* treat a cleared input like a wrong password (color) */
static const int failonclear = 1;

/* default message */
static const char * message = "Matteo Merola: enter password to unlock";

/* text color */
static const char * text_color = "#ffffff";

/* text size (must be a valid size) */
static const char * font_name = "-misc-fira code semibold-semibold-r-normal--0-0-0-0-m-0-ascii-0";
