#define BOARD_ADS1220
#include <SPI.h>
#include <SPIFFS.h>
//#include <TFT_eSPI.h>
#include <HardwareSerial.h>
#include <Button2.h>
#include "time.h"
#include "ADC.h"
#include "UI.h"
#include "wifiSys.h"

#define	LED_BUILTIN	4
#define BUZZER		26
#define BUTTON_1	35
#define BUTTON_2	0

TFT_eSPI tft	= TFT_eSPI (TFT_WIDTH, TFT_HEIGHT);

Button2		btnOK (BUTTON_1);
Button2		btnSelect (BUTTON_2);

bool		fadeComplete = false;
uint16_t	targetBrightness;

time_t		startTime;

void fadePWM (void *param) {
	uint16_t target = *((uint16_t*)param);

	Serial.print ("Target: ");
	Serial.println (target);

	float brightness = ledcRead (0) + 1;
	//bool dir = (bool*)param;
	bool dir = (target - brightness) > 0.0f;
	
	if (target < 0) target = 0;
	if (target > 1023) target = 1023;
	
	fadeComplete = false;

	Serial.print ("Brightness start: ");
	Serial.println (brightness);
	Serial.print ("Richting: ");
	Serial.println (dir);
	//Serial.println (dir ? 1024 : 0);
	while (dir ? (brightness < target) : (brightness > (target + 1))) {
		brightness *= dir ? 1.2 : (1.0f / 1.2f);
		Serial.print ("Brightness naar ");
		if (dir == 0) {
			if (brightness < target) brightness = target;
		}
		else {
			if (brightness > target) brightness = target;
		}
		Serial.println (brightness);
		ledcWrite (0, max (min (1023.0f, brightness), 0.0f));
		delay (10);
	}
	
	fadeComplete = true;	
	vTaskDelete (NULL);
}

void shutDown (void *param) {
	static bool dir2 = false;
	fadeComplete = false;
	targetBrightness = 0;
	xTaskCreate (fadePWM, "fadeOutPWM", 1000, (void*)&targetBrightness, 2, NULL);
	
	ADC.powerDown ();
		
	delay (20);
	esp_sleep_enable_ext1_wakeup (GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);
	while (fadeComplete == false) {
		delay (10);
	}
		
	tft.writecommand (TFT_DISPOFF);
	tft.writecommand (TFT_SLPIN);
	esp_deep_sleep_start ();
	vTaskDelete (NULL);
}

void initADC (void *param) {
	ADC.begin (CS_PIN, DRDY_PIN);
	while (1) {
		ADC.loop ();
	}
	// should never get here
	vTaskDelete (NULL);
}

