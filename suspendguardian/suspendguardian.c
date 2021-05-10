
/* LICENSE 

    Author: Marius Schwarz - support@evolution-hosting.eu  Braunschweig, Germany
    Date of Release: 10.5.2021
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
#include <unistd.h>
#include <sys/stat.h>

int exiting = 0;

void sig_handler( int signum) {

	printf("Sighandler signum=%d\n");
	if ( signum == 0 ) {
		printf("exiting..\n");
		exiting = 1;
	}
}


int set(char *filename, char *value) {

        FILE *fd = fopen(filename, "w");
        if ( fd != NULL ) {
                size_t ret = fwrite(value, 1, 1, fd);
                fclose(fd);
		return 0;
	}
	return 1;
}

char *readfile(char *filename) {
        int number =0;
        FILE *fd = fopen(filename,"r");
        if ( fd != NULL ) {

                struct stat info;
                if ( lstat( filename, &info ) == 0 ) {
                        // read only if it's not tampered
                        if ( S_ISLNK(info.st_mode) == 0 ) {
                                // sane check if the content of the file is possible
                                // just in case you ask... kernel files can be 4k nodes
                                if ( info.st_size > 0 && info.st_size < 8192 ) {
                                        char *buffer = malloc(info.st_size+2);
                                        if ( buffer != NULL ) {
                                                size_t ret = fread(buffer, 1, info.st_size, fd);
                                                if ( ret > 0 && ret <= info.st_size ) {
                                                        fclose(fd);
                                                        return buffer;
                                                } else {
                                                        printf("READ ERROR for %s:could not read %d bytes:aborting.\n", filename, info.st_size);
                                                }
                                        } else {
                                                printf("READ ERROR for %s:could not allocate enough memory:aborting.\n", filename );
                                        }
                                } else {
                                        printf("READ ERROR for %s:sanity check failed size=%d:aborting.\n", filename,info.st_size );
                                }
                        } else {
                                printf("READ ERROR for %s:file is a link:aborting.\n", filename );
                        }
                } else {
                        printf("READ ERROR for %s:file is a link:aborting.\n", filename );
                }
                // close file for all errorcodes
                fclose(fd);
        } else {
                printf("READ ERROR for %s:could not open file:aborting.\n", filename );
        }
        return NULL;
}


int get(char *filename) {
	int number =0;
	FILE *fd = fopen(filename,"r");
        if ( fd != NULL ) {

		struct stat info;
		if ( lstat( filename, &info ) == 0 ) {
			// read only if it's not tampered
			if ( S_ISLNK(info.st_mode) == 0 ) {
				// sane check if the content of the file is possible
				// just in case you ask... kernel files can be 4k nodes
				if ( info.st_size > 0 && info.st_size < 8192 ) {
					char *buffer = malloc(info.st_size+2);
					if ( buffer != NULL ) {
						size_t ret = fread(buffer, 1, info.st_size, fd);
						if ( ret > 0 && ret <= info.st_size ) {
		                                        number = atoi(buffer);
                	                                fclose(fd);
                        	                        return number;
						} else {
							printf("READ ERROR for %s:could not read %d bytes:aborting.\n", filename, info.st_size);
						}
					} else {
						printf("READ ERROR for %s:could not allocate enough memory:aborting.\n", filename );
					}
				} else {
					printf("READ ERROR for %s:sanity check failed size=%d:aborting.\n", filename,info.st_size );
				}
			} else {
				printf("READ ERROR for %s:file is a link:aborting.\n", filename );
			} 
		} else {
			printf("READ ERROR for %s:file is a link:aborting.\n", filename );
		}
		// close file for all errorcodes
		fclose(fd);
	} else {
		printf("READ ERROR for %s:could not open file:aborting.\n", filename );
	}
	return -1;
}

const int STATE_OFF = 4;
const int STATE_ON = 0;
const char* charging = "Charging\n";  // this is what you get by reading a kernel file... 
const char* suspend  = "'suspend'";

int main (int    argc, char *argv[]) {

        char *buffer = malloc(100);
	memset(buffer,0,100);
	
	int timer = 0; // TIMER in seconds from the last Switch from ON => OFF
	int laststate =  get( "/sys/class/backlight/backlight/bl_power");
	int state = laststate;
	size_t ret = 0;

	if ( state == -1 ) {
		printf("somethign is very wrong, as we can't read the backlights status. aborting!\n");
		return 1;
	}

	int suspendoncharge = get( "/etc/suspendguardian/suspendoncharge");
	int gotosuspend     = get( "/etc/suspendguardian/suspendtimer");
	int cpupowerdown    = get( "/etc/suspendguardian/cpupowerdown");
	int respect         = get( "/etc/suspendguardian/respectgsettings");

	if ( respect == -1 ) respect = 0;		 // do not respect gsettings of pine
	if ( suspendoncharge == -1) suspendoncharge = 0; // DEFAULT is NOT TO suspend when the charger is connected!
	if ( gotosuspend == -1 ) gotosuspend = 60;	// DEFAULTS in case file is unreadable of not present at all
	if ( cpupowerdown == -1 ) cpupowerdown = 1;	// DEFAULTS in case file is unreadable of not present at all

	do {
		// IT COULD be required to interrupt the timer, in case KDE connect or any other background task needs to stay online.

		// Let's read the display power status:

		int state = get( "/sys/class/backlight/backlight/bl_power");
	        if ( state > -1 ) {
//			printf("status=%d\n", state);
			// Code 4 means = Offline

			// RESET TIMER everytime the display turns from ON => OFF
			//
			if ( laststate == STATE_ON && state == STATE_OFF ) {
				timer = 0;
			}

			// Restore CPU Power if display switches from off to online AND it's enabled ( > 0 ) 
			if ( laststate == STATE_OFF && state == STATE_ON && cpupowerdown > 0 ) {
				printf("Switching ON CPU Cores 1-3\n");
				set("/sys/devices/system/cpu/cpu1/online","1");
                                set("/sys/devices/system/cpu/cpu2/online","1");
                                set("/sys/devices/system/cpu/cpu3/online","1");

			}
			// remember the last state or you can't see a difference..
			//
			laststate = state;

			// COUNT only if the display is offline
			if ( state == STATE_OFF ) {
				timer++;
			}

			// if we reach CPUPOWERDOWN => disable 3 cores .. 
			if ( timer == cpupowerdown ) {
				printf("Switching OFF CPU Cores 1-3\n");
				set("/sys/devices/system/cpu/cpu1/online","0");
				set("/sys/devices/system/cpu/cpu2/online","0");
				set("/sys/devices/system/cpu/cpu3/online","0");
			}
			// it's logically impossible to have timer=0 here, but just in case ;)
			if ( timer == gotosuspend && gotosuspend > 0 ) {

				// READ Batterystatus
				char *buffer = readfile("/sys/class/power_supply/axp20x-battery/status"); 
				if ( buffer != NULL && strlen(buffer)==strlen(charging) && strncmp(buffer,charging,strlen(buffer))==0 && suspendoncharge==0) {
					// do not suspend while charging		
					printf("SUSPEND IGNORED.. battery is charging, no need to suspend.\n");
				} else {

					system("su -c \"gsettings get org.gnome.settings-daemon.plugins.power sleep-inactive-battery-type\" pine > /var/run/suspendguardian.pipe");
					buffer = readfile("/var/run/suspendguardian.pipe");
					if ( respect == 1 && strlen(buffer)>2 && strncmp(buffer,suspend,strlen(suspend))!=0 ) {
						 printf("SUSPEND IGNORED.. we honor gsettings and it does not want to suspend -> do %s\n", buffer);

					} else {

						// IT COULD be required to interrupt the timer, in case KDE connect or any other background task needs to stay online.
			 			if ( access("/var/run/suspendguardian.intercept",F_OK) != 0 ) {
							printf("Going to suspend...\n");
							int status = system("systemctl suspend");
							printf("call suspend rc=%d\n", status);
						} else {
							printf("SUSPEND INTERCEPTED .. rewind TIMER by 10 seconds!\n");
							if ( timer > 10 ) {
								timer -= 10;
							} else timer = 1;
						}
					}
				}
			}
		} else {
			printf("could not read display status..Exiting\n");
			perror("File open failed!");
			return 1;
		}
		sleep(1);
	} while ( 1==1 ); 

	printf("exiting..");
	return 0;
}
