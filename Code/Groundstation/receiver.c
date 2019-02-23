#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <libusb.h>


struct axis{
	double dista;
	double veloc;
	double accel;
};

// Parse the packet for the timestamp, so we know when this was sent
double parse_time(){
	
}

// Read packet, parse into variables
void parse_packet(int packet, struct axis* x, struct axis* y, struct axis*z, double* temperature, int* timestamp){
	
}


int main(){
	struct axis x;
	struct axis y;
	struct axis z;
	double temperature;
	double thrust;
	double mass;
	double magnetometer; // May be unnecessary
	int timestamp;
	int stop = 0; // Becomes true when we read a stop signal.
	
	
	char outfile[50]; // File path
	memset(outfile, '\0', sizeof(outfile));
	time_t rawtime = time(NULL);
	struct tm *timeinfo = localtime(&rawtime);
	strftime(outfile, sizeof(outfile), "%F-%T.JSON", timeinfo);
	int file = open(outfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	
	// USB setup
	libusb_device_handle* interface = NULL; // Handle for antenna interface USB connection
	libusb_device** devs; //pointer to pointer of device, used to retrieve a list of devices
	libusb_context* ctx = NULL; //a libusb session
	int ret;
	char packet[64];
	ssize_t cnt; // holding number of devices in list
	ret = libusb_int(&ctx); // initialize library session
	
	libusb_set_debug(ctx, 3);
	while(interface == NULL){
		int devslen = 0;
		while(devslen < 1) // Can't proceed until we see the antenna interface. It should be plugged in before starting this, but just in case, don't try to read from a non-existant interface.
			devslen = libusb_get_device_list(ctx, &devs);
		
		interface = libusb_open_device__with_vid_pid(ctx, VID, PID);
		
		libusb_free_device_list(devs, 1);
		free(devs);	
	}
	
	if(libusb_kernel_driver_active(interface, 0) == 1)
		while(libusb_detach_kernel_driver(interface, 0)); // While the kernel is trying to use the interface, keep trying to detach it
	
	// may need to set configuration here?
	
	ret = libusb_claim_interface(interface, 0); // Dibs the interface so our parents don't try to use the phone while we're on the modem
	
	do{
		// Read in data
		// Read from interface,
		// Read until we encounter a null-terminator
		// Read into packet// Read no more than 64 bytes
		// Store number of bytes in ret
		// Timeout of 1000ms (1s)
		int err = libusb_bulk_transfer(interface, '\0', packet, 64, ret, 1000);
		// Check status flags (no gps, staging event, etc)
		
		// Compare timestamp
	//	int newtime = parse_time();
		
		// If staging, output special line
		
		// else, parse for variables
	//	parse_packet(packet, &x, &y, &z, &temperature, &timestamp);
		
		// Write variables to file
		write(file, packet, strlen(packet));
		
	} while(!stop); // When we read a stop signal, no more data
	
	libusb_release_interface(interface, 0);
	libusb_close(interface);
	libusb_exit(ctx); // Deinitialize library
	close(file);
	return 0;
}



