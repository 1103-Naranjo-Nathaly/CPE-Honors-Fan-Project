//CPE Honors Project: Smart Fan
//Nathaly Naranjo 
//Date: 05/2/2026
#define RDA 0x80
#define TBE 0x20  
#define DHTPIN 13 // Pin where your sensor is connected
#define DHTTYPE DHT11

#include <LiquidCrystal.h>
#include <Keypad.h>
#include <RTClib.h>
#include <DHT.h>

unsigned char* ddr_b = (unsigned char*) 0x24;
unsigned char* port_b = (unsigned char*) 0x25;
volatile unsigned char* pin_b = (unsigned char*) 0x23;
unsigned char* ddr_h = (unsigned char*) 0x101;
unsigned char* port_h = (unsigned char*) 0x102;
unsigned char* port_j = (unsigned char*) 0x105;
unsigned char* ddr_j = (unsigned char*) 0x104;
unsigned char* port_d = (unsigned char*) 0x2B;
unsigned char* ddr_d = (unsigned char*) 0x2A;

//Millis stuff
unsigned long previousMillis = 0;
const long interval = 60000;

const int ROW_NUM = 4; //four rows
const int COLUMN_NUM = 4; //four columns

//Analog Reading Stuff
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

//Delay Stuff
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *myTCNT1  = (unsigned  int *) 0x84;
volatile unsigned char *myTIFR1 =  (unsigned char *) 0x36;

//Serial Stuff
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;


volatile byte state = 0;

RTC_DS3231 rtc;
DHT dht(DHTPIN, DHTTYPE);
int speedPin = 6;
//int dir1 = 14;
//int dir2 = 15;
int mSpeed=90;

const int RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3', 'A'},
  {'4','5','6', 'B'},
  {'7','8','9', 'C'},
  {'*','0','#', 'D'}
};

byte pin_rows[ROW_NUM] = {48, 46, 44, 42}; //connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = {40, 38, 36, 34}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

bool state_change = true;
bool fan = false;
int value = 4;

void setup() {
  U0init(9600);
  *ddr_h |= 0x40; //pin 9 ACTIVE GREEN
  *ddr_h |= 0x20; //pin 8 OFF BLUE 
  *ddr_h |= 0x10; //pin 7 ERROR RED
  *ddr_b |= 0x10; //pin 10 IDLE YELLOW
  *ddr_h |= 0x02; //pin 16 MANUAL WHITE

  *ddr_b |= 0x80; // pin 13
  *ddr_j |= 0x02; // pin 14
  *ddr_j |= 0x01; // pin 15
  *ddr_h |= 0x08; //pin 6
  //*ddr_d |= 0x02; // pin 20
  //*ddr_d |= 0x01; // pin 21
  *port_j &= 0xFE; //dir1 low
  *port_j |= 0X01; //dir2 high
  *ddr_d &= 0xF7;  // Pin 18 (for button=input)
  *port_d |= 0x08; // Pin 18 (button=input/pullup)

  lcd.begin(16,2);
  dht.begin();
  lcd.setCursor(0,0);
 

  adc_init();
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  attachInterrupt(digitalPinToInterrupt(18), off_to_idle, FALLING);
}

