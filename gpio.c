/*
 *  Squeezelite - lightweight headless squeezebox emulator
 *
 *  (c) Adrian Smith 2012-2015, triode1@btinternet.com
 *      Ralph Irving 2015-2023, ralph_irving@hotmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * gpio.c (c) Paul Hermann, 2015-2023 under the same license terms
 *   -Control of Raspberry pi GPIO for amplifier power
 *   -Launch script on power status change from LMS
 */

#if GPIO

#include "squeezelite.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if RPI
#include <lgpio.h>

#endif  // RPI

static int gpio_state = -1;
static bool initialized = false;
static int power_state = -1;
static int chip = -1;

bool gpio_init(){
	// Set up gpio using kernel interface
	if (! initialized){
		chip = lgGpiochipOpen(gpio_chip);

		if ( chip >= 0 ){
			if (lgGpioClaimOutput(chip, LINEFLAGS, gpio_pin, 0) == LG_OKAY) {
				initialized = true;
			} else {
				LOG_ERROR("Unable to open GPIO:%d for output", gpio_pin);
			}
		} else {
			LOG_ERROR("Unable to open gpio chip:%d", gpio_chip);
		}
	}
	return initialized;
}

void gpio_close(){
	if (initialized && chip >= 0){
		lgGpiochipClose(chip);
	}
}

void relay( int state) {
#ifdef RPI
    gpio_state = state;
    int status;

	if (initialized){
		if(gpio_state == 1)
			status = lgGpioWrite(chip, gpio_pin, LG_HIGH^gpio_active_low);
		else if(gpio_state == 0)
			status = lgGpioWrite(chip, gpio_pin, LG_LOW^gpio_active_low);

		if (status != 0)
			LOG_ERROR("Unable to write to GPIO Pin:%d, Error Code:%d", gpio_pin, status);
	}
  // Done!
#endif

}

char *cmdline;
int argloc;

void relay_script( int state) {
	gpio_state = state;
	int err;

  // Call script with init parameter
	if (initialized == -1){
		int strsize = strlen(power_script);
		cmdline = (char*) malloc(strsize+3);
		argloc = strsize + 1;

		strcpy(cmdline, power_script);
		strcat(cmdline, " 2");
		if ((err = system(cmdline)) != 0){ 
			fprintf (stderr, "%s exit status = %d\n", cmdline, err);
		}
		else{
			initialized = 1;
		}
  	}

  // Call Script to turn on or off  on = 1, off = 0
  // Checks current status to avoid calling script excessivly on track changes where alsa re-inits.

	if( (gpio_state == 1) && (power_state != 1)){
		cmdline[argloc] = '1';
		if ((err = system(cmdline)) != 0){ 
			fprintf (stderr, "%s exit status = %d\n", cmdline, err);
		}
		else {
			power_state = 1;
		}
	}
	else if( (gpio_state == 0) && (power_state != 0)){
		cmdline[argloc] = '0';
		if ((err = system(cmdline)) != 0){ 
			fprintf (stderr, "%s exit status = %d\n", cmdline, err);
		}
		else {
			power_state = 0;
		}
	}
// Done!
}

#endif // GPIO
