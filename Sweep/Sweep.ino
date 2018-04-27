#include <Servo.h> 
#include <NewPing.h>
#include <Wire.h>

#define MAX_PITCH 128
#define MAX_YAW 128

#define PITCH_PIN 11
#define YAW_PIN 12
#define ECHO_PIN 10
#define TRIG_PIN 9

const unsigned char width = 64;
const unsigned char height = 64;

Servo pitch, yaw;  
NewPing sonar(TRIG_PIN, ECHO_PIN, 255);
 
const int p_beg = (MAX_PITCH-height)/2;
const int p_end = p_beg+height;
const int y_beg = (MAX_YAW-width)/2;
const int y_end = y_beg+width;

#define ADDR 0x5A
#define TOBJ1 0x07

unsigned ReadReg( uint8_t a ) {
  
  unsigned val = 0;
  
  Wire.beginTransmission( ADDR );
  Wire.write( a );
  Wire.endTransmission( false );

  Wire.requestFrom( ADDR, (uint8_t)3 );
  val = Wire.read();
  val |= (Wire.read() << 8);

  delay(50);
  return val;
}

void setup() {
  
  Serial.begin(4800);

  Wire.begin();
  while( Serial.available() ) {
    char junk = Serial.read();
  }
  
  pitch.attach(PITCH_PIN);        
  pitch.write(p_end);
  yaw.attach(YAW_PIN);  
  yaw.write(y_beg);
  
  float temp = (ReadReg( TOBJ1 )* 0.02f) - 273.15;
  Serial.print("Temperature: ");  Serial.println( temp );
  
  while(1) {
    while( !Serial.available() ) {}
    if(Serial.read() == 'b') {
       delay(1000);
       Serial.write('r');
       delay(100);
       Serial.write(width);
       delay(100);
       Serial.write(height);
       delay(100);
       break;
    }
    
  }

  bool dir = 0;

  for( unsigned p = p_end; p >= p_beg ; --p ) {
    pitch.write( p ); 
    for( unsigned y = y_beg; y < y_end; ++y ) {
      Serial.write( (unsigned char)sonar.ping_cm() );
      yaw.write( dir ? y : (y_end - y) + y_beg ); 
      delay(50);  
    }
    dir = !dir;
  }
  
}
 
 
void loop() {} 
