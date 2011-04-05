#include "drips.h"

#define MAX_DRIPS	100
#define BITMAP_WIDTH 640
#define BITMAP_HEIGHT 480


struct Drip {
	int			_x;
	int			_y;
	int			_speed;
	int			_thickness;
	int			_oldx;
	int			_oldy;
};

Drip drips[MAX_DRIPS];
int numDrips;


//--------------------------------------------------------------
// Setup
void Drips::setup(){
	
	// load brushes
	dripBrush.loadImage("brushes/001-soft.png");
	
	steps = 20;
	imageNumBytes = 4;
	
	// default to white
	red				= 150;
	green			= 255;
	blue			= 150;	
	
	pixels = new unsigned char[BITMAP_WIDTH * BITMAP_HEIGHT * imageNumBytes];
}

//--------------------------------------------------------------
void Drips::update(){
	
	for (int i=0; i<numDrips; i++) {
	
		drawDrip(i);
		
		
	}

}


//--------------------------------------------------------------
void Drips::draw(){
	
//	Canvas.draw(0,0, PROJECTION_WIDTH, PROJECTION_HEIGHT);
	
	
}

void Drips::addDrip(int _x, int _y) {
	
	drips[numDrips]._x = _x;
	drips[numDrips]._y = _y;
	drips[numDrips]._speed = 4;
	drips[numDrips]._thickness = 10;
	drips[numDrips]._oldx = _x;
	drips[numDrips]._oldy = _y;
	
}

unsigned char * Drips::getPixelData() {

	return pixels;

}

void Drips::drawDrip(int _d) {

	bool _new = false;
	
	IMG.clone(dripBrush);
	
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
	stride		= BITMAP_WIDTH * imageNumBytes;
	int startX 		= drips[_d]._x;
	int startY 		= drips[_d]._x;
	
	int tx = 0;
	int ty = 0;
	float dx = 0;
	float dy = 0;
	
	if(_new){
		steps	= 1;	//then just one image
		drips[_d]._oldx	= startX;
		drips[_d]._oldy	= startY;
		dx		= 0;
		dy		= 0;
	}else{
		dx = ((float)(startX - drips[_d]._oldx)/steps);
		dy = ((float)(startY - drips[_d]._oldy)/steps);
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
			tx = (drips[_d]._oldx + (int)(dx*(float)i)) - TMP.width/2;
			ty = (drips[_d]._oldy + (int)(dy*(float)i)) - TMP.height/2;
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
				
				r 	= (float)pixels[pix  ] * ival + red   * value;
				g 	= (float)pixels[pix+1] * ival + green * value;
				b 	= (float)pixels[pix+2] * ival + blue  * value;

				pixels[pix  ] = (unsigned char)r;
				pixels[pix+1] = (unsigned char)g;
				pixels[pix+2] = (unsigned char)b;
				pixels[pix+3] = 255;
				
				tPix++;					
			}
			tPix += offSetCorrectionRight;
		}
	
	}
	
	drips[_d]._oldx = startX;
	drips[_d]._oldy = startY;
//	drips[_d]._x = 
	drips[_d]._y += drips[_d]._speed;

	

}




//--------------------------------------------------------------
// Clear Screen
void Drips::clearScreen() {

	numDrips = 0;
	
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

	
}




