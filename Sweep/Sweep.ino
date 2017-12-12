#include <Servo.h> 
#include <NewPing.h>

#define MAX_PITCH 120
#define MAX_YAW 132

#define PITCH_PIN 9
#define YAW_PIN 10
#define ECHO_PIN 11
#define TRIG_PIN 12

const unsigned char width = MAX_YAW;
const unsigned char height = MAX_PITCH;

Servo pitch; 
Servo yaw;  
NewPing sonar(TRIG_PIN, ECHO_PIN, 255);
 
const int p_beg = (MAX_PITCH-height)/2;
const int p_end = p_beg+height;
const int y_beg = (MAX_YAW-width)/2;
const int y_end = y_beg+width;

int p_pos = p_end;
int y_pos = p_beg;
 
void setup() {
  
  Serial.begin(9600);
  pitch.attach(PITCH_PIN); 
  yaw.attach(YAW_PIN);
    
  while(1) {
      if(Serial.read() == 'B') {
         pitch.write(MAX_PITCH/2);
         yaw.write(MAX_YAW/2);
         delay(1000);
         Serial.write('R');
         delay(1000);
         Serial.write(width);
         delay(100);
         Serial.write(height);
         break;
      }
  }  
} 
 
 
void loop() { 
  
  unsigned char dist = (unsigned char)sonar.ping_cm();
  
  delay(30); 
  Serial.write( dist );
  
  while(1) {
    if(Serial.read() == dist) break;
  }
  
  y_pos++;
  
  if( y_pos > y_end ) {
    y_pos = y_beg;
    p_pos--;
  }
  
  pitch.write(p_pos);   
  yaw.write(y_pos); 
 
 if( p_pos < p_beg ) {
    while(1){} 
 }
} 
