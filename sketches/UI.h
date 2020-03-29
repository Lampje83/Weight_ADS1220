#include <Arduino.h>
#define ESP32
#include <TFT_eSPI.h>
// UI element types
#define UI_TEXT		1
#define UI_LINE		2
#define UI_CIRCLE	3
#define UI_BOX		4

#define UI_TRANSPARENT	-1

#define UI_NORMAL		0
#define UI_SOLID		1

typedef struct uiTxtItem_s		uiTxtItem_t;
typedef struct uiCircleItem_s	uiCircleItem_t;

typedef struct {
	char	name[16];
	void*	action;
} uiAction_t;

typedef struct {
	char	name[32];
	
	void		*elements[32];
	uint8_t		elementCount;
	
	uiAction_t	actions[32];
	uint8_t		actionCount;
	
	void		*enter;
	void		*exit;
	void		*loop;
} uiScreen_t;

extern char		txtWeight[];
extern char		txtSensorTemp[];
extern char		txtMcuTemp[];
extern char		txtTime[10];
extern char		txtCode[];
extern char		txtShortedCode[];

extern uiTxtItem_s	UI_MS_Weight;
extern uiTxtItem_s	UI_MS_WeightUnit;

extern uiScreen_t	UI_MainScreen;

namespace UI {
	void drawElement (void* element);
	void drawScreen (uiScreen_t screen);
}
