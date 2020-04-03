/*
 * Class for usage differential ADC (ADS1220) and weigh scale functions
 * 
 * By Erik van Beilen
 */

#include "ADC.h"

/* Variables */
Protocentral_ADS1220	ads1220;
ADCClass				ADC;

int32_t					zerooffset = 0;

#define DRDY_PIN 22
static void IRAM_ATTR dataISR () {
	//detachInterrupt (DRDY_PIN);
	Serial.print ("(DRDY)");
	ADC.handleDRDY ();
	delay (1);
	
	//attachInterrupt (DRDY_PIN, dataISR, FALLING);
}

ADCClass::ADCClass () {

}

void ADCClass::calcAverage () {
	int32_t	adc_data,
			adc_min,
			adc_max;
	uint8_t count;
	adc_data = 0;
	adc_max = -8388608;
	adc_min = 8388607;
	
	for (count = 0; count < ADC_BUFFERSIZE; count++) {
		adc_data += weightBuffer[count];
		adc_max = max (adc_max, weightBuffer[count]);
		adc_min = min (adc_min, weightBuffer[count]);
	}
	// Remove highest and lowest reading from accumulation buffer
	adc_data -= adc_max + adc_min;
	
	// Store the result
	int32_t oldPrev = prevAverage;
	prevAverage = weightAverage;
	weightAverage = (adc_data - adcValue[MUX_AINP_AINN_SHORTED >> 4]) / (ADC_BUFFERSIZE - 2);
	avgIsDirty = false;
	Serial.printf ("\tChange: %i %i ", abs (weightAverage - prevAverage), abs (weightAverage - oldPrev));
	if ((abs (weightAverage - prevAverage) > CHANGE_THRESHOLD) &&
		(abs (weightAverage - oldPrev) > (CHANGE_THRESHOLD * 2))) {
			significantChange = true;
	} 
}

void ADCClass::begin (uint8_t cs_pin, uint8_t drdy_pin) {
	this->drdy_pin = drdy_pin;
	ads1220.begin (cs_pin, drdy_pin);
	ads1220.set_data_rate (DR_20SPS);
	ads1220.set_pga_gain (PGA_GAIN_128);
	ads1220.writeRegister (2, 7 + 8 + 16 + 128); 	// IDAC current at 1500uA, Low-side power switch on, 50Hz+60Hz filter on, VREF using AIN0/AIN3
	//ads1220.writeRegister (3, 32 + 4);	// Connect IDAC1+IDAC2 to AIN0
	activeChannel = MUX_AINP_AINN_SHORTED;
	ads1220.select_mux_channels (activeChannel);
	ads1220.set_conv_mode_continuous ();
	ads1220.Start_Conv ();	

	Serial.println ("Config_Reg : ");
	Serial.println (ads1220.readRegister (CONFIG_REG0_ADDRESS), HEX);
	Serial.println (ads1220.readRegister (CONFIG_REG1_ADDRESS), HEX);
	Serial.println (ads1220.readRegister (CONFIG_REG2_ADDRESS), HEX);
	Serial.println (ads1220.readRegister (CONFIG_REG3_ADDRESS), HEX);
	Serial.println (" ");
	
	waitForDRDY ();
	getAdcValue (MUX_AINP_AINN_SHORTED);
	delay (100);
	
	startConversion (MUX_AIN1_AIN2, true);
	//delay (500);
	while (!avgIsValid) delay (10);
	ADC.tare ();
}

void ADCClass::loop (void) {
	static uint8_t	cycleCount = 0;
	static bool		oldDRDY;
	
	writeBuffer (ads1220.Read_WaitForData ());

	if (cycleCount == 0) {
		Serial.println ("Overige waardes inlezen");
		ads1220.set_data_rate (DR_90SPS);
		ads1220.set_operating_mode (OM_TURBO);
			
		// Temperatuur uitlezen
		startConversion (TEMPERATURE_CHANNEL, false);
		waitForDRDY ();
		
		// Offset ADC uitlezen
		startConversion (MUX_AINP_AINN_SHORTED, false);
		waitForDRDY ();
		
		// Normale omzetting hervatten
		ads1220.set_data_rate (DR_20SPS);
		ads1220.set_operating_mode (OM_NORMAL);
		startConversion (MUX_AIN1_AIN2, true);
	}
	cycleCount = (cycleCount + 1) & 15;
	if (digitalRead (DRDY_PIN) != oldDRDY) {
		oldDRDY = !oldDRDY;
		if (oldDRDY == false) {
			handleDRDY ();
		}
	}
	
	delay (10);	// give the processor some piece
}

