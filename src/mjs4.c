/***************************************************************************
 *   Copyright (C) 2008 by Stefan Lenselink   *
 *   Stefan@lenselink.org   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "engine.h"

int main(int argc, char *argv[])
{
/* int result;
  int loop = 1;
  mpg123_handle * ha;
  ao_device * ao;
  ao_sample_format fmt;
  
  fmt.bits = 16;
  fmt.rate = 44100;
  fmt.channels = 2;
  fmt.byte_format = AO_FMT_NATIVE;
  
  ao_initialize();

  ao = ao_open_live(ao_driver_id("pulse"), &fmt, NULL);
  if(ao == NULL)
  {
    fprintf(stderr, "Whaaa!\n");
  }
  
  

  
  result = mpg123_init();
  if(result != MPG123_OK)
  {
    fprintf (stderr, "Cannot initialize mpg123 library: %s", mpg123_plain_strerror(result));
    exit(77);
  }
  
  ha = mpg123_new(NULL, &result);
  
  if(result != MPG123_OK)
  {
    fprintf (stderr, "Cannot new mpg123 library: %s", mpg123_plain_strerror(result));
    exit(77);
  }
  
  result = mpg123_open(ha, "/tmp/test.mp3");
  
  if(result != MPG123_OK)
  {
    fprintf (stderr, "Cannot open file: %s", mpg123_plain_strerror(result));
    exit(77);
  }
  
  while(loop)
  {
    char mem[1024];
    int bla = 0;
    int returnVal = mpg123_read(ha, mem, 1024, &bla);
    fprintf (stderr, "Before %d\n", bla);
    ao_play(ao, mem, bla);
    fprintf (stderr, "After\n");
    if(returnVal == MPG123_DONE)
    {
      loop = 0;
    }
  }
  
  
  ao_shutdown();
  
  mpg123_delete(ha);*/
    engine_init();
    
   engine_play();
   sleep(2);
   engine_ffwd(1000);
  sleep(60);
  printf("Hello, world!\n");
  
  return EXIT_SUCCESS;
}
