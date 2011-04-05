#include "testApp.h"

#define MAX_POINTS	10000
#define MAX_STROKES	1000
#define MAX_DRIPS	1000
#define MAX_BLOBS	4
#define NUM_IMAGES	9
#define NUM_VIDEOS	9
#define NUM_BRUSHES	7
#define PROJECTION_WIDTH 1024
#define PROJECTION_HEIGHT 768
#define BITMAP_WIDTH 640
#define BITMAP_HEIGHT 480

char	reportStr[1024];
string	msgStr;


struct Stroke {
	float		points[MAX_POINTS][6];
	int			numPoints;
	//	int			0x333333;
	int			oldX;
	int			oldY;
};

Stroke strokes[MAX_STROKES];
int numStrokes;

Stroke blobs[MAX_BLOBS];
int numBlobs;

struct Drip {
	int			_x;
	int			_y;
	int			_speed;
	int			_thickness;
	int			_length;
	int			oldX;
	int			oldY;
	bool		_dripping;
	int			_r;
	int			_g;
	int			_b;
};

Drip drips[MAX_DRIPS];
int numDrips;


ofImage images[NUM_IMAGES];
ofVideoPlayer videos[NUM_VIDEOS];
ofImage brushes[NUM_BRUSHES];


//--------------------------------------------------------------
// Setup
void testApp::setup(){
	
	// TUIO Listeners
	ofAddListener(tuio.cursorAdded,this,&testApp::tuioAdded);
	ofAddListener(tuio.cursorRemoved,this,&testApp::tuioRemoved);
	ofAddListener(tuio.cursorUpdated,this,&testApp::tuioUpdated);
	
	// load images
	for (int i=0; i<NUM_IMAGES; i++) {
		images[i].loadImage("images/image"+ofToString(i+1)+".jpg");
	}	
	// load brushes
	for (int i=0; i<NUM_BRUSHES; i++) {
		string brushFileName;
		switch (i) {
			case 0:
				brushFileName = "brushes/001-soft.png";
				break;
			case 1:
				brushFileName = "brushes/001-soft.png";
				break;
			case 2:
				brushFileName = "brushes/003-spotty.png";
				break;
			case 3:
				brushFileName = "brushes/011-air-insane.png";
				break;
			case 4:
				brushFileName = "brushes/002-slash.png";
				break;
			case 5:
				brushFileName = "brushes/009-calligLeft.png";
				break;
			case 6:
				brushFileName = "brushes/006-pent.png";
				break;
			case 7:
				brushFileName = "brushes/010-paint.png";
				break;
		}
		brushes[i].loadImage(brushFileName);
	}	
	
	drawColour = 0xffffff; // for vector brushes -- OLD!!
	
	// start TUIO
	tuio.start(3333);
	
	// start osc receiver for control messages
	receiver.setup( CONTROL_PORT );
	
	
	// some overall config
	minLineThickness	= 4;		// min line thickness
	maxLineThickness	= 64;		// max line thickness

	minBlobDistance		= 1;		// minimum distance between dots
	maxBlobDistance		= 60;		// maximum distance between dots - stops it freaking out a little
	
	imageNumBytes		= 4;		// 4 bytes for rgba, 3 bytes for rgb

	colourOscillateAmount = 100;		// oscillate colour
	
	dripsOn = false;
	showFPS = false;
	
	
	// initialise app
	initialise();
	
}

//--------------------------------------------------------------
// Initialise
void testApp::initialise() {
	
//	ofSetWindowTitle("LightGraff | Graffiti Research Lab Perth");
	ofSetFrameRate(60);
	ofSetVerticalSync(true);
//	ofSetFullscreen(true);
	
	numStrokes = 0;
	isDrawing = true;
	lineThickness = 16;
	
	iToggle = false;
	vToggle = false;//
	bToggle = false;
	
	displayImage = 0;
	displayVideo = 0;
	displayBrush = 2; // default is soft circle
	
	ofHideCursor();
	mouseToggle = false;
	newStroke = true;
	turbo = false;

	ofSetPolyMode(OF_POLY_WINDING_POSITIVE);
	ofNoFill();
	ofEnableSmoothing();
	
	
	// our previous points
	oldX			= 0;
	oldY			= 0;
	
	// other vars
	numBrushes  	= 0;
	numSteps		= 40;
	
	// default to white
	red				= 255;
	green			= 255;
	blue			= 255;	
	
	pixels = new unsigned char[BITMAP_WIDTH * BITMAP_HEIGHT * imageNumBytes];

	Canvas.allocate(BITMAP_WIDTH, BITMAP_HEIGHT, GL_RGBA);
	Canvas.loadData(pixels, BITMAP_WIDTH, BITMAP_HEIGHT, GL_RGBA);

    
//	clearScreen();


	
	
}

