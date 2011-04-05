#ifndef _DRIP
#define _DRIP


#include "ofMain.h"

#define ONE_OVER_255 0.00392157


//--------------------------------------------------------
class Drips {
	
public:
	
	void setup();
	void update();
	void draw();
	unsigned char * getPixelData();
	void drawDrip(int _d);
	void clearScreen();
	void addDrip(int _x, int _y);
	
	unsigned char * pixels;
	
	ofImage				dripBrush;
	ofImage				IMG;
	ofImage				TMP;
	
	int					steps;
	int		red;
	int		green;
	int		blue;
	int		imageNumBytes;
	int		stride;
	
};

#endif
