#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "s2.h"

#include "config.h"

static int
next_arg(int argc, char *argv[], int *i, const char **out)
{
	if (*i + 1 >= argc) {
		return 0;
	}
	*i += 1;
	*out = argv[*i];
	return 1;
}

void
print_usage(const char *argv0)
{
	fprintf(stderr,
	        "usage: %s [input_path] [-i path|-] [-o path] [-C class] [-D absdir] [--normal-window] [-c] [-h] [-v]\n"
	        "\n"
	        "  -i <path|->  input image path or stdin ('-')\n"
	        "  -o <path>    output image path\n"
	        "  -C <class>   X11 WM_CLASS instance/class value\n"
	        "  -D <absdir>  absolute save directory override for Ctrl+S\n"
	        "  --normal-window  use normal window type instead of dialog\n"
	        "  -c           copy rendered result to clipboard on finish\n"
	        "  -h           show help\n"
	        "  -v           show version\n",
	        argv0);
}

int
parse_args(int argc, char *argv[], struct app_config *cfg)
{
	int i;
	const char *arg;

	memset(cfg, 0, sizeof(*cfg));
	cfg->output_path = "./s2.png";
	cfg->window_class = default_window_class;

	for (i = 1; i < argc; i++) {
		arg = argv[i];
		if (strcmp(arg, "-h") == 0) {
			cfg->show_help = 1;
		} else if (strcmp(arg, "-v") == 0) {
			cfg->show_version = 1;
		} else if (strcmp(arg, "-c") == 0) {
			cfg->copy_on_finish = 1;
		} else if (strcmp(arg, "--normal-window") == 0) {
			cfg->normal_window = 1;
		} else if (strcmp(arg, "-i") == 0) {
			if (!next_arg(argc, argv, &i, &cfg->input_path)) {
				fprintf(stderr, "s2: missing value for -i\n");
				return 2;
			}
			cfg->input_kind = (strcmp(cfg->input_path, "-") == 0) ? INPUT_STDIN : INPUT_FILE;
		} else if (strcmp(arg, "-o") == 0) {
			if (!next_arg(argc, argv, &i, &cfg->output_path)) {
				fprintf(stderr, "s2: missing value for -o\n");
				return 2;
			}
		} else if (strcmp(arg, "-C") == 0) {
			if (!next_arg(argc, argv, &i, &cfg->window_class)) {
				fprintf(stderr, "s2: missing value for -C\n");
				return 2;
			}
		} else if (strcmp(arg, "-D") == 0) {
			if (!next_arg(argc, argv, &i, &cfg->save_dir_override)) {
				fprintf(stderr, "s2: missing value for -D\n");
				return 2;
			}
			if (!cfg->save_dir_override || cfg->save_dir_override[0] != '/') {
				fprintf(stderr, "s2: -D requires an absolute path\n");
				return 2;
			}
		} else if (arg[0] == '-') {
			fprintf(stderr, "s2: unknown option: %s\n", arg);
			return 2;
		} else {
			if (cfg->input_kind != INPUT_NONE) {
				fprintf(stderr, "s2: input already specified\n");
				return 2;
			}
			cfg->input_kind = INPUT_FILE;
			cfg->input_path = arg;
		}
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	int rc;
	struct app_config cfg;
	struct image img;
	(void)font_name;
	(void)default_save_directory;

	rc = parse_args(argc, argv, &cfg);
	if (rc != 0) {
		print_usage(argv[0]);
		return rc;
	}

	if (cfg.show_help) {
		print_usage(argv[0]);
		return 0;
	}

	if (cfg.show_version) {
		printf("s2-%s\n", VERSION);
		return 0;
	}

	if (cfg.input_kind == INPUT_NONE) {
		fprintf(stderr, "s2: no input provided (use -i <path|-> or input path)\n");
		print_usage(argv[0]);
		return 2;
	}

	memset(&img, 0, sizeof(img));
	rc = image_load(&cfg, &img);
	if (rc != 0) {
		return 1;
	}

	rc = editor_run(&cfg, &img);
	image_free(&img);
	if (rc != 0) {
		return 1;
	}

	return 0;
}
