#include <Wire.h>
#include <Adafruit_MCP23017.h>

Adafruit_MCP23017 mcp1;
Adafruit_MCP23017 mcp2;

struct MPin {
  MPin(Adafruit_MCP23017 *m, int p) : mcp(m), pin(p) {}
  
  Adafruit_MCP23017 *mcp;
  int pin;
};

MPin DATABUS[8] = {
  MPin(&mcp1, 8),
  MPin(&mcp1, 9),
  MPin(&mcp1, 10),
  MPin(&mcp1, 11),
  MPin(&mcp1, 12),
  MPin(&mcp1, 13),
  MPin(&mcp1, 14),
  MPin(&mcp1, 15),
};

MPin ADDRESSBUS[13] = {
  MPin(&mcp2, 10),
  MPin(&mcp2, 9),
  MPin(&mcp2, 8),
  MPin(&mcp2, 0),  
  MPin(&mcp2, 1),
  MPin(&mcp2, 2),
  MPin(&mcp2, 3),  
  MPin(&mcp2, 4),  
  MPin(&mcp2, 5),  
  MPin(&mcp2, 6),  
  MPin(&mcp2, 7),
  MPin(&mcp2, 11),  
  MPin(&mcp2, 12),    
};

MPin M1(&mcp1, 0);
MPin RESET(&mcp1, 1);
MPin WR(&mcp1, 2);
MPin RD(&mcp1, 3);
MPin NMI(&mcp1, 5);
MPin MREQ(&mcp1, 6);
MPin IORQ(&mcp1, 7);


int clockPin = 2;

void setup()
{
  Serial.begin(9600);
  
  mcp1.begin(0x0);
  mcp2.begin(0x1);
  
  setBusMode(ADDRESSBUS, 13, INPUT);
  setBusMode(DATABUS, 8, INPUT);  
  
  pinMode(clockPin, OUTPUT);
}

void loop()  
{
    
}

void setBusMode(struct MPin *a, int n, int mode)
{
  for (int i = 0; i < n; i++) {
    a[i].mcp->pinMode(a[i].pin, mode);
  }
}

void writePinArray(struct MPin *a, int n)
{
  int val = 0;
 
  for (int i = n - 1; i >= 0; i--) {
    boolean pval = (a[i].mcp->digitalRead(a[i].pin) != 0);
    Serial.print(pval ? "1" : "0");
    val |= (pval << i);
  }
  
  Serial.print(" "); Serial.print(val); Serial.print(" ");
  Serial.println(""); 
}

void writeBusState()
{
  Serial.print("Address: ");
  writePinArray(ADDRESSBUS, 13);
  
  Serial.print("Data: ");
  writePinArray(DATABUS, 8);
  
}

void tick()
{
 digitalWrite(clockPin, HIGH);
 delay(1000);
 digitalWrite(clockPin, LOW);
}
    
void serialEvent() {

  char cmd;
  
  cmd = Serial.read();
  
  switch (cmd) {
  case 'c':
    Serial.println("Executing Clock");
    writeBusState();
    tick();
    writeBusState();
    break;
  default:
    break;
  }
  Serial.flush();
}
