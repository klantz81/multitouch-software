#ifndef TRACKER2_H
#define TRACKER2_H

#include <opencv/cv.h>

#include "blob.h"
#include "disjointset.h"

class cTracker2 {
  private:
	double min_area, max_radius, scale_x, scale_y;
	int screen_width, screen_height;
	
	node **labels; unsigned int width, height;

	// storage of the current blobs and the blobs from the previous frame
	std::vector<cBlob> blobs, blobs_previous;
	int id_index;				// for blob identification
	
	cDisjointSet ds;

	bool calibrated;
	point *calibration_points; int h_points, v_points;

  protected:

  public:

	cTracker2(double min_area, double max_radius, int screen_width, int screen_height, double scale_x, double scale_y);
	~cTracker2();

	void extractBlobs(cv::Mat &mat);
	void trackBlobs(cv::Mat &mat, bool history, std::vector<point>& tuio);
	void scaleBlobs();
	std::vector<cBlob>& getBlobs();

	void resetCalibration();
	void updateCalibration(point* calibration_points, int h_points, int v_points);
	void applyCalibration();
};


#endif
