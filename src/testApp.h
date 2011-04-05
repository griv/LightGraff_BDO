#ifndef _TEST_APP
#define _TEST_APP

#include <fstream>


#include "ofMain.h"

#include "ofxVectorGraphics.h"
#include "ofxOsc.h"
#include "ofxTuio.h"

#define CONTROL_PORT 4444
#define NUM_MSG_STRINGS 20

#define ONE_OVER_255 0.00392157


//--------------------------------------------------------
class testApp : public ofSimpleApp{
	
public:
	
	void setup();
	void update();
	void draw();
	void initialise();
	
	void keyPressed(int key);
	void keyReleased(int key);
	
	void tuioAdded(ofxTuioCursor & tuioCursor);
	void tuioRemoved(ofxTuioCursor & tuioCursor);
	void tuioUpdated(ofxTuioCursor & tuioCursor);
	
	void addPoint(ofxTuioCursor & tuioCursor);
	void drawChisel();
	void drawRound();
	void drawBitmapBrush(float _x, float _y, int _b, bool newStroke);
	void saveScreen();
	void clearScreen();
	void updateDrips();
	void addDrip(int _x, int _y);
	void saveData();
	void ofAppendToFile(string filename, string str);
	
	string getTimestampString();
	
	int 				numDots;
	int					minBlobDistance;
	int					maxBlobDistance;
	int					lineThickness;
	int					minLineThickness;
	int					maxLineThickness;
	bool				isDrawing;
	bool				fullscreen;
	
	int					screenWidth;
	int					screenHeight;
	
	bool				iToggle;
	bool				vToggle;
	bool				bToggle;
	int					displayImage;
	int					displayVideo;
	int					displayBrush;
	
	int					drawColour;

	bool				mouseToggle;
	bool				turbo;
	
	bool				newStroke;
	
	ofImage				saveImage;
	ofVideoPlayer		videoPlayer;
	
//	ofxVectorGraphics	output;
	
	ofImage				IMG;
	ofImage				TMP;
	ofTexture			Canvas;

	unsigned char * pixels;
	
	int oldX, oldY, numBrushes, imageNumBytes, numSteps, stride;
	
	int		red, green, blue;
	int		hue, saturation, brightness;
	
	int		colourOscillate;
	int		colourOscillateAmount;

	bool	dripsOn;
	bool	showFPS;
	
	int		dripAmount;
	
	
private:
	
	myTuioClient tuio;
	
	ofxOscReceiver	receiver;
	
	int				current_msg_string;
	string			msg_strings[NUM_MSG_STRINGS];
	float			timers[NUM_MSG_STRINGS];
	
	int				mouseX, mouseY;
	string			mouseButtonState;
	
	//ofxTuioEventManager test;
};

#endif
