#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#define __USE_XOPEN /* glibc2 needs this */
#include <time.h>
#undef __USE_XOPEN
#include "top.h"
#include "struct.h"
#include "config.h"

typedef struct _record{
	char *name;
	int count;
	struct _record *next;
} record;

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

void usage(int err)
{
	fprintf (stdout, "Usage: mjscount [options] mp3log-file\n");
	fprintf (stdout, "Options:\n");
	fprintf (stdout, "   -p, --playlist FILE   Output in the mjs playlist format in FILE\n");
	fprintf (stdout, "   -n, --lines N         Display the top N songs\n");
	fprintf (stdout, "   -o, --output FILE     Use FILE instead of stdout\n");
	fprintf (stdout, "   -t, --title TITLE     Use TITLE above the hitlist\n");
	fprintf (stdout, " \n");
	fprintf (stdout, "   -s, --start DATE      Start date yyyy-mm-dd\n");
	fprintf (stdout, "   -e, --end DATE        End date yyyy-mm-dd\n");
	fprintf (stdout, "   -d, --days NUM        Reaching back NUM days\n");
	fprintf (stdout, " \n");
	fprintf (stdout, "   -h, --help            Display this help and warranty\n");
	fprintf (stdout, " \n\n");
	fprintf (stdout, " Part of:\n");
	fprintf (stdout, "   MP3 Jukebox System (mjs) v%s\n", VERSION);
	fprintf (stdout, "   By Marijn van Galen. (M.P.vanGalen@ITS.TUDelft.nl)\n");
	fprintf (stdout, " \n");
	fprintf (stdout, "Copyright (C) 2002 by Marijn van Galen\n");
	switch (err) {
		case 0:
			fprintf (stdout, " \n");
			fprintf (stdout, " This program is free software; you can redistribute it and/or modify it\n");
			fprintf (stdout, " under the terms of the GNU General Public License as published by the Free\n");
			fprintf (stdout, " Software Foundation; version 2 of the License.\n\n");
			fprintf (stdout, " This program is distributed in the hope that it will be useful, but WITHOUT\n");
			fprintf (stdout, " ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or\n");
			fprintf (stdout, " FITNESS FOR A PARTICULAR PURPOSE. \n\n");
			fprintf (stdout, " See the GNU GPL (see LICENSE file) for more details.\n\n");
			break;
		case 2:
	 		fprintf (stdout, "\n\nERROR: Startdate invalid or wrong syntax!\n\n");
			break;
		case 3:
			fprintf (stdout, "\n\nERROR: Enddate invalid or wrong syntax!\n\n");
			break;
		case 4:
			fprintf (stdout, "\n\nERROR: The inputfile does not exist!\n\n");
			break;
	}
	fflush(stdout);
	exit(err);
}

