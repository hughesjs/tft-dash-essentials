// C library headers
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <string.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

#define NORMAL			0
#define FASTLEAK 		1
#define HIGHPRESSURE 	2
#define LOWPRESSURE 	3
#define HIGHTEMP 		4
#define LOWBATTERY 		5


int tpms_serial_port;
pthread_t tpms_tid[2];

double gSensor1bar = 0.0;
double gSensor2bar = 0.0;
double gSensor3bar = 0.0;
double gSensor4bar = 0.0;

double gSensor1psi = 0.0;
double gSensor2psi = 0.0;
double gSensor3psi = 0.0;
double gSensor4psi = 0.0;

int gSensor1temp = 0;
int gSensor2temp = 0;
int gSensor3temp = 0;
int gSensor4temp = 0;

int gSensor1state = 0;
int gSensor2state = 0;
int gSensor3state = 0;
int gSensor4state = 0;

const char * stringSensorstate (int state) {
	if (state == NORMAL) {
		return "NORM";
	} 

	if (state == FASTLEAK) {
		return "FASTLEAK";
	}

	if (state == HIGHPRESSURE) {
		return "HIGHPRESS";
	}

	if (state == LOWPRESSURE) {
		return "LOWPRESS";
	}

	if (state == HIGHTEMP) {
		return "HIGHTEMP";
	}

	if (state == LOWBATTERY) {
		return "LOWBATT";
	}

	return "NONE";
}

bool file_exists (const char* name) {

	std::ifstream ifile (name);
	return (bool)ifile;

}

int connectTPMSInterface ()
{
// Open the serial port. Change device path as needed (currently set to an standard FTDI USB-UART cable type device)
	//tty.usbserial-D3077502
	//cu.usbserial-D3077502

	// Create new termios struc, we call it 'tty' for convention
	



	fprintf(stderr, "Connect TPMS\n");

	if (file_exists ("/dev/cu.usbserial-D3077502")) {
		tpms_serial_port = open("/dev/cu.usbserial-D3077502", O_RDWR); // Nano board connected!
		fprintf(stderr, "TPMS interface connected!"); 
	} else {
		if (file_exists ("/dev/cu.usbmodem14301")) {		
			fprintf(stderr, "Bike interface connected!"); 
			tpms_serial_port = open("/dev/cu.usbmodem14301", O_RDWR);
		} else {

			if (file_exists ("/dev/ttyACM0")) {
				fprintf(stderr, "Bike interface connected!"); 
				tpms_serial_port = open("/dev/ttyACM0", O_RDWR);
			} else {
				if (file_exists ("/dev/ttyACM1")) {
					fprintf(stderr, "Bike interface connected!"); 
					tpms_serial_port = open("/dev/ttyACM1", O_RDWR);
				} else {

					if (file_exists ("/dev/cu.usbserial-210")) {
						fprintf(stderr, "Attempt 210 connect....");
						tpms_serial_port = open("/dev/cu.usbserial-210", O_RDWR);
						//tty.usbserial-1420
						fprintf(stderr, "TPMS interface connected!"); 
					} else {
						fprintf(stderr, "TPMS interface NOT connected!"); 
						return 0;					
					}
				}
			}
		}		
	}



	fprintf (stderr, "Got here...");

	struct termios tty;
	memset(&tty, 0, sizeof(tty));

	// Read in existing settings, and handle any error
	if(tcgetattr(tpms_serial_port, &tty) != 0) {
	    //printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
	}

	tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
	tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
	tty.c_cflag |= CS8; // 8 bits per byte (most common)
	tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag &= ~ECHOE; // Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo
	tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	// tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
	// tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

	tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
	tty.c_cc[VMIN] = 0;

	// Set in/out baud rate to be 115200
	cfsetispeed(&tty, B19200);
	cfsetospeed(&tty, B19200);

		// Save tty settings, also checking for error
	if (tcsetattr(tpms_serial_port, TCSANOW, &tty) != 0) {
	    printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
	}

	return 0;
}


