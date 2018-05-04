#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "rs232.h"

static int COM_PORT = -1;
unsigned short w = 0;
unsigned short h = 0;
uint8_t* depth;
uint8_t* thermal;
int* rawDepth;
int* rawThermal;

void dump( uint8_t* Matrix ){
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

uint8_t map( int v, int bf, int bt ) {
    return (uint8_t)( (((float)v - bf)/std::abs(bt-bf))*255 );
}

void writeBMP( uint8_t* Matrix, const char* filename ){
    FILE *out;
    long pos = 0;
 
    out = fopen(filename,"wb");
 
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
            fseek(out,pos += fwrite(&Matrix[i*h+j],1,1,out),0);
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
		if (tempC != 'r') {
			junkC++;
			printf("Junk Char %d or %c while waiting for %c so far skipped %d\n", tempC, tempC, 'r', junkC);
		} else break;
	}
	printf("Scanner acknowledged\n");
	if (junkC != 0) printf("/n%d junk bytes skipped\n", junkC);
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

    {       
        unsigned char buf[4];
        memset( buf, 0, sizeof(buf) );
        unsigned char rec = 0;
        
        while( rec < sizeof(buf) ) {
            rec += RS232_PollComport(COM_PORT, &buf[rec], 1);
        }
        
        w = *((unsigned short*)(&buf[0]));
        h = *((unsigned short*)(&buf[2]));
        
        printf( "- Image resolution: %dx%d\n", w, h );  
    }
    
    depth = new uint8_t[w*h];
    thermal = new uint8_t[w*h];
    rawDepth = new int[w*h];
    rawThermal = new int[w*h];

    unsigned x = 0, y = 0;
    short minThermal = SHRT_MAX, maxThermal = SHRT_MIN;
    unsigned char minDepth = UCHAR_MAX, maxDepth = 0;
    
    while(1) {        
        unsigned loc = y * h + (y%2 == 1 ? x : w - x - 1);
        
        unsigned char buf[3];
        memset( buf, 0, sizeof(buf) );
        unsigned char rec = 0;
        
        while( rec < sizeof(buf) ) {
            rec += RS232_PollComport(COM_PORT, &buf[rec], 1);
        }
        
        rawDepth[loc] = buf[0];
        rawThermal[loc] = *((short*)(&buf[1]));
        
        printf("Progress: %d/%d\n", loc, w*h); 
//         printf("Read: %d/%d\n", rawDepth[loc], rawThermal[loc]); 
        
        if( rawDepth[loc] > maxDepth ) maxDepth = rawDepth[loc];
        else if( rawDepth[loc] < minDepth ) minDepth = rawDepth[loc];
        if( rawThermal[loc] > maxThermal ) maxThermal = rawThermal[loc];
        else if( rawThermal[loc] < minThermal ) minThermal = rawThermal[loc];
        
        x++;
        if( x == w ) { 
            x = 0; 
            y++; 
            printf("Progress: %d/%d\n", y, h); 
        }
        if( y == h ) break; 
    }

    for( unsigned i = 0; i < w*h; ++i ) {
        depth[i] = map( rawDepth[i], minDepth, maxDepth );
        thermal[i] = map( rawThermal[i], minThermal, maxThermal );
    }
    
    printf("- Writing image files\n");
    writeBMP( depth, "depth.bmp");
    writeBMP( thermal, "thermal.bmp" );
   
    printf("- Scanning finished\n");

    
    printf("Min: %d/%d\n", minDepth, minThermal );
    printf("Max: %d/%d\n", maxDepth, maxThermal );
//     printf("Data\n");
//     for( unsigned i = 0; i < w*h; ++i ) {
//         printf("%d/%d\n", depth[i], thermal[i] );
//     }
    
    delete[] depth;
    delete[] thermal;
    delete[] rawDepth;
    delete[] rawThermal;
    
    RS232_CloseComport(COM_PORT);
    
    // Successful exit.
    return 0;
    
}
