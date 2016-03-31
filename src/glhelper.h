#ifndef GLHELPER_H
#define GLHELPER_H

#include <opencv/cv.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#include "blob.h"

void setupOrtho(int width, int height);

void setupTexture(GLuint& texture);
void setupTexture(GLuint& texture, SDL_Surface *s);
void deleteTexture(GLuint& texture);
void renderTexture(GLuint texture, cv::Mat& ret, bool luminance, int width, int height);

void renderBlobs(std::vector<cBlob>& blobs, GLfloat r, GLfloat g, GLfloat b, GLfloat drag_r, GLfloat drag_g, GLfloat drag_b);

void renderCircle(float x, float y, float rad, int deg, GLfloat r, GLfloat g, GLfloat b);

void renderText(const TTF_Font *font, const GLubyte& r, const GLubyte& g, const GLubyte& b, const double& x,  const double& y, const double& z,  const std::string& text);

void updateTexture(GLuint texture, SDL_Surface* surface);

#endif