void* pollTPMSInterface(void *arg)
{

	char appendbuf [256];
	
	char read_buf [256];
	bool quit = false;
	int appendpointer = 0;

	connectTPMSInterface();

	while (quit == false) {
		memset(&read_buf, '\0', sizeof(read_buf));
		int num_bytes = read(tpms_serial_port, &read_buf, sizeof(read_buf));

		// n is the number of bytes read. n may be 0 if no bytes were received, and can also be -1 to signal an error.
		if (num_bytes < 0) {
		    printf("Error reading: %s\n", strerror(errno));
		    close (tpms_serial_port);
		    connectTPMSInterface();
		} else {
			fprintf(stderr, "Num bytes: %i\n", num_bytes); 
		}


		for (int r=0;r<num_bytes;r++) {
			
			if (read_buf[r] == -86) {
				if (r < (num_bytes-1)) {
					if (read_buf[r+1] == -95) {
						fprintf(stderr, "\n");
						
						appendpointer = 0;
						memset (appendbuf, 0, 256);
						appendbuf[appendpointer] = read_buf[r];
						appendpointer++;				
					}
				}
				
			} else {
				if (appendpointer < 256) {
					appendbuf[appendpointer] = read_buf[r];
					appendpointer++;
				}
			}

			fprintf(stderr, "%i ", read_buf[r]);
		}



		if (appendbuf[0] == -86 && appendpointer > 13 && appendbuf[1] == -95) {
			
			int sensornum = appendbuf[5];
			int tpbyte1 = appendbuf[9];
			int tpbyte2 = appendbuf[10];
			int tempbyte = appendbuf[11];
			int statebyte = appendbuf[12];
			int sensorstate = 0;

			double bar = 0.025 * ((double) (((tpbyte1 & 3) * 256) + (tpbyte2 & 255)));
			double psi = bar * 14.5;
			int celcius = (tempbyte & 255) - 50;

			if ((statebyte & 3) == 1) {
				sensorstate = FASTLEAK;
			} else if ((statebyte & 16) == 16) {
				sensorstate = HIGHPRESSURE;
			} else if ((statebyte & 8) == 8) {
				sensorstate = LOWPRESSURE;
			} else if ((statebyte & 4) == 4) {
				sensorstate = HIGHTEMP;
			} else if ((statebyte & -128) == 128) {
				sensorstate = LOWBATTERY;
			} else {
				sensorstate = NORMAL;
			}


			if (sensornum == 1) {
				gSensor1bar = bar;
				gSensor1psi = psi;
				gSensor1temp = celcius;
				gSensor1state = sensorstate;
			}

			if (sensornum == 2) {
				gSensor2bar = bar;
				gSensor2psi = psi;
				gSensor2temp = celcius;
				gSensor2state = sensorstate;
			}

			if (sensornum == 3) {
				gSensor3bar = bar;
				gSensor3psi = psi;
				gSensor3temp = celcius;
				gSensor3state = sensorstate;
			}

			if (sensornum == 4) {
				gSensor4bar = bar;
				gSensor4psi = psi;
				gSensor4temp = celcius;
				gSensor4state = sensorstate;
			}
			

			fprintf (stderr, "\nPSI: S1: %f, S2: %f, S3: %f, S4: %f\n", gSensor1psi, gSensor2psi, gSensor3psi, gSensor4psi);
			fprintf (stderr, "TMP: S1: %i, S2: %i, S3: %i, S4: %i\n", gSensor1temp, gSensor2temp, gSensor3temp, gSensor4temp);
			fprintf (stderr, "STATE: S1: %s, S2: %s, S3: %s, S4: %s\n", stringSensorstate(gSensor1state), stringSensorstate(gSensor2state), stringSensorstate(gSensor3state), stringSensorstate(gSensor4state));
			
			appendpointer = 0;
			memset (appendbuf, 0, 256);
		}		
		
	}
	

	close(tpms_serial_port);

	return 0;
}



int main(int argc, char* args[]) {

	/*
	int res = 40 & 3;
	fprintf(stderr, "40 AND 3 - %i\n", res);

	res = 40 & 16;
	fprintf(stderr, "40 AND 16 - %i\n", res);

	res = 40 & 8;
	fprintf(stderr, "40 AND 8 - %i\n", res);

	res = 40 & 4;
	fprintf(stderr, "40 AND 4 - %i\n", res);

	res = 40 & -128;
	fprintf(stderr, "40 AND -128 - %i\n", res);
	
	return 0;
	*/

	int err;
	err = pthread_create(&(tpms_tid[0]), NULL, &pollTPMSInterface, NULL);
    if (err != 0)
        printf("\ncan't create TPMS thread :[%s]", strerror(err));
    else
        printf("\n TPMS Thread created successfully\n");
		
    while (true) {

    }
}