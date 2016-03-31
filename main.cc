#include <fstream>
#include <sstream>

#include "src/filter.h"
#include "src/glhelper.h"
#include "src/tracker.h"
#include "src/tracker2.h"
#include "src/fiducial.h"
#include "src/queue.h"
#include "src/widget.h"
#include "src/tuio.h"

// capture parameters
static const int CDEVICE = 0;
static const int CWIDTH  = 640;
static const int CHEIGHT = 480;

// filter parameters
static const int    FKERNELSIZE = 17;	// for gaussian blur
static const double FSTDDEV     = 2.5;
static const int    FBLOCKSIZE  = 63;	// for adaptive threshold
static const int    FC          = -5;

// display (native resolution of projector)
static const int WIDTH  = 1280; //640; //1280
static const int HEIGHT = 800; //480; //800
static const int BPP    = 32;

// tracker parameters
static const double TMINAREA   = 64;	// minimum area of blob to track
static const double TMAXRADIUS = 32;	// a blob is identified with a blob in the previous frame if it exists within this radius

// calibration paramters
static const int CX = 7;		// horizontal calibration points
static const int CY = 4;		// vertical calibration points


void saveTGA(unsigned char* buffer) {
	static int i = 0;
	std::stringstream out;
	out << "capture" << (i++) << ".tga";
	std::string s = out.str();
	
	glReadPixels(0, 0, WIDTH, HEIGHT, GL_BGRA, GL_UNSIGNED_BYTE, buffer);
	std::fstream of(s.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	char header[18] = { 0 };
	header[2] = 2;
	header[12] = WIDTH & 0xff;
	header[13] = WIDTH >> 8;
	header[14] = HEIGHT & 0xff;
	header[15] = HEIGHT >> 8;
	header[16] = 32;
	of.write(header, 18);
	of.write((char *)buffer, WIDTH * HEIGHT * 4);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// main
int main(int argc, char *argv[]) {
	bool active = true, balance = true, fullscreen = false, cvblobslib = true, renderblobs = true, fluid = false, grab = false;
	int render = 0;

	unsigned char *buffer = new unsigned char[WIDTH * HEIGHT * 4];

	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Surface *screen = SDL_SetVideoMode(WIDTH, HEIGHT, BPP, (fullscreen ? SDL_FULLSCREEN : 0) | SDL_SWSURFACE | SDL_OPENGL);
	SDL_Event event;



	cFilter filter(CDEVICE, CWIDTH, CHEIGHT, FKERNELSIZE, FSTDDEV, FBLOCKSIZE, FC);
	filter.balance(balance);
	cTracker tracker(TMINAREA, TMAXRADIUS, WIDTH, HEIGHT, static_cast<double>(WIDTH)/static_cast<double>(CWIDTH), static_cast<double>(HEIGHT)/static_cast<double>(CHEIGHT));
	cTracker2 tracker2(TMINAREA, TMAXRADIUS, WIDTH, HEIGHT, static_cast<double>(WIDTH)/static_cast<double>(CWIDTH), static_cast<double>(HEIGHT)/static_cast<double>(CHEIGHT));
	cFiducial fid; std::vector<fiducial> fiducials;

	cTUIO tuio;


	cTextureBackground ctb("media/background4.jpg", point(WIDTH/2, HEIGHT/2), 0, 1);		// generate texture background first so it has lowest z index

	cCalibration cal("media/calibration.jpg", tracker, tracker2, CX, CY);

	//cRectangle r0(point(1000,300), M_PI/3, 1.0, point(100,100), rgba(1,0,1,1));
	cTextureBorder t0("media/nature/img0.jpg", "media/corners.png", point(96,54), 0, 0.1);
	cTextureBorder t1("media/nature/img1.jpg", "media/corners.png", point(288,54), 0, 0.1);
	cTextureBorder t2("media/nature/img2.jpg", "media/corners.png", point(480,54), 0, 0.1);
	cTextureBorder t3("media/nature/img3.jpg", "media/corners.png", point(672,54), 0, 0.1);
	cTextureBorder t4("media/nature/img4.jpg", "media/corners.png", point(96,162), 0, 0.1);
	cTextureBorder t5("media/nature/img5.jpg", "media/corners.png", point(288,162), 0, 0.1);
	cTextureBorder t6("media/nature/img6.jpg", "media/corners.png", point(480,162), 0, 0.1);
	cTextureBorder t7("media/nature/img7.jpg", "media/corners.png", point(672,162), 0, 0.1);
	cTextureBorder t8("media/nature/img8.jpg", "media/corners.png", point(96,270), 0, 0.1);
	cTextureBorder t9("media/nature/img9.jpg", "media/corners.png", point(288,270), 0, 0.1);
	cTextureBorder t10("media/nature/img10.jpg", "media/corners.png", point(480,270), 0, 0.1);
	cTextureBorder t11("media/nature/img11.jpg", "media/corners.png", point(672,270), 0, 0.1);
	
	//cScrabble sc(point(512,512),0,0.25);
	cScrabble sc(point(0,0),0,0.25);
	
	//cTextureVideo tv0("media/nature/img10.jpg", "media/corners.png", "media/video.flv", point(480,270), 0, 0.1);
	//cTextureVideo tv1("media/nature/img10.jpg", "media/corners.png", "media/video2.3gp", point(672,270), 0, 0.1);
	//cTextureVideo tv2("media/nature/img10.jpg", "media/corners.png", "media/video3.3gp", point(672,270), 0, 0.1);

	cFluid cfl(point(WIDTH/12,HEIGHT/12), point(WIDTH, HEIGHT), point(WIDTH/2, HEIGHT/2), 0, 1.0);
	
	cQueue queue;
	queue.registerWidget(ctb);

	queue.registerWidget(t0);	queue.registerWidget(t1);	queue.registerWidget(t2);	queue.registerWidget(t3);
	queue.registerWidget(t4);	queue.registerWidget(t5);	queue.registerWidget(t6);	queue.registerWidget(t7);
	queue.registerWidget(t8);	queue.registerWidget(t9);	queue.registerWidget(t10);	queue.registerWidget(t11);
	queue.registerWidget(sc);
	//queue.registerWidget(tv0);
	//queue.registerWidget(tv1);
	//queue.registerWidget(tv2);
	//queue.registerWidget(cfl);
	//queue.registerWidget(cal);

	TTF_Init();
	TTF_Font *font = TTF_OpenFont("media/Coalition.ttf", 18);
	
	setupOrtho(WIDTH, HEIGHT);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
   
	glEnable(GL_POLYGON_SMOOTH);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
   
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLuint texture;
	setupTexture(texture);

	while (active) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				active = false;
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case SDLK_f:			// toggle fullscreen
					fullscreen ^= true;
					screen = SDL_SetVideoMode(WIDTH, HEIGHT, BPP, (fullscreen ? SDL_FULLSCREEN : 0) | SDL_HWSURFACE | SDL_OPENGL);
					break;
				case SDLK_b:			// toggle balance
					balance ^= true;
					filter.balance(balance);
					break;
				case SDLK_e:			// toggle extractor
					cvblobslib ^= true;
					break;
				case SDLK_r:			// toggle render blobs
					renderblobs ^= true;
					break;
				case SDLK_c:			// reset calibration
					tracker.resetCalibration(); tracker2.resetCalibration();
					cal.reset();
					queue.registerWidget(cal);
					break;
				case SDLK_g:			// set flag for screen capture
					grab = true;
					break;
				case SDLK_v:			// register fluid simulation
					fluid ^= true;
					if (fluid) {
						queue.registerWidget(cfl);
						queue.unregisterWidget(t0);	queue.unregisterWidget(t1);	queue.unregisterWidget(t2);	queue.unregisterWidget(t3);
						queue.unregisterWidget(t4);	queue.unregisterWidget(t5);	queue.unregisterWidget(t6);	queue.unregisterWidget(t7);
						queue.unregisterWidget(t8);	queue.unregisterWidget(t9);	queue.unregisterWidget(t10);	queue.unregisterWidget(t11);
						queue.unregisterWidget(sc);
						//queue.unregisterWidget(tv0);
						//queue.unregisterWidget(tv1);
						//queue.unregisterWidget(tv2);
					} else {
						queue.unregisterWidget(cfl);
						queue.registerWidget(t0);	queue.registerWidget(t1);	queue.registerWidget(t2);	queue.registerWidget(t3);
						queue.registerWidget(t4);	queue.registerWidget(t5);	queue.registerWidget(t6);	queue.registerWidget(t7);
						queue.registerWidget(t8);	queue.registerWidget(t9);	queue.registerWidget(t10);	queue.registerWidget(t11);
						queue.registerWidget(sc);
						//queue.registerWidget(tv0);
						//queue.registerWidget(tv1);
						//queue.registerWidget(tv2);
					}
					break;
				case SDLK_1:			// captured frame
					render = 0;
					break;
				case SDLK_2:			// filtered frame
					render = 1;
					break;
				case SDLK_3:			// gaussian blur
					render = 2;
					break;
				case SDLK_4:			// adaptive threshold
					render = 3;
					break;
				case SDLK_5:			// panels
					render = 4;
					break;
				}
				break;
			}
		}

		filter.filter(); // capture, filter, blur, (balance), threshold

		if (cvblobslib) {
			tracker.trackBlobs(filter.thresholdFrame(), false, tuio.getPoints()); tuio.unlock();
			fiducials = fid.extractFiducials(tracker.getBlobs());
		} else {
			tracker2.trackBlobs(filter.thresholdFrame(), false, tuio.getPoints()); tuio.unlock();
			fiducials = fid.extractFiducials(tracker2.getBlobs());
		}

		switch (render) {
		case 0:
			glClear(GL_COLOR_BUFFER_BIT);
			renderTexture(texture, filter.captureFrame(), false, WIDTH, HEIGHT);
			break;
		case 1:
			glClear(GL_COLOR_BUFFER_BIT);
			renderTexture(texture, filter.filterFrame(), true, WIDTH, HEIGHT);
			break;
		case 2:
			glClear(GL_COLOR_BUFFER_BIT);
			renderTexture(texture, filter.gaussianFrame(), true, WIDTH, HEIGHT);
			break;
		case 3:
			glClear(GL_COLOR_BUFFER_BIT);
			renderTexture(texture, filter.thresholdFrame(), true, WIDTH, HEIGHT);
			break;
		case 4:
			glClear(GL_COLOR_BUFFER_BIT);
			if (cal.isCalibrated()) queue.unregisterWidget(cal);
			queue.processEvents(cvblobslib ? tracker.getBlobs() : tracker2.getBlobs());
			break;
		}

		if (renderblobs) renderBlobs(cvblobslib ? tracker.getBlobs() : tracker2.getBlobs(), 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f);
		
		for (int i = 0; i < fiducials.size(); i++) {
			renderCircle(fiducials[i].center.x, fiducials[i].center.y, 16.0f, 30, 1.0f, 0.0f, 1.0f);
			glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
			glBegin(GL_LINES);
			glVertex3f(fiducials[i].center.x, fiducials[i].center.y, 0.0f);
			glVertex3f(fiducials[i].center.x+128*cos(fiducials[i].orientation), fiducials[i].center.y+128*sin(fiducials[i].orientation), 0.0f);
			glEnd();
			std::stringstream out;
			out << fiducials[i].id;
			std::string s = out.str();

			glPushMatrix();
			glTranslatef(fiducials[i].center.x, fiducials[i].center.y, 0.0f);
			glRotatef(fiducials[i].orientation*180/M_PI, 0.0f, 0.0f, 1.0f);
			renderText(font, 127, 255, 127, 0.0f, 0.0f, 0.0f, s);
			glPopMatrix();
		}

		SDL_GL_SwapBuffers();
		
		if (grab) {
			grab = false;
			saveTGA(buffer);
		}
	}

	deleteTexture(texture);

	TTF_CloseFont(font);
	TTF_Quit();

	SDL_Quit();

	delete [] buffer;

	return 0;
}
