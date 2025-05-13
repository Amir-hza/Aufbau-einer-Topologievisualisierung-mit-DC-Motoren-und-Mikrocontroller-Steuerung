
// ===== ===== ===== ===== =====


#include <WiFi.h>
#include <HTTPClient.h>


// ===== ===== ===== ===== =====


const int NumberOfMotors = 3;

const String WiFiName = "Bash";
const String WiFiPass = "bashbashbash";

const String FileName = "01";
const String FileExt = ".txt";

const String FileAddress = "http://192.168.137.1";
const String ReadAddress = FileAddress + "/" + FileName + FileExt;

const int IntrPin_ButtonRetract[NumberOfMotors] = {12, 14, 13};

const int IntrPin_Encoder[NumberOfMotors] = {33, 26, 35}; //Orange
const int Pin_Encoder[NumberOfMotors] = {32, 25, 34};

const int Pin_Motor_Enable[NumberOfMotors] = {5, 4, 2};
const int Pin_Motor_Bwd[NumberOfMotors] = {19, 17, 22};
const int Pin_Motor_Fwd[NumberOfMotors] = {18, 16, 23};


// ===== ===== ===== ===== =====


volatile bool IsRunning[NumberOfMotors] = {true, true, true};
volatile double PositionSP[NumberOfMotors] = {0, 0, 0};

volatile long Pulse[NumberOfMotors] = {0, 0, 0};
volatile double PositionDiff[NumberOfMotors] = {0, 0, 0};
volatile double PositionCurrent[NumberOfMotors] = {0, 0, 0};

int MotorSpeedMin = 120;
int MotorSpeedMax = 220;
int MotorSpeedRet = 150;

const double Cir = 16;
const double pi = 3.14159265360;
const double Radius = Cir / pi / 2;
const double PositionTolerance = 0.1;

const double EncoderNumberOfHoles = 4200;
const double EncoderStepsDegree = 360 / EncoderNumberOfHoles;
const double EncoderStepCentiMeter = EncoderStepsDegree * pi * Radius / 180;


void Intr_Retract_M0() { Retract(0); }
void Intr_Retract_M1() { Retract(1); }
void Intr_Retract_M2() { Retract(2); }
void Intr_Retract_M3() { Retract(3); }

void Intr_Encoder_M0() { EncoderTick(0); }
void Intr_Encoder_M1() { EncoderTick(1); }
void Intr_Encoder_M2() { EncoderTick(2); }
void Intr_Encoder_M3() { EncoderTick(3); }

void InterruptAttach(int i)
{
  if (i == 0)
  {
    attachInterrupt(digitalPinToInterrupt(IntrPin_ButtonRetract[0]), Intr_Retract_M0, CHANGE);
    attachInterrupt(digitalPinToInterrupt(IntrPin_Encoder[0]), Intr_Encoder_M0, CHANGE);
  }
  else if (i == 1)
  {
    attachInterrupt(digitalPinToInterrupt(IntrPin_ButtonRetract[1]), Intr_Retract_M1, CHANGE);
    attachInterrupt(digitalPinToInterrupt(IntrPin_Encoder[1]), Intr_Encoder_M1, CHANGE);
  }
  else if (i == 2)
  {
    attachInterrupt(digitalPinToInterrupt(IntrPin_ButtonRetract[2]), Intr_Retract_M2, CHANGE);
    attachInterrupt(digitalPinToInterrupt(IntrPin_Encoder[2]), Intr_Encoder_M2, CHANGE);
  }
  else if (i == 3)
  {
    attachInterrupt(digitalPinToInterrupt(IntrPin_ButtonRetract[3]), Intr_Retract_M3, CHANGE);
    attachInterrupt(digitalPinToInterrupt(IntrPin_Encoder[3]), Intr_Encoder_M3, CHANGE);
  }
}


void EncoderTick(int Index)
{
  if (digitalRead(IntrPin_Encoder[Index]) == digitalRead(Pin_Encoder[Index])) Pulse[Index]--;
  else Pulse[Index]++;
  PositionCurrent[Index] = EncoderStepCentiMeter * Pulse[Index];
  PositionDiff[Index] = PositionSP[Index] - PositionCurrent[Index];
}


void Retract(int Index)
{
  StopMotor(Index, false);
  if (digitalRead(IntrPin_ButtonRetract[Index]))
  {
    Wait_mSec(200);
    
    if (!digitalRead(IntrPin_ButtonRetract[Index])) return;
    else Jog(Index, MotorSpeedRet, false);
  }
}



void Wait_mSec(uint mSec)
{
  unsigned long currentTime = millis();
  while (millis() - currentTime < mSec);
}


void ResetAllPositions(bool printAll, bool printOnce)
{
  for (int i = 0; i < NumberOfMotors; i++) ResetPosition(i, printAll);
  if (printOnce) Serial.println("All positions reset.");
}


void ResetPosition(int Index, bool printResult)
{
  PositionCurrent[Index] = 0;
  PositionDiff[Index] = 0;
  PositionSP[Index] = 0;
  Pulse[Index] = 0;
  if (printResult)
  {
    Serial.print("Motor ");
    Serial.print(Index);
    Serial.println(" Position RESET!");
  }
}


