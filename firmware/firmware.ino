// Bike model selection - uncomment ONE
//#define BIKE_FZS1000
#define BIKE_FZS600

#include <Eeprom24C32_64.h>

#include <Wire.h>
#include <util/atomic.h>
#include "RTClib.h"

#include <SoftwareSerial.h>

// Pin Mapping (Gen4 board — ATMega32u4)
#define SPEEDSENSORPIN 0
#define RPMSENSORPIN  1
#define FUELPIN     A2
#define COOLANTPIN  A3
#define BATTERYPIN  A4
#define OILLEVELPIN   4 // Goes LOW when oil level is ok 
#define NEUTRALPIN  5 // Goes LOW when in Neutral
#define HIGHBEAMPIN 9 // Goes high when Lit
#define FANPIN   12  // Set high to turn on fan
#define INDICATELEFTPIN  10 // Goes HIGH when lit
#define INDICATERIGHTPIN  11 // Goes HIGH when lit
#define AMBIENTTEMPPIN  A0
#define LIGHTSENSORPIN  A1
// Option and Select buttons
#define OPTIONPIN 8
#define SELECTPIN 7

#define EEPROM_ADDRESS 0x57

// Bike-specific constants
#ifdef BIKE_FZS1000
  #define RPM_CONSTANT          29.60
  #define WHEEL_DIAMETER_M      0.629   // 180/55 ZR17
  #define PRIMARY_DRIVE_RATIO   1.581   // 68/43
  #define GEAR1_RATIO           ((double)35 / 14)  // 2.500
  #define GEAR2_RATIO           ((double)35 / 19)  // 1.842
  #define GEAR3_RATIO           ((double)30 / 20)  // 1.500
  #define GEAR4_RATIO           ((double)28 / 21)  // 1.333
  #define GEAR5_RATIO           ((double)30 / 25)  // 1.200
  #define GEAR6_RATIO           ((double)29 / 26)  // 1.115
#endif

#ifdef BIKE_FZS600
  #define RPM_CONSTANT          29.60
  #define WHEEL_DIAMETER_M      0.624   // 160/60 ZR17
  #define PRIMARY_DRIVE_RATIO   1.708   // 82/48
  #define GEAR1_RATIO           ((double)37 / 13)  // 2.846
  #define GEAR2_RATIO           ((double)37 / 19)  // 1.947
  #define GEAR3_RATIO           ((double)34 / 22)  // 1.545
  #define GEAR4_RATIO           ((double)28 / 21)  // 1.333
  #define GEAR5_RATIO           ((double)25 / 21)  // 1.190
  #define GEAR6_RATIO           ((double)29 / 27)  // 1.074
#endif

// MENU OPTION DEFINITIONS - mapped to choicestate and menulevel
#define MAINDASHBOARD        0
#define RESETTRIP1          1
#define RESETTRIP2          2

#define CONTROLSETUP        3
#define CONTROLCANCEL       300
#define CONTROLLAYOUT1        301
#define CONTROLLAYOUT2        302
#define CONTROLOK         303

#define SETUNITS          4
#define SETUNITSCANCEL        400
#define SETUNITSMILES         401
#define SETUNITSKM          402
#define SETUNITSCELCIUS       403
#define SETUNITSFARENHEIT       404
#define SETUNITSPSI         405
#define SETUNITSBAR         406
#define SETUNITSOK          407

#define SETTIME           5
#define SETTIMECANCEL         500
#define SETTIMEDIGIT1         501
#define SETTIMEDIGIT2         502
#define SETTIMEDIGIT3         503
#define SETTIMEDIGIT4         504
#define SETTIMEOK           505

#define AMBIENTLIGHTSETUP       6
#define AMBIENTLIGHTCANCEL      600
#define DAYTHEMEPLUS        601
#define DAYTHEMEMINUS         602
#define NIGHTTHEMEPLUS        603
#define NIGHTTHEMEMINUS       604
#define LIGHTLEVELPLUS        605
#define LIGHTLEVELMINUS       606
#define AMBIENTLIGHTOK        607

#define SPEEDCORRECTIONMENU     7
#define SPEEDCORRECTIONCANCEL     700
#define SPEEDCORRECTIONPLUSMINUS  701
#define SPEEDCORRECTIONDIGIT1     702
#define SPEEDCORRECTIONDIGIT2     703
#define SPEEDCORRECTIONDIGIT3     704
#define SPEEDCORRECTIONOK       705

#define SETDISPLAYTHEME       8
#define DEFAULTTHEMEWHITE       800
#define GREENTHEME          801
#define REDTHEME          802
#define BLUETHEME           803
#define ORANGETHEME         804
#define YELLOWTHEME         805
#define HCWHITETHEME        806
#define NIGHTTHEME          807
#define HCDARKTHEME         808

#define GEARINDICATORSETUP      9
#define GEARINDICATORCANCEL     900
#define FRONTSPROCKETPLUS       901
#define FRONTSPROCKETMINUS      902
#define REARSPROCKETPLUS      903
#define REARSPROCKETMINUS       904
#define UPDATEINTERVALPLUS      905
#define UPDATEINTERVALMINUS     906
#define GEARINDICATOROK       907

#define COOLANTFANSETUP       10
#define COOLANTSETUPCANCEL      1000
#define COOLANTFANPLUS        1001
#define COOLANTFANMINUS       1002
#define FANMODEALWAYS         1003
#define FANMODE1MINUTE        1004
#define FANMODE50DEGREES      1005
#define FANMODETHROTTLEBLIP     1006
#define COOLANTSETUPOK        1007

#define TPMSSETUP           11
#define TPMSCANCEL          1100
#define FRONTSENSORPLUS       1101
#define FRONTSENSORMINUS      1102
#define REARSENSORPLUS        1103
#define REARSENSORMINUS       1104
#define FRONTLOWPLUS        1105
#define FRONTLOWMINUS         1106
#define REARLOWPLUS         1107
#define REARLOWMINUS        1108
#define TPMSOK            1109

#define ODOSETUP          12
#define ODODIGIT1         1200
#define ODODIGIT2         1201
#define ODODIGIT3         1202
#define ODODIGIT4         1203
#define ODODIGIT5         1204
#define ODODIGIT6         1205
#define ODOCONDIGIT1        1206
#define ODOCONDIGIT2        1207
#define ODOCONDIGIT3        1208
#define ODOCONDIGIT4        1209
#define ODOCONDIGIT5        1210
#define ODOCONDIGIT6        1211
#define ODOOK            1212

// Global Variables
char szNav[255];

volatile unsigned long last_speed_interrupt_time;
volatile bool speedpulsehigh = false;
volatile unsigned int speedpulse_time;

volatile unsigned long avgrpmsum = 0;
volatile unsigned long avgrpmpulse_time = 0;
volatile unsigned long avgrpmpointer = 0;

volatile unsigned long last_rpm_interrupt_time;
volatile bool rpmpulsehigh = false;
volatile unsigned long rpmtimer_start;
volatile unsigned long rpmpulse_time;

unsigned int nv_speedpulse_time;
unsigned int nv_avgspeedpulse_time;
unsigned long nv_rpmpulse_time;
unsigned long nv_last_rpm_interrupt_time;
unsigned long nv_last_speed_interrupt_time;

// Floating speed and speed averaging
unsigned long floatingspeedtime = 0;
unsigned int floatingspeed = 0;
int speedAvgpointer = 0;
int speedsum = 0;
int calcspeedAvg = 0;

// RPM and Floating RPM
int rpm = 0;
unsigned long rpmspeedtime = 0;
unsigned int floatingrpm = 0;
unsigned long rpmAvgpointer = 0;
unsigned long rpmsum = 0;
unsigned long calcrpmAvg = 0;
bool enginerunningten = false;


// Analog sensors
unsigned long senschecktime = 0;
int coolanttemp = 0;
int ambienttemp = 0;
double battvoltage = 0;

// Gear Indicator
int currentgear = 0;
unsigned long gearratiotime = 0;
int floatinggearratio = 0;

// Max Speed
int maxspeed;

// Trip time
int triptimemin = 0;
int triptimehour = 0;
int triptimesec = 0;
unsigned long triptimemillis;

// Dash lights
int neutrallight = 0;
unsigned long timeinneutral = 0;
bool neutraldelaydone = false;

int highbeamlight = 0;
int oillight = 0;
int leftlight = 0;
int rightlight = 0;

// MPG and Range
double litresremaining = 0;
double milesperlitre = 0;
double litresused = 0;
double safempg = 0;
unsigned long avgmpgcount = 0;
unsigned long avgmpg = 0;
double saferange = 0;

// Trip calculation
unsigned long speedchecktime = 0;
double currentspeedperms = 0;
double elapsedspeedtime = 0;
double odotrip = 0; // new values to add to the odometer as it appears with large odo values, the odometer won't increase. (Maybe too many CPU cycles with interrupts going on?)
double lasttrip1 = 0;
double lasttrip2 = 0;
double lastodo = 0;
double lastodotrip = 0;
double startupodo = 0;
double realodo = 0;

