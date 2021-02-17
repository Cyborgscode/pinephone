
/* LICENSE 

    Author: Marius Schwarz - support@evolution-hosting.eu  Braunschweig, Germany
    Date of Release: 17.2.2021
    Revision: 0.1

    This SOFTWARE is provided as is. No warrenties of any sort, that it won't damage something. 

    The use is free of charge of any private person and commercial businesses,
    but you are limited to : 

        - name the original author
        - mark changes you made to the source as yours
        - make it publically available again, free of charge ofcourse and under the same license.

    Changes:
    
        Initial release version

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

int set(char *value) {

  FILE *fd = fopen("/sys/class/leds/white:flash/brightness","w");
  if ( fd != NULL ) {
    size_t ret = fwrite(value, 1, 1, fd);
    fclose(fd);
		return 0;
	}
	return 1;
}


int main (int    argc, char *argv[]) {

  char *buffer = malloc(100);
	memset(buffer,0,100);

	FILE *fd = fopen("/sys/class/leds/white:flash/brightness","r");
	if ( fd != NULL ) {
		size_t ret = fread(buffer, 1, 1, fd);
		fclose(fd);

		//printf("status=%s\n", buffer);

		if ( strncmp("0",buffer,1) == 0 ) {
			set("1");
			return 1;
		}
		set("0");			
		return 0;
	}
	perror("File open failed!");
}