//--------------------------------------------------------------
void testApp::update(){
	
	// get tuio message
	tuio.getMessage();
	
	
	
	// get control messages
	while( receiver.hasWaitingMessages() )
	{
		// get the next message
		ofxOscMessage m;
		receiver.getNextMessage( &m );
		
		// unrecognized message: display on the bottom of the screen
		string msg_string;
		msg_string = m.getAddress();
		msg_string += "-- ";
		for ( int i=0; i<m.getNumArgs(); i++ )
		{
			// get the argument type
			msg_string += m.getArgTypeName( i );
			msg_string += ":";
			
			if (m.getAddress() == "/brushmode") {
				// brush mode
				cout << "brushmode : " << m.getArgAsInt32( i ) << endl;
				displayBrush = m.getArgAsInt32( i );
				switch(displayBrush) {
					case 0:
					case 1:
						dripsOn = false;
						break;
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
						dripsOn = true;
						break;
				}
			} else if (m.getAddress() == "/clear") {
				cout << "clear screen" << endl;
				saveScreen();
				clearScreen();
			} else if (m.getAddress() == "/colour/red") {
				red = m.getArgAsInt32( i );
				cout << "red : " << red << endl;
			} else if (m.getAddress() == "/colour/green") {
				green = m.getArgAsInt32( i );
				cout << "green : " << green << endl;
			} else if (m.getAddress() == "/colour/blue") {
				blue = m.getArgAsInt32( i );
				cout << "blue : " << blue << endl;
			} else if (m.getAddress() == "/thickness") {
				float thicky = (float)m.getArgAsInt32( i ) / 100;
				lineThickness = (int)((maxLineThickness - minLineThickness) * thicky) + minLineThickness;
				cout << " line thickness : " << lineThickness << " " << thicky << endl;
			} else if (m.getAddress() == "/2/push20") {
				displayImage = 0;
				displayVideo = 0;
				videoPlayer.stop();
			} else if (m.getAddress() == "/2/push21") {
				displayImage = 1;
				displayVideo = 0;
			} else if (m.getAddress() == "/2/push22") {
				displayImage = 2;
				displayVideo = 0;
			} else if (m.getAddress() == "/2/push23") {
				displayImage = 3;
				displayVideo = 0;
			} else if (m.getAddress() == "/2/push24") {
				displayImage = 4;
				displayVideo = 0;
			} else if (m.getAddress() == "/2/push25") {
				displayImage = 5;
				displayVideo = 0;
			} else if (m.getAddress() == "/2/push26") {
				displayImage = 6;
				displayVideo = 0;
			} else if (m.getAddress() == "/2/push27") {
				displayImage = 7;
				displayVideo = 0;
			} else if (m.getAddress() == "/2/push28") {
				displayImage = 8;
				displayVideo = 0;
			} else if (m.getAddress() == "/2/push29") {
				displayImage = 9;
				displayVideo = 0;
			} else if (m.getAddress() == "/3/push30") {
				displayImage = 0;
				displayVideo = 0;
				videoPlayer.stop();
			} else if (m.getAddress() == "/3/push31") {
				displayVideo = 1;
				videoPlayer.loadMovie("videos/video"+ofToString(displayVideo)+".mov");
				videoPlayer.play();
				displayImage = 0;
			} else if (m.getAddress() == "/3/push32") {
				displayImage = 0;
				videoPlayer.loadMovie("videos/video"+ofToString(displayVideo)+".mov");
				videoPlayer.play();
				displayVideo = 2;
			} else if (m.getAddress() == "/3/push33") {
				displayImage = 0;
				videoPlayer.loadMovie("videos/video"+ofToString(displayVideo)+".mov");
				videoPlayer.play();
				displayVideo = 3;
			} else if (m.getAddress() == "/3/push34") {
				displayImage = 0;
				videoPlayer.loadMovie("videos/video"+ofToString(displayVideo)+".mov");
				videoPlayer.play();
				displayVideo = 4;
			} else if (m.getAddress() == "/3/push35") {
				displayImage = 0;
				videoPlayer.loadMovie("videos/video"+ofToString(displayVideo)+".mov");
				videoPlayer.play();
				displayVideo = 5;
			} else if (m.getAddress() == "/3/push36") {
				displayImage = 0;
				videoPlayer.loadMovie("videos/video"+ofToString(displayVideo)+".mov");
				videoPlayer.play();
				displayVideo = 6;
			} else if (m.getAddress() == "/3/resetSpeed") {
				videoPlayer.setSpeed(1);
			} else if (m.getAddress() == "/3/videoSpeed") {
				videoPlayer.setSpeed(m.getArgAsFloat( i ));
			} else if (m.getAddress() == "/3/push39") {
				displayImage = 0;
				displayVideo = 9;
			} else if (m.getAddress() == "/1/dripsToggle") {
				dripsOn = !dripsOn;
			}
			
			
			// display the argument - make sure we get the right type
			if( m.getArgType( i ) == OFXOSC_TYPE_INT32 )
				msg_string += ofToString( m.getArgAsInt32( i ) );
			else if( m.getArgType( i ) == OFXOSC_TYPE_FLOAT )
				msg_string += ofToString( m.getArgAsFloat( i ) );
			else if( m.getArgType( i ) == OFXOSC_TYPE_STRING )
				msg_string += m.getArgAsString( i );
			else
				msg_string += "unknown";
		}
//		cout << " osc msg: " + msg_string << endl;
		
	}	

	
	if (displayVideo) {
		videoPlayer.idleMovie();
	}
	
	updateDrips();
	
	
}