// Saved Settings values
int sig = 0;  
int sig2 = 0;
int sig3 = 0;
int sig4 = 0;
int sig5 = 0;
int gearratiointerval = 200;
int frontsprocket = 16;
int rearsprocket = 44;
double trip1 = 0;
double trip2 = 0;
double speedcorrection = 0;
bool km = false;
int theme = 0;
int coolantfantemp = 100;
double lastlitresremaining = 0;
double lastfuelodo = 0; // EXPERIMENT!
int mpg = 0;
int range = 0;
double litresatfillup = 0;
double odo = 0; // EXPERIMENT!
bool fh = false;
bool bar = false;
int frontsensor = 2;
int rearsensor = 4;
int frontpressurelow = 20;
int rearpressurelow = 20;
int controllayout = 0;
int daytheme = 0;
int nighttheme = 6;
int lightswitchvalue = 0;
int fanneutraloption = 0;

bool hardreset = false;

// Fuel Level
int fuelresistance = 0;
int fuelAvgpointer = 0;
int fuelsum = 0;
int calcfuelAvg = 0;
int fuelintfloat = -99;
int fuelintfloatlower = -99;
int fuelintfloatupper = -99;
int fuelintfloatmid = -99;

// Coolant Temp and Ambient Temp
int coolantAvgpointer = 0;
int calccoolantAvg = 0;
int coolantsum = 0;

int ambientAvgpointer = 0;
int calcambientAvg = 0;
int ambientsum = 0;

// Additional variables
bool fanactive = false;
bool enginetimestart = false;
unsigned long engineruntime = 0;
int battAvgpointer = 0;
double battsum = 0;
double calcbattAvg = 0;
int infomode = 0;
int choicestate = 0;
unsigned long lastoutputtime = 0;

// Light sensor
unsigned long lightsenstime = 3000;
int currentlightlevel = 0;
unsigned long nightswitchtime = 2000;

//Navigation
int incomingBLEbyte = 0;
char szBLEreadbuf[255];
int bleReadpointer = 0;
bool navactive = false;

// Option and Select buttons  
bool optionButtontriggered = false;
bool selectButtontriggered = false;
bool bothButtontriggered = false;
bool bothButtonfired = false;
unsigned long bothButtontime = 0;
unsigned long buttonTime = 0;
bool menujustentered = false;

//Menu options
int settimedigit0 = 0;
int settimedigit1 = 0;
int settimedigit2 = 0;
int settimedigit3 = 0;

int spcdigit0 = 0;
int spcdigit1 = 0;
int spcdigit2 = 0;
int spcdigit3 = 0;

// Odometer digits
int ododigit1 = 0;
int ododigit2 = 0;
int ododigit3 = 0;
int ododigit4 = 0;
int ododigit5 = 0;
int ododigit6 = 0;

int odo2digit1 = 0;
int odo2digit2 = 0;
int odo2digit3 = 0;
int odo2digit4 = 0;
int odo2digit5 = 0;
int odo2digit6 = 0;

int odoerror = 0;

unsigned long lastfloatchecktime = 0;

//micros when the pin goes HIGH
volatile unsigned long timer_start;
unsigned long timeingear = 0;
int currentspeed = 0;
double speedcorrectamount = 0;

// Global Instances
static Eeprom24C32_64 eeprom(EEPROM_ADDRESS);
SoftwareSerial mySerial(16, 15); // RX, TX

RTC_DS3231 rtc;

//Refactored menu code
int menulimit = 11; // Length of main menu
int currentsubmenu = 0;
int menulevel = 0; // This will be 300, 400 etc... which will be appended to the choicestate

int mlimits[13] = {
  11,
  0,
  0,
  3,
  7,
  5,
  7,
  5,
  8,
  7,
  7,
  9,
  12
};

bool LoadData ()
{
  byte data[90];
  memset (data, 0, 90);
  
  eeprom.readBytes(0, 74, data);
  
  int datapointer = 0;
  unsigned long oldodo = 0;

  memcpy (&sig, data+datapointer, sizeof (int));
  datapointer+=sizeof (int);

  memcpy (&oldodo, data+datapointer, sizeof (unsigned long));
  datapointer+=sizeof (unsigned long);

  memcpy (&trip1, data+datapointer, sizeof (double));
  datapointer+=sizeof (double);

  memcpy (&trip2, data+datapointer, sizeof (double));
  datapointer+=sizeof (double);

  memcpy (&speedcorrection, data+datapointer, sizeof (double));
  datapointer+=sizeof (double);

  memcpy (&km, data+datapointer, sizeof (bool));
  datapointer+=sizeof (bool);

  memcpy (&theme, data+datapointer, sizeof (int));
  datapointer+=sizeof (int);

  byte fs = 0;
  memcpy (&fs, data+datapointer, sizeof (byte));
  datapointer+=sizeof (byte);
  if (fs < 63 && fs > 10) {
    frontsprocket = fs;
  }
  
  byte rs = 0;
  memcpy (&rs, data+datapointer, sizeof (byte));
  datapointer+=sizeof (byte);
  if (rs < 63 && rs > 10) {
    rearsprocket = rs;  
  }
  
  byte cf = 0;
  memcpy (&cf, data+datapointer, sizeof (byte));
  datapointer+=sizeof (byte);
  coolantfantemp = cf;  

  memcpy (&sig2, data+datapointer, sizeof (int));
  datapointer+=sizeof (int);

  if (sig2 == sig) {
    memcpy (&lastlitresremaining, data+datapointer, sizeof (double));
    datapointer+=sizeof (double);

    memcpy (&lastfuelodo, data+datapointer, sizeof (unsigned long));
    datapointer+=sizeof (unsigned long);

    memcpy (&mpg, data+datapointer, sizeof (int));
    datapointer+=sizeof (int);

    memcpy (&range, data+datapointer, sizeof (int));
    datapointer+=sizeof (int);

    // Signature for litres at fillup
    memcpy (&sig3, data+datapointer, sizeof (int));
    datapointer+=sizeof (int);

    if (sig3 == sig) {
      memcpy (&litresatfillup, data+datapointer, sizeof (double));
      datapointer+=sizeof (double);

      // Signature for new odometer reading (as a double)
      memcpy (&sig4, data+datapointer, sizeof (int));
      datapointer+=sizeof (int);

      if (sig4 == sig) {
     
        memcpy (&odo, data+datapointer, sizeof (double));
        datapointer+=sizeof (double);
    
        // Signature for new data for July 2020 update
        memcpy (&sig5, data+datapointer, sizeof (int));
        datapointer+=sizeof (int);

        if (sig5 == sig) {
                   
          memcpy (&fh, data+datapointer, sizeof (bool));
          datapointer+=sizeof (bool); 
          
          memcpy (&bar, data+datapointer, sizeof (bool));
          datapointer+=sizeof (bool);    

          memcpy (&frontsensor, data+datapointer, sizeof (int));
          datapointer+=sizeof (int);

          memcpy (&rearsensor, data+datapointer, sizeof (int));
          datapointer+=sizeof (int);

          memcpy (&frontpressurelow, data+datapointer, sizeof (int));
          datapointer+=sizeof (int);

          memcpy (&rearpressurelow, data+datapointer, sizeof (int));
          datapointer+=sizeof (int);    

          memcpy (&controllayout, data+datapointer, sizeof (int));
          datapointer+=sizeof (int);

          memcpy (&daytheme, data+datapointer, sizeof (int));
          datapointer+=sizeof (int);

          memcpy (&nighttheme, data+datapointer, sizeof (int));
          datapointer+=sizeof (int);

          memcpy (&lightswitchvalue, data+datapointer, sizeof (int));
          datapointer+=sizeof (int);

          memcpy (&fanneutraloption, data+datapointer, sizeof (int));
          datapointer+=sizeof (int);

          memcpy (&gearratiointerval, data+datapointer, sizeof (int));
          datapointer+=sizeof (int);
        }
        
      } else {
        odo = (double)oldodo;
      }
    }
  }

  realodo = odo;
  startupodo = odo;
  
  if (sig == 1808) {

    if (frontsensor < 0 || frontsensor > 9) {
      frontsensor = 2;
    }

    if (rearsensor < 0 || rearsensor > 9) {
      rearsensor = 4;
    }

    if (frontpressurelow < 10 || frontpressurelow > 50) {
      frontpressurelow = 20;
    }

    if (rearpressurelow < 10 || rearpressurelow > 50) {
      rearpressurelow = 20;
    }

    if (controllayout < 0 || controllayout > 1) {
      controllayout = 0;
    }

    if (daytheme < 0 || daytheme > 8) {
      daytheme = 0;
    }

    if (nighttheme < 0 || nighttheme > 8) {
      nighttheme = 6;
    }

    if (lightswitchvalue < 0 || lightswitchvalue > 900) {
      lightswitchvalue = 0;
    }

    if (fanneutraloption < 0 || fanneutraloption > 3) {
      fanneutraloption = 0;
    }

    if (gearratiointerval < 10 || gearratiointerval > 400) {
      gearratiointerval = 200;
    }
    
    return true;
  } else {
    odo = 0;
    realodo = 0;
    startupodo = 0;
    trip1 = 0;
    trip2 = 0;
    speedcorrection = 0;
    km = false;
    fh = false;
    bar = false;
    theme = 0;
    coolantfantemp = 100;
    frontsprocket = 16;
    rearsprocket = 44;
    lastlitresremaining = 0;
    lastfuelodo = 0;
    mpg = 0;
    range = 0;
    litresatfillup = 0;
    frontsensor = 2;
    rearsensor = 4;
    frontpressurelow = 20;
    rearpressurelow = 20;
    controllayout = 0;
    daytheme = 0;
    nighttheme = 6;
    currentlightlevel = 0;
    lightswitchvalue = 0;
    fanneutraloption = 0;
    gearratiointerval = 200;
    
    return false;
  }
  
  return true;
  
}