int
main (int argc, char *argv[])
{
	int lines = 10;
	long int start_time = 0, 
		end_time = 0,
		days = 0;
	char *output_file = NULL;
	char *playlist_file = NULL;
	char *input_file;
	char *title = NULL;
	FILE *in_file, *out_file, *play_file = NULL;
	int opt;
	char date_str[25];
	char tmp[255];
	char *name;
	int count = 0, max = 0;
	time_t date;
	record *songs = NULL, *tmpsong = NULL;
	record **top;
	int i;
	struct tm *tv;
	Config *conf;
	int length;

	if (argc==1)
		usage(1);
	
	while ( (opt = getopt_long(argc, argv, "p:o:n:hs:e:d:t:", longopts, NULL)) != -1 ) {
		switch ( opt ) {
			case 'p':
				playlist_file = optarg;
				break;
			case 'o':
				output_file = optarg;
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
			case 's': {
				struct tm tv;
				if (strptime(optarg, "%Y-%m-%d", &tv)) {
					tv.tm_sec = tv.tm_min = tv.tm_hour = 0;
					start_time = mktime(&tv);
				} else 
					usage(2);
				break;
			}
			case 'e': {
				struct tm tv;
				if (strptime(optarg, "%Y-%m-%d", &tv)) {
					tv.tm_sec = tv.tm_min = tv.tm_hour = 0;
					end_time = mktime(&tv);
				} else 
					usage(3);
				break;
			}
			case 'd':
				days = atoi(optarg);
				break;
			default:
				usage(1);
				break;
		}
	}
	if ((argc - optind) != 1)
		usage(1);
	input_file = argv[optind];
	
	in_file = fopen (input_file,"r");
	if (!in_file) 
		usage(4);
		
	if (output_file)
		out_file = fopen(output_file, "w");
	else
		out_file = stdout;
	
	if (playlist_file)
		play_file = fopen(playlist_file, "w");
	
	if (days)
		start_time = time(NULL) - (days * 24 * 60 * 60);
	
	conf = calloc (1, sizeof (Config));
	read_config (conf);
	length = strlen(conf->mp3path);
	
	while (!feof(in_file)) {
		time_t date;
		struct tm tv;
		fscanf(in_file, "%24c %254[^\n] ", date_str, tmp);
//remove the trailing .mp3
		tmp[strlen(tmp)-4]='\0';
		name = tmp;
		if (name[0]!='/')
			continue;
		if ((start_time) | (end_time)) {
			date_str[24]='\0';
			strptime(date_str, "%c", &tv);
			date = mktime(&tv);
			if (date < start_time)
				continue;
			if ((end_time!=0) & (date > end_time))
				continue;
		}
		for (tmpsong = songs; tmpsong; tmpsong = tmpsong->next)
			if (tmpsong->name[length]==name[length])
				if (tmpsong->name[length+1]==name[length+1])
					if (!strcmp(tmpsong->name+length+2, name+length+2))
						break;
		if (tmpsong) {
			tmpsong->count++;
			if (tmpsong->count > max)
				max = tmpsong->count;
		} else {
			tmpsong = calloc(1, sizeof(record));
			tmpsong->name = strdup(name);
			tmpsong->count = 1;
			tmpsong->next = songs;
			songs = tmpsong;
		}
		count++;
	}
	fprintf(stdout, "Number of played songs %d\n\n", count);
	top = (record **) calloc(max+1, sizeof(record *));
	memset(top, 0, max * sizeof(record *));
	for (tmpsong = songs; tmpsong; tmpsong = tmpsong->next) {
		if (top[tmpsong->count]==NULL) {
			top[tmpsong->count] = calloc(1, sizeof(record));
			top[tmpsong->count]->name = tmpsong->name;
			top[tmpsong->count]->next = NULL;
		} else {
			record *tmp = calloc(1, sizeof(record));
			tmp->next = top[tmpsong->count];
			tmp->name = tmpsong->name;
			top[tmpsong->count] = tmp;
		}
	}
	if (play_file)
		fprintf(play_file, "Playlist for mjs\n");
	tv = localtime(&start_time);
	fprintf(out_file, "  %s Top %d    %02d-%02d-%d - ", title, lines, tv->tm_mday, tv->tm_mon + 1, tv->tm_year + 1900);
	if (!end_time)
		end_time = time(NULL);
	tv = localtime(&end_time);
	fprintf(out_file, "%02d-%02d-%d\n\n", tv->tm_mday, tv->tm_mon + 1, tv->tm_year + 1900);
	for (i = max, count=0; (count < lines)&(i>0); i--)
		if (top[i]) {
			for (tmpsong = top[i]; (tmpsong) && (count < lines); tmpsong = tmpsong->next) {
				if (tmpsong==NULL)
					abort();
				if (!strcmp(tmpsong->name,"/usr/local/share/intro"))
					continue;
				count++;
				if (play_file)
					fprintf(play_file, "%s.mp3\n", tmpsong->name);
		if (strlen(tmp)-1 > length)
			name = tmp+length;
		else
			name = tmp;

				fprintf(out_file, "%2d (%3d) %s\n", count, i, (strlen(tmpsong->name) > length ? tmpsong->name + length : tmpsong->name));
			}
		}
	date = time(NULL);
	fprintf(out_file, "\nGenerated on: %s\n", ctime(&date));
		
	return 0;
	
}

