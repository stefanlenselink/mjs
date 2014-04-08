#include "config/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

extern Config *conf;

static int binary;
static void usage();
static void print_parameter(char *parameter, char *value);

int main(int argc, char *argv[]) {
	int opt;
	char *parameter;

	binary = 0;
	while ((opt = getopt(argc, argv, "bh")) != -1) {
		switch (opt) {
		case 'b':
			binary = 1;
			break;
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
			break;
		}
	}

	config_init();
	atexit(config_shutdown);

	if (optind == argc) {
		usage();
		exit(EXIT_FAILURE);
	}

	while (optind < argc) {
		parameter = argv[optind++];

		if (!strcasecmp(parameter, "mp3dir")) {
			print_parameter("mp3dir", conf->mp3path);
		} else {
			fprintf(stderr, "%s: no such parameter -- '%s'\n", argv[0], parameter);
			exit(EXIT_FAILURE);
		}
	}

	exit(EXIT_SUCCESS);
}

static void usage() {
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: mjsconfig [options] [parameter ...]\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  -b    print only the value without a new line\n");
	fprintf(stderr, "  -h    display this help and exit\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Recognized parameter(s):\n");
	fprintf(stderr, "  mp3dir\n");
}

static void print_parameter(char *parameter, char *value) {
	if (binary) {
		printf("%s", value);
	} else {
		printf("%s = %s\n", parameter, value);
	}
}