void SaveData () 
{

  /*
  Data to Save to Eeprom:
  SIGNATURE, ODO, Trip1, Trip2, Speed Correction, Miles / KM, Chosen Theme
  2,         4,   2 .  , 2 .  , 4               , 1         , 2
  */


  int signature = 1808;

  if (hardreset == true) {
    signature = 7777;
  }
  
  int memsize = sizeof (int) + 
  sizeof (unsigned long) + 
  sizeof (double) + 
  sizeof (double) + 
  sizeof (double) + 
  sizeof (bool) + 
  sizeof (int) + 
  sizeof (byte) + 
  sizeof (byte) +
  sizeof (byte) +
  sizeof (int) + 
  sizeof (double) + 
  sizeof (unsigned long) + 
  sizeof (int) + 
  sizeof (int) +
  sizeof (int) + 
  sizeof (double) + 
  sizeof (int) + 
  sizeof (double) +
  sizeof (int) +
  sizeof (bool) + 
  sizeof (bool) + 
  sizeof (int) +
  sizeof (int) + 
  sizeof (int) + 
  sizeof (int) + 
  sizeof (int) +
  sizeof (int) + 
  sizeof (int) + 
  sizeof (int) +
  sizeof (int) + 
  sizeof (int);
  

  


  //2,4,4,4,4,1,2,1,1,1,2,4,4,2,2,2,4,2,4,2,1,1,2,2,2,2,2 = 24 
  
  byte data[memsize];
  int datapointer = 0;
  unsigned long oldodo = 0;
  
  memset (data, 0, memsize);
  
  memcpy (data+datapointer, &signature, sizeof (int));
  datapointer+=sizeof (int);
  
  memcpy (data+datapointer, &oldodo, sizeof (unsigned long));
  datapointer+=sizeof (unsigned long);

  memcpy (data+datapointer, &trip1, sizeof (double));
  datapointer+=sizeof (double);

  memcpy (data+datapointer, &trip2, sizeof (double));
  datapointer+=sizeof (double);

  memcpy (data+datapointer, &speedcorrection, sizeof (double));
  datapointer+=sizeof (double);

  memcpy (data+datapointer, &km, sizeof (bool));
  datapointer+=sizeof (bool);

  memcpy (data+datapointer, &theme, sizeof (int));
  datapointer+=sizeof (int);

  byte fs = frontsprocket;
  memcpy (data+datapointer, &fs, sizeof (byte));
  datapointer+=sizeof (byte);

  byte rs = rearsprocket;
  memcpy (data+datapointer, &rs, sizeof (byte));
  datapointer+=sizeof (byte);

  byte cf = coolantfantemp;
  memcpy (data+datapointer, &cf, sizeof (byte));
  datapointer+=sizeof (byte);

  memcpy (data+datapointer, &signature, sizeof (int));
  datapointer+=sizeof (int);

  memcpy (data+datapointer, &lastlitresremaining, sizeof (double));
  datapointer+=sizeof (double);

  memcpy (data+datapointer, &lastfuelodo, sizeof (unsigned long));
  datapointer+=sizeof (unsigned long);

  memcpy (data+datapointer, &mpg, sizeof (int));
  datapointer+=sizeof (int);

  memcpy (data+datapointer, &range, sizeof (int));
  datapointer+=sizeof (int);  

  memcpy (data+datapointer, &signature, sizeof (int));
  datapointer+=sizeof (int);  

  memcpy (data+datapointer, &litresatfillup, sizeof (double));
  datapointer+=sizeof (double);

  memcpy (data+datapointer, &signature, sizeof (int));
  datapointer+=sizeof (int);

  memcpy (data+datapointer, &odo, sizeof (double));
  datapointer+=sizeof (double);

  // Additional data for July 2020 Update
  memcpy (data+datapointer, &signature, sizeof (int));
  datapointer+=sizeof (int);

  memcpy (data+datapointer, &fh, sizeof (bool));
  datapointer+=sizeof (bool);

  memcpy (data+datapointer, &bar, sizeof (bool));
  datapointer+=sizeof (bool);

  memcpy (data+datapointer, &frontsensor, sizeof (int));
  datapointer+=sizeof (int);

  memcpy (data+datapointer, &rearsensor, sizeof (int));
  datapointer+=sizeof (int);

  memcpy (data+datapointer, &frontpressurelow, sizeof (int));
  datapointer+=sizeof (int);

  memcpy (data+datapointer, &rearpressurelow, sizeof (int));
  datapointer+=sizeof (int);
  
  memcpy (data+datapointer, &controllayout, sizeof (int));
  datapointer+=sizeof (int);

  memcpy (data+datapointer, &daytheme, sizeof (int));
  datapointer+=sizeof (int);

  memcpy (data+datapointer, &nighttheme, sizeof (int));
  datapointer+=sizeof (int);

  memcpy (data+datapointer, &lightswitchvalue, sizeof (int));
  datapointer+=sizeof (int);

  memcpy (data+datapointer, &fanneutraloption, sizeof (int));
  datapointer+=sizeof (int);

  memcpy (data+datapointer, &gearratiointerval, sizeof (int));
  datapointer+=sizeof (int);

  eeprom.writeBytes(0, memsize, data);

  //i2c_eeprom_write_page(0x57, 0, (byte *)data, memsize);

  /*
  // SECOND PAGE
  memcpy (datasecond+datapointersecond, &signature, sizeof (int));
  datapointersecond+=sizeof (int); 

  memcpy (datasecond+datapointersecond, &lastlitresremaining, sizeof (double));
  datapointersecond+=sizeof (double);

  memcpy (datasecond+datapointersecond, &lastfuelodo, sizeof (unsigned long));
  datapointersecond+=sizeof (unsigned long);

  memcpy (datasecond+datapointersecond, &mpg, sizeof (int));
  datapointersecond+=sizeof (int);

  memcpy (datasecond+datapointersecond, &range, sizeof (int));
  datapointersecond+=sizeof (int);

  i2c_eeprom_write_page(0x57, 0x1, (byte *)datasecond, memsizesecond);
  */
}

void Checkbuttoninput ()
{

  // REMOVE FOR PRODUCTION!!!! Allows you to Set Odometer reading by holding down both buttons for 7 seconds
  if (digitalRead (OPTIONPIN) == HIGH && digitalRead (SELECTPIN) == HIGH) {
    if (bothButtontriggered == false) {
      bothButtontriggered = true;
      bothButtontime = millis();
    } else {
      if ((millis() - bothButtontime) > 7000) {
        if (bothButtonfired == false) {
          bothButtonfired = true;

          hardreset = true;
          SaveData ();          
          choicestate = 1200;
        }
      }
    }
  }
  
  
  if (digitalRead (OPTIONPIN) == HIGH && optionButtontriggered == false) {
    optionButtontriggered = true;
    //Serial.println ("OPTION BUTTON");
    if (controllayout == 0) {
      choiceex();  
    } else {
 
      selectex();
    }

  }

  if (digitalRead (SELECTPIN) == HIGH && selectButtontriggered == false) {
    selectButtontriggered = true;
    //Serial.println ("SELECT BUTTON");
    if (controllayout == 0) {
      selectex();  
    } else {
      choiceex();
    }
  } 


  if ((millis() - buttonTime) > 150) {
    buttonTime = millis();
    if (digitalRead (OPTIONPIN) == LOW) {
      optionButtontriggered = false;
      bothButtontriggered = false;
      bothButtonfired = false;
    }
  
    if (digitalRead (SELECTPIN) == LOW) {
      selectButtontriggered = false;
    }
  }
  
}

void calcSpeedFullSignal() 
{
    last_speed_interrupt_time = micros();    
    if(digitalRead(SPEEDSENSORPIN) == LOW) 
    {
      // Do the calc when it goes low so that we get the width of the full pulse including the time it goes high
      //New here
      if (speedpulsehigh == true) {
        if (timer_start != 0) {
          speedpulse_time = (micros() - timer_start);
        }

        speedpulsehigh = false;
        timer_start = micros();
      } else {
        timer_start = micros();
      }
              
    } else {
      speedpulsehigh = true;
    }
}

void returntodashboard()
{
  choicestate = 0;
  currentsubmenu = 0;
  menulevel = 0;
}

