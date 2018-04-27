#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "rs232.h"

static int COM_PORT = -1;
uint8_t image[360][360];

void dump( uint8_t* Matrix, int w, int h ){
    FILE *out;
    
    out = fopen("img.txt","w");
    
    char str[w*h+1];
    
    for( int i = 0; i < w; ++i ) {
        for( int j = 0; j < h; ++j ) {
            char sub[5];
            memset(sub, 0, sizeof(sub));
            sprintf(str, "%d ", Matrix[j*w+i] );
            strcat(str, sub);
        }
        strcat(str, "\n");
    }
    
    fflush(out);
    fclose(out);
}

void writeBMP( uint8_t Matrix[360][360], int w, int h ){
    FILE *out;
    long pos = 0;
 
    out = fopen("img.bmp","wb");
 
    // Sizes
    static const uint8_t headerSize = 14;
    static const uint8_t infoHeaderSize = 40;
    static const uint16_t colorTableSize = 4*256;
    
    // Header
    uint8_t signature[] = {'B','M'};
    uint32_t filesize = headerSize + infoHeaderSize + colorTableSize + w*h;
    uint32_t reserved = 0;
    uint32_t offset = headerSize + infoHeaderSize + colorTableSize;
    // Info Header
    uint32_t ihsize = infoHeaderSize;
    uint32_t width = w;
    uint32_t height = h;
    uint16_t planes = 1;
    uint16_t bpp = 8;    
    uint32_t comp = 0;
    uint32_t imagesize = w*h;
    uint32_t xppm = 1;
    uint32_t yppm = 1;
    uint32_t colors = 256;
    uint32_t impcolors = 0;
    // Color Table
    uint8_t ctable[colorTableSize];
    
    int off = 0;
    
    for( int i = 0; i < colors; i++ ){
        ctable[off] = i;
        ctable[off+1] = i;
        ctable[off+2] = i;
        ctable[off+3] = 0;
        off += 4;
    }
    
    // Header
    fseek(out,pos+=fwrite(&signature,1,2,out),0);
    fseek(out,pos+=fwrite(&filesize, 1,4,out),0);
    fseek(out,pos+=fwrite(&reserved, 1,4,out),0);
    fseek(out,pos+=fwrite(&offset,   1,4,out),0);
    // Info Header
    fseek(out,pos+=fwrite(&ihsize,   1,4,out),0);
    fseek(out,pos+=fwrite(&width,    1,4,out),0);
    fseek(out,pos+=fwrite(&height,   1,4,out),0);
    fseek(out,pos+=fwrite(&planes,   1,2,out),0);
    fseek(out,pos+=fwrite(&bpp,      1,2,out),0);
    fseek(out,pos+=fwrite(&comp,     1,4,out),0);
    fseek(out,pos+=fwrite(&imagesize,1,4,out),0);
    fseek(out,pos+=fwrite(&xppm,     1,4,out),0);
    fseek(out,pos+=fwrite(&yppm,     1,4,out),0);
    fseek(out,pos+=fwrite(&colors,   1,4,out),0);
    fseek(out,pos+=fwrite(&impcolors,1,4,out),0);
    
    fseek(out,pos+=fwrite(&ctable,   1,colorTableSize,out),0);
    
    uint8_t padding = 0;
    
    for( int i = h-1; i >= 0; --i ) {
        for( int j = 0; j < width; ++j ) {
            fseek(out,pos += fwrite(&Matrix[i][j],1,1,out),0);
        }
        for( int j = 0; j < width%4; ++j ) {
            fseek(out,pos += fwrite(&padding,1,1,out),0);
        }
    }
 
    fflush(out);
    fclose(out);
}

// Wait for "ready" status from the Arduino.
static void waitRDY(void) {
	unsigned char tempC;
	uint32_t junkC = 0;
	while(1) {
		RS232_SendByte(COM_PORT, 'b');
		printf("Sending begin signal\n");
		RS232_PollComport(COM_PORT, &tempC, 1);
		printf("Read: %d\n", tempC);
		if (tempC != 'r') {
			junkC++;
			printf("Junk Char %d or %c while waiting for %c so far skipped %d\n", tempC, tempC, 'r', junkC);
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

    memset(image, 0, sizeof(image));
    
//     int w = 100, h = 200;
//     uint8_t test[w*h];
//     for(int i = 0; i < h; ++i) for(int j = 0; j < w; ++j) test[i*w+j] = i;
//     writeBMP( test, w, h ); 
//     return 0;
    
	if (argc != 2) {
		help(argv);
		return 1;
	}
	
	// Assign COM port number.
	COM_PORT = strtoul(argv[1], NULL, 10);

	// Display info.
	printf("Opening COM port %d\n", COM_PORT);

	// Open COM port.
	if (RS232_OpenComport(COM_PORT, 4800)) {
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
	printf( "- Image resolution: %dx%d\n", width, height );

//     image = (unsigned char*)malloc(height*width*sizeof(unsigned char));
    unsigned x = 0, y = 0;
    
    while(1) {
        unsigned char c;
        RS232_PollComport(COM_PORT, &c, 1);
//         printf("Recieved: %d\n", (int)c);
        
		image[y][(y%2 == 0 ? x : width-x)] = c;
        x++;
		if( x == width ) { x = 0; y++; printf("Progress: %d/%d\n", y, height); }
		if( y == height ) break;
// 		RS232_SendByte(COM_PORT, 'A');        
    }

    //createBMP( image, width, height );
    printf("- Writing image file\n");
    writeBMP( image, width, height );
    
//     printf("- Dumping data to file\n");
//     dump( image, width, height );
    
//     printf("\n");
//     for( int i = 0; i < height; ++i ) {
//         for( int j = 0; j < width; ++j ) {
//             char str[5];
//             memset(str, 0, sizeof(str));
//             sprintf(str, "%d ", image[i][j] );
//             if(image[i][j] < 100) strcat(str, " ");
//             if(image[i][j] < 10) strcat(str, " ");
//             printf("%s", str);
//         }
//         printf("\n");
//     }
//     printf("\n");
    
//     free(image);
   
	printf("- Scanning finished\n");

	// Successful exit.
	return 0;
}