// ===== ===== ===== ===== =====


bool GetPositionSetpoint()
{
  // return ReadPositionSetpointFromSerial();
  // return ParseReadData("20\n40\n60");
  return ReadPositionSetpointFromWiFi();
}


bool ParseReadData(String ReadData)
{
  if (ReadData == "RESET!")
    ResetAllPositions(true, true);

  else if (ReadData != "ERROR")
  {
    int len = ReadData.length();
    char data[len + 1];
    ReadData.toCharArray(data, len + 1);
    char *token = strtok(data, "\n");
    int idx = 0;

    while (token != NULL)
    {
      if(idx >= 0)
      {
        float f = atof(token);
        PositionSP[idx] = (double)f;
        PositionDiff[idx] = PositionSP[idx] - PositionCurrent[idx];

        Serial.print("M");
        Serial.print(idx);
        Serial.print(" Data: ");
        Serial.print(f);
        Serial.print(", SP: ");
        Serial.print(PositionSP[idx]);
        Serial.print(", Diff: ");
        Serial.print(PositionDiff[idx]);
        if (!PositionReached(idx)) Serial.print(": Pos NOT OK");
        Serial.println("");
      }

      token = strtok(NULL, "\n");
      idx++;
    }
    
    return true;
  }

  else
  {
    Serial.println("Received ERROR!");
    return false;
  }
}


bool ReadPositionSetpointFromWiFi()
{
  if(WiFi.status() == WL_CONNECTED)
  {
      HTTPClient http;
      http.begin(ReadAddress.c_str());
      
      int httpResponseCode = http.GET();
      
      if (httpResponseCode > 0)
      {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);

        String payload = http.getString();
        Serial.println(payload);
        http.end();

        return ParseReadData(payload);
      }

      else
      {
        Serial.print("HTTP error code: ");
        Serial.println(httpResponseCode);
        http.end();
        return false;
      }
    }

    else
    {
      Serial.println("WiFi Disconnected!");
      return false;
    }
}


bool ReadPositionSetpointFromSerial()
{
  for (int i = 0; i < NumberOfMotors; i++)
  {
    PositionSP[i] = 0;
    Serial.print("Position motor #");
    Serial.print(i + 1);
    Serial.println("? ");
    while (!Serial.available()) {}
    PositionSP[i] = (double)Serial.parseFloat();
    PositionDiff[i] = PositionSP[i] - PositionCurrent[i];
    while (Serial.available() > 0) Serial.read();
  }
  Serial.println();
  for (int i = 0; i < NumberOfMotors; i++)
  {
    Serial.print("POS #");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(PositionSP[i]);
    Serial.print(" accepted. Current: ");
    Serial.print(PositionCurrent[i]);
    Serial.print(" Diff: ");
    Serial.println(PositionDiff[i]);
  }
  Serial.println();
  return true;
}


bool PositionReached(int Index)
{
  return abs(PositionDiff[Index]) <= abs(PositionTolerance);
}


bool AllPositionsOK(bool printNotOK, bool printAll)
{
  bool _ok = true;
  for (int i = 0; i < NumberOfMotors; i++)
  {
    if (!PositionReached(i))
    {
      if (printNotOK || printAll)
      {
        Serial.print("Position motor ");
        Serial.print(i);
        Serial.print(": NOT OK: ");
        Serial.print(PositionCurrent[i]);
        Serial.print(" (SP: ");
        Serial.print(PositionSP[i]);
        Serial.println(")");
      }

      _ok = false;
      break;
    }

    else if (printAll)
    {
      Serial.print("Position motor ");
      Serial.print(i);
      Serial.print(": OK: ");
      Serial.print(PositionCurrent[i]);
      Serial.print(" (SP: ");
      Serial.print(PositionSP[i]);
      Serial.println(")");
    }
  }
  
  return _ok;
}


int KeepInRange_Int(int Value, int Min, int Max)
{
  if (Value < Min) return Min;
  if (Value > Max) return Max;
  return Value;
}


int MotorSpeed(int Index)
{
  return KeepInRange_Int(map(abs(PositionDiff[Index]), 0, 5, MotorSpeedMin, MotorSpeedMax), MotorSpeedMin, MotorSpeedMax);
}


void Jog(int Index, int Speed, bool JogDown)
{
  if (Index < 0) return;
  int _speed = KeepInRange_Int(Speed, MotorSpeedMin, MotorSpeedMax);
  if (JogDown)
  {
    digitalWrite(Pin_Motor_Fwd[Index], LOW);
    digitalWrite(Pin_Motor_Bwd[Index], HIGH);
  }
  else
  {
    digitalWrite(Pin_Motor_Fwd[Index], HIGH);
    digitalWrite(Pin_Motor_Bwd[Index], LOW);
  }
  analogWrite(Pin_Motor_Enable[Index], _speed);
  IsRunning[Index] = _speed != 0;
}


