#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "rs232.h"

static int COM_PORT = -1;
uint8_t* image;

static void createBMP( unsigned char* data, unsigned char w, unsigned char h ) {
    FILE *f;
    unsigned char *img = NULL;
    int filesize = 54 + 3*w*h;  //w is your image width, h is image height, both int

    img = (unsigned char *)malloc(3*w*h);
    memset(img,0,3*w*h);

    for(int i=0; i<w; i++)
    {
        for(int j=0; j<h; j++)
        {
            int x=i, y=(h-1)-j;
            img[(x+y*w)*3+2] = data[i*j];
            img[(x+y*w)*3+1] = data[i*j];
            img[(x+y*w)*3+0] = data[i*j];
        }
    }

    unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
    unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
    unsigned char bmppad[3] = {0,0,0};

    bmpfileheader[ 2] = (unsigned char)(filesize    );
    bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
    bmpfileheader[ 4] = (unsigned char)(filesize>>16);
    bmpfileheader[ 5] = (unsigned char)(filesize>>24);

    bmpinfoheader[ 4] = (unsigned char)(       w    );
    bmpinfoheader[ 5] = (unsigned char)(       w>> 8);
    bmpinfoheader[ 6] = (unsigned char)(       w>>16);
    bmpinfoheader[ 7] = (unsigned char)(       w>>24);
    bmpinfoheader[ 8] = (unsigned char)(       h    );
    bmpinfoheader[ 9] = (unsigned char)(       h>> 8);
    bmpinfoheader[10] = (unsigned char)(       h>>16);
    bmpinfoheader[11] = (unsigned char)(       h>>24);

    f = fopen("img.bmp","wb");
    fwrite(bmpfileheader,1,14,f);
    fwrite(bmpinfoheader,1,40,f);
    for(int i=0; i<h; i++)
    {
        fwrite(img+(w*(h-i-1)*3),3,w,f);
        fwrite(bmppad,1,(4-(w*3)%4)%4,f);
    }

    free(img);
    fclose(f);
}

// Wait for "ready" status from the Arduino.
static void waitRDY(void) {
	unsigned char tempC;
	uint32_t junkC = 0;
	while(1) {
		printf("Sending begin signal\n");
		RS232_SendByte(COM_PORT, 'B');
		RS232_PollComport(COM_PORT, &tempC, 1);
		printf("Read: %d\n", tempC);
		if (tempC != 'R') {
			junkC++;
			printf("Junk Char %d or %c while waiting for %c so far skipped %d\n", tempC, tempC, 'R', junkC);
		} else break;
	}
	printf("Scanner acknowledged\n");
	if (junkC != 0)
		printf("/n%d junk bytes skipped\n", junkC);
}

// Show COM port list.
static void printCOM() {
	printf("\nCOM Port ID Table:\n");
	for (unsigned i = 0; i < sizeof(comports) / sizeof(comports[0]); ++i)
		printf("\t %d %s\n", i, comports[i]);
}

// Show help info.
static void help(char** argv) {
	printf("Usage: %s COM_PORT_ID\n", argv[0]);
	// Print the comport IDs.
	printCOM();
}


// Main
int main(int argc, char** argv) {

	// Determine flashing/dumping mode.
	if (argc != 2) {
		help(argv);
		return 1;
	}
	
	// Assign COM port number.
	COM_PORT = strtoul(argv[1], NULL, 10);

	// Display info.
	printf("Opening COM port %d\n", COM_PORT);

	// Open COM port.
	if (RS232_OpenComport(COM_PORT, 9600)) {
		printf("ERROR: COM port %i could not be opened\n", COM_PORT);
		printCOM();
		return 1;
	}

	// Wait for RDY from the Arduino.
	waitRDY();

	printf("\n- Scanner ready\n");

	unsigned char width, height;
	RS232_PollComport(COM_PORT, &width, 1);
	RS232_PollComport(COM_PORT, &height, 1);
	printf( " Image resolution: %dx%d\n", width, height );

    image = (unsigned char*)malloc(height*width*sizeof(unsigned char));
    unsigned count = 0;
    
    while(count < width*height) {
        unsigned char c;
        RS232_PollComport(COM_PORT, &c, 1);
        image[count] = c;
        count++;
		RS232_SendByte(COM_PORT, c);
// 		printf("%d ", c);
// 		if(count%width==0) printf("\n");
        if(count%width==0) printf("Progress: %d/%d\n", (count/width), width);
        
    }

    createBMP( image, width, height );
    
    free(image);
   
	printf("Scanning finished\n");

	// Successful exit.
	return 0;
}