//--------------------------------------------------------------
void testApp::draw(){
	
	ofBackground(0,0,0);
	//	tuio.drawCursors();
	//	tuio.drawObjects();
	
	
	ofSetLineWidth(lineThickness);
	
	// what to draw ...
	if (displayImage > 0) {
		// image
		images[displayImage-1].draw(0, 0, PROJECTION_WIDTH, PROJECTION_HEIGHT);
	} else if (displayVideo > 0) {
		// video
		videoPlayer.draw(0, 0, PROJECTION_WIDTH, PROJECTION_HEIGHT);
	} else {
		
		switch (displayBrush) {
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
				Canvas.draw(0,0, PROJECTION_WIDTH, PROJECTION_HEIGHT);
				break;
//			case 8:
//				drawChisel();
//				break;
//			case 9:
//			default:
//				drawRound();
//				break;
		}
		
	}
	
	
	// DEV OUTPUT
	if (showFPS) {
		string fps;
		fps = ofToString(ofGetFrameRate(), 0) + " fps";
		ofDrawBitmapString( fps, 10, 10 );
	}
	
	
	
}



void testApp::drawBitmapBrush(float _x, float _y, int _b, bool _new) {


	
	IMG.clone(brushes[displayBrush-1]);
	//no need for color we will use the image as a mask to draw with and multiply it with our drawing color.
	IMG.setImageType(OF_IMAGE_GRAYSCALE); 
	TMP.setImageType(OF_IMAGE_GRAYSCALE); 
	// update temp image
	TMP.clone(IMG);
	
	float tmpW 		= TMP.width;
	float tmpH 		= TMP.height;
	float ratio 	= (float)lineThickness/tmpW;
	int newWidth  	= lineThickness;
	int newHeight  	= tmpH*ratio;
	TMP.resize(newWidth, newHeight);

	//calc some info for where to draw from						
	int pix			= 0;
	stride			= BITMAP_WIDTH * imageNumBytes;
	int startX 		= _x;
	int startY 		= _y;
	int steps		= numSteps;
	
	int tx = 0;
	int ty = 0;
	float dx = 0;
	float dy = 0;
	
	if(_new){
		steps	= 1;	//then just one image
		blobs[_b].oldX	= startX;
		blobs[_b].oldY	= startY;
		dx		= 0;
		dy		= 0;
	}else{
		dx = ((float)(startX - blobs[_b].oldX)/steps);
		dy = ((float)(startY - blobs[_b].oldY)/steps);
	}
	
	//get the pixels of the brush
	unsigned char * brushPix = TMP.getPixels();
	
	//if no distance is to be travelled draw only one point
	if(dx == 0 && dy == 0) steps = 1;
	
	// lets draw the brushNumber many times to make a line
	for(int i = 0; i < steps; i++){
		
//		colourOscillate = (int)(sin(colourOscillateAmount * ofGetElapsedTimef()) - (colourOscillateAmount / 2));

		// we do - TMP.width/2 because we want the brushNumber to be
		// drawn from the center
		if(_new){
			tx = startX - TMP.width/2;
			ty = startY - TMP.height/2;
		} else {
			tx = (blobs[_b].oldX + (int)(dx*(float)i)) - TMP.width/2;
			ty = (blobs[_b].oldY + (int)(dy*(float)i)) - TMP.height/2;
		}
		
		//this is what we use to move through the brushNumber array									
		int tPix = 0;
		int destX = (TMP.width  + tx);
		int destY = (TMP.height + ty);	
		
		// lets check that we don't draw out outside the projected image
		if(destX >= BITMAP_WIDTH)   destX = BITMAP_WIDTH - 1;
		if(destY >= BITMAP_HEIGHT)  destY = BITMAP_HEIGHT - 1;
		
		// if the brushNumber is a bit off the screen on the left side we need to figure this amount out so we only copy part of the brushNumber image 
		int offSetCorrectionLeft = 0;	
		if(tx < 0){
			offSetCorrectionLeft = -tx;
			tx = 0;
		}
		
		// same here for y - we need to figure out the y offset for the cropped brush
		if(ty < 0){
			tPix    = -ty * TMP.width; 
			ty 		= 0;
		}
		
		// this is for the right hand side cropped brush 
		int offSetCorrectionRight = ((TMP.width + tx) -  destX);	
		tPix += offSetCorrectionLeft;
		
		// some vars we are going to need
		// put here to optimise? 
		float r, g, b, value, ival;
		
		for(int y = ty; y < destY; y++){
			for(int x = tx; x < destX; x++){
				pix = x*imageNumBytes + (y*stride);
				if(brushPix[tPix] == 0){
					// we don't need to do anything
					tPix++;
					continue;
				}						
				// okay so here we are going to use the pixel value of the brush as a multiplier to add into the image
				value = (float)brushPix[tPix] * ONE_OVER_255;
				ival  = 1.0 - value;
				
				r 	= (float)pixels[pix  ] * ival + red   * value;
				g 	= (float)pixels[pix+1] * ival + green * value;
				b 	= (float)pixels[pix+2] * ival + blue  * value;
				pixels[pix  ] = (unsigned char)r;
				pixels[pix+1] = (unsigned char)g;
				pixels[pix+2] = (unsigned char)b;
				pixels[pix+3] = 255; // alpha
				
				tPix++;					
			}
			tPix += offSetCorrectionRight;
		}
		
		// random add drips
		if (ofRandom(1,1000) < 4 && dripsOn) {
			addDrip((tx+destX)/2, (ty+destY)/2);
		}
	
	}
	
	blobs[_b].oldX = startX;
	blobs[_b].oldY = startY;
	
	Canvas.loadData(pixels, BITMAP_WIDTH, BITMAP_HEIGHT, GL_RGBA);
	
}