void setup ()
{
	pinMode (LED_BUILTIN, OUTPUT);

	digitalWrite (TFT_BL, 0);
	pinMode (TFT_BL, OUTPUT);

	Serial.begin (115200);

	xTaskCreate (initADC, "initADC", 3000, NULL, 5, NULL);

	ledcSetup (0, 4000, 10);
	ledcWrite (0, 0);
	ledcAttachPin (TFT_BL, 0);	
	
	tft.init ();	
		
	setup_t tft_setting;
	tft.getSetup (tft_setting);
	Serial.print ("TFT_eSPI ver = " + tft_setting.version + "\n");
	Serial.printf ("Processor    = ESP%x\n", tft_setting.esp);
	Serial.printf ("Frequency    = %i MHz\n", ESP.getCpuFreqMHz ());
#ifdef ESP8266
	Serial.printf ("Voltage      = %2.2f V\n", ESP.getVcc () / 918.0);   // 918 empirically determined
#endif
	Serial.printf ("Transactions = %s \n", (tft_setting.trans  ==  1) ? "Yes" : "No");
	Serial.printf ("Interface    = %s \n", (tft_setting.serial ==  1) ? "SPI" : "Parallel");
#ifdef ESP8266
	if (tft_setting.serial ==  1)
		Serial.printf ("SPI overlap  = %s \n\n", (tft_setting.overlap == 1) ? "Yes" : "No");
#endif
	if (tft_setting.tft_driver != 0xE9D) // For ePaper displays the size is defined in the sketch
	{
		Serial.printf ("Display driver = %x \n", tft_setting.tft_driver);   // Hexadecimal code
		Serial.printf ("Display width  = %i \n", tft_setting.tft_width);    // Rotation 0 width and height
		Serial.printf ("Display height = %i \n\n", tft_setting.tft_height);
	} else if (tft_setting.tft_driver == 0xE9D) Serial.printf ("Display driver = ePaper\n\n");

	if (tft_setting.r0_x_offset  != 0)  Serial.printf ("R0 x offset = %i \n", tft_setting.r0_x_offset);   // Offsets, not all used yet
	if (tft_setting.r0_y_offset  != 0)  Serial.printf ("R0 y offset = %i \n", tft_setting.r0_y_offset);
	if (tft_setting.r1_x_offset  != 0)  Serial.printf ("R1 x offset = %i \n", tft_setting.r1_x_offset);
	if (tft_setting.r1_y_offset  != 0)  Serial.printf ("R1 y offset = %i \n", tft_setting.r1_y_offset);
	if (tft_setting.r2_x_offset  != 0)  Serial.printf ("R2 x offset = %i \n", tft_setting.r2_x_offset);
	if (tft_setting.r2_y_offset  != 0)  Serial.printf ("R2 y offset = %i \n", tft_setting.r2_y_offset);
	if (tft_setting.r3_x_offset  != 0)  Serial.printf ("R3 x offset = %i \n", tft_setting.r3_x_offset);
	if (tft_setting.r3_y_offset  != 0)  Serial.printf ("R3 y offset = %i \n\n", tft_setting.r3_y_offset);
	if (tft_setting.pin_tft_mosi != -1) Serial.printf ("MOSI    = (GPIO %i)\n", tft_setting.pin_tft_mosi);
	if (tft_setting.pin_tft_miso != -1) Serial.printf ("MISO    = (GPIO %i)\n", tft_setting.pin_tft_miso);
	if (tft_setting.pin_tft_clk  != -1) Serial.printf ("SCK     = (GPIO %i)\n", tft_setting.pin_tft_clk);

#ifdef ESP8266
	if (tft_setting.overlap == true) {
		Serial.printf ("Overlap selected, following pins MUST be used:\n");
  
		Serial.printf ("MOSI     = SD1 (GPIO 8)\n");
		Serial.printf ("MISO     = SD0 (GPIO 7)\n");
		Serial.printf ("SCK      = CLK (GPIO 6)\n");
		Serial.printf ("TFT_CS   = D3  (GPIO 0)\n\n");

		Serial.printf ("TFT_DC and TFT_RST pins can be user defined\n");
	}
#endif
	if (tft_setting.pin_tft_cs   != -1) Serial.printf ("TFT_CS   = (GPIO %i)\n", tft_setting.pin_tft_cs);
	if (tft_setting.pin_tft_dc   != -1) Serial.printf ("TFT_DC   = (GPIO %i)\n", tft_setting.pin_tft_dc);
	if (tft_setting.pin_tft_rst  != -1) Serial.printf ("TFT_RST  = (GPIO %i)\n\n", tft_setting.pin_tft_rst);

	if (tft_setting.pin_tch_cs   != -1) Serial.printf ("TOUCH_CS = (GPIO %i)\n\n", tft_setting.pin_tch_cs);

	if (tft_setting.pin_tft_wr   != -1) Serial.printf ("TFT_WR   = (GPIO %i)\n", tft_setting.pin_tft_wr);
	if (tft_setting.pin_tft_rd   != -1) Serial.printf ("TFT_RD   = (GPIO %i)\n\n", tft_setting.pin_tft_rd);

	if (tft_setting.pin_tft_d0   != -1) Serial.printf ("TFT_D0   = (GPIO %i)\n", tft_setting.pin_tft_d0);
	if (tft_setting.pin_tft_d1   != -1) Serial.printf ("TFT_D1   = (GPIO %i)\n", tft_setting.pin_tft_d1);
	if (tft_setting.pin_tft_d2   != -1) Serial.printf ("TFT_D2   = (GPIO %i)\n", tft_setting.pin_tft_d2);
	if (tft_setting.pin_tft_d3   != -1) Serial.printf ("TFT_D3   = (GPIO %i)\n", tft_setting.pin_tft_d3);
	if (tft_setting.pin_tft_d4   != -1) Serial.printf ("TFT_D4   = (GPIO %i)\n", tft_setting.pin_tft_d4);
	if (tft_setting.pin_tft_d5   != -1) Serial.printf ("TFT_D5   = (GPIO %i)\n", tft_setting.pin_tft_d5);
	if (tft_setting.pin_tft_d6   != -1) Serial.printf ("TFT_D6   = (GPIO %i)\n", tft_setting.pin_tft_d6);
	if (tft_setting.pin_tft_d7   != -1) Serial.printf ("TFT_D7   = (GPIO %i)\n\n", tft_setting.pin_tft_d7);

	uint16_t fonts = tft.fontsLoaded ();
	if (fonts & (1 << 1))        Serial.printf ("Font GLCD   loaded\n");
	if (fonts & (1 << 2))        Serial.printf ("Font 2      loaded\n");
	if (fonts & (1 << 4))        Serial.printf ("Font 4      loaded\n");
	if (fonts & (1 << 6))        Serial.printf ("Font 6      loaded\n");
	if (fonts & (1 << 7))        Serial.printf ("Font 7      loaded\n");
	if (fonts & (1 << 9))        Serial.printf ("Font 8N     loaded\n"); else
	if (fonts & (1 << 8))        Serial.printf ("Font 8      loaded\n");
	if (fonts & (1 << 15))       Serial.printf ("Smooth font enabled\n");
	Serial.printf ("\n");

	if (tft_setting.serial == 1)          Serial.printf ("Display SPI frequency = %2.1f MHz \n", tft_setting.tft_spi_freq / 10.0);
	if (tft_setting.pin_tch_cs != -1)   Serial.printf ("Touch SPI frequency   = %2.1f MHz \n", tft_setting.tch_spi_freq / 10.0);

	tft.setRotation (3);
	tft.setTextFont (1);
	tft.fillScreen (TFT_WHITE);

	targetBrightness = 255;
	xTaskCreate (fadePWM, "fadeInPWM", 1000, (void*)&targetBrightness, 2, NULL);

	btnOK.setLongClickHandler ([](Button2 & b) {
		xTaskCreate (shutDown, "shutDown", 3000, NULL, 1, NULL);
	});
	
	btnSelect.setReleasedHandler ([](Button2 & b) {
		ADC.tare ();
		time (&startTime);
	});
	
	time (&startTime);
	
	wifiSys.begin ();
}

