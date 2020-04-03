//#pragma once

#define UI_TEXT		1

class uiItem {
public:
	uint8_t		type;
	uint16_t	x;
	uint16_t	y;
	uint16_t	color;
};

class uiTextItem : uiItem {
	uint8_t		type = UI_TEXT;
	char		*text;
	uint16_t	backColor;
	uint8_t		datum;
	uint8_t		font;
	uint16_t	totalWidth;
	
};

struct uiTxtItem_s {
	uiTxtItem_s (int xx, int yy)
		: type(UI_TEXT)
		, x(xx)
		, y(yy) { }
	uint8_t			type;
	uint16_t		x;
	uint16_t		y;
	char			*text;
	uint16_t		foreColor;
	uint16_t		backColor;
	uint8_t			datum;
	uint8_t			font;
	uint16_t		totalWidth;
};
/*
const void* MainScreen[] = { 
	uiTxtItem_s (3, 5)
};
*/