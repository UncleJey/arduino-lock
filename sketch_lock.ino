#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <Ethernet.h>

  ///////////////////////////
 //       Messages        //
///////////////////////////

#define LED1  7 // индикатор готовности
#define LOCK1 5 // Замок
#define MODE  A4 // Индикатор режима

void  successWrite()
{

}

void failedWrite()
{

}

void successDelete()
{

}

void filedEthernet()
{
  
}

void Busy()
{
  digitalWrite(LED1,LOW);
}

  ///////////////////////////
 //         LOCK          //
///////////////////////////

int openTime = 0;
void granted () 
{
  openTime = 100;
  digitalWrite(LOCK1,HIGH);
}

void denied() 
{

}

void AutoLock()
{
  if (openTime > 0)
    openTime --;
  else
  {
    digitalWrite(LOCK1,LOW);
  }
}

  ///////////////////////////
 //       Ethernet        //
///////////////////////////
 
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192,168,1,254);
EthernetServer server(80);

String RepeatData;

void SetupEthernet()
{
  Ethernet.begin(mac, ip);
  delay(1000);
  server.begin();
}

String seda = "";
void SendData(byte sensorNo, byte pID[], bool pFull = false)
{
  String kno = String(sensorNo)+":";
  if (pFull) // Не всегда отправляем полный отчёт. В штатных случаях только две последние цифры
    kno += String(pID[0],HEX);

  kno += String(pID[1],HEX);
  kno += String(pID[2],HEX);
  kno += String(pID[3],HEX);

  seda = seda + "|"+kno;
  if (seda.length() > 220)
    seda = "";
  Serial.println("store... "+kno);
}