bool		oldDRDY;

void loop ()
{
	char			text[16];
	time_t			now;
	int32_t			value;
	static int32_t	shownValue;
	static int32_t	oldCode;
	float			temperature;
	
	if (ADC.avgIsValid) {
		if (ADC.significantChange || 
			(((int)(ADC.getAverage () * 100) != shownValue) && 
			(abs (ADC.getAverage () - oldCode) > 100))) {
			sprintf (txtWeight, "%4.2f", ADC.getWeight ());
			
			shownValue = ADC.getWeight () * 100;
			oldCode = ADC.getAverage ();
			sprintf (txtCode, "%8i", shownValue);
		} else if ((int)(ADC.getAverage () * 100) == shownValue) {
			oldCode = ADC.getAverage () * 100;
		}
		if (ADC.getTemperature (&temperature)) {			
			Serial.print ("\tTemperatuur: ");
			Serial.print (temperature);
			sprintf (txtSensorTemp, "%2.1f", temperature);
			ADC.invalidate (TEMPERATURE_CHANNEL);
		}
		if (ADC.getAdcValue (MUX_AINP_AINN_SHORTED, &value)) {
			Serial.print ("\r\nKortgesloten: ");
			Serial.print (value);
			sprintf (txtShortedCode, " %8i", value);
			ADC.invalidate (MUX_AINP_AINN_SHORTED);
		}
		tm	timeInfo;
		getLocalTime (&timeInfo, 0);
		strftime (txtTime, sizeof (txtTime), "%H:%M:%S", &timeInfo);
		
		UI::drawScreen (UI_MainScreen);
	}
	
	time (&now);
	if ((now - startTime) > 300) {
		xTaskCreate (shutDown, "shutDown", 3000, NULL, 1, NULL);		
	}
	
	delay (40);  // give the processor some piece
	btnOK.loop ();
	btnSelect.loop ();
}