// Update Drips
void testApp::updateDrips() {

	bool _new = false;
	
	for (int _d=0; _d<numDrips; _d++) {
		
		if (drips[_d]._dripping) {
		
		int _x = drips[_d]._x;
		int _y = drips[_d]._y;

		IMG.clone(brushes[1]);
		
		//no need for color
		//we will use the image as a mask to 
		//draw with and multiply it with
		//our drawing color.
		IMG.setImageType(OF_IMAGE_GRAYSCALE); 
		TMP.setImageType(OF_IMAGE_GRAYSCALE); 
		
		// update temp image
		TMP.clone(IMG);
		
		float tmpW 		= TMP.width;
		float tmpH 		= TMP.height;
		
		float ratio 	= (float)drips[_d]._thickness/tmpW;
		
		int newWidth  	= drips[_d]._thickness;
		int newHeight  	= tmpH*ratio;
		
		TMP.resize(newWidth, newHeight);
		
		
		//calc some info for where to draw from						
		int pix			= 0;
		stride			= BITMAP_WIDTH * imageNumBytes;
		int startX 		= _x;
		int startY 		= _y;
		int steps		= 1;
		
		int tx = 0;
		int ty = 0;
		float dx = 0;
		float dy = 0;
		
		if(_new){
			steps	= 1;	//then just one image
			drips[_d].oldX	= startX;
			drips[_d].oldY	= startY;
			dx		= 0;
			dy		= 0;
		}else{
			dx = ((float)(startX - drips[_d].oldX)/steps);
			dy = ((float)(startY - drips[_d].oldY)/steps);
		}
		
		//get the pixels of the brush
		unsigned char * brushPix = TMP.getPixels();
		
		//if no distance is to be travelled
		//draw only one point
		if(dx == 0 && dy == 0) steps = 1;
		
		// lets draw the brushNumber many times to make a line
		for(int i = 0; i < steps; i++){				
			
			// we do - TMP.width/2 because we want the brushNumber to be
			// drawn from the center
			if(_new){
				tx = startX - TMP.width/2;
				ty = startY - TMP.height/2;
			}else{
				tx = (drips[_d].oldX + (int)(dx*(float)i)) - TMP.width/2;
				ty = (drips[_d].oldY + (int)(dy*(float)i)) - TMP.height/2;
			}
			
			//this is what we use to move through the
			//brushNumber array									
			int tPix = 0;
			
			int destX = (TMP.width  + tx);
			int destY = (TMP.height + ty);	
			
			//lets check that we don't draw out outside the projected
			//image
			if(destX >= BITMAP_WIDTH)   destX = BITMAP_WIDTH - 1;
			if(destY >= BITMAP_HEIGHT)  destY = BITMAP_HEIGHT - 1;
			
			//if the brushNumber is a bit off the screen on the left side
			//we need to figure this amount out so we only copy part
			//of the brushNumber image 
			int offSetCorrectionLeft = 0;	
			if(tx < 0){
				offSetCorrectionLeft = -tx;
				tx = 0;
			}
			
			//same here for y - we need to figure out the y offset
			//for the cropped brush
			if(ty < 0){
				tPix    = -ty * TMP.width; 
				ty 		= 0;
			}
			
			//this is for the right hand side cropped brush 
			int offSetCorrectionRight = ((TMP.width + tx) -  destX);	
			tPix += offSetCorrectionLeft;
			
			//some vars we are going to need
			//put here to optimise? 
			float r, g, b, value, ival;
			
			
			for(int y = ty; y < destY; y++){
				for(int x = tx; x < destX; x++){
					
					pix = x*imageNumBytes + (y*stride);
					
					if(brushPix[tPix] == 0){
						//we don't need to do anything
						tPix++;
						continue;
					}						
					//okay so here we are going to use the pixel value of the brush
					//as a multiplier to add into the image
					value = (float)brushPix[tPix] * ONE_OVER_255;
					ival  = 1.0 - value;
					
					r 	= (float)pixels[pix  ] * ival + drips[_d]._r  * value;
					g 	= (float)pixels[pix+1] * ival + drips[_d]._g * value;
					b 	= (float)pixels[pix+2] * ival + drips[_d]._b  * value;
					
					pixels[pix  ] = (unsigned char)r;
					pixels[pix+1] = (unsigned char)g;
					pixels[pix+2] = (unsigned char)b;
					pixels[pix+3] = 255;
					
					tPix++;					
				}
				tPix += offSetCorrectionRight;
			}
			
			
			
			
			
			
		}
		
		drips[_d].oldX = startX;
		drips[_d].oldY = startY;
//		drips[_d]._speed--;	
		drips[_d]._y += drips[_d]._speed;
		drips[_d]._length -= drips[_d]._speed;
		if (drips[_d]._length < 0 || drips[_d]._speed <= 0) {
			drips[_d]._dripping = false;
		}

		
		}
		
	}

	Canvas.loadData(pixels, BITMAP_WIDTH, BITMAP_HEIGHT, GL_RGBA);

}


