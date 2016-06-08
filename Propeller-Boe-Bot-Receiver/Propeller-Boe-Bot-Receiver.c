#include "simpletools.h"
#include "servo.h"
#include "fdserial.h"
#include "wavplayer.h"
#include "servodiffdrive.h"
#include "ping.h"                             // Include ping header

#define PIN_SOUND       26 // pin for the speaker, default=26
#define PIN_XBEE_RX      0
#define PIN_XBEE_TX      1
#define PIN_SERVO_LEFT  14
#define PIN_SERVO_RIGHT 15
#define PIN_SERVO_MID 16

volatile int ch;
volatile int XeeChange=0;
volatile int cmDist=0;
volatile int turn=0;
volatile unsigned int lockR;            // Lock ID 
int DO = 22, CLK = 23, DI = 24, CS = 25;        // SD I/O pins
int * cog_ptr[9];
const char *WAVfile[] = {"jagger.wav","somebody.wav","callme.wav","rolling.wav","origin.wav","techloop.wav"};       // Set up techloop string
int degree[] = {1080,1260,1440,1620,1800,
                1620,1440,1260,1080,900,
                720,540,360,180,0,
                180,360,540,720,900};
int degreeindex=0;
int pinLeft, pinRight, rampLeft, rampRight;   // Variables shared by functions
volatile fdserial *xbee;

void XBee();
void music(int ch);
void robot();
void PING();
char char2lowercase (char c);

//int cmDist;

void getDist() // main function
{
while(1){
cmDist = ping_cm(17); // Get cm distance from Ping)))
print("cmDist = %d\n", cmDist); // Display distance
pause(200); // Wait 1/5 second
} 
}


int main (void) {
  xbee = fdserial_open(PIN_XBEE_RX, PIN_XBEE_TX, 0, 9600);
  sd_mount(DO, CLK, DI, CS);                      // Mount SD card
  drive_pins(PIN_SERVO_LEFT, PIN_SERVO_RIGHT);    // Set the Left and Right servos
  print("=================================\n");
  print("  Propeller Boe-Bot Audio Player \n");
  print("=================================\n\n");
  print("[1] Move like jagger\n");
  print("[2] Somebody that I used to know\n");
  print("[3] Call me maybe\n");
  print("[4] Rolling in the deep\n");
  print("[5] Origin\n");
  print("[6] Techloop\n\n");
  
  print("[X] Stop playback\n");
  print("[S] Stop robot\n");
  print("[R] Start robot\n");
  
  cog_ptr[1] = cog_run(&XBee,32);
  
  //int *cog_dist = cog_run(getDist,128);
  int distance;
  
  while(1)
  {
    distance = ping_cm(17);
    print("%d", distance);
    
    if(distance > 5){

      if(XeeChange)
      {
        ch = char2lowercase(ch);
        
        switch ((char) ch)
        {
          case '1':
            servo_speed(18, -20); /* set pin 14 to 50% of full speed */
            servo_speed(19, 20); /* set pin 15 to 50% of full speed */ 
            pause(500);
            break;
          case '2':
            servo_speed(18, -20); /* set pin 14 to 50% of full speed */
            servo_speed(19, -20); /* set pin 15 to 50% of full speed */ 
            pause(500);
            break;
          case '3':
            servo_speed(18, 20); /* set pin 14 to 50% of full speed */
            servo_speed(19, 20); /* set pin 15 to 50% of full speed */ 
            pause(500);
            break;
          case '4':
            servo_speed(18, 10); 
            servo_speed(19, -10); 
            pause(500);
            break;          

          case '0':
            servo_speed(18, 0); /* set pin 14 to 50% of full speed */
            servo_speed(19, 0); /* set pin 15 to 50% of full speed */ 
            pause(500);
            break;
          /*  
          case '6':
            music(ch);
            break;
          case 'x':
            wav_stop();                                      // Stop playing
            break;
          case 'r':
            cog_ptr[6] = cog_run(&robot,64);                //Ask a cog to run the function robot()
            break;
          case 's':
            cog_end(cog_ptr[7]);
            cog_end(cog_ptr[6]);
            break;
          */  
          default:
             print("%c", (char) ch);
        }     
        XeeChange = 0;
      }   
    }    
    else if(distance < 5){
      break;
      //break;
      /*
      servo_speed(18, 25);
      servo_speed(19, 25);
      pause(2000);
      servo_stop();
      */  
    }    
    pause(1);
  }    
  
  
  servo_speed(18, 0); /* set pin 14 to 50% of full speed */
  servo_speed(19, 0); /* set pin 15 to 50% of full speed */ 

  int k = 1;

 int a=1;

 while(1)
  {
      
      if(a == 1)
      {
        ch = char2lowercase(ch);

        switch ((char) ch)
        {
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
            music(ch);
            break;
          /*  
          case 'x':
            wav_stop();                                      // Stop playing
            break;
          case 'r':
            cog_ptr[6] = cog_run(&robot,64);                //Ask a cog to run the function robot()
            break;
          case 's':
            cog_end(cog_ptr[7]);
            cog_end(cog_ptr[6]);
            break;
          */
          default:
             music(ch);
             break;
        }     
        a = 0;
      }     
      pause(1);
  }    

  
  return 0;
}


