#ifndef WIDGET_H
#define WIDGET_H

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_mutex.h>
#include <vlc/vlc.h>
#include <GL/gl.h>
#include <math.h>
#include <string>
#include "blob.h"
#include "glhelper.h"
#include "tracker.h"
#include "tracker2.h"



struct rgba {
	double r, g, b, a;
	rgba(const rgba& c) { r = c.r; g = c.g; b = c.b; a = c.a; }
	rgba(double r, double g, double b, double a) : r(r), g(g), b(b), a(a) { }
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cWidget
class cWidget {
  private:

  protected:
	int type;

	point translate; double rotate, scale;
	point translate_previous; double rotate_previous, scale_previous;
	rgba color;

	std::vector<cBlob *> events;

  public:
	int z;
	static int zindex;
	
	cWidget(point translate, double rotate, double scale, rgba color);
	~cWidget();

	void resetEvents();			// reset events vector to empty
	void addEvent(cBlob& event);		// event queue populates events vector
	void addEvent(cBlob* event);		// event queue populates events vector

	void bringForward();			// increase z index
	void translateWidget(const point& p);
	void rotateWidget(const point& p0, const point& p1, const point& p2);
	void scaleWidget(const point& p0, const point& p1, const point& p2);

	virtual bool containsPoint(const point& p) const = 0;		// so event queue knows if an event belongs to a particular instance of cWidget
	virtual void update() = 0;					// called when events vector is populated
	virtual void render() = 0;					// called once events have been processed
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cWidgetQueue
class cWidgetQueue : public cWidget {
  private:

  protected:
	std::vector<cWidget *> widgets;

  public:
	cWidgetQueue(point translate, double rotate, double scale, rgba color);
	~cWidgetQueue();

	virtual bool containsPoint(const point& p) const = 0;		// so event queue knows if an event belongs to a particular instance of cWidget
	virtual void update() = 0;					// called when events vector is populated
	virtual void render() = 0;					// called once events have been processed

	void registerWidget(cWidget& widget);				// for registering and unregistering subwidgets
	void unregisterWidget(cWidget& widget);
	void registerWidget(cWidget* widget);
	void unregisterWidget(cWidget* widget);
	void processEvents();						// to parse out events to subwidgets
	void transformEvent(cBlob* event);				// to transform an event to the current widget space
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cRectangle
class cRectangle : public cWidget {
  private:
    
  protected:
	point dimensions;
    
  public:
	cRectangle(point translate, double rotate, double scale, point dimensions, rgba color);
	~cRectangle();
	
	virtual bool containsPoint(const point& p) const;
	virtual void update();
	virtual void render();
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cRectangleQueue
class cRectangleQueue : public cWidgetQueue {
  private:
    
  protected:
	point dimensions;
    
  public:
	cRectangleQueue(point translate, double rotate, double scale, point dimensions, rgba color);
	~cRectangleQueue();
	
	virtual bool containsPoint(const point& p) const;
	virtual void update();
	virtual void render();
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cTexture
class cTexture : public cRectangle {
  private:
    
  protected:
	GLuint texture;

  public:
	cTexture(std::string filename);
	cTexture(std::string filename, point translate, double rotate, double scale);
	~cTexture();

	virtual void render();
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cScrabbleTile
class cScrabbleTile : public cRectangle {
  private:
    
  protected:
	unsigned char letter;

  public:
	cScrabbleTile(unsigned char letter);
	cScrabbleTile(unsigned char letter, point translate, double rotate, double scale);
	~cScrabbleTile();

	virtual void update();
	virtual void render();
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cScrabble
class cScrabble : public cRectangleQueue {
  private:
	std::vector<cScrabbleTile*> tiles;

  protected:
	GLuint texture;

  public:
	cScrabble();
	cScrabble(point translate, double rotate, double scale);
	~cScrabble();

	virtual void render();
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cTextureBackground
class cTextureBackground : public cTexture {
  private:
    
  protected:

  public:
	cTextureBackground(std::string filename);
	cTextureBackground(std::string filename, point translate, double rotate, double scale);
	~cTextureBackground();

	virtual void update();
	//virtual void render();
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cTextureBorder
class cTextureBorder : public cTexture {
  private:
    
  protected:
	GLuint corners;

  public:
	cTextureBorder(std::string filename, std::string corners);
	cTextureBorder(std::string filename, std::string corners, point translate, double rotate, double scale);
	~cTextureBorder();

	void renderBorders();
	virtual void render();
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cTextureVideo
struct context {
    SDL_Surface *surf;
    SDL_mutex *mutex;
};

class cTextureVideo : public cTextureBorder {
  private:

  protected:
	context ctx;

	libvlc_instance_t *libvlc;
	libvlc_media_t *m;
	libvlc_media_player_t *mp;

  public:
	cTextureVideo(std::string filename, std::string corners, std::string video);
	cTextureVideo(std::string filename, std::string corners, std::string video, point translate, double rotate, double scale);
	~cTextureVideo();

	virtual void render();
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cCalibration
class cCalibration : public cTexture {
  private:
	point *calibration_points;
	int count;
	cTracker *tracker;
	cTracker2 *tracker2;
	int h_points, v_points;
	bool calibrated;

  protected:

  public:
	cCalibration(std::string filename, cTracker& tracker, cTracker2& tracker2, int h_points, int v_points);
	~cCalibration();

	void reset();
	bool isCalibrated();
	
	virtual void update();
	virtual void render();
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cFluid
struct particle {
	double life;
	point location;
};

class cFluid : public cRectangle {
  private:
	point grid_dimensions;

	std::vector<particle> particles;
	double *u, *v, *u_prev, *v_prev;	// velocity, velocity previous
	double *d0, *d_prev0;			// density, density previous
	double *d1, *d_prev1;			// density, density previous
	double *d2, *d_prev2;			// density, density previous
	double *curl;

  protected:

  public:
	cFluid(point grid_dimensions, point dimensions, point translate, double rotate, double scale);
	~cFluid();
	
	void add_source(double* x, double* s, double dt);
	void vorticity_confinement(double* u, double* v, double* u0, double* v0);
	void diffuse(int b, double* x, double* x0, double diff, double dt);
	void advect(int b, double* d, double* d0, double* u, double* v, double dt);
	void dens_step(double* x, double* x0, double* u, double* v, double diff, double dt);
	void vel_step(double* u, double* v, double* u0, double* v0, double visc, double dt);
	void project(double* u, double* v, double* p, double* div);
	void set_bnd(int b, double* x);

	virtual void update();
	virtual void render();
};



#endif