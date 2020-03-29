#define BOARD_ADS1220
#include <Protocentral_ADS1220.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <TFT_eSPI.h>
#include <HardwareSerial.h>
#define	LED_BUILTIN	4
#define PGA			1
#define VREF         2.048            // Internal reference of 2.048V
#define VFSR         VREF/PGA
#define FULL_SCALE   (((long int)1<<23)-1)
#define BUZZER		26

Protocentral_ADS1220	ads1220;

SPIClass adsSPI = SPIClass (HSPI);

void setup()
{
	pinMode(LED_BUILTIN, OUTPUT);
	
	Serial.begin (115200);

	ads1220.begin (15, 22);
	ads1220.set_data_rate (DR_20SPS);
	//ads1220.set_operating_mode (OM_DUTY_CYCLE);
	ads1220.set_pga_gain (PGA_GAIN_128);
	ads1220.set_conv_mode_single_shot ();
	//ads1220.set_conv_mode_continuous ();
	ads1220.writeRegister (2, 7 + 8 + 32 + 128);	// IDAC current at 1500uA, Low-side power switch on, 50Hz filter on, VREF using AIN0/AIN3
	ads1220.writeRegister (3, 32 + 4);	// Connect IDAC1+IDAC2 to AIN0
	ads1220.select_mux_channels (MUX_AIN1_AIN2);

	Serial.println ("Config_Reg : ");
	Serial.println (ads1220.readRegister (CONFIG_REG0_ADDRESS), HEX);
	Serial.println (ads1220.readRegister (CONFIG_REG1_ADDRESS), HEX);
	Serial.println (ads1220.readRegister (CONFIG_REG2_ADDRESS), HEX);
	Serial.println (ads1220.readRegister (CONFIG_REG3_ADDRESS), HEX);
	Serial.println (" ");

	digitalWrite (BUZZER, 1);
	for (int i = 0; i < 3000; i++) {
		pinMode (BUZZER, OUTPUT);
		delayMicroseconds (30);
		pinMode (BUZZER, INPUT);
		delayMicroseconds (210);
	}
}

#define ZEROVAL		150490
#define TESTVAL		253140
#define TESTWEIGHT	30.71
#define REFTEMP_T20	20.16

float convertToWeight (int32_t i32data) {
	return (float)(i32data - ZEROVAL) / (float)(TESTVAL - ZEROVAL) * (float)TESTWEIGHT;
}
float convertToMilliV (int32_t i32data)
{
	return (float)((i32data*VFSR * 1000) / FULL_SCALE);
}

#define BUFFER_SIZE		8
int32_t		buffer[BUFFER_SIZE];
byte		bufferpos = 0;

void loop()
{
	int		adc_data;

	ads1220.Start_Conv ();
	buffer[bufferpos] = ads1220.Read_WaitForData ();
	adc_data = 0;
	for (uint8_t count = 0; count < BUFFER_SIZE; count++) {
		adc_data += buffer[count];
	}
	adc_data /= BUFFER_SIZE;
	
	Serial.print ("\r\nAIN1-AIN2: ");
	Serial.print (adc_data);
	Serial.print ("\tGewicht: ");
	Serial.print (convertToWeight(adc_data));

	if (bufferpos == 0) {
		ads1220.set_temp_sens_mode (true);
		ads1220.Start_Conv ();
		adc_data = ads1220.Read_WaitForData ();
		ads1220.set_temp_sens_mode (false);

		Serial.print ("\tTemperatuur: ");
		Serial.print ((adc_data >> 10) * 0.03125);
	}

	bufferpos = (bufferpos + 1) % BUFFER_SIZE;
	//delay (100);	
}