void odosetupmenu()
{
  choicestate = ODOSETUP;
  currentsubmenu = choicestate;
  menulevel = choicestate * 100;
  choicestate = 0;
}

void choiceex () 
{
  choicestate++;

  if (choicestate > mlimits[currentsubmenu]) {
    choicestate = 0;
  }
  
  //Serial.print ("Choicestate ");
  //Serial.println (menulevel + choicestate);
}

void selectex () 
{
  
  if (currentsubmenu == 0) {
    if (mlimits[choicestate] != 0 && choicestate > 0) {
      // Advance the menu level
      currentsubmenu = choicestate;
      menulevel = choicestate * 100;
      choicestate = 0;
      menujustentered = true;
    }
  }

  //Serial.print ("Select ");
  //Serial.print ("Choicestate ");
  //Serial.println (menulevel + choicestate);

  if (menujustentered == false) {
    executemenufunction (menulevel + choicestate);
  }
  menujustentered = false;
}

void executemenufunction (int menuoption)
{
  switch (menuoption) {
    case MAINDASHBOARD:
    {
      toggleinfomode ();
    }
    break;
    case RESETTRIP1:
    {
      resettrip (1);
      returntodashboard();
    }
    break;
    case RESETTRIP2:
    {
      resettrip (2);
      returntodashboard();
    }
    break;
    // CONTROL SETUP
    case CONTROLLAYOUT1:
      controllayout = 0;
      break;
    case CONTROLLAYOUT2:
      controllayout = 1;
      break;    
    case SETUNITSMILES:
      km = false;
      break;
    case SETUNITSKM:
      km = true;
      break;
    case SETUNITSCELCIUS:
      fh = false;
      break;
    case SETUNITSFARENHEIT:
      fh = true;
      break;
    case SETUNITSPSI:
      bar = false;
      break;
    case SETUNITSBAR:
      bar = true;
      break;

    // SET TIME
    case SETTIMEDIGIT1:
    {
      settimedigit0++;
      if (settimedigit0 > 2) {
        settimedigit0 = 0;
      }      
    }
    break;
    case SETTIMEDIGIT2:
    {
      settimedigit1++;
      if (settimedigit1 > 9) {
        settimedigit1 = 0;
      }
  
      if (settimedigit0 == 2) {
        if (settimedigit1 > 3) {
          settimedigit1 = 0;
        }
      }      
    }
    break;
    case SETTIMEDIGIT3:
    {
      settimedigit2++;
      if (settimedigit2 > 5) {
        settimedigit2 = 0;
      }      
    }
    break;
    case SETTIMEDIGIT4:
    {
      settimedigit3++;
      if (settimedigit3 > 9) {
        settimedigit3 = 0;
      }      
    }
    break;
    case SETTIMEOK:
    {
      int h = (settimedigit0*10) + settimedigit1;
      int m = (settimedigit2*10) + settimedigit3;        
      setTime (h, m);
      returntodashboard();
    }
    break;

    //AMBIENT LIGHT SETUP
    case DAYTHEMEPLUS:
    {
      daytheme++;
      if (daytheme > 8) {
        daytheme = 0;
      }
    }
    break;
    case DAYTHEMEMINUS:
    {
      daytheme--;
      if (daytheme < 0) {
        daytheme = 8;
      }
    }
    break;
    case NIGHTTHEMEPLUS:
    {
      nighttheme++;
      if (nighttheme > 8) {
        nighttheme = 0;
      }
    }
    break;
    case NIGHTTHEMEMINUS:
    {
      nighttheme--;
      if (nighttheme < 0) {
        nighttheme = 8;
      }
    }
    break;
    case LIGHTLEVELPLUS:
    {
      lightswitchvalue+=10;
      if (lightswitchvalue > 900) {
        lightswitchvalue = 0;
      }
    }
    break;
    case LIGHTLEVELMINUS:
    {
      lightswitchvalue-=10;  
      if (lightswitchvalue < 0) {
        lightswitchvalue = 900;
      }
    }
    break;
    
    // SPEED CORRECTION
    case SPEEDCORRECTIONPLUSMINUS:
    {
      spcdigit0++;
      if (spcdigit0 > 1) {
        spcdigit0 = 0;
      }
    }
    break;
    case SPEEDCORRECTIONDIGIT1:
    {
      spcdigit1++;
      if (spcdigit1 > 9) {
        spcdigit1 = 0;
      }
    }
    break;
    case SPEEDCORRECTIONDIGIT2:
    {
      spcdigit2++;
      if (spcdigit2 > 9) {
        spcdigit2 = 0;
      }
    }
    break;
    case SPEEDCORRECTIONDIGIT3:
    {
      spcdigit3++;
      if (spcdigit3 > 9) {
        spcdigit3 = 0;
      }
    }
    break;
    case SPEEDCORRECTIONOK:
    {
      speedcorrection = (double)(spcdigit1*10) + (double)spcdigit2 + (double)spcdigit3/10;
      if (spcdigit0 == 0) {
        speedcorrection = 0-speedcorrection;
      }
      SaveData();
      returntodashboard();
    }
    break;

    // SET THEME
    case DEFAULTTHEMEWHITE:
    {
      theme = 0;
      daytheme = 0;
      SaveData();
      returntodashboard();
    }
    break;
    case GREENTHEME:
    {
      theme = 1;
      daytheme = 1;
      SaveData();
      returntodashboard();
    }
    break;
    case REDTHEME:
    {
      theme = 2;
      daytheme = 2;
      SaveData();
      returntodashboard();
    }
    break;
    case BLUETHEME:
    {
      theme = 3;
      daytheme = 3;
      SaveData();
      returntodashboard();
    }
    break;
    case ORANGETHEME:
    {
      theme = 4;
      daytheme = 4;
      SaveData();
      returntodashboard();
    }
    break;
    case YELLOWTHEME:
    {
      theme = 5;
      daytheme = 5;
      SaveData();
      returntodashboard();
    }
    break;
    case HCWHITETHEME:
    {
      theme = 7;
      daytheme = 7;
      SaveData();
      returntodashboard();
    }
    break;
    case NIGHTTHEME:
    {
      theme = 6;
      daytheme = 6;
      SaveData();
      returntodashboard();
    }
    break;
    case HCDARKTHEME:
    {
      theme = 8;
      daytheme = 8;
      SaveData();
      returntodashboard();
    }
    break;

    // GEAR INDICATOR SETUP
    case FRONTSPROCKETPLUS:
    {
      if (frontsprocket < 21) {
        frontsprocket++;
      }
    }
    break;
    case FRONTSPROCKETMINUS:
    {
      if (frontsprocket > 10) {
        frontsprocket--;
      }
    }
    break;
    case REARSPROCKETPLUS:
    {
      if (rearsprocket < 62) {
        rearsprocket++;
      }
    }
    break;
    case REARSPROCKETMINUS:
    {
      if (rearsprocket > 35) {
        rearsprocket--;
      }
    }
    break;
    case UPDATEINTERVALPLUS:
    {
      if (gearratiointerval < 400) {
        gearratiointerval += 10;      
      }
    }
    break;
    case UPDATEINTERVALMINUS:
    {
      if (gearratiointerval > 10) {
        gearratiointerval -= 10;      
      }
    }
    break;

    // COOLANT FAN TEMP MENU
    case COOLANTFANPLUS:
    {
      if (coolantfantemp < 120) {
        coolantfantemp++;
      }
    }
    break;
    case COOLANTFANMINUS:
    {
      if (coolantfantemp > 70) {
        coolantfantemp--;
      }
    }
    break;
    case FANMODEALWAYS:
    {
      fanneutraloption = 0;
    }
    break;
    case FANMODE1MINUTE:
    {
      fanneutraloption = 1;
    }
    break;
    case FANMODE50DEGREES:
    {
      fanneutraloption = 2;
    }
    break;
    case FANMODETHROTTLEBLIP:
    {
      fanneutraloption = 3;
    }
    break;


    // TPMS SETUP MENU
    case FRONTSENSORPLUS:
    {
      frontsensor++;
      if (frontsensor > 9) {
        frontsensor = 0;
      }
    }
    break;
    case FRONTSENSORMINUS:
    {
      frontsensor--;
      if (frontsensor < 0) {
        frontsensor = 9;    
      }
    }
    break;
    case REARSENSORPLUS:
    {
      rearsensor++;
      if (rearsensor > 9) {
        rearsensor = 0;
      }
    }
    break;
    case REARSENSORMINUS:
    {
      rearsensor--;
      if (rearsensor < 0) {
        rearsensor = 9;    
      }
    }
    break;
    case FRONTLOWPLUS:
    {
      frontpressurelow++;
      if (frontpressurelow > 50) {
        frontpressurelow = 10;
      }
    }
    break;
    case FRONTLOWMINUS:
    {
      frontpressurelow--;
      if (frontpressurelow < 10) {
        frontpressurelow = 50;
      }
    }
    break;
    case REARLOWPLUS:
    {
      rearpressurelow++;
      if (rearpressurelow > 50) {
        rearpressurelow = 10;
      }
    }
    break;
    case REARLOWMINUS:
    {
      rearpressurelow--;
      if (rearpressurelow < 10) {
        rearpressurelow = 50;
      }
    }
    break;

    // ODOMETER SETUP
    case ODODIGIT1:
      ododigit1 = cycledigit (ododigit1, 9);
      break;
    case ODODIGIT2:
      ododigit2 = cycledigit (ododigit2, 9);
      break;
    case ODODIGIT3:
      ododigit3 = cycledigit (ododigit3, 9);
      break;
    case ODODIGIT4:
      ododigit4 = cycledigit (ododigit4, 9);
      break;
    case ODODIGIT5:
      ododigit5 = cycledigit (ododigit5, 9);
      break;
    case ODODIGIT6:
      ododigit6 = cycledigit (ododigit6, 9);
      break;
    case ODOCONDIGIT1:
      odo2digit1 = cycledigit (odo2digit1, 9);
      break;
    case ODOCONDIGIT2:
      odo2digit2 = cycledigit (odo2digit2, 9);
      break;
    case ODOCONDIGIT3:
      odo2digit3 = cycledigit (odo2digit3, 9);
      break;
    case ODOCONDIGIT4:
      odo2digit4 = cycledigit (odo2digit4, 9);
      break;
    case ODOCONDIGIT5:
      odo2digit5 = cycledigit (odo2digit5, 9);
      break;
    case ODOCONDIGIT6:
      odo2digit6 = cycledigit (odo2digit6, 9);
      break;
    case ODOOK:
    {
      // Set and save the odometer here
      if (ododigit1 != odo2digit1) {odoerror = 1;}
      if (ododigit2 != odo2digit2) {odoerror = 1;}
      if (ododigit3 != odo2digit3) {odoerror = 1;}
      if (ododigit4 != odo2digit4) {odoerror = 1;}
      if (ododigit5 != odo2digit5) {odoerror = 1;}
      if (ododigit6 != odo2digit6) {odoerror = 1;}

      int numdigitsatzero = 0;
      if (ododigit1 == 0) {numdigitsatzero++;}
      if (ododigit2 == 0) {numdigitsatzero++;}
      if (ododigit3 == 0) {numdigitsatzero++;}
      if (ododigit4 == 0) {numdigitsatzero++;}
      if (ododigit5 == 0) {numdigitsatzero++;}
      if (ododigit6 == 0) {numdigitsatzero++;}
      if (odo2digit1 == 0) {numdigitsatzero++;}
      if (odo2digit2 == 0) {numdigitsatzero++;}
      if (odo2digit3 == 0) {numdigitsatzero++;}
      if (odo2digit4 == 0) {numdigitsatzero++;}
      if (odo2digit5 == 0) {numdigitsatzero++;}
      if (odo2digit6 == 0) {numdigitsatzero++;}

      if (numdigitsatzero == 12) {
        odoerror = 2;        
      }

      odo = (ododigit1 * 100000) + (ododigit2 * 10000) + (ododigit3 * 1000) + (ododigit4 * 100) + (ododigit5 * 10) + ododigit6;
      realodo = odo;
      startupodo = odo;
      hardreset = false;

      SaveData();
      returntodashboard();
    }
    break;
 
    case CONTROLCANCEL:
    case SETUNITSCANCEL:
    case SETTIMECANCEL:
    case AMBIENTLIGHTCANCEL:
    case SPEEDCORRECTIONCANCEL:
    case GEARINDICATORCANCEL:
    case COOLANTSETUPCANCEL:
    case TPMSCANCEL:
      returntodashboard();
      break;

    
    case CONTROLOK:
    case SETUNITSOK:
    case AMBIENTLIGHTOK:
    case GEARINDICATOROK:
    case COOLANTSETUPOK:
    case TPMSOK:
      SaveData();
      returntodashboard();
      break;
      
    default:
      break;
  }

  
}