// read data into selected buffer
void ADCClass::handleDRDY () {
	int32_t	*value = &adcValue[activeChannel >> 4];
	
	// store read value
	*value = ads1220.Read_Data ();
	adcModified[activeChannel >> 4] = true;
	if (activeChannel == bufferedChannel) {
		writeBuffer (*value);
	}
	newData = true;
}

void ADCClass::waitForDRDY () {
	detachInterrupt (drdy_pin);
	while (digitalRead (drdy_pin) == true) yield ();
	handleDRDY ();
}

void ADCClass::invalidate (uint8_t channel) {
	adcModified[channel >> 4] = false;
}

void ADCClass::writeBuffer (int32_t value) {
	weightBuffer[bufferPos] = value;
	avgIsDirty = true;
	bufferPos++;
	if (bufferPos >= ADC_BUFFERSIZE) {
		bufferPos = 0;
		avgIsValid = true;
	}
}

int32_t ADCClass::getAverage () {
	if (avgIsDirty) {
		significantChange = false;
		// Buffer was updated, recalculate
		calcAverage ();
	}
	return weightAverage;
}

bool ADCClass::getAdcValue (uint8_t channel, int32_t *value) {
	*value = adcValue[channel >> 4];
	return adcModified[channel >> 4];
}
int32_t ADCClass::getAdcValue (uint8_t channel) {
	if (!adcModified[channel >> 4]) {
		startConversion (channel, continuousMode);
		waitForDRDY ();
	}
	return adcValue[channel >> 4];
}

float ADCClass::getWeight () {
	return convertToWeight (getAverage ());
}

float ADCClass::getTemperature () {
	return (adcValue[TEMPERATURE_CHANNEL >> 4] >> 10) * 0.03125;
}

bool ADCClass::getTemperature (float *temp) {
	*temp = getTemperature ();
	return adcModified[TEMPERATURE_CHANNEL >> 4];	
}

void ADCClass::tare () {
	int32_t accum = 0;
	
	for (uint8_t count = 0; count < 4; count++) {
		delay (100);
		if (count != 0) accum += getAverage ();	// eerste resultaat weggooien
	}		
	zerooffset = ZEROVAL - (accum / 3.0f);
}

void ADCClass::powerDown () {
	detachInterrupt (drdy_pin);
	ads1220.SPI_Command (ADS1220_POWERDOWN);
}

void ADCClass::startConversion (uint8_t channel = 255, bool continuous = false) {
	detachInterrupt (drdy_pin);
	if (channel != 255 && channel != activeChannel) {
		if (channel != TEMPERATURE_CHANNEL) {
			ads1220.select_mux_channels (channel);
		}
		if (activeChannel == TEMPERATURE_CHANNEL || channel == TEMPERATURE_CHANNEL) {
			// switch temperature mode on or off accordingly
			ads1220.set_temp_sens_mode (channel == TEMPERATURE_CHANNEL);
		}
		// store selected channel
		activeChannel = channel;
	}
	if (continuous != continuousMode) {
		continuous ? ads1220.set_conv_mode_continuous () :
					 ads1220.set_conv_mode_single_shot ();
		continuousMode = continuous;
	}
	ads1220.Start_Conv ();
	attachInterrupt (drdy_pin, dataISR, FALLING);
}

float ADCClass::convertToWeight (int32_t i32data) {
	return (float)(i32data + zerooffset - ZEROVAL) / 
		(float)(TESTVAL - ZEROVAL) * (float)TESTWEIGHT;
}

float ADCClass::convertToMilliV (int32_t i32data)
{
	return (float)((i32data*VFSR * 1000) / FULL_SCALE);
}
