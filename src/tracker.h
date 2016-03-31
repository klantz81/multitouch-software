#ifndef TRACKER_H
#define TRACKER_H

#include <opencv/cv.h>

#include "../cvblobslib/BlobResult.h"
#include "blob.h"

class cTracker {
  private:
	CBlobResult blob_result;
	CBlob *current_blob;
	
	double min_area, max_radius, scale_x, scale_y;
	int screen_width, screen_height;

	// instances of helper classes for obtaining blob location and bounding box
	CBlobGetXCenter XCenter;
	CBlobGetYCenter YCenter;
	CBlobGetMinX MinX;
	CBlobGetMinY MinY;
	CBlobGetMaxX MaxX;
	CBlobGetMaxY MaxY;

	// we will convert the matrix object passed from our cFilter class to an object of type IplImage for calling the CBlobResult constructor
	IplImage img;

	// storage of the current blobs and the blobs from the previous frame
	std::vector<cBlob> blobs, blobs_previous;
	int id_index;				// for blob identification
	
	bool calibrated;
	point *calibration_points; int h_points, v_points;
	
  protected:

  public:
	cTracker(double min_area, double max_radius, int screen_width, int screen_height, double scale_x, double scale_y);
	~cTracker();
	
	void trackBlobs(cv::Mat &mat, bool history, std::vector<point>& tuio);
	void scaleBlobs();
	std::vector<cBlob>& getBlobs();
	
	void resetCalibration();
	void updateCalibration(point* calibration_points, int h_points, int v_points);
	void applyCalibration();
};


#endif