void toggleinfomode ()
{
  if (navactive == true) {
    if (infomode == 3) {
      infomode = 0;
      return;
    }
    
    if (infomode == 2) {
      infomode = 3;
      return;
    }  
  } else {
    if (infomode == 2) {
      infomode = 0;
      return;
    }
  }
        
  if (infomode == 1) {
    infomode = 2;
    return;
  }

  if (infomode == 0) {
    infomode = 1;
    return;
  }
}

void resettrip (int tripnumber) {
  switch (tripnumber)
  {
    case 1:
    {
      // Reset trip one
      trip1 = 0;
  
      // Record the fuel level and tank level
      litresatfillup = litresremaining;
      lastfuelodo = odo;
      
      SaveData();
      
    }
    break;
    case 2:
    {
      trip2 = 0;
      SaveData();
      
    }
    break;
  }
}

int cycledigit (int value, int maxvalue) {
  int t = value;
  if (value < maxvalue) {
    //fprintf(stderr, "returning value: %d\n", value++);
    t++;
    return t;
  }

  if (value >= maxvalue) {
    //fprintf(stderr, "returning 0");
    return 0;
  }

  //fprintf(stderr, "returning default");
  return value;
}

void setTime (int h, int m) {
  rtc.adjust (DateTime (2018, 8, 4, h, m, 0));
}

void calcRpmFullSignal()
{
    last_rpm_interrupt_time = micros();    
    if(digitalRead(RPMSENSORPIN) == LOW) 
    {
      // Do the calc when it goes low so that we get the width of the full pulse including the time it goes high
      
      if (rpmpulsehigh == true) {
        if (rpmtimer_start != 0) {
          rpmpulse_time = (micros() - rpmtimer_start);

          
          if (avgrpmpointer < 20) {
            avgrpmsum += rpmpulse_time;
            avgrpmpointer++;
          }

          if (avgrpmpointer >= 20) {
            avgrpmpulse_time = avgrpmsum / 20;
            avgrpmpointer = 0;
            avgrpmsum = 0;
          }
                    
        }
        rpmpulsehigh = false;
        rpmtimer_start = micros();
      } else {
        rpmtimer_start = micros();  
      }
     
    } else {
            
      rpmpulsehigh = true;     
    }
}

int CalcSpeedFromPulseWidth(unsigned int pulsewidth)
{
    // new method for working out speed based on pulses per meter.
    // Much less processing time.
    if (pulsewidth < 300 && pulsewidth != 0) {
      return 0;
    }

    if (pulsewidth == 0 || pulsewidth > 60000) {
      return 0;
    }

    double numpulsespersecond = 1000000 / (double)pulsewidth;
    double speedinms = numpulsespersecond / 40.3;
    double speedinmph = speedinms * 2.23694;
    return (int) speedinmph;
}

int CalcRPMFromPulseWidth(int pulsewidth)
{
    // RPM from pulse width. Both FZS1000 and FZS600 fire 2 pulses per revolution
    // (wasted spark, 2 coil packs). Theoretical constant = 60/2 = 30.
    // RPM_CONSTANT is defined per-bike at top of file.
    double numpulses = 1000000 / (double)pulsewidth;
    double rpm = numpulses * RPM_CONSTANT;
    
    return (int) rpm;
}

void floatrpm()
{
    if (millis() - rpmspeedtime > 10)
    {
        rpmspeedtime = millis();
  
        if (rpm > floatingrpm)
        {
  
            if (rpm - floatingrpm >= 3000)
            {
                floatingrpm += 1000;
            }
  
            if (rpm - floatingrpm >= 2000)
            {
                floatingrpm += 500;
            }
  
            if (rpm - floatingrpm >= 1000)
            {
                floatingrpm += 100;
            }
  
            if (rpm - floatingrpm >= 500)
            {
                floatingrpm += 10;
            }
  
            if (rpm - floatingrpm >= 250)
            {
                floatingrpm += 5;
            }
  
            floatingrpm+=1;
        }
  
        if (rpm < floatingrpm)
        {
            if (floatingrpm - rpm >= 3000)
            {
                floatingrpm -= 1000;
            }
  
            if (floatingrpm - rpm >= 2000)
            {
                floatingrpm -= 500;
            }
  
            if (floatingrpm - rpm >= 1000)
            {
                floatingrpm -= 100;
            }
  
            if (floatingrpm - rpm >= 500)
            {
                floatingrpm -= 10;
            }
  
            if (floatingrpm - rpm >= 250)
            {
                floatingrpm -= 5;
            }                        
  
            floatingrpm -=1;
        }      
    }
}

double GetMeasuredAnalogResistance(int analogPin)
{
    double R1resistorvalue = 2000;
    double vin = 5.1;
    //return (measuredvoltage * R1resistorvalue) / (5 - measuredvoltage);

    double buffervalue = (analogRead (analogPin) * vin);
    double vout = (buffervalue) / 1023.0;
    buffervalue = (vin / vout) -1;

    double R2 = R1resistorvalue * buffervalue;
    return R2;
}

double CalcTemp(double measured_resistance, double stock_resistance, double betavalue)
{
    double t = measured_resistance / stock_resistance;
    t = log(t);
    t = (1 / betavalue) * t;  
    t = (1 / 298.15) + t;
    t = 1 / t;
    t = t - 273.15;
    t = round(t * 100) / 100;

    if (t < -99) {
      return -99;
    }

    return t;
}