// add Drip
void testApp::addDrip(int _x, int _y) {
	
	drips[numDrips]._x = _x;
	drips[numDrips]._y = _y;
	drips[numDrips]._speed = 1;
	drips[numDrips]._thickness = (int)ofRandom(2, 3);
	drips[numDrips]._length = (int)ofRandom(40, 170);
	drips[numDrips].oldX = _x;
	drips[numDrips].oldY = _y;
	drips[numDrips]._dripping = true;
	drips[numDrips]._r = red;
	drips[numDrips]._g = green;
	drips[numDrips]._b = blue;
	
	numDrips++;

}


// TUIO Added
void testApp::tuioAdded(ofxTuioCursor & tuioCursor){
	//	cout << " new cursor: " + ofToString(tuioCursor.getFingerId())+" X: "+ofToString(tuioCursor.getX())+" Y: "+ofToString(tuioCursor.getY()) << endl;
	if (isDrawing && tuioCursor.getFingerId() < MAX_BLOBS) {
		addPoint(tuioCursor);
		blobs[tuioCursor.getFingerId()].numPoints++;
		numBlobs++;
	}
}
// TUIO Updated
void testApp::tuioUpdated(ofxTuioCursor & tuioCursor){
	//	cout << " cursor updated: " + ofToString(tuioCursor.getFingerId())+" X: "+ofToString(tuioCursor.getX())+" Y: "+ofToString(tuioCursor.getY())<< endl;
	if (isDrawing && tuioCursor.getFingerId() < MAX_BLOBS) {
		addPoint(tuioCursor);
		blobs[tuioCursor.getFingerId()].numPoints++;
	}
}
// TUIO Removed
void testApp::tuioRemoved(ofxTuioCursor & tuioCursor){
	//	cout << " cursor removed: " + ofToString(tuioCursor.getFingerId())+" X: "+ofToString(tuioCursor.getX())+" Y: "+ofToString(tuioCursor.getY())<< endl;
	if (isDrawing && tuioCursor.getFingerId() < MAX_BLOBS && numStrokes < MAX_STROKES) {
		addPoint(tuioCursor);
		strokes[numStrokes] = blobs[tuioCursor.getFingerId()];
	
		blobs[tuioCursor.getFingerId()].numPoints = 0;
		numStrokes++;
		numBlobs--;
	}
}

