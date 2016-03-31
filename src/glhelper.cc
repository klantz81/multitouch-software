#include "glhelper.h"

void setupOrtho(int width, int height) {
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void setupTexture(GLuint& texture) {
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void setupTexture(GLuint& texture, SDL_Surface *s) {
	setupTexture(texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s->w, s->h, 0, s->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, s->pixels);
	//gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, s->w, s->h, GL_RGBA, GL_UNSIGNED_BYTE, s->pixels);
}

void deleteTexture(GLuint& texture) {
	glDeleteTextures(1, &texture);
}

void renderTexture(GLuint texture, cv::Mat& ret, bool luminance, int width, int height) {
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ret.cols, ret.rows, 0, luminance ? GL_LUMINANCE : GL_BGR, GL_UNSIGNED_BYTE, ret.data);
	//gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, ret.cols, ret.rows, luminance ? GL_LUMINANCE : GL_BGR, GL_UNSIGNED_BYTE, ret.data);
	glBegin(GL_QUADS);
	glTexCoord3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f,  0.0f,   0.0f);
	glTexCoord3f(1.0f, 0.0f, 0.0f); glVertex3f(width, 0.0f,   0.0f);
	glTexCoord3f(1.0f, 1.0f, 0.0f); glVertex3f(width, height, 0.0f);
	glTexCoord3f(0.0f, 1.0f, 0.0f); glVertex3f(0.0f,  height, 0.0f);
	glEnd();
}

void renderBlobs(std::vector<cBlob>& blobs, GLfloat r, GLfloat g, GLfloat b, GLfloat drag_r, GLfloat drag_g, GLfloat drag_b) {
	glDisable(GL_TEXTURE_2D);
	for (int i = 0; i < blobs.size(); i++) {
		glColor4f(r, g, b, 1.0f);
		glBegin(GL_LINE_LOOP);
		glVertex3f(blobs[i].min.x, blobs[i].min.y, 0.0f);
		glVertex3f(blobs[i].min.x, blobs[i].max.y, 0.0f);
		glVertex3f(blobs[i].max.x, blobs[i].max.y, 0.0f);
		glVertex3f(blobs[i].max.x, blobs[i].min.y, 0.0f);
		glEnd();
		glColor4f(drag_r, drag_g, drag_b, 1.0f);
		glBegin(GL_LINES);
		glVertex3f(blobs[i].origin.x,   blobs[i].origin.y,   0.0f);
		glVertex3f(blobs[i].location.x, blobs[i].location.y, 0.0f);
		glEnd();
	}
}

void renderCircle(float x, float y, float rad, int deg, GLfloat r, GLfloat g, GLfloat b) {
	glDisable(GL_TEXTURE_2D);
	glColor4f(r, g, b, 1.0f);
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < 360; i+= deg) glVertex3f(x+cos(i/180.0*M_PI)*rad, y+sin(i/180.0*M_PI)*rad, 0);
	glEnd();
}

void renderText(const TTF_Font *font, const GLubyte& r, const GLubyte& g, const GLubyte& b, const double& x,  const double& y, const double& z,  const std::string& text) {
	SDL_Color color = {r, g, b};
	//SDL_Surface *Message = TTF_RenderText_Solid(const_cast<TTF_Font*>(font), text.c_str(), color);
	//SDL_Surface *Message = TTF_RenderText_Shaded(const_cast<TTF_Font*>(font), text.c_str(), color);
	SDL_Surface *message = TTF_RenderText_Blended(const_cast<TTF_Font*>(font), text.c_str(), color);
	GLuint texture;
	setupTexture(texture);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, message->w, message->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, message->pixels);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex3f(x,              y,              z);
	glTexCoord2f(1, 0); glVertex3f(x + message->w, y,              z);
	glTexCoord2f(1, 1); glVertex3f(x + message->w, y + message->h, z);
	glTexCoord2f(0, 1); glVertex3f(x,              y + message->h, z);
	glEnd();
	glDisable(GL_BLEND);
	deleteTexture(texture);
	SDL_FreeSurface(message);
}

void updateTexture(GLuint texture, SDL_Surface* surface) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, surface->w, surface->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, surface->pixels);
}