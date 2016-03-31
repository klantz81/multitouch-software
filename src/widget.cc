#include "widget.h"
#include "functors.h"

#include <iostream>



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cWidget
int cWidget::zindex = 0;

cWidget::cWidget(point translate, double rotate, double scale, rgba color) :
	translate(translate), rotate(rotate), scale(scale),
	translate_previous(point(0,0)), rotate_previous(0.0), scale_previous(1.0),
	color(color), z(zindex++) {
}

cWidget::~cWidget() {
}

void cWidget::resetEvents() {
	events.clear();
}

void cWidget::addEvent(cBlob& event) {
	events.push_back(&event);
}

void cWidget::addEvent(cBlob* event) {
	events.push_back(event);
}

void cWidget::bringForward() {
	z = zindex++;
}

void cWidget::translateWidget(const point& p) {
	translate_previous = translate_previous * 0.5 + p * 0.5;
	translate += p;
}

void cWidget::rotateWidget(const point& p0, const point& p1, const point& p2) {
	double angle = (atan2(p1.y, p1.x) - atan2(p0.y, p0.x));
	rotate_previous = rotate_previous * 0.5 + angle * 0.5;
	rotate += angle;

	point s0 = translate - p2, s1;
	s1.x = s0.x*cos(angle) - s0.y*sin(angle);
	s1.y = s0.x*sin(angle) + s0.y*cos(angle);
	translate = p2 + s1;
}