double getBatteryVoltage ()
{
  double batReading = analogRead (BATTERYPIN);
  //volts = voltsout*(ohms1+ohms2)/ohms2;

  double voltsout = batReading * 0.0074;

  double batVolts = voltsout * (10000 + 5090)/5090;

  return batVolts;
  
}

int getDifference (int valueone, int valuetwo)
{
  if (valueone > valuetwo) {
    return valueone-valuetwo;
  }

  if (valuetwo >= valueone) {
    return valuetwo-valueone;
  }
}

double CalculateSpeedRatio(double rpm, int gear)
{
  
    double wheeldiametermetres = WHEEL_DIAMETER_M;
    double primarydriveratio = PRIMARY_DRIVE_RATIO;
    double gearboxratio = 0;

    // Gearbox ratios defined per-bike at top of file
    if (gear == 1) { gearboxratio = GEAR1_RATIO; }
    if (gear == 2) { gearboxratio = GEAR2_RATIO; }
    if (gear == 3) { gearboxratio = GEAR3_RATIO; }
    if (gear == 4) { gearboxratio = GEAR4_RATIO; }
    if (gear == 5) { gearboxratio = GEAR5_RATIO; }
    if (gear == 6) { gearboxratio = GEAR6_RATIO; }

    double finaldriveratio = (double)rearsprocket / (double)frontsprocket;            
    double spd = rpm / primarydriveratio;
   
    spd = spd / gearboxratio; 
    spd = spd / finaldriveratio;
    spd = spd * 3.1415926;
    spd = spd * wheeldiametermetres;
    spd = spd * 60;
    spd = spd / 1609.344;
    
    return (rpm / spd);
}

int CalculateGear(double currentspeed, double currentrpm)
{
    //unsigned long gearratiotime = 0;
    //int floatinggearratio = 0;
    //163 @ 30mph ish in first ratio
    // 121 in second @ 30 mph
    // 100 in third @ 30

    if ((millis() - gearratiotime) > gearratiointerval) {
      gearratiotime = millis();

      double gearratio = currentrpm / currentspeed;
      if (gearratio > floatinggearratio) {
        
        if (floatinggearratio < 500) {
          if ((gearratio - floatinggearratio) > 20) {
            floatinggearratio+=5;
          }
  
          
          floatinggearratio++;  
        }
         
      }

      if (gearratio < floatinggearratio) {

        if (floatinggearratio > 0) {
          if ((floatinggearratio - gearratio) > 20) {
            floatinggearratio-=5;
          }
        
          floatinggearratio--;
        }
      }

      //range = (int)floatinggearratio;

    }
    
    double smallestdiff = 1000;
    double speedratioingear = 0;
    int mostlikelygear = -1;

    for (int a = 1; a <= 6; a++)
    {
        speedratioingear = CalculateSpeedRatio(currentrpm, a);
        
        if (getDifference((int)speedratioingear, (int)floatinggearratio) < smallestdiff)
        {
            smallestdiff = getDifference((int)speedratioingear, (int)floatinggearratio);
            mostlikelygear = a;
        }
    }

    return mostlikelygear;
  
}

int GetUnitsPerLitre () {

  // Depending on where the fuel float is it will have a different rate of change 
  // as the fuel decreases. These values are based on observations I made
  // Each 'Unit' per litre is how many units make up one litre for the current float reading

  if (fuelintfloat >= 0 && fuelintfloat < 93) {
    litresremaining = (21000 - (((double)(1000 / 93)) * ((double)fuelintfloat - 0))) / 1000;
    return 93;
  }

  if (fuelintfloat > 93 && fuelintfloat < 121) {
    litresremaining = (20000 - (((double)(1000 / 28)) * ((double)fuelintfloat - 93))) / 1000; // 8.47
    return 28;
  }
  
  if (fuelintfloat > 121 && fuelintfloat < 148) {
    litresremaining = (18000 - (((double)(1000 / 27)) * ((double)fuelintfloat - 121))) / 1000; // 30.30
    return 27;
  }

  if (fuelintfloat > 148 && fuelintfloat < 180) {
    litresremaining = (17000 - (((double)(1000 / 32)) * ((double)fuelintfloat - 148))) / 1000;
    return 32;
  }

  if (fuelintfloat > 180 && fuelintfloat < 211) {
    litresremaining = (15000 - (((double)(1000 / 31)) * ((double)fuelintfloat - 180))) / 1000;
    return 31;
  }

  if (fuelintfloat > 211 && fuelintfloat < 241) {
    litresremaining = (14000 - (((double)(1000 / 30)) * ((double)fuelintfloat - 211))) / 1000;
    return 30;
  }

  if (fuelintfloat > 241 && fuelintfloat < 281) {
    litresremaining = (12000 - (((double)(1000 / 40)) * ((double)fuelintfloat - 241))) / 1000;
    return 40;
  }

  if (fuelintfloat > 281 && fuelintfloat < 320) {
    litresremaining = (10000 - (((double)(1000 / 39)) * ((double)fuelintfloat - 281))) / 1000;
    return 39;
  }

  if (fuelintfloat > 320 && fuelintfloat < 367) {
    litresremaining = (9000 - (((double)(1000 / 47)) * ((double)fuelintfloat - 320))) / 1000;
    return 47;
  }

  if (fuelintfloat > 367 && fuelintfloat < 426) {
    litresremaining = (7000 - (((double)(1000 / 59)) * ((double)fuelintfloat - 367))) / 1000;
    return 59;
  }

  if (fuelintfloat > 426 && fuelintfloat < 481) {
    litresremaining = (6000 - (((double)(1000 / 55)) * ((double)fuelintfloat - 426))) / 1000;
    return 55;
  }

  if (fuelintfloat > 481 && fuelintfloat < 506) {
    litresremaining = (5000 - (((double)(1000 / 25)) * ((double)fuelintfloat - 481))) / 1000;
    return 25;
  }

  return 0;
}

void FanControl (bool active) {

  if (active == true) {
    if (fanactive == false) {
      fanactive = true;      
      digitalWrite (FANPIN, HIGH); // WAS LOW for old 2 relay board        
    }
  } else {
    if (fanactive == true) {
      fanactive = false;           
      digitalWrite (FANPIN, LOW);  // WAS high for old 2 relay board              
    }    
  }
  
}

