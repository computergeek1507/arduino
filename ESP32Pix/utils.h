
#ifndef UTILS_H_
#define UTILS_H_

#include "ESP32Pix.h"

int chase = 0;

void testMode()
{
	switch (configData.testMode)
	{
		case 1:
			for (int i = 0; i < NUM_PORTS; i++)
			{
				for (int j = 0; j < configData.ports[i].pixCount; j++)
				{
					leds[i][j] = CRGB::Red;
				}
			}
			break;
		case 2:
			for (int i = 0; i < NUM_PORTS; i++)
			{
				for (int j = 0; j < configData.ports[i].pixCount; j++)
				{
					leds[i][j] = CRGB::Green;
				}
			}
			break;
		case 3:
			for (int i = 0; i < NUM_PORTS; i++)
			{
				for (int j = 0; j < configData.ports[i].pixCount; j++)
				{
					leds[i][j] = CRGB::Blue;
				}
			}
			break;
		case 4:
		
			for (int i = 0; i < NUM_PORTS; i++)
			{
				for (int j = 0; j < configData.ports[i].pixCount; j++)
				{
					if((j + chase)%3==0)
						leds[i][j] = CRGB::Red;
					else if((j + chase)%3==1)
						leds[i][j] = CRGB::Green;
					else if((j + chase)%3==2)
						leds[i][j] = CRGB::Blue;
				}
			}
			chase++;
			if(chase>=3)
				chase = 0;
		default:
			break;
	}
	delay(1000);
}

#endif 