void cWidget::scaleWidget(const point& p0, const point& p1, const point& p2) {
	double s = p1.magnitude() / p0.magnitude();
	scale_previous = scale_previous * 0.5 + s * 0.5;
	scale *= s;
	
	point s0 = translate - p2, s1;
	s1 = s0*s;
	translate = p2 + s1;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cWidgetQueue
cWidgetQueue::cWidgetQueue(point translate, double rotate, double scale, rgba color) : cWidget(translate, rotate, scale, color) {
}

cWidgetQueue::~cWidgetQueue() {
}

void cWidgetQueue::registerWidget(cWidget& widget) {
	int i;
	for (i = 0; i < widgets.size(); i++) {
		if (widgets[i] == &widget) {
			break;
		}
	}
	if (i == widgets.size()) { widgets.push_back(&widget); widget.bringForward(); }
}

void cWidgetQueue::unregisterWidget(cWidget& widget) {
	for (int i = 0; i < widgets.size(); i++) {
		if (widgets[i] == &widget) {
			widgets.erase(widgets.begin() + i);
			break;
		}
	}
}

void cWidgetQueue::registerWidget(cWidget* widget) {
	int i;
	for (i = 0; i < widgets.size(); i++) {
		if (widgets[i] == widget) {
			break;
		}
	}
	if (i == widgets.size()) { widgets.push_back(widget); widget->bringForward(); }
}

void cWidgetQueue::unregisterWidget(cWidget* widget) {
	for (int i = 0; i < widgets.size(); i++) {
		if (widgets[i] == widget) {
			widgets.erase(widgets.begin() + i);
			break;
		}
	}
}

void cWidgetQueue::processEvents() {
	sort(events.begin(), events.end(), compareBlobIDp());		// process older blobs first
	sort(widgets.begin(), widgets.end(), compareWidgetsZ());	// process widgets based on zindex ordering

	for (int i = 0; i < widgets.size(); i++) widgets[i]->resetEvents();

	for (int i = 0; i < events.size(); i++) {
		events[i]->tracked = false;
	}

	for (int i = widgets.size() - 1; i > -1; i--) {
		for (int j = 0; j < events.size(); j++) {
			if (events[j]->tracked == true) continue;

			transformEvent(events[j]);
			if (widgets[i]->containsPoint(events[j]->locationw)) {
				widgets[i]->addEvent(events[j]);
				events[j]->tracked = true;
				events.erase(events.begin()+j);
				j++;

			}
		}
	}
}

void cWidgetQueue::transformEvent(cBlob* event) {
	event->originw = ((event->origin - translate)*1/scale).rotate(-rotate);
	event->locationw = ((event->location - translate)*1/scale).rotate(-rotate);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cRectangle
cRectangle::cRectangle(point translate, double rotate, double scale, point dimensions, rgba color) : cWidget(translate, rotate, scale, color), dimensions(dimensions) {
//	type = RECTANGLE;
}

cRectangle::~cRectangle() { }

bool cRectangle::containsPoint(const point& p) const {
	point p0(p.x - translate.x, p.y - translate.y);
	point p1(p0.x * cos(-rotate) + p0.y * -sin(-rotate), p0.x * sin(-rotate) + p0.y * cos(-rotate));
	return (p1.x >= -dimensions.x / 2 * scale && p1.x <= dimensions.x / 2 * scale && p1.y >= -dimensions.y / 2 * scale && p1.y <= dimensions.y / 2 * scale);
}

void cRectangle::update() {
	if (events.size() == 0 || events.size() == 1 && events[0]->event == BLOB_UP) {
		translate += translate_previous; translate_previous *= 0.9;
		rotate    += rotate_previous;    rotate_previous    *= 0.9;
	} else if (events.size() == 1) {
		if (events[0]->event == BLOB_DOWN) bringForward();
		if (events[0]->event == BLOB_MOVE) translateWidget(events[0]->location-events[0]->origin);
	} else if (events.size() > 1) {
		if (events[0]->event == BLOB_DOWN || events[1]->event == BLOB_DOWN) {
			bringForward();
			if (events[0]->event == BLOB_MOVE) {
				translateWidget(events[0]->location - events[0]->origin);
			} else if (events[1]->event == BLOB_MOVE) {
				translateWidget(events[1]->location - events[1]->origin);
			}
		} else if (events[0]->event == BLOB_MOVE && events[1]->event == BLOB_MOVE) {
			point p0 = events[1]->origin + events[0]->origin;
			point p1 = events[1]->location + events[0]->location;
			point p2 = (p1 - p0) * 0.5;
			translateWidget(p2);
			rotateWidget(events[1]->origin - events[0]->origin, events[1]->location - events[0]->location, p1/2);
			scaleWidget(events[1]->origin - events[0]->origin, events[1]->location - events[0]->location, p1/2);
		} else if (events[0]->event == BLOB_MOVE && events[1]->event == BLOB_UP) {
			translateWidget(events[0]->location-events[0]->origin);
		} else if (events[1]->event == BLOB_MOVE && events[0]->event == BLOB_UP) {
			translateWidget(events[1]->location-events[1]->origin);
		}
	}
}

void cRectangle::render() {
	glDisable(GL_TEXTURE_2D);
	glColor4f(color.r, color.g, color.b, color.a);
	glPushMatrix();
	glTranslatef(translate.x, translate.y, 0);
	glRotatef(rotate * 180 / M_PI, 0, 0, 1);
	glScalef(scale, scale, scale);
	glBegin(GL_QUADS);
	glVertex3f(-dimensions.x / 2, -dimensions.y / 2, 0.0);
	glVertex3f(-dimensions.x / 2,  dimensions.y / 2, 0.0);
	glVertex3f( dimensions.x / 2,  dimensions.y / 2, 0.0);
	glVertex3f( dimensions.x / 2, -dimensions.y / 2, 0.0);
	glEnd();
	glPopMatrix();
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cRectangleQueue
cRectangleQueue::cRectangleQueue(point translate, double rotate, double scale, point dimensions, rgba color) : cWidgetQueue(translate, rotate, scale, color), dimensions(dimensions) {
//	type = RECTANGLE;
}

cRectangleQueue::~cRectangleQueue() { }

bool cRectangleQueue::containsPoint(const point& p) const {
	point p0(p.x - translate.x, p.y - translate.y);
	point p1(p0.x * cos(-rotate) + p0.y * -sin(-rotate), p0.x * sin(-rotate) + p0.y * cos(-rotate));
	return (p1.x >= -dimensions.x / 2 * scale && p1.x <= dimensions.x / 2 * scale && p1.y >= -dimensions.y / 2 * scale && p1.y <= dimensions.y / 2 * scale);
}

void cRectangleQueue::update() {
	processEvents();

	if (events.size() == 0 || events.size() == 1 && events[0]->event == BLOB_UP) {
		translate += translate_previous; translate_previous *= 0.9;
		rotate    += rotate_previous;    rotate_previous    *= 0.9;
	} else if (events.size() == 1) {
		if (events[0]->event == BLOB_DOWN) bringForward();
		if (events[0]->event == BLOB_MOVE) translateWidget(events[0]->location-events[0]->origin);
	} else if (events.size() > 1) {
		if (events[0]->event == BLOB_DOWN || events[1]->event == BLOB_DOWN) {
			bringForward();
			if (events[0]->event == BLOB_MOVE) {
				translateWidget(events[0]->location - events[0]->origin);
			} else if (events[1]->event == BLOB_MOVE) {
				translateWidget(events[1]->location - events[1]->origin);
			}
		} else if (events[0]->event == BLOB_MOVE && events[1]->event == BLOB_MOVE) {
			point p0 = events[1]->origin + events[0]->origin;
			point p1 = events[1]->location + events[0]->location;
			point p2 = (p1 - p0) * 0.5;
			translateWidget(p2);
			rotateWidget(events[1]->origin - events[0]->origin, events[1]->location - events[0]->location, p1/2);
			scaleWidget(events[1]->origin - events[0]->origin, events[1]->location - events[0]->location, p1/2);
		} else if (events[0]->event == BLOB_MOVE && events[1]->event == BLOB_UP) {
			translateWidget(events[0]->location-events[0]->origin);
		} else if (events[1]->event == BLOB_MOVE && events[0]->event == BLOB_UP) {
			translateWidget(events[1]->location-events[1]->origin);
		}
	}

	for (int i = widgets.size() - 1; i > -1; i--) {
		widgets[i]->update();
	}
}

void cRectangleQueue::render() {
	glDisable(GL_TEXTURE_2D);
	glColor4f(color.r, color.g, color.b, color.a);
	glPushMatrix();
	glTranslatef(translate.x, translate.y, 0);
	glRotatef(rotate * 180 / M_PI, 0, 0, 1);
	glScalef(scale, scale, scale);
	glBegin(GL_QUADS);
	glVertex3f(-dimensions.x / 2, -dimensions.y / 2, 0.0);
	glVertex3f(-dimensions.x / 2,  dimensions.y / 2, 0.0);
	glVertex3f( dimensions.x / 2,  dimensions.y / 2, 0.0);
	glVertex3f( dimensions.x / 2, -dimensions.y / 2, 0.0);
	glEnd();

	for (int i = 0; i < widgets.size(); i++) {
		widgets[i]->render();
	}

	glPopMatrix();
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cTexture
cTexture::cTexture(std::string filename) : cRectangle(point(0.0, 0.0), 0.0, 1.0, point(1.0, 1.0), rgba(1.0, 1.0, 1.0, 1.0)) {
	SDL_Surface *s = IMG_Load(filename.c_str());
	dimensions.x = s->w; dimensions.y = s->h;
	setupTexture(texture, s);
	SDL_FreeSurface(s);
}

cTexture::cTexture(std::string filename, point translate, double rotate, double scale) : cRectangle(translate, rotate, scale, point(1.0, 1.0), rgba(1.0, 1.0, 1.0, 1.0)) {
	SDL_Surface *s = IMG_Load(filename.c_str());
	dimensions.x = s->w; dimensions.y = s->h;
	setupTexture(texture, s);
	SDL_FreeSurface(s);
}

cTexture::~cTexture() {
	deleteTexture(texture);
}

void cTexture::render() {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
	glColor4f(color.r, color.g, color.b, color.a);
	glPushMatrix();
	glTranslatef(translate.x, translate.y, 0);
	glRotatef(rotate * 180 / M_PI, 0, 0, 1);
	glScalef(scale, scale, scale);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(-dimensions.x / 2, -dimensions.y / 2, 0.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(-dimensions.x / 2,  dimensions.y / 2, 0.0);
	glTexCoord2f(1.0, 1.0); glVertex3f( dimensions.x / 2,  dimensions.y / 2, 0.0);
	glTexCoord2f(1.0, 0.0); glVertex3f( dimensions.x / 2, -dimensions.y / 2, 0.0);
	glEnd();
	glPopMatrix();
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cScrabble
cScrabble::cScrabble() : cRectangleQueue(point(0.0, 0.0), 0.0, 1.0, point(1.0, 1.0), rgba(1.0, 1.0, 1.0, 1.0)) {
	SDL_Surface *s = IMG_Load("media/scrabble.png");
	dimensions.x = 1928; dimensions.y = 1928;
	//dimensions.x = s->w; dimensions.y = s->h;
	setupTexture(texture, s);
	SDL_FreeSurface(s);

	tiles.push_back(new cScrabbleTile('A'));
	tiles.push_back(new cScrabbleTile('B'));
	tiles.push_back(new cScrabbleTile('C'));
	tiles.push_back(new cScrabbleTile('D'));
	tiles.push_back(new cScrabbleTile('E'));
	tiles.push_back(new cScrabbleTile('F'));
	tiles.push_back(new cScrabbleTile('G'));
	tiles.push_back(new cScrabbleTile('H'));
	tiles.push_back(new cScrabbleTile('I'));
	tiles.push_back(new cScrabbleTile('J'));
	tiles.push_back(new cScrabbleTile('L'));
	tiles.push_back(new cScrabbleTile('M'));
	tiles.push_back(new cScrabbleTile('N'));
	tiles.push_back(new cScrabbleTile('O'));
	tiles.push_back(new cScrabbleTile('P'));
	tiles.push_back(new cScrabbleTile('Q'));
	tiles.push_back(new cScrabbleTile('R'));
	tiles.push_back(new cScrabbleTile('S'));
	tiles.push_back(new cScrabbleTile('T'));
	tiles.push_back(new cScrabbleTile('U'));
	tiles.push_back(new cScrabbleTile('V'));
	tiles.push_back(new cScrabbleTile('W'));
	tiles.push_back(new cScrabbleTile('X'));
	tiles.push_back(new cScrabbleTile('Y'));
	tiles.push_back(new cScrabbleTile('Z'));
	for (int i = 0; i < tiles.size(); i++) registerWidget(tiles[i]);
}

cScrabble::cScrabble(point translate, double rotate, double scale) : cRectangleQueue(translate, rotate, scale, point(1.0, 1.0), rgba(1.0, 1.0, 1.0, 1.0)) {
	SDL_Surface *s = IMG_Load("media/scrabble.png");
	dimensions.x = 1928; dimensions.y = 1928;
	//dimensions.x = s->w; dimensions.y = s->h;
	setupTexture(texture, s);
	SDL_FreeSurface(s);

	tiles.push_back(new cScrabbleTile('T'));
	tiles.push_back(new cScrabbleTile('H'));
	tiles.push_back(new cScrabbleTile('E'));
	tiles.push_back(new cScrabbleTile('Q'));
	tiles.push_back(new cScrabbleTile('U'));
	tiles.push_back(new cScrabbleTile('I'));
	tiles.push_back(new cScrabbleTile('C'));
	tiles.push_back(new cScrabbleTile('K'));
	tiles.push_back(new cScrabbleTile('B'));
	tiles.push_back(new cScrabbleTile('R'));
	tiles.push_back(new cScrabbleTile('O'));
	tiles.push_back(new cScrabbleTile('W'));
	tiles.push_back(new cScrabbleTile('N'));
	tiles.push_back(new cScrabbleTile('F'));
	tiles.push_back(new cScrabbleTile('O'));
	tiles.push_back(new cScrabbleTile('X'));
	tiles.push_back(new cScrabbleTile('J'));
	tiles.push_back(new cScrabbleTile('U'));
	tiles.push_back(new cScrabbleTile('M'));
	tiles.push_back(new cScrabbleTile('P'));
	tiles.push_back(new cScrabbleTile('S'));
	tiles.push_back(new cScrabbleTile('O'));
	tiles.push_back(new cScrabbleTile('V'));
	tiles.push_back(new cScrabbleTile('E'));
	tiles.push_back(new cScrabbleTile('R'));
	tiles.push_back(new cScrabbleTile('T'));
	tiles.push_back(new cScrabbleTile('H'));
	tiles.push_back(new cScrabbleTile('E'));
	tiles.push_back(new cScrabbleTile('L'));
	tiles.push_back(new cScrabbleTile('A'));
	tiles.push_back(new cScrabbleTile('Z'));
	tiles.push_back(new cScrabbleTile('Y'));
	tiles.push_back(new cScrabbleTile('D'));
	tiles.push_back(new cScrabbleTile('O'));
	tiles.push_back(new cScrabbleTile('G'));

	for (int i = 0; i < tiles.size(); i++) registerWidget(tiles[i]);
}

cScrabble::~cScrabble() {
	deleteTexture(texture);
	for (int i = 0; i < tiles.size(); i++) delete tiles[i];
	tiles.clear();
}

void cScrabble::render() {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
	glColor4f(color.r, color.g, color.b, color.a);
	glPushMatrix();
	glTranslatef(translate.x, translate.y, 0);
	glRotatef(rotate * 180 / M_PI, 0, 0, 1);
	glScalef(scale, scale, scale);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.123636364); glVertex3f(-dimensions.x / 2, -dimensions.y / 2, 0.0);
	glTexCoord2f(0.0, 1.0);         glVertex3f(-dimensions.x / 2,  dimensions.y / 2, 0.0);
	glTexCoord2f(1.0, 1.0);         glVertex3f( dimensions.x / 2,  dimensions.y / 2, 0.0);
	glTexCoord2f(1.0, 0.123636364); glVertex3f( dimensions.x / 2, -dimensions.y / 2, 0.0);
	glEnd();
	
	for (int i = 0; i < widgets.size(); i++) {
		widgets[i]->render();
	}

	glPopMatrix();
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cScrabbleTile
cScrabbleTile::cScrabbleTile(unsigned char letter) : letter(letter), cRectangle(point(0.0, 0.0), 0.0, 1.0, point(1.0, 1.0), rgba(1.0, 1.0, 1.0, 1.0)) {
	dimensions.x = 136; dimensions.y = 136;
}

cScrabbleTile::cScrabbleTile(unsigned char letter, point translate, double rotate, double scale) : letter(letter), cRectangle(translate, rotate, scale, point(1.0, 1.0), rgba(1.0, 1.0, 1.0, 1.0)) {
	dimensions.x = 136; dimensions.y = 136;
}

cScrabbleTile::~cScrabbleTile() {
}

void cScrabbleTile::update() {
	if (events.size() > 0) {
		if (events[0]->event == BLOB_UP) {
			translate.x = round(events[0]->locationw.x/128.0)*128.0;
			translate.y = round(events[0]->locationw.y/128.0)*128.0;
		}
		if (events[0]->event == BLOB_DOWN) bringForward();
		if (events[0]->event == BLOB_MOVE) translateWidget(events[0]->locationw-events[0]->originw);
	} else {
			translate.x = round(translate.x/128.0)*128.0;
			translate.y = round(translate.y/128.0)*128.0;
	}
}

void cScrabbleTile::render() {
	//glEnable(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, texture);
	//glColor4f(color.r, color.g, color.b, color.a);
	glPushMatrix();
	glTranslatef(translate.x, translate.y, 0);
	//glRotatef(rotate * 180 / M_PI, 0, 0, 1);
	//glScalef(scale, scale, scale);
	float x0 = ((letter - 65) % 13) * 0.070539419, y0 = floor((letter - 65) / 13.0) * 0.061818182;
	float x1 = x0 + 0.070539419, y1 = y0 + 0.061818182;
	glBegin(GL_QUADS);
	glTexCoord2f(x0, y0); glVertex3f(-dimensions.x / 2, -dimensions.y / 2, 0.0);
	glTexCoord2f(x0, y1); glVertex3f(-dimensions.x / 2,  dimensions.y / 2, 0.0);
	glTexCoord2f(x1, y1); glVertex3f( dimensions.x / 2,  dimensions.y / 2, 0.0);
	glTexCoord2f(x1, y0); glVertex3f( dimensions.x / 2, -dimensions.y / 2, 0.0);
	glEnd();
	glPopMatrix();

/*	if (events.size() > 0) {
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_LINES);
		glVertex3f(events[0]->originw.x, events[0]->originw.y, 0);
		glVertex3f(events[0]->locationw.x, events[0]->locationw.y, 0);
		glEnd();
		renderCircle(events[0]->locationw.x, events[0]->locationw.y, 16, 30, 1.0, 1.0, 1.0);
	}*/
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cTextureBackground
cTextureBackground::cTextureBackground(std::string filename) : cTexture(filename) {
}

cTextureBackground::cTextureBackground(std::string filename, point translate, double rotate, double scale) :
	cTexture(filename, translate, rotate, scale) {
}

cTextureBackground::~cTextureBackground() {
}

void cTextureBackground::update() {
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cTextureBorder
cTextureBorder::cTextureBorder(std::string filename, std::string corners) : cTexture(filename) {
	SDL_Surface *s = IMG_Load(corners.c_str());
	setupTexture(this->corners, s);
	SDL_FreeSurface(s);
}

cTextureBorder::cTextureBorder(std::string filename, std::string corners, point translate, double rotate, double scale) :
	cTexture(filename, translate, rotate, scale) {
	SDL_Surface *s = IMG_Load(corners.c_str());
	setupTexture(this->corners, s);
	SDL_FreeSurface(s);
}

cTextureBorder::~cTextureBorder() {
	deleteTexture(corners);
}

void cTextureBorder::renderBorders() {
	glDisable(GL_TEXTURE_2D);
	glColor4f(color.r, color.g, color.b, color.a * 0.7);
	glBegin(GL_QUADS);
	glVertex3f(-dimensions.x / 2, -dimensions.y / 2 + 36, 0.0);
	glVertex3f(-dimensions.x / 2, dimensions.y / 2 - 36, 0.0);
	glVertex3f(-dimensions.x / 2 + 36, dimensions.y / 2 - 36, 0.0);
	glVertex3f(-dimensions.x / 2 + 36, -dimensions.y / 2 + 36, 0.0);
	glVertex3f(-dimensions.x / 2 + 36, dimensions.y / 2 - 36, 0.0);
	glVertex3f(-dimensions.x / 2 + 36, dimensions.y / 2, 0.0);
	glVertex3f( dimensions.x / 2 - 36, dimensions.y / 2, 0.0);
	glVertex3f( dimensions.x / 2 - 36, dimensions.y / 2 - 36, 0.0);
	glVertex3f(dimensions.x / 2 - 36, -dimensions.y / 2 + 36, 0.0);
	glVertex3f(dimensions.x / 2 - 36, dimensions.y / 2 - 36, 0.0);
	glVertex3f(dimensions.x / 2, dimensions.y / 2 - 36, 0.0);
	glVertex3f(dimensions.x / 2, -dimensions.y / 2 + 36, 0.0);
	glVertex3f(-dimensions.x / 2 + 36, -dimensions.y / 2, 0.0);
	glVertex3f(-dimensions.x / 2 + 36, -dimensions.y / 2 + 36, 0.0);
	glVertex3f( dimensions.x / 2 - 36, -dimensions.y / 2 + 36, 0.0);
	glVertex3f( dimensions.x / 2 - 36, -dimensions.y / 2, 0.0);
	glEnd();


	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, corners);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(-dimensions.x / 2, -dimensions.y / 2, 0.0);
	glTexCoord2f(0.0, 0.5); glVertex3f(-dimensions.x / 2, -dimensions.y / 2 + 36, 0.0);
	glTexCoord2f(0.5, 0.5); glVertex3f(-dimensions.x / 2 + 36, -dimensions.y / 2 + 36, 0.0);
	glTexCoord2f(0.5, 0.0); glVertex3f(-dimensions.x / 2 + 36, -dimensions.y / 2, 0.0);
	glTexCoord2f(0.0, 0.5); glVertex3f(-dimensions.x / 2, dimensions.y / 2 - 36, 0.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(-dimensions.x / 2, dimensions.y / 2, 0.0);
	glTexCoord2f(0.5, 1.0); glVertex3f(-dimensions.x / 2 + 36, dimensions.y / 2, 0.0);
	glTexCoord2f(0.5, 0.5); glVertex3f(-dimensions.x / 2 + 36, dimensions.y / 2 - 36, 0.0);
	glTexCoord2f(0.5, 0.5); glVertex3f(dimensions.x / 2 - 36, dimensions.y / 2 - 36, 0.0);
	glTexCoord2f(0.5, 1.0); glVertex3f(dimensions.x / 2 - 36, dimensions.y / 2, 0.0);
	glTexCoord2f(1.0, 1.0); glVertex3f(dimensions.x / 2, dimensions.y / 2, 0.0);
	glTexCoord2f(1.0, 0.5); glVertex3f(dimensions.x / 2, dimensions.y / 2 - 36, 0.0);
	glTexCoord2f(0.5, 0.0); glVertex3f(dimensions.x / 2 - 36, -dimensions.y / 2, 0.0);
	glTexCoord2f(0.5, 0.5); glVertex3f(dimensions.x / 2 - 36, -dimensions.y / 2 + 36, 0.0);
	glTexCoord2f(1.0, 0.5); glVertex3f(dimensions.x / 2, -dimensions.y / 2 + 36, 0.0);
	glTexCoord2f(1.0, 0.0); glVertex3f(dimensions.x / 2, -dimensions.y / 2, 0.0);
	glEnd();
}

void cTextureBorder::render() {
	glPushMatrix();
	glTranslatef(translate.x, translate.y, 0);
	glRotatef(rotate * 180 / M_PI, 0, 0, 1);
	glScalef(scale, scale, scale);

	renderBorders();

	glColor4f(color.r, color.g, color.b, color.a);
	glBindTexture(GL_TEXTURE_2D, texture);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(-dimensions.x / 2 + 36, -dimensions.y / 2 + 36, 0.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(-dimensions.x / 2 + 36,  dimensions.y / 2 - 36, 0.0);
	glTexCoord2f(1.0, 1.0); glVertex3f( dimensions.x / 2 - 36,  dimensions.y / 2 - 36, 0.0);
	glTexCoord2f(1.0, 0.0); glVertex3f( dimensions.x / 2 - 36, -dimensions.y / 2 + 36, 0.0);
	glEnd();

	glPopMatrix();
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cTextureVideo
const int VIDEOWIDTH = 512; const int VIDEOHEIGHT = 304;

static void* lock(void *data, void **p_pixels) {
    context *ctx = (context *)data;
    SDL_LockMutex(ctx->mutex);
    SDL_LockSurface(ctx->surf);
    *p_pixels = ctx->surf->pixels;
    return NULL; /* picture identifier, not needed here */
}

static void unlock(void *data, void *id, void *const *p_pixels) {
	context *ctx = (context *)data;

    /* VLC just rendered the video, but we can also render stuff 
    uint16_t *pixels = *p_pixels;
    int x, y;

    for(y = 10; y < 40; y++)
        for(x = 10; x < 40; x++)
            if(x < 13 || y < 13 || x > 36 || y > 36)
                pixels[y * VIDEOWIDTH + x] = 0xffff;
            else
                pixels[y * VIDEOWIDTH + x] = 0x0;
    */

	SDL_UnlockSurface(ctx->surf);
	SDL_UnlockMutex(ctx->mutex);
}

static void display(void *data, void *id) {
	/* VLC wants to display the video */
	(void) data;
}

cTextureVideo::cTextureVideo(std::string filename, std::string corners, std::string video) : cTextureBorder(filename, corners) {
	ctx.surf = SDL_CreateRGBSurface(SDL_SWSURFACE, VIDEOWIDTH, VIDEOHEIGHT, 32, 0x001f, 0x07e0, 0xf800, 0);
	ctx.mutex = SDL_CreateMutex();
	// add here
}

cTextureVideo::cTextureVideo(std::string filename, std::string corners, std::string video, point translate, double rotate, double scale) :
	cTextureBorder(filename, corners, translate, rotate, scale) {
	ctx.surf = SDL_CreateRGBSurface(SDL_SWSURFACE, VIDEOWIDTH, VIDEOHEIGHT, 32, 0x001f, 0x07e0, 0xf800, 0);
	ctx.mutex = SDL_CreateMutex();

	char const *vlc_argv[] = {
	    //"--no-audio", /* skip any audio track */
	    "--no-xlib", /* tell VLC to not use Xlib */
	};
	int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);
    
	libvlc = libvlc_new(vlc_argc, vlc_argv);
	m = libvlc_media_new_path(libvlc, video.c_str());
	mp = libvlc_media_player_new_from_media(m);
	libvlc_media_release(m);

	libvlc_video_set_callbacks(mp, lock, unlock, display, &ctx);
	libvlc_video_set_format(mp, "RV32", VIDEOWIDTH, VIDEOHEIGHT, VIDEOWIDTH*4);
	libvlc_media_player_play(mp);
}

cTextureVideo::~cTextureVideo() {
	libvlc_media_player_stop(mp);
	libvlc_media_player_release(mp);
	libvlc_release(libvlc);
    
	SDL_DestroyMutex(ctx.mutex);
	SDL_FreeSurface(ctx.surf);
}

void cTextureVideo::render() {
	if (!libvlc_media_player_is_playing(mp)) { libvlc_media_player_stop(mp); libvlc_media_player_play(mp); }

        SDL_LockMutex(ctx.mutex);
	updateTexture(texture, ctx.surf);
        //SDL_BlitSurface(ctx.surf, NULL, screen, &rect);
        SDL_UnlockMutex(ctx.mutex);

	glPushMatrix();
        glTranslatef(translate.x, translate.y, 0);
        glRotatef(rotate * 180 / M_PI, 0, 0, 1);
        glScalef(scale, scale, scale);

	renderBorders();
	
        glColor4f(color.r, color.g, color.b, color.a);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0, 0.0); glVertex3f(-dimensions.x / 2 + 36, -dimensions.y / 2 + 36, 0.0);
        glTexCoord2f(0.0, 1.0); glVertex3f(-dimensions.x / 2 + 36,  dimensions.y / 2 - 36, 0.0);
        glTexCoord2f(1.0, 1.0); glVertex3f( dimensions.x / 2 - 36,  dimensions.y / 2 - 36, 0.0);
        glTexCoord2f(1.0, 0.0); glVertex3f( dimensions.x / 2 - 36, -dimensions.y / 2 + 36, 0.0);
        glEnd();

        glPopMatrix();
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cTexture
cCalibration::cCalibration(std::string filename, cTracker& tracker, cTracker2& tracker2, int h_points, int v_points) :
	cTexture(filename, point(0.0, 0.0), 0.0, 1.0), count(0), tracker(&tracker), tracker2(&tracker2), calibration_points(NULL), h_points(h_points), v_points(v_points), calibrated(false) {
	translate += dimensions/2;
	calibration_points = new point[h_points * v_points];
}

cCalibration::~cCalibration() {
	if (calibration_points) delete [] calibration_points;
}

bool cCalibration::isCalibrated() {
	return calibrated;
}

void cCalibration::reset() {
	bringForward();
	count = 0; calibrated = false;
}

void cCalibration::update() {
	if (calibrated) return;
	if (events.size() == 1 && events[0]->event == BLOB_DOWN) {
		calibration_points[count++] = events[0]->location;
	}
	if (count == h_points * v_points) {
		calibrated = true;
		tracker->updateCalibration(calibration_points, h_points, v_points);
		tracker2->updateCalibration(calibration_points, h_points, v_points);
	}
}

void cCalibration::render() {
	if (calibrated) return;
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
	glColor4f(color.r, color.g, color.b, 1.0);
	glPushMatrix();
	glTranslatef(translate.x, translate.y, 0);
	glRotatef(rotate * 180 / M_PI, 0, 0, 1);
	glScalef(scale, scale, scale);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(-dimensions.x / 2, -dimensions.y / 2, 0.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(-dimensions.x / 2,  dimensions.y / 2, 0.0);
	glTexCoord2f(1.0, 1.0); glVertex3f( dimensions.x / 2,  dimensions.y / 2, 0.0);
	glTexCoord2f(1.0, 0.0); glVertex3f( dimensions.x / 2, -dimensions.y / 2, 0.0);
	glEnd();
	int x = count % h_points, y = count / h_points;
	double x0 = dimensions.x / (h_points - 1) * x - dimensions.x / 2, y0 = dimensions.y / (v_points - 1) * y - dimensions.y / 2;
	//void renderCircle(float x, float y, float rad, int deg, GLfloat r, GLfloat g, GLfloat b);
	renderCircle(x0, y0, 16, 30, 1, 1, 1);
	glPopMatrix();
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// cFluid
const int GSR = 10;

cFluid::cFluid(point grid_dimensions, point dimensions, point translate, double rotate, double scale) :
	cRectangle(translate, rotate, scale, dimensions, rgba(1.0, 1.0, 1.0, 1.0)), grid_dimensions(grid_dimensions) {
	int size = (int)grid_dimensions.x * (int)grid_dimensions.y;
	u = new double[size];
	v = new double[size];
	u_prev = new double[size];
	v_prev = new double[size];
	d0 = new double[size];
	d_prev0 = new double[size];
	d1 = new double[size];
	d_prev1 = new double[size];
	d2 = new double[size];
	d_prev2 = new double[size];
	curl = new double[size];
	for (int i = 0; i < size; i++) u[i] = v[i] = u_prev[i] = v_prev[i] = d0[i] = d_prev0[i] = d1[i] = d_prev1[i] = d2[i] = d_prev2[i] = 0.0;
}

cFluid::~cFluid() {
	delete [] u;
	delete [] v;
	delete [] u_prev;
	delete [] v_prev;
	delete [] d0;
	delete [] d_prev0;
	delete [] d1;
	delete [] d_prev1;
	delete [] d2;
	delete [] d_prev2;
	delete [] curl;
}

void cFluid::add_source(double* x, double* s, double dt) {
	for (int i = 0; i < (int)grid_dimensions.x * (int)grid_dimensions.y; i++) x[i] += s[i] * dt;
}

void cFluid::diffuse(int b, double* x, double* x0, double diff, double dt) {
	int i, j, k; double a = (grid_dimensions.x - 2) * (grid_dimensions.y - 2) * diff * dt;
	for (k = 0; k < GSR; k++) {
		for (i = 1; i < grid_dimensions.x - 1; i++) {
			for (j = 1; j < grid_dimensions.y - 1; j++) {
				x[j * (int)grid_dimensions.x + i] = (x0[j * (int)grid_dimensions.x + i] +
								a * (x[j * (int)grid_dimensions.x + i - 1] + x[j * (int)grid_dimensions.x + i + 1] + x[(j - 1) * (int)grid_dimensions.x + i] + x[(j + 1) * (int)grid_dimensions.x + i])
							  ) / (1 + 4 * a);
			}
		}
		set_bnd(b, x);
	}
}

void cFluid::advect(int b, double* d, double* d0, double* u, double* v, double dt) {
	int i, j, i0, j0, i1, j1;
	double x, y, s0, t0, s1, t1, dt0, dt1;
	dt0 = (grid_dimensions.x - 2) * dt;
	dt1 = (grid_dimensions.y - 2) * dt;
	for (i = 1; i < grid_dimensions.x - 1; i++) {
		for (j = 1; j < grid_dimensions.y - 1; j++) {
			x = i - dt0 * u[j * (int)grid_dimensions.x + i]; y = j - dt1 * v[j * (int)grid_dimensions.x + i];
			if (x < 0.5) x = 0.5; if (x > grid_dimensions.x - 1.5) x = grid_dimensions.x - 1.5; i0 = (int)x; i1 = i0 + 1;
			if (y < 0.5) y = 0.5; if (y > grid_dimensions.y - 1.5) y = grid_dimensions.y - 1.5; j0 = (int)y; j1 = j0 + 1;
			s1 = x - i0; s0 = 1 - s1; t1 = y - j0; t0 = 1 - t1;
			d[j * (int)grid_dimensions.x + i] = s0 * (t0 * d0[j0 * (int)grid_dimensions.x + i0] + t1 * d0[j1 * (int)grid_dimensions.x + i0]) +
							    s1 * (t0 * d0[j0 * (int)grid_dimensions.x + i1] + t1 * d0[j1 * (int)grid_dimensions.x + i1]);
		}
	}
	set_bnd(b, d);
}

void cFluid::dens_step(double* x, double* x0, double* u, double* v, double diff, double dt) {
	double *temp;

	add_source(x, x0, dt);

	temp = x; x = x0; x0 = temp;
	diffuse(0, x, x0, diff, dt);

	temp = x; x = x0; x0 = temp;
	advect(0, x, x0, u, v, dt);
}

void cFluid::vorticity_confinement(double* u, double* v, double* u0, double* v0) {
	int i,j;
	for (i = 1; i < grid_dimensions.x - 1; i++) {
		for (j = 1; j < grid_dimensions.y - 1; j++) {
			curl[j*(int)grid_dimensions.x+i] =
				(u[(j+1)*(int)grid_dimensions.x+i]-u[(j-1)*(int)grid_dimensions.x+i])*0.5 -
				(v[j*(int)grid_dimensions.x+i+1]-v[j*(int)grid_dimensions.x+i-1])*0.5;
		}
	}
	double dw_dx; double dw_dy; double length;
	for (i = 2; i < grid_dimensions.x - 2; i++) {
		for (j = 2; j < grid_dimensions.y - 2; j++) {
			dw_dx = (abs(curl[j*(int)grid_dimensions.x+i+1])-abs(curl[j*(int)grid_dimensions.x+i-1]))*0.5;
			dw_dy = (abs(curl[(j+1)*(int)grid_dimensions.x+i])-abs(curl[(j-1)*(int)grid_dimensions.x+i]))*0.5;
			length = sqrt(dw_dx*dw_dx+dw_dy*dw_dy)+0.000001;
			dw_dx /= length; dw_dy /= length;
			u0[j*(int)grid_dimensions.x+i] = (dw_dy) * -curl[j*(int)grid_dimensions.x+i];
			v0[j*(int)grid_dimensions.x+i] = (dw_dx) *  curl[j*(int)grid_dimensions.x+i];
		}
	}
}

void cFluid::vel_step(double* u, double* v, double* u0, double* v0, double visc, double dt) {
	double *temp;

	add_source(u, u0, dt);
	add_source(v, v0, dt);
	
	vorticity_confinement(u, v, u0, v0);
	add_source(u, u0, dt);
	add_source(v, v0, dt);

	temp = u; u = u0; u0 = temp;
	temp = v; v = v0; v0 = temp;
	diffuse(1, u, u0, visc, dt);
	diffuse(2, v, v0, visc, dt);
	project(u, v, u0, v0);

	temp = u; u = u0; u0 = temp;
	temp = v; v = v0; v0 = temp;
	advect(1, u, u0, u0, v0, dt);
	advect(2, v, v0, u0, v0, dt);
	project(u, v, u0, v0);
}

void cFluid::project(double* u, double* v, double* p, double* div) {
	int i, j, k;
	double h0 = 1.0 / grid_dimensions.x;
	double h1 = 1.0 / grid_dimensions.y;
	for (i = 1; i < (int)grid_dimensions.x - 1; i++) {
		for (j = 1; j < (int)grid_dimensions.y - 1; j++) {
			div[j * (int)grid_dimensions.x + i] = -0.5 * h0 * (u[j * (int)grid_dimensions.x + i + 1] - u[j * (int)grid_dimensions.x + i - 1])
							      -0.5 * h1 * (v[(j + 1) * (int)grid_dimensions.x + i] - v[(j - 1) * (int)grid_dimensions.x + i]);
			p[j * (int)grid_dimensions.x + i] = 0;
		}
	}
	set_bnd(0, div); set_bnd(0, p);
	for (k = 0; k < GSR; k++) {
		for (i = 1; i < (int)grid_dimensions.x - 1; i++) {
			for (j = 1; j< (int)grid_dimensions.y - 1; j++) {
				p[j * (int)grid_dimensions.x + i] = (div[j * (int)grid_dimensions.x + i] + p[j * (int)grid_dimensions.x + i - 1] + p[j * (int)grid_dimensions.x + i + 1] + p[(j - 1) * (int)grid_dimensions.x + i] + p[(j + 1) * (int)grid_dimensions.x + i])/4;
			}
		}
		set_bnd(0, p);
	}
	for (i = 1; i < (int)grid_dimensions.x - 1; i++) {
		for (j = 1; j < (int)grid_dimensions.y - 1; j++) {
			u[j * (int)grid_dimensions.x + i] -= 0.5 * (p[j * (int)grid_dimensions.x + i + 1] - p[j * (int)grid_dimensions.x + i - 1])/h0;
			v[j * (int)grid_dimensions.x + i] -= 0.5 * (p[(j + 1) * (int)grid_dimensions.x + i] - p[(j - 1) * (int)grid_dimensions.x + i])/h1;
		}
	}
	set_bnd(1, u); set_bnd(2, v);
}

void cFluid::set_bnd(int b, double* x) {
	for (int i = 1; i < (int)grid_dimensions.x - 1; i++) {
		x[i] = b == 2 ? -x[(int)grid_dimensions.x + i] : x[(int)grid_dimensions.x + i];
		x[(int)(grid_dimensions.y - 1) * (int)grid_dimensions.x + i] = b == 2 ? -x[(int)(grid_dimensions.y - 2) * (int)grid_dimensions.x + i] : x[(int)(grid_dimensions.y - 2) * (int)grid_dimensions.x + i];
	}
	for (int j = 1; j < (int)grid_dimensions.y - 1; j++) {
		x[j * (int)grid_dimensions.x] = b == 1 ? -x[j * (int)grid_dimensions.x + 1] : x[j * (int)grid_dimensions.x + 1];
		x[j * (int)grid_dimensions.x + (int)(grid_dimensions.x - 1)] = b == 1 ? -x[j * (int)grid_dimensions.x + (int)(grid_dimensions.x - 2)] : x[j * (int)grid_dimensions.x + (int)(grid_dimensions.x - 2)];
	}
	x[0] = 0.5 * (x[1] + x[(int)grid_dimensions.x]);
	x[(int)(grid_dimensions.y - 1) * (int)grid_dimensions.x] = 0.5 * (x[(int)(grid_dimensions.y - 2) * (int)grid_dimensions.x] + x[(int)(grid_dimensions.y - 1) * (int)grid_dimensions.x + 1]);
	x[(int)(grid_dimensions.x - 1)] = 0.5 * (x[(int)(grid_dimensions.x - 2)] + x[(int)(grid_dimensions.x) + (int)(grid_dimensions.x - 1)]);
	x[(int)(grid_dimensions.y - 1) * (int)grid_dimensions.x + (int)grid_dimensions.x - 1] = 0.5 * (x[(int)(grid_dimensions.y - 1) * (int)grid_dimensions.x + (int)grid_dimensions.x - 2] + x[(int)(grid_dimensions.y - 2) * (int)grid_dimensions.x + (int)grid_dimensions.x - 1]);
}

void cFluid::update() {
	double visc = 0.000015, dt = 0.04, diff = 0.000001;
	static int offset = -1;
	offset = offset == 359 ? 0 : offset + 1;
	double hp = offset/60.0;
	double val = 1;
	double s = 1;
	double c = val * s;
	double x = c * (1 - abs(hp/2.0-(int)(hp/2.0)-1));
	double r, g, b;
	if (0 <= hp && hp < 1) {
	  r = c; g = x; b = 0;
	} else if (1 <= hp && hp < 2) {
	  r = x; g = c; b = 0;
	} else if (2 <= hp && hp < 3) {
	  r = 0; g = c; b = x;
	} else if (3 <= hp && hp < 4) {
	  r = 0; g = x; b = c;
	} else if (4 <= hp && hp < 5) {
	  r = x; g = 0; b = c;
	} else if (5 <= hp && hp < 6) {
	  r = c; g = 0; b = x;
	}
	double m = val-c;
	r += m; g += m; b += m;
	
	particle p0;
	for (int i = 0; i < events.size(); i++) {
		if (events[i]->event == BLOB_MOVE) {
			point v = events[i]->location - events[i]->origin; v = v/(v.magnitude() + 0.000001);
			point l = events[i]->location;
			l.x = (l.x/dimensions.x*grid_dimensions.x);
			l.y = (l.y/dimensions.y*grid_dimensions.y);

			for (int p = 0; p < 10; p++) {
				p0.location.x = l.x+(rand()%41-20)/5.0;
				p0.location.y = l.y+(rand()%41-20)/5.0;
				p0.life = 1.0;
				particles.push_back(p0);
			}
			
			u_prev[(int)l.y * (int)grid_dimensions.x + (int)l.x] = v.x*100;
			v_prev[(int)l.y * (int)grid_dimensions.x + (int)l.x] = v.y*100;
			d_prev0[(int)l.y * (int)grid_dimensions.x + (int)l.x] = 200*r;
			d_prev1[(int)l.y * (int)grid_dimensions.x + (int)l.x] = 200*g;
			d_prev2[(int)l.y * (int)grid_dimensions.x + (int)l.x] = 200*b;
		}
	}

	vel_step(u, v, u_prev, v_prev, visc, dt);
	dens_step(d0, d_prev0, u, v, diff, dt);
	dens_step(d1, d_prev1, u, v, diff, dt);
	dens_step(d2, d_prev2, u, v, diff, dt);
	for (int i = 0; i < (int)grid_dimensions.x*(int)grid_dimensions.y; i++) {
		u_prev[i] = v_prev[i] = 0;
		d_prev0[i] = d_prev1[i] = d_prev2[i] = 0;
		d0[i] *= 0.98;
		d1[i] *= 0.98;
		d2[i] *= 0.98;
	}
}

void cFluid::render() {
	glDisable(GL_TEXTURE_2D);
	glColor4f(color.r, color.g, color.b, color.a);
        glPushMatrix();
        glTranslatef(translate.x, translate.y, 0);
        glRotatef(rotate * 180 / M_PI, 0, 0, 1);
        glScalef(scale, scale, scale);

	float x0, y0, x1, y1, s0, s1, t0, t1; int i0, i1, j0, j1;

	glBegin(GL_QUADS); double scale = 1.0;
	for (int j = 0; j < (int)grid_dimensions.y-1; j++) {
		for (int i = 0; i < (int)grid_dimensions.x-1; i++) {
			x0 = (i-(grid_dimensions.x-1)/2)/(grid_dimensions.x-1)*dimensions.x;
			y0 = (j-(grid_dimensions.y-1)/2)/(grid_dimensions.y-1)*dimensions.y;
			x1 = d0[j*(int)grid_dimensions.x+i]/scale;
			s1 = d1[j*(int)grid_dimensions.x+i]/scale;
			t1 = d2[j*(int)grid_dimensions.x+i]/scale;
			if (x1 > 1) x1 = 1; if (s1 > 1) s1 = 1; if (t1 > 1) t1 = 1;
			glColor4f(color.r*x1, color.g*s1, color.b*t1, color.a);
			glVertex3f(x0, y0, 0.0);

			x0 = ((i+1)-(grid_dimensions.x-1)/2)/(grid_dimensions.x-1)*dimensions.x;
			y0 = (j-(grid_dimensions.y-1)/2)/(grid_dimensions.y-1)*dimensions.y;
			x1 = d0[j*(int)grid_dimensions.x+i+1]/scale;
			s1 = d1[j*(int)grid_dimensions.x+i+1]/scale;
			t1 = d2[j*(int)grid_dimensions.x+i+1]/scale;
			if (x1 > 1) x1 = 1; if (s1 > 1) s1 = 1; if (t1 > 1) t1 = 1;
			glColor4f(color.r*x1, color.g*s1, color.b*t1, color.a);
			glVertex3f(x0, y0, 0.0);

			x0 = ((i+1)-(grid_dimensions.x-1)/2)/(grid_dimensions.x-1)*dimensions.x;
			y0 = ((j+1)-(grid_dimensions.y-1)/2)/(grid_dimensions.y-1)*dimensions.y;
			x1 = d0[(j+1)*(int)grid_dimensions.x+i+1]/scale;
			s1 = d1[(j+1)*(int)grid_dimensions.x+i+1]/scale;
			t1 = d2[(j+1)*(int)grid_dimensions.x+i+1]/scale;
			if (x1 > 1) x1 = 1; if (s1 > 1) s1 = 1; if (t1 > 1) t1 = 1;
			glColor4f(color.r*x1, color.g*s1, color.b*t1, color.a);
			glVertex3f(x0, y0, 0.0);

			x0 = (i-(grid_dimensions.x-1)/2)/(grid_dimensions.x-1)*dimensions.x;
			y0 = ((j+1)-(grid_dimensions.y-1)/2)/(grid_dimensions.y-1)*dimensions.y;
			x1 = d0[(j+1)*(int)grid_dimensions.x+i]/scale;
			s1 = d1[(j+1)*(int)grid_dimensions.x+i]/scale;
			t1 = d2[(j+1)*(int)grid_dimensions.x+i]/scale;
			if (x1 > 1) x1 = 1; if (s1 > 1) s1 = 1; if (t1 > 1) t1 = 1;
			glColor4f(color.r*x1, color.g*s1, color.b*t1, color.a);
			glVertex3f(x0, y0, 0.0);
		}
	}
        glEnd();
	

	glBegin(GL_POINTS);
	for (int i = 0; i < particles.size(); i++) {
		particles[i].life *= 0.98;
		if (particles[i].life < 0.01) { particles.erase(particles.begin()+i); i--; continue; }
		i0 = (int)particles[i].location.x; i1 = i0 + 1;
		j0 = (int)particles[i].location.y; j1 = j0 + 1;
		if (i0 < 0) i0 = 0;
		if (j0 < 0) j0 = 0;
		if (i1 < 1) i1 = 1;
		if (j1 < 1) j1 = 1;
		if (i0 > grid_dimensions.x - 2) i0 = grid_dimensions.x - 2;
		if (j0 > grid_dimensions.y - 2) j0 = grid_dimensions.y - 2;
		if (i1 > grid_dimensions.x - 1) i1 = grid_dimensions.x - 1;
		if (j1 > grid_dimensions.y - 1) j1 = grid_dimensions.y - 1;
		s0 = particles[i].location.x - i0; s1 = 1 - s0;
		t0 = particles[i].location.y - j0; t1 = 1 - t0;
		x0 = s0 * t0 * u[j1 * (int)grid_dimensions.x + i1] +
		     s0 * t1 * u[j0 * (int)grid_dimensions.x + i1] +
		     s1 * t0 * u[j1 * (int)grid_dimensions.x + i0] +
		     s1 * t1 * u[j0 * (int)grid_dimensions.x + i0];
		y0 = s0 * t0 * v[j1 * (int)grid_dimensions.x + i1] +
		     s0 * t1 * v[j0 * (int)grid_dimensions.x + i1] +
		     s1 * t0 * v[j1 * (int)grid_dimensions.x + i0] +
		     s1 * t1 * v[j0 * (int)grid_dimensions.x + i0];

		x1 = (particles[i].location.x-(grid_dimensions.x-1)/2)/(grid_dimensions.x-1)*dimensions.x;
		y1 = (particles[i].location.y-(grid_dimensions.y-1)/2)/(grid_dimensions.y-1)*dimensions.y;

		particles[i].location.x += x0*2;
		particles[i].location.y += y0*2;

		x0 = (particles[i].location.x-(grid_dimensions.x-1)/2)/(grid_dimensions.x-1)*dimensions.x;
		y0 = (particles[i].location.y-(grid_dimensions.y-1)/2)/(grid_dimensions.y-1)*dimensions.y;
		
		glColor4f(color.r*particles[i].life, color.g*particles[i].life, color.b*particles[i].life, color.a*particles[i].life);	  
		glVertex3f(x0, y0, 0.0);
		glVertex3f(x1, y1, 0.0);
	}
	glEnd();
	

        glPopMatrix();
}