void XBee(){
   
   putchar(CLS);
   fdserial_rxFlush(xbee);
   while(1){
     ch = fdserial_rxChar(xbee);
     XeeChange = 1;
     fdserial_txChar(xbee, ch);
     fdserial_txFlush(xbee);
   }
}  

void music(int ch){
  wav_stop();
  wav_play(WAVfile[ch-(int)'0']);                 // Pass the wavfile name to wav player (cost 2 core)
  //wav_play(WAVfile[ch]);
  wav_volume(12);                                  // Adjust volume
}  

void move(){
  while(1){
    if(cmDist>=10){ 
      servo_speed(18, -20); /* set pin 14 to 50% of full speed */
      servo_speed(19, 20); /* set pin 15 to 50% of full speed */
    } 
    else{
      servo_speed(18, 0); /* set pin 14 to 50% of full speed */
      servo_speed(19, 0); /* set pin 15 to 50% of full speed */
    } 
 }
}  

void robot(){
  lockR = locknew();                              // Check out a lock
  servo_angle(PIN_SERVO_MID, 900);                // Turn the standard servo to 90 degrees
  servo_stop();                                   // Close those Cogs which control the servo 
  cog_ptr[7] = cog_run(&PING,16);                 // Ask a cog to run the function PING()
  while(1){
    pause(10);
    while(lockset(lockR));                        // Set the lock
      if(cmDist>=40){               
        servo_speed(18, -50); /* set pin 14 to 50% of full speed */
        servo_speed(19, -50); /* set pin 15 to 50% of full speed */                 
        pause(100);
        turn = rand()%2;
      }
      else if(cmDist>=20){
        if (turn){
           servo_speed(18, -50); /* set pin 14 to 50% of full speed */
           servo_speed(19, 50); /* set pin 15 to 50% of full speed */  
        }                
        else{
           servo_speed(18, 50); /* set pin 14 to 50% of full speed */
           servo_speed(19, -50); /* set pin 15 to 50% of full speed */    
        }        
        pause(100);
      }
      else
      {
           servo_speed(18, 50); /* set pin 14 to 50% of full speed */
           servo_speed(19, 50); /* set pin 15 to 50% of full speed */              
        pause(100);
      }
      servo_stop();
      degreeindex=(++degreeindex)%20;
      servo_angle(PIN_SERVO_MID, degree[degreeindex]);
      pause(50);
      servo_stop();
      cog_ptr[7] = cog_run(&PING,16);    
      lockclr(lockR);

  }    
}  

void PING(){                //Use Ping))) to measure distance
  while(lockset(lockR));   //Set the lock 
    cmDist = ping_cm(17);   //get the distance
    lockclr(lockR);        // Clear the lock
    cog_end(cog_ptr[7]);    //close itself
}  


char char2lowercase (char c) {
	return ('A'<=c && c<='Z') ? c+32 : c;
}
