#ifndef BLOB_H
#define BLOB_H

#include <vector>
#include <math.h>

enum { BLOB_NULL, BLOB_DOWN, BLOB_MOVE, BLOB_UP }; // event types

struct point {
	double x, y;
	point() : x(0.0), y(0.0) { }
	point(double x, double y) : x(x), y(y) { }
	// need an assigment operator?
	point operator-(const point& p) const {
		point p0(x-p.x, y-p.y);
		return p0;
	}
	point operator+(const point& p) const {
		point p0(x+p.x, y+p.y);
		return p0;
	}
	point& operator+=(const point& p) {
		this->x += p.x; this->y += p.y;
		return *this;
	}
	point& operator-=(const point& p) {
		this->x -= p.x; this->y -= p.y;
		return *this;
	}
	point& operator*=(const double s) {
		this->x *= s; this->y *= s;
		return *this;
	}
	point& operator=(const point& p) {
		this->x = p.x; this->y = p.y;
		return *this;
	}
	point operator*(const double s) const {
		point p0(x*s, y*s);
		return p0;
	}
	point operator/(const double d) const {
		point p0(x/d, y/d);
		return p0;
	}
	point rotate(const double theta) const {
		point p(x * cos(theta) - y * sin(theta), x * sin(theta) + y * cos(theta));
		return p;
	}
	double inner(const point& p) const {
		return (x * p.x + y * p.y);
	}
	double magnitude() const {
		return sqrt(x*x+y*y);
	}
};

class cBlob {
  private:

  protected:

  public:
	point location, origin;		// current location and origin for defining a drag vector
	point locationw, originw;
	point min, max;			// to define our axis-aligned bounding box
	int event;			// event type: one of BLOB_NULL, BLOB_DOWN, BLOB_MOVE, BLOB_UP
	bool tracked;			// a flag to indicate this blob has been processed
	int id;				// blob identification
	void *widget;			// pointer to previous

	std::vector<int> children;
	int height;
	
	cBlob();
	cBlob& operator=(const cBlob& b);
	bool operator<(const cBlob& b) const;
	bool contains(const cBlob& b, int i);
	
	void resetIndex();
};

#endif
