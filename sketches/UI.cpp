#include "UI.h"
#pragma GCC diagnostic ignored "-Wwritable-strings"

extern TFT_eSPI tft;

typedef struct uiTxtItem_s {
	uiTxtItem_s (int16_t x,
		int16_t y,
		char* text, 
		uint16_t foreColor,
		int32_t backColor = UI_TRANSPARENT,
		uint8_t datum = TL_DATUM,
		uint8_t font = 0,
		uint16_t totalWidth = 0)
		: type(UI_TEXT)
		, x(x)
		, y(y)
		, text(text)
		, foreColor(foreColor)
		, backColor(backColor == UI_TRANSPARENT ? foreColor : backColor)
		, datum(datum)
		, font(font)
		, totalWidth(totalWidth) {}
	uint8_t		type;
	int16_t		x;
	int16_t		y;
	char		*text;
	uint16_t	foreColor;
	uint16_t	backColor;
	uint8_t		datum;
	uint8_t		font;
	uint16_t	totalWidth;
} uiTxtItem_t;

typedef struct uiCircleItem_s {
	uiCircleItem_s (int16_t x,
		int16_t y,
		uint16_t r,
		uint16_t color,
		uint8_t mode = UI_NORMAL)
		: type(UI_CIRCLE)
		, x(x)
		, y(y)
		, r(r)
		, color(color)
		, mode(mode) {}
	uint8_t		type;
	int16_t		x;
	int16_t		y;
	uint16_t	r;
	uint16_t	color;
	uint8_t		mode;
} uiCircleItem_t;

char txtWeight[16];
char txtSensorTemp[8];
char txtMcuTemp[8];
char txtTime[10];
char txtCode[12];
char txtShortedCode[12];

uiTxtItem_s UI_MS_Weight (180, 100, txtWeight, TFT_BLACK, TFT_WHITE, BR_DATUM, 7, 180);
uiTxtItem_s UI_MS_WeightUnit (180, 100, "g", TFT_BLACK, TFT_WHITE, BL_DATUM, 4);
uiTxtItem_s UI_MS_WeightCode (112, 100, txtCode, TFT_RED, TFT_WHITE, TR_DATUM, 2);
uiTxtItem_s UI_MS_Temperature (180, 106, txtSensorTemp, TFT_BLUE, TFT_WHITE, TR_DATUM, 4, 60);
uiCircleItem_s UI_MS_SensTempCircle (182, 111, 2, TFT_BLUE, UI_NORMAL);
uiTxtItem_s UI_MS_SensTempUnit (186, 106, "C", TFT_BLUE, TFT_WHITE, TL_DATUM, 2);
uiTxtItem_s UI_MS_ShortedCode (52, 106, txtShortedCode, TFT_RED, TFT_WHITE, TR_DATUM, 1);
uiTxtItem_s UI_MS_Time (0, 0, txtTime, TFT_BLACK, TFT_WHITE, TL_DATUM, 2);

uiScreen_t UI_MainScreen = {
	"Gewicht", {
		&UI_MS_Weight,
		&UI_MS_WeightUnit,
		&UI_MS_WeightCode,
		&UI_MS_Temperature,
		&UI_MS_SensTempCircle,
		&UI_MS_SensTempUnit,
		&UI_MS_ShortedCode,
		&UI_MS_Time
	}, 8
};

namespace UI {
	void drawText (uiTxtItem_s *uiTxt) {
		tft.setTextPadding (uiTxt->totalWidth);
		tft.setTextDatum (uiTxt->datum);
		tft.setTextColor (uiTxt->foreColor, uiTxt->backColor);
		tft.drawString (uiTxt->text, uiTxt->x, uiTxt->y, uiTxt->font);
	}

	void drawCircle (uiCircleItem_s *uiCircle) {
		if (uiCircle->mode == UI_NORMAL) {
			tft.drawCircle (uiCircle->x, uiCircle->y, uiCircle->r, uiCircle->color);
		} else {
			tft.fillCircle (uiCircle->x, uiCircle->y, uiCircle->r, uiCircle->color);
		}
	}
	
	void drawElement (void* element) {
		uint8_t	*elementType = (uint8_t *)element;
		
		switch (*elementType) {
		case UI_TEXT:
			drawText ((uiTxtItem_s *)element);
			break;
		case UI_CIRCLE:
			drawCircle ((uiCircleItem_s *)element);
			break;
		}
	}
	
	void drawElements (void* elements[], uint8_t count) {
		for (uint8_t index = 0; index < count; index++) {
			UI::drawElement (elements[index]);
		}
	}
	
	void drawScreen (uiScreen_t screen) {
		drawElements (screen.elements, screen.elementCount);
	}
};