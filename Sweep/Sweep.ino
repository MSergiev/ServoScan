#include <Servo.h> 
#include <NewPing.h>
#include <Wire.h>

#define MAX_PITCH 2000 //128
#define MAX_YAW 2000 //128

#define PITCH_PIN 11
#define YAW_PIN 12
#define ECHO_PIN 9
#define TRIG_PIN 8

const unsigned short width = 500;
const unsigned short height = 500;

Servo pitch, yaw;  
NewPing sonar(TRIG_PIN, ECHO_PIN, 255);
 
const unsigned p_beg = (MAX_PITCH-height)/2 + 600;
const unsigned p_end = p_beg + height;
const unsigned y_beg = (MAX_YAW-width)/2 + 600;
const unsigned y_end = y_beg+width;

int p = p_end-1;
int y = y_beg;

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

  delay(60);
  return val;
}

inline float readTemp() {
  return ((ReadReg( TOBJ1 )* 0.02f) - 273.15f);
}

void begin() {
  
  while( Serial.available() ) {
    char junk = Serial.read();
  } 
  
  pitch.writeMicroseconds(p_end);
  yaw.writeMicroseconds(y_beg);

  while(1) {
    while( !Serial.available() ) {
      //Serial.print("T: ");
      //Serial.println(readTemp());
    }
    
    if(Serial.read() == 'b') {
       delay(1000);
       Serial.write('r');
       delay(100);
       Serial.write( (unsigned char*)(&width), 2 );
       delay(100);
       Serial.write( (unsigned char*)(&height), 2 );
       delay(100);
       break;
    }
  }
  
}

void scan() {
  
  for( unsigned p = p_end-1; p >= p_beg ; --p ) {
    pitch.writeMicroseconds( p ); 
    for( unsigned y = y_beg; y < y_end; ++y ) {  
      unsigned char s = (unsigned char)sonar.ping_cm();
      short t = (short)(readTemp()*100);
      Serial.write( s );
      Serial.write( (unsigned char*)(&t), 2 );
      yaw.writeMicroseconds( p%2 == 1 ? y : (y_end - y) + y_beg - 1 ); 
      //delay(100);  
    }
  }
  
}

void getNext() {
  
  unsigned char s = (unsigned char)sonar.ping_cm();
  short t = (short)(readTemp()*100);
  Serial.write( s );
  Serial.write( (unsigned char*)(&t), 2 );
  pitch.writeMicroseconds( p );
  yaw.writeMicroseconds( p%2 == 1 ? y : (y_end - y) + y_beg - 1 ); 
  ++y;
  if( y == y_end ) { 
    y = y_beg; 
    --p; 
  }
}

void setup() {
  
  Serial.begin(9600);

  digitalWrite(2, HIGH);
  delay(500);
  digitalWrite(2, LOW);
  
  Wire.begin();

  
  pitch.attach(PITCH_PIN);        
  yaw.attach(YAW_PIN); 
 
}
 
 
void loop() {
  
  begin();
  
  scan();  
  
} 
