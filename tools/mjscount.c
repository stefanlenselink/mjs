#define _XOPEN_SOURCE
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <string.h>

typedef struct Song {
	char *name;
	int count;
} Song;

const struct option longopts[] = {
	{ "playlist",	1, 0, 'p' },
	{ "output",	1, 0, 'o' },
	{ "title",	1, 0, 't' },
	{ "lines",	1, 0, 'n' },
	{ "help",	0, 0, 'h' },
	{ "start",	1, 0, 's' },
	{ "end",	1, 0, 'e' },
	{ "days",	1, 0, 'd' },
	{ 0, 0, 0, 0 }
};

void
usage(int err)
{
	fprintf(stderr, "Usage: mjscount [options] [logfile]\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "   -p, --playlist FILE   Output in the mjs playlist format in FILE\n");
	fprintf(stderr, "   -n, --lines N         Display the top N songs\n");
	fprintf(stderr, "   -o, --output FILE     Use FILE instead of stdout\n");
	fprintf(stderr, "   -t, --title TITLE     Use TITLE above the hitlist\n");
	fprintf(stderr, " \n");
	fprintf(stderr, "   -s, --start DATE      Start date yyyy-mm-dd\n");
	fprintf(stderr, "   -e, --end DATE        End date yyyy-mm-dd\n");
	fprintf(stderr, "   -d, --days NUM        Reaching back NUM days\n");
	fprintf(stderr, " \n");
	fprintf(stderr, "   -h, --help            Display this help and warranty\n");
	fprintf(stderr, " \n");
	fprintf(stderr, "Copyright 2002 Marijn van Galen\n");
	fprintf(stderr, "Copyright 2013 Kees de Rijke\n");
	fflush(stdout);

	exit(err);
}

time_t
scantime(const char *str) {
	struct tm tv;
	time_t time;

	memset(&tv, 0, sizeof (struct tm));
	if (strptime(str, "%Y-%m-%d", &tv) == NULL)
		return 0;

	if ((time = mktime(&tv)) == -1)
		return 0;

	return (time);
}

time_t
scanlocaltime(const char *str) {
	struct tm tv;
	time_t time;

	memset(&tv, 0, sizeof (struct tm));
	if ((strptime(str, "%c", &tv)) == NULL)
		return 0;

	if ((time = mktime(&tv)) == -1)
		return 0;

	return (time);
}

void
printtime(const time_t *time)
{
	struct tm *tv;
	char time_str[11];

	tv = localtime(time);
	strftime(time_str, 11, "%Y-%m-%d", tv);
	printf(time_str);
}

int
comparesongs(const void *s1, const void *s2)
{
	Song *song1 = *((Song **)s1);
	Song *song2 = *((Song **)s2);

	if (song1->count == song2->count)
		return 0;
	else if (song1->count < song2->count)
		return 1;
	else
		return -1;
}

int
main(int argc, char *argv[])
{
	int opt;
	int lines = 10;

	time_t start_time = 0;
	time_t end_time = 0;
	int days = 0;

	char *output = NULL;
	char *playlist = NULL;
	FILE *playlistfile = NULL;
	char *input = NULL;

	char *title = NULL;

	char date_str[25];
	time_t date;
	char name[255];
	
	int max = 0;
	int count = 0;
	Song **songs;
	Song **songstmp;
	int songcount = 0;
	Song *song;

	int i;

	if (argc == 1)
		usage(1);

	while ((opt = getopt_long(argc, argv, "p:o:n:hs:e:d:t:", longopts, NULL)) != -1) {
		switch (opt) {
		case 'p':
			playlist = optarg;
			break;
		case 'o':
			output = optarg;
			break;
		case 't':
			title = optarg;
			break;
		case 'n':
			lines = atoi(optarg);
			break;
		case 'h':
			usage(0);
			break;
		case 's':
			if ((start_time = scantime(optarg)) == 0)
				usage(3);
			break;
		case 'e':
			if ((end_time = scantime(optarg)) == 0)
				usage(3);
			break;
		case 'd':
			days = atoi(optarg);
			break;
		default:
			usage(1);
			break;
		}
	}

	if (argc - optind != 1)
		usage(1);

	input = argv[optind];

	if (input != NULL)
		freopen(input, "r", stdin);

	if (output != NULL)
		freopen(output, "w", stdout);

	if (playlist)
		playlistfile = fopen(playlist, "w");

	if (days)
		start_time = time(NULL) - (days * 24 * 60 * 60);

	songs = malloc((songcount + 1) * sizeof (Song *));
	songs[songcount] = NULL;

	while (!feof(stdin)) {
		fscanf(stdin, "%24c %254[^\n] ", date_str, name);
		date_str[24]='\0';
		date = scanlocaltime(date_str);

		if (name[0] != '/')
			continue;
		if (start_time != 0 && date < start_time)
			continue;
		if (end_time != 0 && date > end_time)
			continue;

		song = NULL;
		for (songstmp = songs; *songstmp != NULL; songstmp++) {
			if (strcmp((*songstmp)->name, name) == 0) {
				song = *songstmp;
				break;
			}
		}

		if (song == NULL) {
			songcount++;

			songs = realloc(songs, (songcount + 1) * sizeof (Song *));
			songs[songcount] = NULL;

			song = calloc(1, sizeof (Song));
			song->name = strdup(name);

			songs[songcount - 1] = song;
		}

		song->count++;
		if (song->count > max)
			max = song->count;
		count++;
	}

	printf("Number of played songs %d and unique songs %d\n\n", count, songcount);
	
	qsort(songs, songcount, sizeof (Song *), comparesongs);

	if (playlistfile)
		fprintf(playlistfile, "Playlist for mjs\n");

	if (title)
		printf("%s ", title);
	printf("Top %d (", lines);
	printtime(&start_time);
	printf(" - ");
	if (!end_time)
		end_time = time(NULL);
	printtime(&end_time);
	printf(")\n");

	for (songstmp = songs, count = 1; *songstmp != NULL; songstmp++, count++) {
		song = *songstmp;

		if (playlistfile)
			fprintf(playlistfile, "%s\n", song->name);

		printf("%2d (%3d) %s\n", count, song->count, song->name);
	}

	date = time(NULL);
	printf("\n");
	printf("Generated on: %s\n", ctime(&date));

	return (0);
}
