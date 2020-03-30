#define BOARD_ADS1220
#include <Protocentral_ADS1220.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <TFT_eSPI.h>
#include <HardwareSerial.h>
#include <Button2.h>
#include "time.h"

#define	LED_BUILTIN	4
#define PGA			1
#define VREF         2.048            // Internal reference of 2.048V
#define VFSR         VREF/PGA
#define FULL_SCALE   (((long int)1<<23)-1)
#define BUZZER		26
#define BUTTON_1	35
#define BUTTON_2	0

Protocentral_ADS1220	ads1220;

//SPIClass adsSPI = SPIClass (HSPI);
TFT_eSPI tft	= TFT_eSPI (TFT_WIDTH, TFT_HEIGHT);

Button2		btnOK (BUTTON_1);
bool		fadeComplete = false;

time_t		startTime;

void fadePWM (void *param) {
	bool dir = (bool*)param;

	fadeComplete = false;

	float brightness = ledcRead (0) + 1;
	Serial.print ("Brightness start: ");
	Serial.println (brightness);
	Serial.print ("Richting: ");
	Serial.print (dir);
	Serial.print (" ");
	Serial.println (dir ? 1024 : 0);
	while (dir ? (brightness < 1024) : (brightness > 1)) {
		brightness *= dir ? 1.2 : (1.0f / 1.2f);
		Serial.print ("Brightness naar ");
		Serial.println (brightness);
		ledcWrite (0, max (min (1023.0f, brightness), 0.0f));
		vTaskDelay (10);
	}
	
	fadeComplete = true;	
	vTaskDelete (NULL);
}

void shutDown (void *param) {
	static bool dir2 = false;
	fadeComplete = false;
	xTaskCreate (fadePWM, "fadeOutPWM", 1000, (void*)false, 2, NULL);
	ads1220.SPI_Command (2); 	// Power Down
		
	vTaskDelay (20);
	esp_sleep_enable_ext1_wakeup (GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);
	while (fadeComplete == false) {
		vTaskDelay (10);
	}
		
	tft.writecommand (TFT_DISPOFF);
	tft.writecommand (TFT_SLPIN);
	esp_deep_sleep_start ();
	vTaskDelete (NULL);
}

void setup()
{
	pinMode(LED_BUILTIN, OUTPUT);

	digitalWrite (TFT_BL, 0);
	pinMode (TFT_BL, OUTPUT);

	Serial.begin (115200);

	ledcSetup (0, 4000, 10);
	ledcWrite (0, 0);
	ledcAttachPin (TFT_BL, 0);	

	ads1220.begin (15, 22);
	ads1220.set_data_rate (DR_20SPS);
	//ads1220.set_operating_mode (OM_DUTY_CYCLE);
	ads1220.set_pga_gain (PGA_GAIN_128);
	//ads1220.set_conv_mode_single_shot ();
	ads1220.set_conv_mode_continuous ();
	ads1220.writeRegister (2, 7 + 8 + 32 + 128);	// IDAC current at 1500uA, Low-side power switch on, 50Hz filter on, VREF using AIN0/AIN3
	//ads1220.writeRegister (3, 32 + 4);	// Connect IDAC1+IDAC2 to AIN0
	ads1220.select_mux_channels (MUX_AIN1_AIN2);

	Serial.println ("Config_Reg : ");
	Serial.println (ads1220.readRegister (CONFIG_REG0_ADDRESS), HEX);
	Serial.println (ads1220.readRegister (CONFIG_REG1_ADDRESS), HEX);
	Serial.println (ads1220.readRegister (CONFIG_REG2_ADDRESS), HEX);
	Serial.println (ads1220.readRegister (CONFIG_REG3_ADDRESS), HEX);
	Serial.println (" ");
	
/*
	digitalWrite (BUZZER, 1);
	for (int i = 0; i < 3000; i++) {
		pinMode (BUZZER, OUTPUT);
		delayMicroseconds (30);
		pinMode (BUZZER, INPUT);
		delayMicroseconds (210);
	}
*/
	tft.init ();
	tft.setRotation (1);
	tft.setCursor (0, 0);
	tft.fillScreen (TFT_BLACK);
	tft.setTextFont (1);
	tft.fillScreen (TFT_WHITE);

	xTaskCreate (fadePWM, "fadeInPWM", 1000, (void*)true, 2, NULL);

	/*
	pinMode (TFT_BL, OUTPUT);
	digitalWrite (TFT_BL, TFT_BACKLIGHT_ON);
	*/
	btnOK.setReleasedHandler ([](Button2 & b) {
		xTaskCreate (shutDown, "shutDown", 3000, NULL, 1, NULL);
	});
	
	time(&startTime);
}

#define ZEROVAL		150490
#define TESTVAL		253140
#define TESTWEIGHT	30.71
#define REFTEMP_T20	20.16

int32_t zerooffset = 0;
float convertToWeight (int32_t i32data) {
	return (float)(i32data + zerooffset - ZEROVAL) / (float)(TESTVAL - ZEROVAL) * (float)TESTWEIGHT;
}
float convertToMilliV (int32_t i32data)
{
	return (float)((i32data*VFSR * 1000) / FULL_SCALE);
}

