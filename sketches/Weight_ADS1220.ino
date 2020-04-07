#define BOARD_ADS1220
#include <SPI.h>
#include <SPIFFS.h>
//#include <TFT_eSPI.h>
#include <HardwareSerial.h>
#include <Button2.h>
#include "time.h"
#include "ADC.h"
#include "UI.h"

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

	//ADC.begin ();//CS_PIN, DRDY_PIN);
	xTaskCreate (initADC, "initADC", 3000, NULL, 5, NULL);

	ledcSetup (0, 4000, 10);
	ledcWrite (0, 0);
	ledcAttachPin (TFT_BL, 0);	
	
	tft.init ();
	tft.setRotation (1);
	tft.setCursor (0, 0);
	tft.fillScreen (TFT_BLACK);
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
}

bool		oldDRDY;

void loop ()
{
	char	text[16];
	time_t	now;
	int32_t	value;
	static int32_t	shownValue;
	float	temperature;
	
	if (ADC.avgIsValid) {
		if (ADC.significantChange || (abs (ADC.getAverage () - shownValue) > (CHANGE_THRESHOLD * 1.5))) {
			sprintf (txtWeight, "%4.2f", ADC.getWeight ());
			
			shownValue = ADC.getAverage ();
			sprintf (txtCode, "%8i", shownValue);
		}
		UI::drawScreen (UI_MainScreen);
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
	}
	
	time (&now);
	if ((now - startTime) > 300) {
		xTaskCreate (shutDown, "shutDown", 3000, NULL, 1, NULL);		
	}
	
	delay (40);  // give the processor some piece
	btnOK.loop ();
	btnSelect.loop ();
}
