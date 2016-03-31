#include "blob.h"

cBlob::cBlob() : location(point(0,0)), origin(point(0,0)), min(point(0,0)), max(point(0,0)), event(BLOB_NULL), tracked(false), id(0), widget(NULL), children(), height(0) { }

cBlob& cBlob::operator=(const cBlob& b) {
	this->location = b.location; this->origin = b.origin; this->min = b.min; this->max = b.max;
	this->event = b.event; this->tracked = b.tracked; this->id = b.id; this->widget = b.widget;
	this->children = b.children;
	this->height = b.height;
}

bool cBlob::operator<(const cBlob& b) const {
	return ( (max.x-min.x)*(max.y-min.y) < (b.max.x-b.min.x)*(b.max.y-b.min.y) );
}

bool cBlob::contains(const cBlob& b, int i) {
	double radius0 = (max.x-min.x)/2 > (max.y-min.y)/2 ? (max.x-min.x)/2 : (max.y-min.y)/2;
	double radius1 = (b.max.x-b.min.x)/2 > (b.max.y-b.min.y)/2 ? (b.max.x-b.min.x)/2 : (b.max.y-b.min.y)/2;

	double dist = sqrt(pow(b.location.x-location.x,2.0) + pow(b.location.y-location.y,2.0));

	if (dist < radius0 - radius1) {
		children.push_back(i);
		if (!(b.height < height)) height = b.height + 1;
		return true;
	}
	return false;
}