void loop() {
  //unsigned long currentMillis = millis();
  DateTime now = rtc.now();
  char key = keypad.getKey();
  if(key == '0'){ //off
    analogWrite(speedPin,0);
    state_change = true;
    state = 0;
  }else if(key == '1'){ //reset
    state_change = true;
    state = 1;
  }else if(key == '2'){ // low speed
    state_change = true;
    state = 4;
    fan = true;
  }else if(key == '3'){ //med speed
    state_change = true;
    state = 5;
    fan = true;
  }else if(key == '4'){ //high speed
    state_change = true;
    state = 6;
    fan = true;
  }

  if(state_change){
    ValueADC(now.hour());
    U0putchar(':');
    ValueADC(now.minute());
    U0putchar(':');
    ValueADC(now.second());
    
    int difference = value-state;
    if(difference == 1){
      U0putchar(' ');
      U0putchar('E');
      U0putchar('R');
      U0putchar('R');
      U0putchar('O');
      U0putchar('R');
      U0putchar(':');
      U0putchar(' ');
      U0putchar('S');
      U0putchar('e');
      U0putchar('n');
      U0putchar('s');
      U0putchar('o');
      U0putchar('r');
      U0putchar(' ');
      U0putchar('O');
      U0putchar('u');
      U0putchar('t');
      U0putchar(' ');
      U0putchar('O');
      U0putchar('f');
      U0putchar(' ');
      U0putchar('R');
      U0putchar('a');
      U0putchar('n');
      U0putchar('g');
      U0putchar('e');    
    }else if(difference == 2){
      U0putchar(' ');
      U0putchar('A');
      U0putchar('C');
      U0putchar('T');
      U0putchar('I');
      U0putchar('V');
      U0putchar('E');
      U0putchar(' ');
      U0putchar('S');
      U0putchar('t');
      U0putchar('a');
      U0putchar('t');
      U0putchar('e');
      U0putchar(' ');
      U0putchar('E');
      U0putchar('n');
      U0putchar('t');
      U0putchar('e');
      U0putchar('r');
      U0putchar('e');
      U0putchar('d');
    }else if(difference == 3){
      U0putchar(' ');
      U0putchar('I');
      U0putchar('D');
      U0putchar('L');
      U0putchar('E');
    }else if(difference == 4){
      U0putchar(' ');
      U0putchar('S');
      U0putchar('y');
      U0putchar('s');
      U0putchar('t');
      U0putchar('e');
      U0putchar('m');
      U0putchar(' ');
      U0putchar('O');
      U0putchar('F');
      U0putchar('F');
    }else if(difference == 0){//manual low
      U0putchar(' ');
      U0putchar('M');
      U0putchar('a');
      U0putchar('n');
      U0putchar('u');
      U0putchar('a');
      U0putchar('l');
      U0putchar(' ');
      U0putchar('M');
      U0putchar('o');
      U0putchar('d');
      U0putchar('e');
      U0putchar(':');
      U0putchar(' ');
      U0putchar('L');
      U0putchar('o');
      U0putchar('w');
      U0putchar(' ');
      U0putchar('S');
      U0putchar('p');
      U0putchar('e');
      U0putchar('e');
      U0putchar('d');      
    }else if(difference == -1){//manual med
      U0putchar(' ');
      U0putchar('M');
      U0putchar('a');
      U0putchar('n');
      U0putchar('u');
      U0putchar('a');
      U0putchar('l');
      U0putchar(' ');
      U0putchar('M');
      U0putchar('o');
      U0putchar('d');
      U0putchar('e');
      U0putchar(':');
      U0putchar(' ');
      U0putchar('M');
      U0putchar('e');
      U0putchar('d');
      U0putchar('i');
      U0putchar('u');
      U0putchar('m');
      U0putchar(' ');
      U0putchar('S');
      U0putchar('p');
      U0putchar('e');
      U0putchar('e');
      U0putchar('d');      
    }else if(difference == -2){//manual high
      U0putchar(' ');
      U0putchar('M');
      U0putchar('a');
      U0putchar('n');
      U0putchar('u');
      U0putchar('a');
      U0putchar('l');
      U0putchar(' ');
      U0putchar('M');
      U0putchar('o');
      U0putchar('d');
      U0putchar('e');
      U0putchar(':');
      U0putchar(' ');
      U0putchar('H');
      U0putchar('i');
      U0putchar('g');
      U0putchar('h');
      U0putchar(' ');
      U0putchar('S');
      U0putchar('p');
      U0putchar('e');
      U0putchar('e');
      U0putchar('d'); 
    } 
    if(fan){
      U0putchar('\n');
      ValueADC(now.hour());
      U0putchar(':');
      ValueADC(now.minute());
      U0putchar(':');
      ValueADC(now.second());
      U0putchar(' ');
      U0putchar('F');
      U0putchar('a');
      U0putchar('n');
      U0putchar(' ');
      U0putchar('O');
      U0putchar('n');
    }
    U0putchar('\n');
    state_change = false;
    fan = false;
  }

  if(state == 0){ //OFF STATE
    *port_h |= 0x20; //blue HIGH
    *port_h &= 0xEF; // red LOW
    *port_h &= 0xBF; // green LOW
    *port_b &= 0xEF; // yellow LOW
    *port_h &= 0xFD; //white LOW
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("System OFF");
  }
  else if(state == 1){ //IDLE STATE
    analogWrite(speedPin,0);
    unsigned int photoValue = adc_read(0);
    *port_h &= 0xDF; //blue LOW
    *port_h &= 0xEF; // red LOW
    *port_h &= 0xBF; // green LOW
    *port_b |= 0x10; // yellow HIGH
    *port_h &= 0xFD; //white LOW
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("IDLE");
    lcd.setCursor(0,1);
    lcd.print("Light: ");
    lcd.print(photoValue); //if over 550

    if(photoValue > 550){
      state = 2;
      state_change = true;
    }

  }
  else if(state == 2){//ACTIVE
    unsigned long currentMillis = millis();
    int temperature = dht.readTemperature(); 
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("ACTIVE");
    lcd.setCursor(0,1);
       
    //Activating fan, light, and lcd because we had correct temp reading
    *port_h |= 0x40;//green high
    *port_h &= 0xEF;//red low
    *port_h &= 0xDF;//blue low
    *port_b &= 0xEF;//yellow low
    *port_h &= 0xFD; //white LOW

    if(isnan(temperature)){
      analogWrite(speedPin,0);
      //*port_h &= 0xBF; ; //Light is off because it didn't detect temperature
      return;
    }
    if(currentMillis-previousMillis >= interval){
      previousMillis = currentMillis;
      lcd.print("Temp: ");
      lcd.print(temperature);
      lcd.print(" C"); 
    }

    //fan logic
    if(temperature < 15.6){
      state = 3;
      state_change = true;
    }
    else if(temperature >=15.6 && temperature < 26.67){
      analogWrite(speedPin,90);
      fan = true;
    }
    else if(temperature >= 26.67 && temperature < 32.22 ){
      analogWrite(speedPin, 255);
      fan = true;
    } 
    else if(temperature >= 32.22){
      analogWrite(speedPin, 255);
      fan = true;
    }
    
  }
  else if(state == 3){//ERROR
    analogWrite(speedPin,0);
    *port_h |= 0x10; //red high
    *port_h &= 0xBF;//green low
    *port_b &= 0xEF;//yellow low
    *port_h &= 0xDF; //blue low
    *port_h &= 0xFD; //white LOW
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("ERROR:" );
    lcd.setCursor(0,1);
    lcd.print("Sensor Fault");
  }
  else if(state == 4){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("MANUAL");
    lcd.setCursor(0,1);
    lcd.print("Low Speed");

    analogWrite(speedPin, 90);

    *port_h &= 0x10; //red low
    *port_h &= 0xBF;//green low
    *port_b &= 0xEF;//yellow low
    *port_h &= 0xDF; //blue low
    *port_h |= 0x02; //white LO
  }
  else if(state == 5){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("MANUAL");
    lcd.setCursor(0,1);
    lcd.print("Medium Speed");

    analogWrite(speedPin, 150);
    
    *port_h &= 0x10; //red low
    *port_h &= 0xBF;//green low
    *port_b &= 0xEF;//yellow low
    *port_h &= 0xDF; //blue low
    *port_h |= 0x02; //white LOW    
  }
  else if(state == 6){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("MANUAL");
    lcd.setCursor(0,1);
    lcd.print("High Speed");

    analogWrite(speedPin, 250);
    
    *port_h &= 0x10; //red low
    *port_h &= 0xBF;//green low
    *port_b &= 0xEF;//yellow low
    *port_h &= 0xDF; //blue low
    *port_h |= 0x02; //white LOW
  }
  
  my_delay(150);
}

