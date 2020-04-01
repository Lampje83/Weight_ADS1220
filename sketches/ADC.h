//#pragma once

/* Includes */
#define BOARD_ADS1220
#include <Protocentral_ADS1220.h>

/* Defines */
#define PGA			1
#define VREF		2.048            // Internal reference of 2.048V
#define VFSR		VREF / PGA
#define FULL_SCALE	(((long int)1<<23)-1)

#define ADC_BUFFERSIZE	8

// reference value
#define ZEROVAL		150490
#define TESTWEIGHT	30.71
#define TESTVAL		253140
#define REFTEMP_T20	20.16

#define ZERO_RANGE	500
#define NOISE_RANGE	400

// ADS1220 commands
#define ADS1220_POWERDOWN		2
#define TEMPERATURE_CHANNEL		0xF0	// actually reserved channel of mux,
										// but we use this index to indicate temperature mode
/* Functions */

class ADCClass {
private:
	int32_t	weightBuffer[ADC_BUFFERSIZE];
	uint8_t bufferPos = 0;
	int32_t	weightAverage;
	int32_t prevAverage;
	bool	avgIsDirty = true;		// goes to FALSE when calcAverage has run, 
									// TRUE when weightBuffer is updated
	// HACK: using buffer for reserved channel 15 to store temperature
	int32_t adcValue[16]; 			// reference values and temperature
	bool	adcValid[16];			// false if not read yet
	uint8_t bufferedChannel = MUX_AIN1_AIN2;		// readings from this channel are stored in buffer
	uint8_t	activeChannel;  		// actual mux channel being sampled
	bool	continuousMode;
	
	uint8_t	drdy_pin;
	
	void	calcAverage ();
public:
	bool	avgIsValid = false; 	// buffer completely filled

	ADCClass ();
	void	begin (uint8_t cs_pin, uint8_t drdy_pin);
	
	void	loop ();

	void	handleDRDY ();
	void	waitForDRDY ();
	
	void	invalidate (uint8_t channel);
	void	writeBuffer (int32_t buffer);
	void	writeAdcValue (int32_t value, uint8_t channel);
	int32_t	getAverage ();
	bool	getAdcValue (uint8_t channel, int32_t *value);
	int32_t getAdcValue (uint8_t channel);
	float	getWeight ();
	float	getTemperature ();
	
	void	tare ();
	void	powerDown ();
	void	startConversion (uint8_t channel, bool continuous);
	float	convertToWeight (int32_t i32data);
	float	convertToMilliV (int32_t i32data);
};

/* Exported variables */
extern ADCClass				ADC;

// TODO: Make these internal
extern Protocentral_ADS1220	ads1220;
extern int32_t zerooffset;
