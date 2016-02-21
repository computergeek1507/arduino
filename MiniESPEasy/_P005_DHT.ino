//#######################################################################################################
//######################## Plugin 006: Temperature and Humidity sensor DHT 11/22 ########################
//#######################################################################################################

#define PLUGIN_005
#define PLUGIN_ID_005         5
#define PLUGIN_NAME_005       "Temperature & Humidity - DHT"
#define PLUGIN_VALUENAME1_005 "Temperature"
#define PLUGIN_VALUENAME2_005 "Humidity"

uint8_t Plugin_005_DHT_Pin;

boolean Plugin_005(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_005;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
        Device[deviceCount].VType = SENSOR_TYPE_TEMP_HUM;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 2;
        Device[deviceCount].SendDataOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_005);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_005));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_005));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        byte choice = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        String options[2];
        options[0] = F("DHT 11");
        options[1] = F("DHT 22");
        int optionValues[2];
        optionValues[0] = 11;
        optionValues[1] = 22;
        string += F("<TR><TD>DHT Type:<TD><select name='plugin_005_dhttype'>");
        for (byte x = 0; x < 2; x++)
        {
          string += F("<option value='");
          string += optionValues[x];
          string += "'";
          if (choice == optionValues[x])
            string += F(" selected");
          string += ">";
          string += options[x];
          string += F("</option>");
        }
        string += F("</select>");

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        String plugin1 = WebServer.arg("plugin_005_dhttype");
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = plugin1.toInt();
        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        byte dht_dat[5];
        byte dht_in;
        byte i;
        byte Retry = 0;
        boolean error = false;

        byte Par3 = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        Plugin_005_DHT_Pin = Settings.TaskDevicePin1[event->TaskIndex];

        pinMode(Plugin_005_DHT_Pin, OUTPUT);
        // DHT start condition, pull-down i/o pin for 18ms
        digitalWrite(Plugin_005_DHT_Pin, LOW);              // Pull low
        delay(18);
        digitalWrite(Plugin_005_DHT_Pin, HIGH);             // Pull high
        delayMicroseconds(40);
        pinMode(Plugin_005_DHT_Pin, INPUT);                 // change pin to input
        delayMicroseconds(10);

        dht_in = digitalRead(Plugin_005_DHT_Pin);
        if (!dht_in)
        {
          delayMicroseconds(80);
          dht_in = digitalRead(Plugin_005_DHT_Pin);
          if (dht_in)
          {
            delayMicroseconds(40);                     // now ready for data reception
            for (i = 0; i < 5; i++)
            {
              byte data = Plugin_005_read_dht_dat();
              if (data != -1)
                dht_dat[i] = data;
              else
              {
                addLog(LOG_LEVEL_ERROR, (char*)"DHT  : protocol timeout!");
                error = true;
              }
            }

            if (!error)
            {

              // Checksum calculation is a Rollover Checksum by design!
              byte dht_check_sum = dht_dat[0] + dht_dat[1] + dht_dat[2] + dht_dat[3]; // check check_sum

              if (dht_dat[4] == dht_check_sum)
              {
                float temperature = 0;
                float humidity = 0;
                
                if (Par3 == 11)
                {
                  temperature = float(dht_dat[2]); // Temperature
                  humidity = float(dht_dat[0]); // Humidity
                }

                if (Par3 == 22)
                {
                  if (dht_dat[2] & 0x80) // negative temperature
                    temperature = -0.1 * word(dht_dat[2] & 0x7F, dht_dat[3]);
                  else
                    temperature = 0.1 * word(dht_dat[2], dht_dat[3]);
                  humidity = word(dht_dat[0], dht_dat[1]) * 0.1; // Humidity
                }
                if (temperature == 0 && humidity == 0)
                {
                  String log = F("DHT  : No reading!");
                  log += UserVar[event->BaseVarIndex];
                  addLog(LOG_LEVEL_INFO, log);
                }
                else
                {
                  UserVar[event->BaseVarIndex] = temperature;
                  UserVar[event->BaseVarIndex + 1] = humidity;
                  String log = F("DHT  : Temperature: ");
                  log += UserVar[event->BaseVarIndex];
                  addLog(LOG_LEVEL_INFO, log);
                  log = F("DHT  : Humidity: ");
                  log += UserVar[event->BaseVarIndex + 1];
                  addLog(LOG_LEVEL_INFO, log);
                  success = true;
                }
              } // checksum
            } // error
          } // dht
        }  // !dht
        if(!success)
        {
          UserVar[event->BaseVarIndex] = NAN;
          UserVar[event->BaseVarIndex + 1] = NAN;
        }
        break;
      }
  }
  return success;
}


/*********************************************************************************************\
* DHT sub to get an 8 bit value from the receiving bitstream
\*********************************************************************************************/
int Plugin_005_read_dht_dat(void)
{
  byte i = 0;
  byte result = 0;
  byte counter = 0;
  //noInterrupts();
  for (i = 0; i < 8; i++)
  {
    while ((!digitalRead(Plugin_005_DHT_Pin)) && (counter < 100))
    {
      delayMicroseconds(1);
      counter++;
    }
    if (counter >= 100)
    {
      //interrupts();
      return -1;
    }
    delayMicroseconds(30);
    if (digitalRead(Plugin_005_DHT_Pin))
      result |= (1 << (7 - i));
    counter = 0;
    while ((digitalRead(Plugin_005_DHT_Pin)) && (counter < 100))
    {
      delayMicroseconds(1);
      counter++;
    }
    if (counter >= 100)
    {
      //interrupts();
      return -1;
    }
  }
  //interrupts();
  return result;
}