// add point to current blobs stroke
void testApp::addPoint(ofxTuioCursor & tuioCursor){
	
	int blobId = tuioCursor.getFingerId();
	int distX = 0;
	int distY = 0;

	// check distance between blobs
	if (blobs[blobId].numPoints > 0) {
		int prevX = blobs[blobId].points[blobs[blobId].numPoints-1][0];
		int prevY = blobs[blobId].points[blobs[blobId].numPoints-1][1];
		int thisX = tuioCursor.getX() * BITMAP_WIDTH;
		int thisY = tuioCursor.getY() * BITMAP_HEIGHT;
		distX = abs(thisX - prevX);
		distY = abs(thisY - prevY);
	} else {
		distX = 10;
		distY = 10;
	}

	
	if (distX > maxBlobDistance || distY > maxBlobDistance) {
		cout << "distX : " << ofToString(distX) << " distY : " << ofToString(distY) << " max blob dist : " << maxBlobDistance << endl;
		
		// remove cursor
		strokes[numStrokes] = blobs[tuioCursor.getFingerId()];
		
		blobs[tuioCursor.getFingerId()].numPoints = 0;
		numStrokes++;
		numBlobs--;
		
	}
	
	// records points to arrays
	if (blobId < MAX_BLOBS && blobs[blobId].numPoints < MAX_POINTS && distX < maxBlobDistance && distY < maxBlobDistance) {
		if (blobs[blobId].numPoints > 1) {
			// check for minimum distance from previous point
			float _dx = abs(((int)blobs[blobId].points[blobs[blobId].numPoints-1][0]) - ((int)tuioCursor.getX() * BITMAP_WIDTH));
			float _dy = abs(((int)blobs[blobId].points[blobs[blobId].numPoints-1][1]) - ((int)tuioCursor.getY() * BITMAP_HEIGHT));
			//	cout << "blob: " + ofToString(blobId)+" _dx: "+ofToString(blobs[blobId].points[blobs[blobId].numPoints-2][0])+" _dy: "+ofToString(tuioCursor.getX() * PROJECTION_WIDTH) << endl;
			
			if (_dx > minBlobDistance || _dy > minBlobDistance) {
				blobs[blobId].points[blobs[blobId].numPoints][0] = tuioCursor.getX() * BITMAP_WIDTH;
				blobs[blobId].points[blobs[blobId].numPoints][1] = tuioCursor.getY() * BITMAP_HEIGHT;
				blobs[blobId].points[blobs[blobId].numPoints][2] = tuioCursor.getXSpeed();
				blobs[blobId].points[blobs[blobId].numPoints][3] = tuioCursor.getYSpeed();
				blobs[blobId].points[blobs[blobId].numPoints][4] = tuioCursor.getMotionSpeed();
				blobs[blobId].points[blobs[blobId].numPoints][5] = tuioCursor.getMotionAccel();
				//				blobs[blobId].numPoints++;
			}
			//			cout << "blob: " + ofToString(blobId)+" xspeed: "+ofToString(tuioCursor.getXSpeed())+" yspeed: "+ofToString(tuioCursor.getYSpeed()) << endl;
			
		} else {
			blobs[blobId].points[blobs[blobId].numPoints][0] = tuioCursor.getX() * BITMAP_WIDTH;
			blobs[blobId].points[blobs[blobId].numPoints][1] = tuioCursor.getY() * BITMAP_HEIGHT;
			blobs[blobId].points[blobs[blobId].numPoints][2] = tuioCursor.getXSpeed();
			blobs[blobId].points[blobs[blobId].numPoints][3] = tuioCursor.getYSpeed();
			blobs[blobId].points[blobs[blobId].numPoints][4] = tuioCursor.getMotionSpeed();
			blobs[blobId].points[blobs[blobId].numPoints][5] = tuioCursor.getMotionAccel();
			//			blobs[blobId].numPoints++;
		}
	
	}

	
	// update bitmap brush image
	if (displayBrush > 0 && displayBrush < NUM_BRUSHES && distX < maxBlobDistance && distY < maxBlobDistance) {
		
		bool ns = false;
		if (blobs[blobId].numPoints == 0) {
			ns = true;
		}
		
		drawBitmapBrush(tuioCursor.getX() * BITMAP_WIDTH, tuioCursor.getY() * BITMAP_HEIGHT, blobId, ns);
		
	}
	
	
}