void livemode() {
  
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
      // When reading the volatile variables used by the Interrupt routines - ensure that
      // Interrupts are temporarily disabled whilst we copy the variables. This ensures that the volatile vars don't
      // change whilst we are copying them.
       nv_speedpulse_time = speedpulse_time;
       nv_avgspeedpulse_time = avgrpmpulse_time;
       nv_rpmpulse_time = rpmpulse_time;
       nv_last_rpm_interrupt_time = last_rpm_interrupt_time;
       nv_last_speed_interrupt_time = last_speed_interrupt_time;
  }


  // Introduce a Neutral delay so we only start working out speed once we are out of neutral for more than 
  // 1.5 seconds. This avoids the random speed reading from the 'thud' of putting the bike in gear
  if (neutrallight == 1) {
    timeinneutral = millis();
    neutraldelaydone = false;
  }

  if (neutrallight == 0) {
    timeingear = millis();
    if ((millis() - timeinneutral) > 1500) {
      neutraldelaydone = true;
    }    
  }

  if (neutrallight == 0 && neutraldelaydone == true) { // Only work out speed if bike is in gear and neutral delay has elapsed
    currentspeed = CalcSpeedFromPulseWidth (nv_speedpulse_time);  

    if (speedcorrection > 0) {
      speedcorrectamount = (currentspeed * speedcorrection) / 100;
      currentspeed = currentspeed + (int)speedcorrectamount;
    }

    if (speedcorrection < 0) {
      speedcorrectamount = (currentspeed * (0-speedcorrection)) / 100;
      currentspeed = currentspeed - (int)speedcorrectamount;
    }
  }

  if ((micros() - nv_last_speed_interrupt_time) > 1000000) {
    currentspeed = 0; 
  }


  // Float the speedometer reading for a smoother rate of change and iron out any odd values
  if ((millis() - floatingspeedtime) > 30) {
    floatingspeedtime = millis();

    if (currentspeed > floatingspeed) {
      floatingspeed++;
    }

    if (currentspeed < floatingspeed) {
      floatingspeed--;
    }
  }

  // Average the speed
  if (speedAvgpointer < 20) {
    speedsum += floatingspeed;
    speedAvgpointer++;
  }

  if (speedAvgpointer >= 20) {
    speedAvgpointer = 0;
    calcspeedAvg = speedsum / 20;
    speedsum = 0;
  }

  // RPM and Floating RPM
  if ((micros() - nv_last_rpm_interrupt_time) < 30000) {
    if (nv_rpmpulse_time < 30000 && nv_rpmpulse_time > 1500) {
      rpm = CalcRPMFromPulseWidth (nv_rpmpulse_time);    
    }  
  }
  
  if ((micros() - nv_last_rpm_interrupt_time) > 1000000) {
    rpm = 0; 
  }

  floatrpm();

  // Average the rpm for gear calculation
  if (rpmAvgpointer < 20) {
    rpmsum += floatingrpm;
    rpmAvgpointer++;
  }

  if (rpmAvgpointer >= 20) {
    rpmAvgpointer = 0;
    calcrpmAvg = rpmsum / 20;
    rpmsum = 0;
  }

  if ((millis() - senschecktime) > 1000) {
    senschecktime = millis();
    coolanttemp = CalcTemp (GetMeasuredAnalogResistance(COOLANTPIN), 30000, 4000);  
    ambienttemp = CalcTemp (GetMeasuredAnalogResistance(AMBIENTTEMPPIN), 10000, 3750);
    battvoltage = getBatteryVoltage();
  }

  if (calcrpmAvg > 500 && calcspeedAvg >= 0) {
    //currentgear = CalculateGear (calcspeedAvg, calcrpmAvg);
    currentgear = CalculateGear (calcspeedAvg, rpm);

    // Max Speed
    if (calcspeedAvg > maxspeed) {
      maxspeed = calcspeedAvg;
    }
  }

    // Trip time calculation
  if (rpm > 1000) {
    if ((millis() - triptimemillis) > 1000) {
      triptimemillis = millis();

      if (triptimesec < 60) {
        triptimesec++;

        if (triptimesec > 59) {
          triptimesec = 0;

          if (triptimemin < 60) {
            triptimemin++;

            if (triptimemin > 59) {
              triptimemin = 0;
              triptimehour++;

              if (triptimehour > 99) {
                triptimehour = 0;
              }
            }
          }          
        }
      }      
    }      
  }

  // Append the Trip1, Trip2, and Odo
  elapsedspeedtime = (double)millis() - speedchecktime;
  if (elapsedspeedtime > 300) {
    speedchecktime = millis();      
    currentspeedperms = (double)currentspeed / 3600;

    trip1 = trip1 + (currentspeedperms * (double)elapsedspeedtime / 1000);
    trip2 = trip2 + (currentspeedperms * (double)elapsedspeedtime / 1000);     
    odotrip = odotrip + (currentspeedperms * (double)elapsedspeedtime / 1000);
    

    // Only save new values to the chip if the values are different - don't save all the time. Preserves life of the eeprom.
    if ((trip1 - lasttrip1) >= 0.1) {
      lasttrip1 = trip1;        
      SaveData();
    }

    if ((trip2 - lasttrip2) >= 0.1) {
      lasttrip2 = trip2;
      SaveData();
    }

    if ((odo - lastodo) >= 0.1) {
      lastodo = odo;
      SaveData();
    }

    if ((odotrip - lastodotrip) >= 1.0) {
      lastodotrip = odotrip;
      odo = startupodo + odotrip;
      SaveData();
    }
  }  


  // Warning / Info lights
  if (digitalRead (HIGHBEAMPIN) == HIGH) {
    highbeamlight = 1;
  } else {
    highbeamlight = 0;
  }
  
  
  if (digitalRead (INDICATELEFTPIN) == HIGH) {
    leftlight = 1;
  } else {
    leftlight = 0;
  }

  if (digitalRead (INDICATERIGHTPIN) == HIGH) {
    rightlight = 1;
  } else {
    rightlight = 0;
  }

  if (digitalRead (OILLEVELPIN) == LOW) {
    oillight = 0; // Oil level is ok, it goes LOW when OK and HIGH when not ok.
  } else {
    oillight = 1;
  }

  if (digitalRead (NEUTRALPIN) == LOW) {
    neutrallight = 1; // Goes LOW when in Neutral
  } else {
    neutrallight = 0; // Goes HIGH when in Gear
  }


  fuelresistance = analogRead (FUELPIN);  
  
  if (fuelresistance < 0) {
    fuelresistance = 0;
  }

  if (fuelAvgpointer < 70) {
    fuelsum += fuelresistance;
    fuelAvgpointer++;
  }

  if (fuelAvgpointer >= 70) {
    fuelAvgpointer = 0;
    calcfuelAvg = fuelsum / 70;
    fuelsum = 0;
  }

  // FUEL READING - INTERNAL FLOAT
  if (fuelintfloat == -99) { // First time initialisation of values
    fuelintfloat = fuelresistance;
    fuelintfloatlower = fuelresistance;
    fuelintfloatupper = fuelresistance;
    fuelintfloatmid = fuelresistance;
  }

  if (millis() - lastfloatchecktime > 2000) { // EXPERIMENT! This was greater than 2000
    lastfloatchecktime = millis();


    if (fuelresistance > fuelintfloat) {
      fuelintfloat++;
    }

    if (fuelresistance > fuelintfloatupper) {
      fuelintfloatupper++;
    }

    if (fuelresistance < fuelintfloat) {
      fuelintfloat--;
    }

    if (fuelresistance < fuelintfloatlower) {
      fuelintfloatlower--;
      fuelintfloatupper--; // If the lower amount has fallen then bring down the upper value too (fuel goes down with usage)
    }

    if (fuelintfloatupper > fuelintfloatlower) {
      fuelintfloatmid = fuelintfloatlower + ((fuelintfloatupper - fuelintfloatlower) / 2);
    } else {
      fuelintfloatmid = fuelintfloat;
    }

    /*
    MPG AND RANGE CALCULATION - DO THIS AT THE SAME INTERVAL AS FUEL FLOAT INTERVAL
    */
    // Another way to calculate MPG and Range
    // Base it on fuel usage per litre
    // Update - the method was too irattic - this time base it on fuel and miles done since fillup
    if (GetUnitsPerLitre() != 0) { // This will return zero if we don't have an accurate fuel level reading
      
      if (litresatfillup == 0) { //Initialise litres at fillup
        litresatfillup = litresremaining;
        lastfuelodo = odo;
      }

      if (litresremaining > lastlitresremaining) {
        // Tank has been refilled - reset last values
        if ((litresremaining - lastlitresremaining) > 2) {
          // More than 2 litres has been put in
          litresatfillup = litresremaining;
          lastfuelodo = odo;
        }
      }
      
      lastlitresremaining = litresremaining;    
    }
    
    // WORK OUT MPG AND RANGE BASED ON LITRES USED SINCE FILLUP AND MILES DONE SINCE FILLUP       
    milesperlitre = (odo-lastfuelodo);

    if (litresatfillup > litresremaining) { // Avoid weird values if we are doing a fuel gauge calibration exercise
      litresused = litresatfillup - litresremaining;
      safempg = (4.546 / litresused) * milesperlitre;
  
      if (safempg >=0 && safempg < 100) {
  
        if (avgmpgcount > 10) {
          avgmpg = 0;
          avgmpgcount = 0;
        }
  
        avgmpg = avgmpg + (unsigned long) safempg;
        avgmpgcount++;
        
        mpg = avgmpg / avgmpgcount;
  
        if (litresremaining > 0 && litresremaining <= 22) {
          saferange = ((double)mpg / 4.546) * litresremaining;
          range = (int)saferange;
        }
      }  
    }  
  }

  /*
    COOLANT TEMP - AVERAGING
  */
  if (coolantAvgpointer < 70) {
    coolantsum += coolanttemp;
    coolantAvgpointer++;
  }

  if (coolantAvgpointer >= 70) {
    coolantAvgpointer = 0;
    calccoolantAvg = coolantsum / 70;
    coolantsum = 0;
  }

  /*
    AMBIENT TEMP - AVERAGING
  */
 if (ambientAvgpointer < 170) {
    ambientsum += ambienttemp;
    ambientAvgpointer++;
  }

  if (ambientAvgpointer >= 170) {
    ambientAvgpointer = 0;
    calcambientAvg = ambientsum / 170;
    ambientsum = 0;
  }

  // Fan control

  if (fanneutraloption == 0) { // Always turn on engine fan when engine running and in neutral
    if (calcrpmAvg > 100 && neutrallight == 1 && enginerunningten == true) {
      FanControl (true); // Turn the fan on if the engine is running for 10 seconds and bike is in neutral
    }    
  }

  if (fanneutraloption == 1) { // Coolant temp fan on after 1 minute
    if (calcrpmAvg > 100 && neutrallight == 1 && enginerunningten == true) {
      if ((millis() - timeingear) > 60000) { // Been in neutral for 1 minute
        FanControl (true); // Turn the fan on if the engine is running for 10 seconds and bike is in neutral  
      }           
    }
  }

  if (fanneutraloption == 2) { // Coolant temp fan on after 50 degrees engine temp
    if (calcrpmAvg > 100 && neutrallight == 1 && enginerunningten == true) {
      if (calccoolantAvg >= 50) {
        FanControl (true); // Turn the fan on
      }
    }
  }

  if (fanneutraloption == 3) { // Coolant temp fan on after throttle blip
    if (calcrpmAvg > 2500 && neutrallight == 1 && enginerunningten == true) {      
      FanControl (true); // Turn the fan on      
    }
  }

  // Turn on the fan if engine temperature has reached preset temp for turning on the fan
  if (calcrpmAvg > 100 && calccoolantAvg >= coolantfantemp) {
    FanControl (true); // Turn the fan on
  } //else {
    //FanControl (false); // Turn the fan off    
  //}

  // Decide when to turn the fan OFF
  if (neutrallight == 0 && calccoolantAvg < coolantfantemp) {
    FanControl (false); // Turn the fan off
  } else {
    if (calcrpmAvg < 200) {
      FanControl (false); // Turn the fan off
    }
  }

  // Bit of code just to set a flag when the engine has been running for 10 seconds or more
  // Useful for fan control
  if (calcrpmAvg > 500 && enginerunningten == false) { // IF engine running
    if (enginetimestart == false) {
      engineruntime = millis();
      enginetimestart = true;
    } else {
      if ((millis() - engineruntime) > 10000) { // Engine has been running for 10 seconds
        timeingear = millis();
        enginerunningten = true;        
      }
    }    
  }

  /*
    BATTERY VOLTAGE + AVERAGING
  */
  
  if (battAvgpointer < 70) {
    battsum+= battvoltage;
    battAvgpointer++;
  }

  if (battAvgpointer >= 70) {
    battAvgpointer = 0;
    calcbattAvg = battsum / 70;
    battsum = 0;
  }

  if ((millis() - lightsenstime) > 500) {
    lightsenstime = millis();
    currentlightlevel = analogRead (LIGHTSENSORPIN);

    if (currentlightlevel < lightswitchvalue) { // It's gone really dark
       theme = nighttheme;
       nightswitchtime = millis();
    }

    if (getDifference (currentlightlevel, lightswitchvalue) > 20) {
      if (currentlightlevel > lightswitchvalue) {
        theme = daytheme;
        nightswitchtime = millis();
      }  
    }
  }   

  /* CHECK IF ANY DATA HAS ARRIVED ON THE BLUETOOTH MODULE*/
  // Any data arriving from a Navigation source gets sent directly to the Serial port for use by the TFT Dash Display software
  if (mySerial.available() > 0) {

    incomingBLEbyte = mySerial.read();

    szBLEreadbuf[bleReadpointer] = incomingBLEbyte;
    bleReadpointer++;

    if (szBLEreadbuf[strlen(szBLEreadbuf)-1] == '>' && szBLEreadbuf[strlen (szBLEreadbuf)-2] == '%') {
      bleReadpointer = 0;
      memset (szNav, 0, 255);
      strcpy (szNav, szBLEreadbuf);
      memset (szBLEreadbuf, 0, 255);
      if (navactive == false) {
        navactive = true;
        infomode = 3;
      }
    }

    // Just some buffer protection just incase the message we get is too long
    if (bleReadpointer > 253) {
      bleReadpointer = 0;
      memset (szBLEreadbuf, 0, 255);
    }
  }
  
}