void ReadEthernet()
{
  EthernetClient client = server.available();
  if (client) 
  {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) 
    {
      if (client.available()) 
      {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        //if (c == '\n' && currentLineIsBlank) 
        {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("seda");
          client.println();
          client.println(seda);
          seda = "";
          client.println();
          client.println("</html>");
          break;
        }
      }
    }
    // give the web browser time to receive the data
    delay(10);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

  ///////////////////////////
 //         MFC           //
///////////////////////////
/*
 * 
 * library https://github.com/ljos/MFRC522
 */

#define RST_PIN        8           // Configurable, see typical pin layout above
#define SS_1_PIN       6          // Configurable, see typical pin layout above
#define SS_2_PIN       9           // Configurable, see typical pin layout above

#define NR_OF_READERS   2

byte ssPins[] = {SS_1_PIN, SS_2_PIN};

//MFRC522 mfrc522[NR_OF_READERS];   // Create MFRC522 instance.
MFRC522 nfc1(SS_1_PIN, RST_PIN);
MFRC522 nfc2(SS_2_PIN, RST_PIN);

void dump_byte_array(byte *buffer, byte bufferSize) 
{
  for (byte i = 0; i < bufferSize; i++) 
  {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

  ///////////////////////////
 //       EEPROM          //
///////////////////////////

int successRead;    // Variable integer to keep if we have Successful Read from Reader
boolean programMode = false;  // initialize programming mode to false

byte storedCard[4];   // Stores an ID read from EEPROM
byte readCard[5];   // Stores scanned ID read from RFID Module
byte masterCard[4] = {0x00, 0x00, 0x00, 0x00};   // Stores master card's ID read from EEPROM
byte lastCard[5] = {0x00, 0x00, 0x00, 0x00};

//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( int number ) 
{
  int start = (number * 4 ) + 2;     // Figure out starting position
  for ( int i = 0; i < 4; i++ )      // Loop 4 times to get the 4 Bytes
    storedCard[i] = EEPROM.read(start + i);   // Assign values read from EEPROM to array
}

///////////////////////////////////////// Check Bytes   ///////////////////////////////////
boolean checkTwo ( byte a[], byte b[] ) 
{
  for ( int k = 0; k < 4; k++ )    // Loop 4 times
    if ( a[k] != b[k] )     // IF a != b then set match = false, one fails, all fail
    {
      return false;
      break;
    }

  return true;
}

///////////////////////////////////////// Find Slot   ///////////////////////////////////
int findIDSLOT( byte find[] ) 
{
  int count = EEPROM.read(0);       // Read the first Byte of EEPROM that
  for ( int i = 1; i <= count; i++ )     // Loop once for each EEPROM entry
  {
    readID(i);                // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) )    // Check to see if the storedCard read from EEPROM
    {
      // is the same as the find[] ID card passed
      return i;         // The slot number of the card
      break;          // Stop looking we found it
    }
  }
  return 0;
}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
boolean findID( byte find[] ) 
{
  int count = EEPROM.read(0);     // Read the first Byte of EEPROM that
  for ( int i = 1; i <= count; i++ )     // Loop once for each EEPROM entry
  {
    readID(i);          // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) )    // Check to see if the storedCard read from EEPROM
    {
      return true;
      break;  // Stop looking we found it
    }
  }
  return false;
}

///////////////////////////////////////// Add ID to EEPROM   ///////////////////////////////////
void writeID( byte a[] ) 
{
  if ( !findID( a ) )      // Before we write to the EEPROM, check to see if we have seen this card before!
  {
    int num = EEPROM.read(0);     // Get the numer of used spaces, position 0 stores the number of ID cards
    int start = ( num * 4 ) + 6;  // Figure out where the next slot starts
    num++;                // Increment the counter by one
    EEPROM.write( 0, num );     // Write the new count to the counter
    for ( int j = 0; j < 4; j++ )    // Loop 4 times
    {
      EEPROM.write( start + j, a[j] );  // Write the array values to EEPROM in the right position
    }
    successWrite();
    Serial.println(F("Succesfully added ID record to EEPROM"));
  }
  else 
  {
    failedWrite();
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  }
}

///////////////////////////////////////// Remove ID from EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) 
{
  if ( !findID( a ) )      // Before we delete from the EEPROM, check to see if we have this card!
  {
    failedWrite();      // If not
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  }
  else 
  {
    int num = EEPROM.read(0);   // Get the numer of used spaces, position 0 stores the number of ID cards
    int slot;       // Figure out the slot number of the card
    int start;      // = ( num * 4 ) + 6; // Figure out where the next slot starts
    int looping;    // The number of times the loop repeats
    int j;
    int count = EEPROM.read(0); // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT( a );   // Figure out the slot number of the card to delete
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      // Decrement the counter by one
    EEPROM.write( 0, num );   // Write the new count to the counter

    for ( j = 0; j < looping; j++ )          // Loop the card shift times
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Shift the array values to 4 places earlier in the EEPROM

    for ( int k = 0; k < 4; k++ )          // Shifting loop
      EEPROM.write( start + j + k, 0);

    successDelete();
    Serial.println(F("Succesfully removed ID record from EEPROM"));
  }
}

////////////////////// Check readCard IF is masterCard   ///////////////////////////////////
// Check to see if the ID passed is the master programing card
boolean isMaster( byte test[] ) 
{
  return checkTwo( test, masterCard );
}


  ///////////////////////////
 //      MAINLOOP         //
///////////////////////////

void setup()
{
 /// For Debug
  Serial.begin(115200);
  Serial.println("Start ..");
  
  pinMode(LED1, OUTPUT);
  pinMode(LOCK1, OUTPUT);
  pinMode(MODE, OUTPUT);

  digitalWrite(MODE,HIGH);
  
  Serial.println("Prepare MFC ..");
  /// MFC
  SPI.begin();      // Init SPI bus

  Serial.println("Looking for MFRC522.");
  nfc1.begin();
  nfc2.begin();

  // Get the firmware version of the RFID chip
  byte version = nfc1.getFirmwareVersion();
  if (! version) 
    Serial.print("Didn't find MFRC522 board.");

  Serial.print("Found chip MFRC522 ");
  Serial.print("Firmware ver. 0x");
  Serial.print(version, HEX);
  Serial.println(".");

  Serial.println("Prepare Ethernet ..");
  /// Ethernet
  SetupEthernet();
  digitalWrite(MODE,LOW);
  
  Serial.println("Ready ..");
}

int i = 0; int reader = 0; int cycles = 0;
void loop()
{
   digitalWrite(LED1,HIGH); // ready
   AutoLock();
   
  //displayTime(); // display the real-time clock data on the Serial Monitor,
  ReadEthernet();
  //delay(100);

  byte status;
  byte data[MAX_LEN];

  cycles++;
  if (cycles > 2000)
    cycles = 1000;
  
  reader ++;
  if (reader >= NR_OF_READERS)
    reader = 0;

  if (reader == 0)
    status = nfc1.requestTag(MF1_REQIDL, data);
  else
    status = nfc2.requestTag(MF1_REQIDL, data);
 
  if (status == MI_OK)
  {
    // calculate the anti-collision value for the currently detected
    // tag and write the serial into the data array.
    if (reader == 0)
      status = nfc1.antiCollision(data);
    else
      status = nfc2.antiCollision(data);

    if (status != MI_OK)
      return;
    
    memcpy(readCard, data, 5);

    if (checkTwo(lastCard, readCard))
    {
      if (cycles < 50)    
      {
        Serial.println(cycles);
        cycles = 0;
        return;
      }
    }
    memcpy(lastCard, readCard, 5);
    cycles = 0;

    dump_byte_array(readCard, 5);
    Serial.println("");

      if (programMode && reader == 0)
      {
        if ( isMaster(readCard) )  //If master card scanned again exit program mode
        {
          Serial.println(F("Master Card Scanned"));
          Serial.println(F("Exiting Program Mode"));
          Serial.println(F("-----------------------------"));
          programMode = false;
          digitalWrite(MODE,LOW);
          return;
        }
        else 
        {
          if ( findID(readCard) )  // If scanned card is known delete it
          {
            Serial.println(F("I know this PICC, removing..."));
            deleteID(readCard);
            SendData(0x04, readCard); // delete
            Serial.println("-----------------------------");
          }
          else if (!isMaster(readCard))                    // If scanned card is not known add it
          {
            Serial.println(F("I do not know this PICC, adding..."));
            writeID(readCard);
            SendData(0x03, readCard, true); // add
            Serial.println(F("-----------------------------"));
          }
        }
      }
      else // normal mode
      {
        if (isMaster(readCard) && reader == 0)     // If scanned card's ID matches Master Card's ID enter program mode
        {
          programMode = true;
          digitalWrite(MODE,HIGH);
          Serial.println(F("Hello Master - Entered Program Mode"));
          int count = EEPROM.read(0);   // Read the first Byte of EEPROM that
          Serial.print(F("I have "));     // stores the number of ID's in EEPROM
          Serial.print(count);
          Serial.print(F(" record(s) on EEPROM"));
          Serial.println("");
          Serial.println(F("Scan a PICC to ADD or REMOVE"));
          Serial.println(F("-----------------------------"));
        }
        else 
        {
          if ( findID(readCard) )  // If not, see if the card is in the EEPROM
          {
            Serial.println("Welcome, You shall pass");
            granted();         // Open the door lock for 5 s
            SendData(reader, readCard);
          }
          else       // If not, show that the ID was not valid
          {
            Serial.println("You shall not pass");
            denied();
            SendData(reader+10, readCard, true);
          }
        }
      } // normal mode
    } //if (mfrc522[reader].PICC_IsNewC
}