void StartMotor(int Index, bool printResult)
{
  if (printResult)
  {
    Serial.print("Motor ");
    Serial.print(Index);
  }

  if (PositionDiff[Index] > 0)
  {
    digitalWrite(Pin_Motor_Fwd[Index], LOW);
    digitalWrite(Pin_Motor_Bwd[Index], HIGH);
    if (printResult) Serial.print(" FW: ");
  }
  else
  {
    digitalWrite(Pin_Motor_Fwd[Index], HIGH);
    digitalWrite(Pin_Motor_Bwd[Index], LOW);
    if (printResult) Serial.print(" BW: ");
  }

  int _speed = MotorSpeed(Index);
  analogWrite(Pin_Motor_Enable[Index], _speed);
  
  IsRunning[Index] = _speed != 0;

  if (printResult)
  {
    Serial.print(_speed);
    Serial.print(" SP: ");
    Serial.print(PositionSP[Index]);
    Serial.print(" Act: ");
    Serial.print(PositionCurrent[Index]);
    Serial.print(" Diff: ");
    Serial.println(PositionDiff[Index]);
  }
}


void StopMotor(int Index, bool printResult)
{
  digitalWrite(Pin_Motor_Fwd[Index], LOW);
  digitalWrite(Pin_Motor_Bwd[Index], LOW);
  analogWrite(Pin_Motor_Enable[Index], 0);
  IsRunning[Index] = false;

  if (printResult)
  {
    Serial.print("Motor #");
    Serial.print(Index);
    Serial.print(" stopped.");
    Serial.println();
  }
}


void StartAllMotors(bool printAll, bool printOnce)
{
  for (int i = 0; i < NumberOfMotors; i++)
    if (!PositionReached(i)) StartMotor(i, printAll);
    else StopMotor(i, printAll);
  if (printOnce) Serial.println("All motors started.");
}


void StopAllMotors(bool printAll, bool printOnce)
{
  for (int i = 0; i < NumberOfMotors; i++) StopMotor(i, printAll);
  if (printOnce) Serial.println("All motors stoped.");
}


void PrintDetails(int i)
{
  Serial.print("M"); Serial.print(i);
  //Serial.print(":\tCycles: "); Serial.print(CycleNumber);
  Serial.print("\tPosSP: "); Serial.print(PositionSP[i]);
  Serial.print("\tPosAct: "); Serial.print(PositionCurrent[i]);
  Serial.print("\tPosDiff: "); Serial.print(PositionDiff[i]);
  Serial.println();
}


void setup()
{
  for (int i = 0; i < NumberOfMotors; i++)
  {
    pinMode(IntrPin_ButtonRetract[i], INPUT);

    pinMode(IntrPin_Encoder[i], INPUT);
    pinMode(Pin_Encoder[i], INPUT);

    pinMode(Pin_Motor_Fwd[i], OUTPUT);
    pinMode(Pin_Motor_Bwd[i], OUTPUT);

    pinMode(Pin_Motor_Enable[i], OUTPUT);
    analogWrite(Pin_Motor_Enable[i], 0);

    InterruptAttach(i);
  }

  Serial.begin(115200);
  delay(3000);
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WiFiName, WiFiPass);

  bool connected = false;
  while (!connected)
  {
    connected = WiFi.status() == WL_CONNECTED;
    Serial.print("\nConnecting to WiFi ");

    for (int i = 0; i < 10; i++)
    {
      if (connected) break;
      else
      {
        Serial.print(".");
        delay(250);
        connected = WiFi.status() == WL_CONNECTED;
      }
    }
  }

  Serial.println();
  Serial.print("\nConnected: ");
  Serial.println(WiFi.localIP());

  Serial.println();
  Serial.println();
  Serial.print("Cir: ");
  Serial.println(Cir);
  Serial.print("Pi: ");
  Serial.println(pi);
  Serial.print("Radius: ");
  Serial.println(Radius);
  Serial.println("");
  Serial.print("EncoderNumberOfHoles: ");
  Serial.println(EncoderNumberOfHoles);
  Serial.print("EncoderStepsDegree: ");
  Serial.println(EncoderStepsDegree);
  Serial.print("EncoderStepCentiMeter: ");
  Serial.println(EncoderStepCentiMeter);
  Serial.println();
  
  for (int i = 0; i < NumberOfMotors; i++)
  {
    Pulse[i] = 0;
    PositionCurrent[i] = 0;
    PositionDiff[i] = 0;
  }

  Serial.println("SETUP DONE");
}


void loop()
{
  loopMain(false, false);
}


void loopMain(bool printAll, bool printOnce)
{
  Serial.println();
  Serial.println("===== ===== ===== =====");
  for (int i = 0; i < NumberOfMotors; i++) PrintDetails(i);
  delay(2000);
  Serial.println();
  Serial.println("===== ===== ===== =====");
  if (GetPositionSetpoint())
  {
    bool Retract = false;
    for (int i = 0; i < NumberOfMotors; i++)
      if (digitalRead(IntrPin_ButtonRetract[i]))
      {
        Retract = true;
        break;
      }

    while (!Retract && !AllPositionsOK(true, printAll)) StartAllMotors(printOnce, printAll);
    StopAllMotors(printOnce, printAll);
  }
}