//--------------------------------------------------------------
// on KeyDown
void testApp::keyPressed (int key) {
	//	cout << "keyPressed = " + ofToString(key) << endl;	
	switch (key){
		case 'i': // Image trigger
			if (!iToggle)
				iToggle = true;
			break;
		case 'v': // Video trigger
			if (!vToggle)
				vToggle = true;
			break;
		case 'b': // Brush trigger
			if (!bToggle)
				bToggle = true;
			break;
		case 'd': // toggle Drips
			dripsOn = !dripsOn;
			break;
		case 's': // Save drawing
			saveScreen();
			break;
		case 'c': // save and Clear drawing
			saveScreen();
			clearScreen();
			break;
		case 'f': // toggle Fullscreen
			ofToggleFullscreen();
			break;
		case 'q': // toggle FPS
			showFPS = !showFPS;
			break;
		case 't': // toggle Turbo
			turbo = !turbo;
			break;
		case 'm': // toggle Mouse
			if (mouseToggle) {
				ofHideCursor();
			} else {
				ofShowCursor();
			}
			mouseToggle = !mouseToggle;		
			break;
		case ',': // decrease minimum blob distance
			minBlobDistance--;
			if (minBlobDistance < 0) minBlobDistance = 0;
			break;
		case '.': // increase minimum blob distance
			minBlobDistance++;
			break;
		case '[': // decrease line thickness
			lineThickness--;
			if (lineThickness < minLineThickness) lineThickness = minLineThickness;
			break;
		case ']': // increase line thickness
			lineThickness++;
			if (lineThickness > maxLineThickness) lineThickness = maxLineThickness;
			break;
		case '-': // minus key
			if (vToggle) { // decrease video speed
				videoPlayer.setSpeed(videoPlayer.getSpeed()*0.8);
			}
			break;
		case '=': // plus key
			if (vToggle) {  // increase video speed
				videoPlayer.setSpeed(videoPlayer.getSpeed()*1.25);
			}
			break;
		case '1': // load image, video or brush by number
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (iToggle) {
				displayImage = key - 48;
				displayVideo = 0;
			} else if (vToggle) {
				displayImage = 0;
				displayVideo = key - 48;
				videoPlayer.loadMovie("videos/video"+ofToString(displayVideo)+".mov");
				videoPlayer.play();
			} else if (bToggle) {
				displayBrush = key - 48;
			}
			break;
		case '0': // hide image or video
			displayImage = 0;
			displayVideo = 0;
			videoPlayer.stop();
			break;
	}
}

//--------------------------------------------------------------
// on KeyUp
void testApp::keyReleased (int key) {
	switch (key){
		case 'i': // Image trigger
			iToggle = false;
			break;
		case 'v': // Video trigger
			vToggle = false;
			break;
		case 'b': // Brush trigger
			bToggle = false;
			break;
	}
}

//--------------------------------------------------------------
// Clear Screen
void testApp::clearScreen() {

	numStrokes = 0;
	numBlobs = 0;
	numDrips = 0;
	
//	Canvas.clear();
	
	for (int x=0; x<stride; x++) {
		for (int y=0; y<BITMAP_HEIGHT; y++) {
			int pix = x*imageNumBytes + (y*stride);
			 
			float r 	= 0;
			float g 	= 0;
			float b 	= 0;
			
			pixels[pix  ] = (unsigned char)r;
			pixels[pix+1] = (unsigned char)g;
			pixels[pix+2] = (unsigned char)b;
			pixels[pix+3] = 0;

		}
	}
	Canvas.loadData(pixels, BITMAP_WIDTH, BITMAP_HEIGHT, GL_RGBA);

	
	
	
	
}


// draw Round
void testApp::drawRound() {
	
	ofEnableAlphaBlending();
	
	// strokes
	for(int i=0; i<numStrokes; i++) {
		glBegin(GL_QUAD_STRIP);		
		ofSetColor(drawColour);
		for(int j = 0; j < strokes[i].numPoints; j++){
			float angle = asin(blobs[i].points[j][2] / blobs[i].points[j][3]);
			
			float _x = strokes[i].points[j][0];
			float _y = strokes[i].points[j][1];
			float _vx = strokes[i].points[j][4];
			float _vy = strokes[i].points[j][5];
			float _dx = (j>0) ? strokes[i].points[j][0] - strokes[i].points[j-1][0] : 0;
			float _dy = (j>0) ? strokes[i].points[j][1] - strokes[i].points[j-1][1] : 0;
			
			if ((_dx >= 0 && _dy < 0) || (_dx < 0 && _dy >= 0)) {		// top right or bottom left quadrant
				if (_dx == 0) {
					glVertex2f(_x, _y + (_dx+lineThickness/10));
					glVertex2f(_x, _y - (_dx+lineThickness/10));
				} else if (_dy == 0) {
					glVertex2f(_x + (_dy+lineThickness/10), _y);
					glVertex2f(_x - (_dy+lineThickness/10), _y);
				} else {
					glVertex2f(_x + (_dy+lineThickness/10), _y + (_dx+lineThickness/10));
					glVertex2f(_x - (_dy+lineThickness/10), _y - (_dx+lineThickness/10));
				}
			} else {
				glVertex2f(_x + (_dy+lineThickness/10), _y - (_dx+lineThickness/10));
				glVertex2f(_x - (_dy+lineThickness/10), _y + (_dx+lineThickness/10));
			}
		}
		glEnd();	
	}
	// blobs
	for(int i=0; i<numBlobs; i++) {
		glBegin(GL_QUAD_STRIP);		
		ofSetColor(drawColour);
		for(int j = 0; j < blobs[i].numPoints; j++){
			// angle
			float angle = asin(blobs[i].points[j][2] / blobs[i].points[j][3]);
			
			float _x = blobs[i].points[j][0];
			float _y = blobs[i].points[j][1];
			float _vx = blobs[i].points[j][4];
			float _vy = blobs[i].points[j][5];
			float _dx = (j>0) ? blobs[i].points[j][0] - blobs[i].points[j-1][0] : 0;
			float _dy = (j>0) ? blobs[i].points[j][1] - blobs[i].points[j-1][1] : 0;
			
			if ((_dx >= 0 && _dy < 0) || (_dx < 0 && _dy >= 0)) {		// top right or bottom left quadrant
				if (_dx == 0) {
					glVertex2f(_x, _y + (_dx+lineThickness/10));
					glVertex2f(_x, _y - (_dx+lineThickness/10));
				} else if (_dy == 0) {
					glVertex2f(_x + (_dy+lineThickness/10), _y);
					glVertex2f(_x - (_dy+lineThickness/10), _y);
				} else {
					glVertex2f(_x + (_dy+lineThickness/10), _y + (_dx+lineThickness/10));
					glVertex2f(_x - (_dy+lineThickness/10), _y - (_dx+lineThickness/10));
				}
			} else {
				glVertex2f(_x + (_dy+lineThickness/10), _y - (_dx+lineThickness/10));
				glVertex2f(_x - (_dy+lineThickness/10), _y + (_dx+lineThickness/10));
			}
			// cout << "dx: " + ofToString(_dx)+" dy: " + ofToString(_dy) << endl;
			
		}
		glEnd();	
	}
	
	ofDisableAlphaBlending();
}