//analog read
void adc_init() {
  *my_ADCSRA |= 0b10000000;
  *my_ADCSRA &= 0b11011111;
  *my_ADCSRA &= 0b11110111;
  *my_ADCSRA &= 0b11111000;
  *my_ADCSRB &= 0b11110111;
  *my_ADCSRB &= 0b11111000;
  *my_ADMUX &= 0b01111111;
  *my_ADMUX |= 0b01000000;
  *my_ADMUX &= 0b11011111;
  *my_ADMUX &= 0b11100000;
}

unsigned int adc_read(unsigned char adc_channel_num){
  *my_ADMUX &= 0b11100000;
  *my_ADCSRB &= 0b11110111;
  *my_ADCSRA |= 0b01000000;
  while((*my_ADCSRA & 0x40) != 0);
  unsigned int val = *my_ADC_DATA;
  return val;
}

//interrupt
void off_to_idle(){
  state = 1;
  state_change = true;
}

//delay
void my_delay(unsigned int freq){
  double period = 1.0/double(freq);
  double half_period = period/2;
  double clk_period = 0.0000000625;
  unsigned int ticks = half_period/clk_period;
  *myTCCR1B &= 0xF8;
  *myTCNT1 = (unsigned int)(65536 - ticks);
  *myTCCR1A = 0x0;
  *myTCCR1B |= 0x01;
  while((*myTIFR1 & 0x01)==0); 
  *myTCCR1B &= 0xF8;        
  *myTIFR1 |= 0x01;
}

//Serial Stuff
void U0init(int U0baud)
{
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}
unsigned char U0kbhit()
{
  return *myUCSR0A & RDA;
}
unsigned char U0getchar()
{
  return *myUDR0;
}
void U0putchar(unsigned char U0pdata)
{
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}

void ValueADC(unsigned int num){
  unsigned hunds,ten,one;
  //hunds = num/100;
  ten = (num%100)/10;
  one = num%10;
  //U0putchar(hunds + '0');
  U0putchar(ten + '0');
  U0putchar(one + '0');
}