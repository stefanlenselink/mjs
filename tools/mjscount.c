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
#include "info.h"

Config *conf;

typedef struct _record{
	char *name;
	char *shortname;
	int count;
	struct _record *next;
} record;

const struct option longopts[] = {
	{ "playlist",	1, 0, 'p' },
	{ "output",	1, 0, 'o' },
	{ "title",	1, 0, 't' },
	{ "lines",	1, 0, 'n' },
	{ "help",	0, 0, 'h' },
	{ "html",	0, 0, 'H' },
	{ "links",	0, 0, 'l' },	
	{ "start",	1, 0, 's' },
	{ "winamp",	1, 0, 'w' },
	{ "end",	1, 0, 'e' },
	{ "days",	1, 0, 'd' },
	{ "readconf",	0, 0, 'c' },
	{ "quiet",	0, 0, 'q' },
	{ 0, 0, 0, 0 }
};

void usage(int err)
{
	fprintf (stdout, "Usage: mjscount [options] mp3log-file\n");
	fprintf (stdout, "Options:\n");
	fprintf (stdout, "   -p, --playlist FILE   Output in the mjs playlist format in FILE\n");
	fprintf (stdout, "   -w, --winamp   FILE   Output in the winamp playlist format in FILE\n");
	fprintf (stdout, "   -n, --lines N         Display the top N songs\n");
	fprintf (stdout, "   -o, --output FILE     Use FILE instead of stdout\n");
	fprintf (stdout, "   -H, --html            Add html tags to output\n");
	fprintf (stdout, "   -l, --links           Add links to HTML output\n");
	fprintf (stdout, "   -t, --title TITLE     Use TITLE above the hitlist\n");
	fprintf (stdout, " \n");
	fprintf (stdout, "   -s, --start DATE      Start date yyyy-mm-dd\n");
	fprintf (stdout, "   -e, --end DATE        End date yyyy-mm-dd\n");
	fprintf (stdout, "   -d, --days NUM        Reaching back NUM days\n");
	fprintf (stdout, " \n");
	fprintf (stdout, "   -c, --readconf        Read .mjsrc file to remove mp3path from the filenames\n");
	fprintf (stdout, "   -q, --quiet           Don't show progres\n");
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
	/* command line defaults */
	int max_lines = 10, html = 0, length = 0;
	char *title_tag = "", *title_untag = "", *artist_tag = "", *artist_untag = "";
	char *number_tag = "", *number_untag = "", *name_tag = "", *name_untag = "";
	char *link_tag = "", *link_tag2 = "", *link_untag = "";
	long int start_time = 0, end_time = 0, quiet = 0, days = 0, links = 0;
	char *output_file = NULL, *playlist_file = NULL, *winamp_file_ = NULL;
	char *input_file;
	char *title = NULL;
	
	FILE *in_file, *out_file, *play_file = NULL, *winamp_file = NULL;
	int opt;
	char date_str[25];
	char tmp[255];
//	char *name;
	int count = 0, different = 0, max = 0, filesize = 0, line = 0;
	time_t date;
	record *songs = NULL, *tmpsong = NULL;
	record **top;
	int i;
	struct tm *tv;

	if (argc==1)
		usage(1);
	
	while ( (opt = getopt_long(argc, argv, "p:o:n:hHls:e:d:t:cqw:", longopts, NULL)) != -1 ) {
		switch ( opt ) {
			case 'c':
				conf = calloc (1, sizeof (Config));
				read_config (conf);
				length = strlen(conf->mp3path);
				break;
			case 'd':
				days = atoi(optarg);
				break;
			case 'e': {
				struct tm tv;
				if (strptime(optarg, "%Y-%m-%d", &tv)) {
					tv.tm_sec = tv.tm_min = tv.tm_hour = 0;
					end_time = mktime(&tv);
				} else 
					usage(3);
				break;
			}
			case 'h':
				usage(0);
				break;
			case 'H':
				html = 1;
				title_tag = "<H3>";
				title_untag = "</H3>";
				artist_tag = "<SMALL>";
				artist_untag = "</SMALL>";
				number_tag = "<B>";
				number_untag = "</B>";
				name_tag = "<B>";
				name_untag = "</B>";
				break;
			case 'l':
				links = 1;
				link_tag = "<A HREF=\"file://printserver/mp3/Genre/";
				link_tag2 = "\">";
				link_untag = "</A>";
				break;
			case 'n':
				max_lines = atoi(optarg);
				break;
			case 'o':
				output_file = optarg;
				break;
			case 'p':
				playlist_file = optarg;
				break;
			case 'q':
				quiet = 1;
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
			case 't':
				title = optarg;
				break;

			case 'w':
				winamp_file_ = optarg;
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
	while (!feof (in_file)) {
		char buf[500];
		filesize++;
		fgets ((char*)&buf, 500, in_file);
	}
	fclose(in_file);
	in_file = fopen (input_file,"r");
	
	if (output_file)
		out_file = fopen(output_file, "w");
	else
		out_file = stdout;
	
	if (playlist_file)
		play_file = fopen(playlist_file, "w");

	if (winamp_file_)
		winamp_file = fopen(winamp_file_, "w");
	
	if (days)
		start_time = time(NULL) - (days * 24 * 60 * 60);
	
	if (!quiet)
		fprintf(stdout, "Parsing file:\n");
	
/* Parse log-file */	
	
	while (!feof(in_file)) {
	char *name;
		time_t date;
		struct tm tv;
		fscanf(in_file, "%24c %254[^\n] ", date_str, tmp);
//remove the trailing .mp3
		//tmp[strlen(tmp)-4]='\0';
		if (!quiet) {
			line++;
			if ((line % (filesize/40)) == 0){
				fprintf(stdout,".");
				fflush(stdout);
			}
			if ((line % (filesize / 10))  == 0){
				fprintf(stdout,"%d%%",(line) / (filesize/100));
				fflush(stdout);
			}
		}
		if (tmp[0]!='/')
			continue;
		if (!(name = strrchr(tmp,'/')))
			abort();
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
			if (tmpsong->shortname[1]==name[1])
				if (tmpsong->shortname[2]==name[2])
					if (!strcmp(tmpsong->shortname+2, name+2))
						break;
		if (tmpsong) {
			tmpsong->count++;
			if (tmpsong->count > max)
				max = tmpsong->count;
			free(tmpsong->name);
			tmpsong->name = strdup (tmp);
		} else {
			tmpsong = calloc(1, sizeof(record));
			tmpsong->shortname = strdup(name);
			tmpsong->name = strdup(tmp);
			tmpsong->count = 1;
			tmpsong->next = songs;
			songs = tmpsong;
			different++;
		}
		count++;
		
	}
	
/* Select most played songs */
	
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
	
	
	
	
	if (!quiet)
		fprintf(stdout, "\n\n");
	if (play_file)
		fprintf(play_file, "Playlist for mjs\n");

	tv = localtime(&start_time);
	fprintf(out_file, "%s  %s Top %d    %02d-%02d-%d - ", title_tag, title, max_lines, tv->tm_mday, tv->tm_mon + 1, tv->tm_year + 1900);

	if (!end_time)
		end_time = time(NULL);
	tv = localtime(&end_time);
	fprintf(out_file, "%02d-%02d-%d%s\n\n", tv->tm_mday, tv->tm_mon + 1, tv->tm_year + 1900, title_untag);

	fprintf(out_file, "Place   Songname\n            %sArtist  [Album]%s\n\n", artist_tag, artist_untag);

/* start walking through the list, most played (i) first, with numbers played the same number of times in no special order. */
	
	for (i = max, line=0; (line < max_lines)&(i>0); i--)
		if (top[i]) {
			int k = 0;
			
			for (tmpsong = top[i]; (tmpsong) && (line < max_lines); tmpsong = tmpsong->next) {
				flist *file = NULL;
				char *dir, *filename;
				
				if (!strcmp(tmpsong->name,"/usr/local/share/intro.mp3"))
					continue;

				line++;

				if (play_file)
					fprintf(play_file, "%s\n", tmpsong->name);
				if (winamp_file)
					fprintf(winamp_file, "%s\n", tmpsong->name);
				
				filename = chop_filename(&tmpsong->name);
				dir = tmpsong->name;

				if(length)
					file = mp3_info(dir, filename, NULL, 0, L_NEW);
				
				
				if (k)
					fprintf(out_file, "   ");
				else
					fprintf(out_file, "%s%2d%s ", number_tag, line, number_untag);

				if (file) {
					if ((file->filename[0]>='0') & (file->filename[0]<='9')) {
						char *p;
						if ((p = strchr(file->filename, ' ')))
							file->filename = p + 1;
					} else if (!strncasecmp(file->filename,"cd",2))
						file->filename = file->filename+7;	

					
					fprintf(out_file, "(%2d) %s%s%s%s%s%s%s\n            %s%s  [%s]%s\n", i, \
						link_tag, (links ? file->relpath : "\0"), link_tag2, \
						name_tag, file->filename, name_untag, \
						link_untag, \
						artist_tag, file->artist, (file->album ? file->album : "\0"), artist_untag);	
					
				} else {
					filename[strlen(filename)-4]='\0';
					fprintf(out_file, "(%2d) %s%s%s\n            %s%s%s\n", i, \
						name_tag, filename, name_untag, \
						artist_tag, dir+length, artist_untag);				
	
				}
				k++;
			}
		}
	date = time(NULL);
	if (links && winamp_file)
		fprintf(out_file, "<A HREF=\"%s\">As Winamp playlist</A>", winamp_file_);
	fprintf(out_file, "\nNumber of played songs %d consisting of %d different songs\n", count, different);
	fprintf(out_file, "Generated on: %s\n", ctime(&date));
		
	return 0;
	
}