// draw Chisel
void testApp::drawChisel() {
	
	ofEnableAlphaBlending();
	
	// strokes
	for(int i=0; i<numStrokes; i++) {
		glBegin(GL_QUAD_STRIP);
		if (turbo) {
			ofSetColor(ofRandom(0,255), ofRandom(0,255), ofRandom(0,255));
		} else {
			ofSetColor(drawColour);
		}
		for(int j = 0; j < strokes[i].numPoints; j++){
			glVertex2f(strokes[i].points[j][0] + (lineThickness/2), strokes[i].points[j][1] - (lineThickness/2));
			glVertex2f(strokes[i].points[j][0] - (lineThickness/2), strokes[i].points[j][1] + (lineThickness/2));
		}
		glEnd();	
	}
	// blobs
	for(int i=0; i<numBlobs; i++) {
		glBegin(GL_QUAD_STRIP);		
		if (turbo) {
			ofSetColor(ofRandom(0,255), ofRandom(0,255), ofRandom(0,255));
		} else {
			ofSetColor(drawColour);
		}
		for(int j = 0; j < blobs[i].numPoints; j++){
			glVertex2f(blobs[i].points[j][0] + (lineThickness/2), blobs[i].points[j][1] - (lineThickness/2));
			glVertex2f(blobs[i].points[j][0] - (lineThickness/2), blobs[i].points[j][1] + (lineThickness/2));
		}
		glEnd();	
	}
	
	ofDisableAlphaBlending();
}





//--------------------------------------------------------------
// Save Screenshot
void testApp::saveScreen() {
	
	// image file
	saveImage.grabScreen(0,0,PROJECTION_WIDTH,PROJECTION_HEIGHT);
	saveImage.saveImage("savedImages/"+getTimestampString()+".jpg");
	
}

void testApp::saveData() {

    // argh can't get this to work!!!
    
	ofstream myfile; 
	myfile.open("ARGH.txt"); 
	myfile << "Hey this is my first written text file.\n"; 
	myfile.close();
	cout << "Hey this is my first written text file." << endl; 

}


void testApp::ofAppendToFile(string filename, string str) {
//    if (filename=="") die("ofAppendToFile:no filename");
    FILE * pFile;
    pFile = fopen (filename.c_str(),"a");
//    if (pFile==NULL) die("ofAppendToFile:could not open file: " + filename);
    fprintf (pFile, str.c_str());
    fclose(pFile);
}

string testApp::getTimestampString() {
	// make timestamp for filename
	string timestamp = ofToString(ofGetYear()) + "-";
	if (ofGetMonth() < 10) { timestamp += "0"; }
	timestamp += ofToString(ofGetMonth()) + "-";
	if (ofGetDay() < 10) { timestamp += "0"; }
	timestamp += ofToString(ofGetDay()) + "_";
	if (ofGetHours() < 10) { timestamp += "0"; }
	timestamp += ofToString(ofGetHours());
	if (ofGetMinutes() < 10) { timestamp += "0"; }
	timestamp += ofToString(ofGetMinutes());
	if (ofGetSeconds() < 10) { timestamp += "0"; }
	timestamp += ofToString(ofGetSeconds());
	return timestamp;
}