unsigned long MilesOrKM(unsigned long miles)
{
    if (km == true) {
      double kilometres = (double) miles * 1.60934;
      return (unsigned long) kilometres;
    } else {
      return miles;
    }    
}

double DoubleMilesOrKM (double miles) {
  if (km == true) {
      double kilometres = miles * 1.60934;
      return kilometres;
    } else {
      return miles;
    }
}

void Appendcommadigit (int digit) {
  Serial.print (",");
  Serial.print (digit);
}

void Outputmenudata () {
  Serial.print ("[,");
  Serial.print (menulevel+choicestate);

  Appendcommadigit (ododigit1);
  Appendcommadigit (ododigit2);
  Appendcommadigit (ododigit3);
  Appendcommadigit (ododigit4);
  Appendcommadigit (ododigit5);
  Appendcommadigit (ododigit6);
  Appendcommadigit (odo2digit1);
  Appendcommadigit (odo2digit2);
  Appendcommadigit (odo2digit3);
  Appendcommadigit (odo2digit4);
  Appendcommadigit (odo2digit5);
  Appendcommadigit (odo2digit6);
  Appendcommadigit (odoerror);
  Appendcommadigit (settimedigit0);
  Appendcommadigit (settimedigit1);
  Appendcommadigit (settimedigit2);
  Appendcommadigit (settimedigit3);
  Appendcommadigit (spcdigit0);
  Appendcommadigit (spcdigit1);
  Appendcommadigit (spcdigit2);
  Appendcommadigit (spcdigit3);
  Appendcommadigit (frontsprocket);
  Appendcommadigit (rearsprocket);
  Appendcommadigit (coolantfantemp);
  Appendcommadigit (km);
  Appendcommadigit (fh);
  Appendcommadigit (bar);
  Appendcommadigit (frontsensor);
  Appendcommadigit (rearsensor);
  Appendcommadigit (frontpressurelow);
  Appendcommadigit (rearpressurelow);
  Appendcommadigit (controllayout);
  Appendcommadigit (daytheme);
  Appendcommadigit (nighttheme);
  Appendcommadigit (currentlightlevel);
  Appendcommadigit (lightswitchvalue);
  Appendcommadigit (fanneutraloption);
  Appendcommadigit (gearratiointerval);

  Serial.print (",]");
}

void Outputlivedata () {

  DateTime now = rtc.now();

  Serial.print ("{,");
  Serial.print ((unsigned int)MilesOrKM (calcspeedAvg));
  Serial.print (",");
  Serial.print (floatingrpm);
  Serial.print (",");
  Serial.print (calccoolantAvg);
  Serial.print (",");
  Serial.print (calcbattAvg);
  Serial.print (",");
  Serial.print (now.hour());
  Serial.print (",");
  Serial.print (now.minute());
  Serial.print (",");
  Serial.print (fuelintfloat);
  Serial.print (",");
  Serial.print (neutrallight);
  Serial.print (",");
  Serial.print (oillight);
  Serial.print (",");
  Serial.print (highbeamlight);
  Serial.print (",");
  Serial.print (leftlight);
  Serial.print (",");
  Serial.print (rightlight);
  Serial.print (",");
  Serial.print (menulevel+choicestate);
  Serial.print (",");
  Serial.print (infomode);
  Serial.print (",");
  Serial.print (DoubleMilesOrKM (trip1));
  Serial.print (",");
  Serial.print (DoubleMilesOrKM (trip2));
  Serial.print (",");
  Serial.print (DoubleMilesOrKM (odo));
  Serial.print (",");
  if (km == true) {    
    Serial.print ("1");
  } else {    
    Serial.print ("0");
  }
  Serial.print (",");
  Serial.print (speedcorrection);
  Serial.print (",");
  Serial.print (theme);
  Serial.print (",");
  Serial.print (calcambientAvg);
  Serial.print (",");
  Serial.print (currentgear);
  Serial.print (",");
  Serial.print (mpg);
  Serial.print (",");
  Serial.print (range);
  Serial.print (",");
  Serial.print ((unsigned int)MilesOrKM (maxspeed));
  Serial.print (",");
  Serial.print (triptimehour);
  Serial.print (",");
  Serial.print (triptimemin);
  Serial.print (",0,0,0,");
  if (fh == true) {
    Serial.print (1);
  } else {
    Serial.print (0);
  }
  Serial.print (",");
  if (bar == true) {
    Serial.print (1);
  } else {
    Serial.print (0);
  }
  Serial.print (",");
  Serial.print (frontsensor);
  Serial.print (",");
  Serial.print (rearsensor);
  Serial.print (",");
  Serial.print (frontpressurelow);
  Serial.print (",");
  Serial.print (rearpressurelow);
  Serial.print (",");
  Serial.print (szNav);
  Serial.print (",}");
}

void setup() {
  
  // put your setup code here, to run once:
  memset (szNav, 0, 255);  
  Serial.begin(115200);

  mySerial.begin (9600);

  pinMode (SPEEDSENSORPIN, INPUT);
  pinMode (RPMSENSORPIN, INPUT);
  pinMode (FUELPIN, INPUT);
  pinMode (COOLANTPIN, INPUT);
  pinMode (BATTERYPIN, INPUT);

  pinMode (OILLEVELPIN, INPUT);
  pinMode (NEUTRALPIN, INPUT);
  pinMode (HIGHBEAMPIN, INPUT);
  pinMode (INDICATELEFTPIN, INPUT);
  pinMode (INDICATERIGHTPIN, INPUT);

  pinMode (OPTIONPIN, INPUT);
  pinMode (SELECTPIN, INPUT);
  pinMode (FANPIN, OUTPUT);

  digitalWrite (FANPIN, LOW); // Production version with single relay board 

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");    
  }

  //setTime (19, 47);
  Wire.begin(); // initialise the connection
  // Initiliaze EEPROM library.
  eeprom.initialize();

  timer_start = 0; 
  attachInterrupt(digitalPinToInterrupt(SPEEDSENSORPIN), calcSpeedFullSignal, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RPMSENSORPIN), calcRpmFullSignal, CHANGE);

  if (LoadData() == false) {
    odosetupmenu(); // Odometer setup
  }
}

void loop() {

  Checkbuttoninput();

  if ((menulevel+choicestate) == 0) {
    livemode();
    Outputlivedata();
  } else {
    livemode();
    Outputmenudata ();
  }
}
