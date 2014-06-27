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
int memCE = 12;

void setup()
{
  Serial.begin(9600);
  
  mcp1.begin(0x0);
  mcp2.begin(0x1);
  
  setBusMode(ADDRESSBUS, 16, INPUT);
  setBusMode(DATABUS, 8, INPUT);  
  
  pinMode(memCE, OUTPUT);
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
  
  digitalWrite(memCE, HIGH);
  
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

void runToFetch(int *data, int *addr)
{
  
  while (1) {
   bool prevM1 = M1.digitalRead();
   bool prevMREQ = MREQ.digitalRead();

  *data = readBus(DATABUS, 8);
  *addr = readBus(ADDRESSBUS, 16);   
  
   tick();
  
   if (!prevM1 && !prevMREQ && M1.digitalRead() && MREQ.digitalRead()) {
    return;
   }
  
  }
  
}

void tick()
{
  
  digitalWrite(clockPin, LOW);
  LED.digitalWrite(HIGH);
  enableMemChip();
  delayMicroseconds(1);
   
  //writePinState();
  //writeBusState();
  digitalWrite(clockPin, HIGH);
  //writePinState();
  //writeBusState();
  LED.digitalWrite(LOW);
   
  enableMemChip();
  //delayMicroseconds(1);
  
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
  RESET.digitalWrite(LOW);
  tick();
  tick();
  tick();
  tick();
  tick();
  tick();
  tick();
  RESET.digitalWrite(HIGH);  
}


void enableMemChip()
{
  if (ADDRESSBUS[13].digitalRead() || ADDRESSBUS[14].digitalRead() || ADDRESSBUS[15].digitalRead()) {
    digitalWrite(memCE, LOW);
  } else {
    digitalWrite(memCE, HIGH);
  }
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

void runToLine()
{
  char buf[5];
  
  Serial.setTimeout(60 * 1000);
  int nread = Serial.readBytesUntil('\n', buf, 5);
  buf[nread] = 0;
  
  String hex = String("0x") + String(buf);

  Serial.println(hex);

  char bufStr[hex.length() + 1];
  
  hex.toCharArray(bufStr, hex.length() + 1);
  bufStr[hex.length()] = 0;
  
  Serial.println(bufStr);
  
  int n = strtol(bufStr, 0, 0);
  
  Serial.print("Running to Line: ");
  Serial.println(n);
    
  int addr, data;
  runToFetch(&data, &addr);
  
  while (1) {
    if (addr == n) {
      break;
    }

    runToFetch(&data, &addr);
  }

    writeBusState();
    writePinState();    
  
}

    
void serialEvent() {

  char cmd;
  
  cmd = Serial.read();
  Serial.flush();
  
  int data, addr;
  switch (cmd) {
  case 'c':
    Serial.println("Executing Clock");
    tick();
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
  case 'l':
    runToLine();
  default:
    Serial.print("Ignored "); Serial.println(cmd);
    break;
  }
}

void readNextOpCode()
{
  int data, addr;
  runToFetch(&data, &addr);
  Serial.print("OP: "); Serial.println(data, HEX);
  Serial.print("ADDRESS: "); Serial.println(addr, HEX);
}