#define BUFFER_SIZE		8
int32_t		buffer[BUFFER_SIZE];
byte		bufferpos = 0;
bool		valid = false;

void loop()
{
	int		adc_data, adc_max, adc_min;
	char	text[16];
	time_t	now;
	
	//ads1220.Start_Conv ();
	buffer[bufferpos] = ads1220.Read_WaitForData ();
	adc_data = 0;
	adc_max = -8388608;
	adc_min = 8388607;
	for (uint8_t count = 0; count < BUFFER_SIZE; count++) {
		adc_data += buffer[count];
		adc_max = max (adc_max, buffer[count]);
		adc_min = min (adc_min, buffer[count]);
	}
	adc_data -= adc_max + adc_min;
	adc_data /= BUFFER_SIZE - 2;
	
	if ((bufferpos + 1) == BUFFER_SIZE && valid == false) {
		// weegschaal op nul stellen bij start
		zerooffset = ZEROVAL - adc_data;
	}
	
	if (valid) {
		Serial.print ("\r\nAIN1-AIN2: ");
		Serial.print (adc_data);
		Serial.print ("\tGewicht: ");
		Serial.print (convertToWeight (adc_data));

		tft.setTextColor (TFT_BLACK, TFT_WHITE);
		tft.setTextDatum (BR_DATUM);
		sprintf (text, "%4.2f", convertToWeight (adc_data));
		tft.fillRect (0, 100 - tft.fontHeight (7), 180 - tft.textWidth(text, 7), tft.fontHeight (7), TFT_WHITE);
		tft.drawString (text, 180, 100, 7);
		tft.setTextDatum (BL_DATUM);
		tft.drawString ("g", 180, 100, 4);
		tft.setTextDatum (TR_DATUM);
		tft.setTextColor (TFT_RED, TFT_WHITE);
		sprintf (text, "  %8i", adc_data);
		tft.drawString (text, 100, 100, 2);
		if (bufferpos == 0) {
			ads1220.set_conv_mode_single_shot ();
			ads1220.set_data_rate (DR_90SPS);
			ads1220.set_operating_mode (OM_TURBO);
			
			// Temperatuur uitlezen
			ads1220.set_temp_sens_mode (true);
			ads1220.Start_Conv ();
			adc_data = ads1220.Read_WaitForData ();
			ads1220.set_temp_sens_mode (false);

			Serial.print ("\tTemperatuur: ");
			Serial.print ((adc_data >> 10) * 0.03125);
			tft.setTextColor (TFT_BLUE, TFT_WHITE);
			tft.setTextDatum (TR_DATUM);
			sprintf (text, "   %2.1f", ((adc_data >> 10) * 0.03125));
			tft.drawString (text, 180, 106, 4);
			tft.setTextDatum (TL_DATUM);
			tft.drawCircle (182, 111, 2, TFT_BLUE);
			tft.drawString ("C", 186, 106, 2);
			
			// Referentiespanning uitlezen			
			ads1220.select_mux_channels (MUX_VREFP_VREFN_DIV_4);
			
			ads1220.Start_Conv ();
			adc_data = ads1220.Read_WaitForData ();
			Serial.print ("\r\nReferentie: ");
			Serial.print (adc_data);
			sprintf (text, " %2.3fV", convertToMilliV (adc_data) / 250.0f);
			tft.setTextDatum (TR_DATUM);
			tft.setTextColor (TFT_GREEN, TFT_WHITE);
			tft.drawString (text, 112, 114, 2);

			ads1220.select_mux_channels (MUX_AVDD_AVSS_DIV_4);
			
			ads1220.Start_Conv ();
			adc_data = ads1220.Read_WaitForData ();
			Serial.print ("\r\nReferentie: ");
			Serial.print (adc_data);
			sprintf (text, " %2.3fV", convertToMilliV (adc_data) / 250.0f);
			tft.setTextDatum (TR_DATUM);
			tft.setTextColor (TFT_GREEN, TFT_WHITE);
			tft.drawString (text, 52, 114, 2);

			ads1220.set_data_rate (DR_20SPS);
			ads1220.set_operating_mode (OM_NORMAL);
			ads1220.select_mux_channels (MUX_AIN1_AIN2);
			ads1220.set_conv_mode_continuous ();
			ads1220.Start_Conv ();

			tm	timeInfo;
			getLocalTime (&timeInfo, 0);
			strftime (text, sizeof (text), "%H:%M:%S", &timeInfo);
			tft.setTextDatum (TL_DATUM);
			tft.setTextColor (TFT_BLACK, TFT_WHITE);
			tft.drawString (text, 0, 0, 2);
		}
	}
	bufferpos = (bufferpos + 1) % BUFFER_SIZE;
	if (bufferpos == 0) {
		valid = true;
	}
	
	time (&now);
	if ((now - startTime) > 300) {
		xTaskCreate (shutDown, "shutDown", 3000, NULL, 1, NULL);		
	}
	btnOK.loop ();
	//delay (100);	
}
