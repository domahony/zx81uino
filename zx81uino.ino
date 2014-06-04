#include <Wire.h>
#include <Adafruit_MCP23017.h>

Adafruit_MCP23017 mcp1;
Adafruit_MCP23017 mcp2;

struct MPin {
  MPin(Adafruit_MCP23017 *m, int p) : mcp(m), pin(p) {}

  void pinMode(int mode) {
    mcp->pinMode(pin, mode);
  }
  
  
  bool digitalRead() {
    return mcp->digitalRead(pin);
  }
  
  void digitalWrite(bool value) {
    mcp->digitalWrite(pin, value);
  }
  
  Adafruit_MCP23017 *mcp;
  int pin;
};

MPin DATABUS[8] = {
  MPin(&mcp1, 0),
  MPin(&mcp1, 1),
  MPin(&mcp1, 2),
  MPin(&mcp1, 3),
  MPin(&mcp1, 4),
  MPin(&mcp1, 5),
  MPin(&mcp1, 6),
  MPin(&mcp1, 7),
};

MPin ADDRESSBUS[16] = {
  MPin(&mcp2, 12),
  MPin(&mcp2, 11),
  MPin(&mcp2, 10),
  MPin(&mcp2, 0),  
  MPin(&mcp2, 1),
  MPin(&mcp2, 2),
  MPin(&mcp2, 3),  
  MPin(&mcp2, 4),  
  MPin(&mcp2, 5),  
  MPin(&mcp2, 6),  
  MPin(&mcp2, 7),
  MPin(&mcp2, 8),  
  MPin(&mcp2, 9),    
  MPin(&mcp2, 15),  
  MPin(&mcp2, 14),  
  MPin(&mcp2, 13),  
};

MPin M1(&mcp1, 9);
MPin RESET(&mcp1, 8);
MPin WR(&mcp1, 10);
MPin RD(&mcp1, 11);
MPin NMI(&mcp1, 12);
MPin MREQ(&mcp1, 13);
MPin IORQ(&mcp1, 14);
MPin LED(&mcp1, 15);

int clockPin = 2;
int cePin = 6;
int oePin = 8; 

void setup()
{
  Serial.begin(9600);
  
  mcp1.begin(0x0);
  mcp2.begin(0x1);
  
  setBusMode(ADDRESSBUS, 16, INPUT);
  setBusMode(DATABUS, 8, INPUT);  
  
  pinMode(clockPin, OUTPUT);
  
  M1.pinMode(INPUT);
  RESET.pinMode(OUTPUT);
  WR.pinMode(INPUT);
  RD.pinMode(INPUT);
  NMI.pinMode(OUTPUT);
  MREQ.pinMode(INPUT);
  IORQ.pinMode(INPUT);
  LED.pinMode(OUTPUT); 
 
  RESET.digitalWrite(HIGH); 
  NMI.digitalWrite(HIGH);
  
  reset();
}

void loop()  
{
	delayMicroseconds(4);
}

void setBusMode(struct MPin *a, int n, int mode)
{
  for (int i = 0; i < n; i++) {
    a[i].pinMode(mode);
  }
}

void writePinArray(struct MPin *a, int n)
{
  int val = 0;
 
  for (int i = n - 1; i >= 0; i--) {
    boolean pval = (a[i].digitalRead() != 0);
    Serial.print(pval ? "1" : "0");
    val |= (pval << i);
  }
  
  Serial.print(" "); Serial.print(val, HEX); Serial.print(" ");
  Serial.println(""); 
}

int readBus(struct MPin *a, int n)
{
  int val = 0;
 
  for (int i = n - 1; i >= 0; i--) {
    boolean pval = (a[i].digitalRead() != 0);
    val |= (pval << i);
  }
  
  return val;
}

void writeBusState()
{
  Serial.print("Address: ");
  writePinArray(ADDRESSBUS, 16);
  
  Serial.print("Data: ");
  writePinArray(DATABUS, 8);
  
}

void pinVal(MPin* p, String s)
{
  Serial.print(s); Serial.print(": "); Serial.print(p->digitalRead()); Serial.print(" ");  
}

void writePinState()
{
  pinVal(&M1, "M1");
  pinVal(&WR, "WR");
  pinVal(&RD, "RD");
  pinVal(&MREQ, "MREQ");
  pinVal(&IORQ, "IORQ");
  Serial.println("");  
}

void tick(int *data, int *addr)
{
 digitalWrite(clockPin, HIGH);
 LED.digitalWrite(HIGH);
 writePinState();
 delay(100);
 LED.digitalWrite(LOW);
 *data = readBus(DATABUS, 8);
 *addr = readBus(ADDRESSBUS, 16);
 writePinState();
 writeBusState();
 digitalWrite(clockPin, LOW);
 delay(100);

}

void memRead(int addr)
{
  pinMode(cePin, OUTPUT);
  pinMode(oePin, OUTPUT);
  
  digitalWrite(cePin, HIGH);
  digitalWrite(oePin, HIGH);
  for (int i = 0; i < 13; i++) {
    ADDRESSBUS[i].mcp->pinMode(ADDRESSBUS[i].pin, OUTPUT);
    ADDRESSBUS[i].digitalWrite((addr >> i) & 0x1);
  }
  
  digitalWrite(cePin, LOW);
  digitalWrite(oePin, LOW);
  
  pinMode(cePin, INPUT);
  pinMode(oePin, INPUT);
  writeBusState();

  for (int i = 0; i < 13; i++) {
    ADDRESSBUS[i].mcp->pinMode(ADDRESSBUS[i].pin, INPUT);
  }    
}

void reset()
{
  int data, addr;
  RESET.digitalWrite(LOW);
  tick(&data, &addr);
  tick(&data, &addr);
  tick(&data, &addr);
  tick(&data, &addr);
  tick(&data, &addr);
  tick(&data, &addr);
  tick(&data, &addr);
  RESET.digitalWrite(HIGH);  
}

void readMemLocation()
{
  char buf[5];
  
  
  Serial.setTimeout(60 * 1000);
  int nread = Serial.readBytesUntil('\n', buf, 5);
  buf[nread] = 0;
  
  int n = atoi(buf);
  
  memRead(n);
  
  Serial.println(n, HEX);
}
    
void serialEvent() {

  char cmd;
  
  cmd = Serial.read();
  Serial.flush();
  
  int data, addr;
  switch (cmd) {
  case 'c':
    Serial.println("Executing Clock");
    tick(&data, &addr);
    break;
  case 'r':
    reset();
    break;
  case 'n':
    readMemLocation();
    break;
  case 'f':
    readNextOpCode();
    break;
  default:
    Serial.print("Ignored "); Serial.println(cmd);
    break;
  }
}

int readNextOpCode()
{
    int op, addr;
    findNextOpCode(&op, &addr);
    Serial.print("Found OP: "); Serial.print(op, HEX); Serial.print(" at "); Serial.println(addr, HEX);
  
}

int findNextOpCode(int *cmd, int* addr)
{
  int c, a;
  while (1) {
    if (!M1.digitalRead()) { 
      while (1) {
         if (!MREQ.digitalRead()) {
           *addr = a;
           *cmd = c;  
         }
         tick(&c, &a);
        if (M1.digitalRead()) {
         return 1;       
        }
      }
    }
    tick(&c, &a);
  }
}
