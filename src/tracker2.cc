#include "tracker2.h"

cTracker2::cTracker2(double min_area, double max_radius, int screen_width, int screen_height, double scale_x, double scale_y) :
	min_area(min_area), max_radius(max_radius), screen_width(screen_width), screen_height(screen_height),
	scale_x(scale_x), scale_y(scale_y),
	labels(NULL), id_index(0), calibrated(false), calibration_points(NULL), h_points(0), v_points(0) {
}

cTracker2::~cTracker2() {
	if (labels) delete [] labels;
	if (calibration_points) delete [] calibration_points;
}

void cTracker2::extractBlobs(cv::Mat &mat) {
	// mat.cols, mat.rows -- allocate vectors
	if (mat.cols != width || mat.rows != height) {
		width = mat.cols; height = mat.rows;
		if (labels) delete [] labels;
		labels = new node*[width*height];
	}

	// reset our data structure for reuse
	ds.Reset();
	int index;

	// generate equivalence sets -- connected component labeling (4-connected)
	labels[0] = ds.MakeSet(0);
	for (int j = 1; j < mat.cols; j++)
		labels[j] = mat.data[j] != mat.data[j-1] ? ds.MakeSet(0) : labels[j-1];
	for (int j = mat.cols; j < mat.rows*mat.cols; j++) {
		if      (mat.data[j] == mat.data[j-1]) {
			labels[j] = labels[j-1];
			if (mat.data[j-1] == mat.data[j-mat.cols]) ds.Union(labels[j-1], labels[j-mat.cols]);
		} else if (mat.data[j] == mat.data[j-mat.cols]) {
			labels[j] = labels[j-mat.cols];
			if (mat.data[j-1] == mat.data[j-mat.cols]) ds.Union(labels[j-1], labels[j-mat.cols]);
		} else
			labels[j] = ds.MakeSet(0);
	}

	// the representative elements in our disjoint set data struct are associated with indices
	// we reduce those indices to 0,1,...,n and allocate our blobs
	cBlob temp;
	blobs.clear();
	for (int i = 0; i < ds.Reduce(); i++)
		blobs.push_back(temp);

	// populate our blob vector
	for (int j = 0; j < mat.rows; j++) {
		for (int i = 0; i < mat.cols; i++) {
			index = ds.Find(labels[j*mat.cols+i])->i;
			if (blobs[index].event == BLOB_NULL) {
				blobs[index].min.x = blobs[index].max.x = i;
				blobs[index].min.y = blobs[index].max.y = j;				 
				blobs[index].event = BLOB_DOWN;
				blobs[index].id = id_index++;
			} else {
				if      (blobs[index].min.x > i) blobs[index].min.x = i;
				else if (blobs[index].max.x < i) blobs[index].max.x = i;
				blobs[index].max.y = j;
			}
		}
	}

	sort(blobs.begin(), blobs.end());

	// apply blob filter
	for (int i = 0; i < blobs.size(); i++) {
		if ((blobs[i].max.x-blobs[i].min.x)*(blobs[i].max.y-blobs[i].min.y) < min_area) {
			blobs.erase(blobs.begin()+i);
			i--;
		} else break;
	}

	blobs.erase(blobs.end());

	// find blob centers
	for (int i = 0; i < blobs.size(); i++) {
		blobs[i].location.x = blobs[i].origin.x = (blobs[i].max.x + blobs[i].min.x) / 2.0;
		blobs[i].location.y = blobs[i].origin.y = (blobs[i].max.y + blobs[i].min.y) / 2.0;
	}

}

void cTracker2::trackBlobs(cv::Mat &mat, bool history, std::vector<point>& tuio) {
  	// clear the blobs from two frames ago
	blobs_previous.clear();
	
	// before we populate the blobs vector with the current frame, we need to store the live blobs in blobs_previous
	for (int i = 0; i < blobs.size(); i++)
		if (blobs[i].event != BLOB_UP)// && blobs[i].event != BLOB_NULL)
			blobs_previous.push_back(blobs[i]);

	extractBlobs(mat);

	applyCalibration();

	cBlob temp;
	for (int i = 0; i < tuio.size(); i++) {
		temp.location.x = temp.origin.x = tuio[i].x * screen_width;
		temp.location.y = temp.origin.y = tuio[i].y * screen_height;
		temp.min.x = temp.location.x - 8;
		temp.max.x = temp.location.x + 8;
		temp.min.y = temp.location.y - 8;
		temp.max.y = temp.location.y + 8;
		temp.event = BLOB_DOWN;
		temp.id = id_index++;
		temp.widget = NULL;
		temp.height = 0;

		blobs.push_back(temp);
	}

	//scaleBlobs();

	// initialize previous blobs to untracked
	for (int i = 0; i < blobs_previous.size(); i++) blobs_previous[i].tracked = false;

	// main tracking loop -- O(n^2) -- simply looks for a blob in the previous frame within a specified radius
	for (int i = 0; i < blobs.size(); i++) {
		for (int j = 0; j < blobs_previous.size(); j++) {
			if (blobs_previous[j].tracked) continue;

			if (sqrt(pow(blobs[i].location.x - blobs_previous[j].location.x, 2.0) + pow(blobs[i].location.y - blobs_previous[j].location.y, 2.0)) < max_radius) {
				blobs_previous[j].tracked = true;
				blobs[i].id = blobs_previous[j].id;
				blobs[i].widget = blobs_previous[j].widget;
				blobs[i].event = BLOB_MOVE;
				blobs[i].origin.x = history ? blobs_previous[j].origin.x : blobs_previous[j].location.x;
				blobs[i].origin.y = history ? blobs_previous[j].origin.y : blobs_previous[j].location.y;
				break;
			}
		}
	}

	// add any blobs from the previous frame that weren't tracked as having been removed
	for (int i = 0; i < blobs_previous.size(); i++) {
		if (!blobs_previous[i].tracked) {
			blobs_previous[i].event = BLOB_UP;
			blobs.push_back(blobs_previous[i]);
		}
	}
	
	if (blobs.size() == 0) id_index = 0;
	
}

void cTracker2::scaleBlobs() {
	for (int i = 0; i < blobs.size(); i++) {
		blobs[i].location.x *= scale_x;
		blobs[i].origin.x *= scale_x;
		blobs[i].min.x *= scale_x;
		blobs[i].max.x *= scale_x;

		blobs[i].location.y *= scale_y;
		blobs[i].origin.y *= scale_y;
		blobs[i].min.y *= scale_y;
		blobs[i].max.y *= scale_y;
	}
}

std::vector<cBlob>& cTracker2::getBlobs() {
	return blobs;
}

void cTracker2::resetCalibration() {
	calibrated = false;
}

void cTracker2::updateCalibration(point* calibration_points, int h_points, int v_points) {
	calibrated = true;
	this->h_points = h_points; this->v_points = v_points;
	if (this->calibration_points) delete [] this->calibration_points;
	this->calibration_points = new point[h_points * v_points];
	for (int i = 0; i < h_points * v_points; i++) this->calibration_points[i] = calibration_points[i];
}

void cTracker2::applyCalibration() {
	if (!calibrated) return;

	int i, j, k;
	double s, t, v0v0, v1v1, v0v1, xv0, xv1, den;
	point p0, p1, p2, v0, v1, x, xp, trans;
	bool blob_found;
	double delta_x = (double)screen_width / (double)(h_points - 1), delta_y = (double)screen_height / (double)(v_points - 1);

	for (k = 0; k < blobs.size(); k++) {
		blob_found = false;
		for (j = 0; j < v_points - 1; j++) {
			for (i = 0; i < h_points - 1; i++) {
				// check both triangles with barycentric coordinates

				p0 = calibration_points[j * h_points + i];
				p1 = calibration_points[j * h_points + i + 1];
				p2 = calibration_points[(j + 1) * h_points + i];
				v0 = p1 - p0;
				v1 = p2 - p0;
				x = blobs[k].location - p0;
				xv0 = x.inner(v0);
				xv1 = x.inner(v1);
				v0v0 = v0.inner(v0);
				v1v1 = v1.inner(v1);
				v0v1 = v0.inner(v1);
				den = (v0v0 * v1v1 - v0v1 * v0v1);
				s = (xv0 * v1v1 - xv1 * v0v1) / den;
				t = (xv1 * v0v0 - xv0 * v0v1) / den;
				if (s >= 0 && t >= 0 && s + t <= 1) {
					// update blob
					xp = point((i + s) * delta_x, (j + t) * delta_y);
					trans = xp - blobs[k].location;
					blobs[k].location = blobs[k].origin = xp;
					blobs[k].min += trans; blobs[k].max += trans;
					blob_found = true; break;
				}

				p0 = calibration_points[(j + 1) * h_points + i + 1];
				p1 = calibration_points[(j + 1) * h_points + i];
				p2 = calibration_points[j * h_points + i + 1];
				v0 = p1 - p0;
				v1 = p2 - p0;
				x = blobs[k].location - p0;
				xv0 = x.inner(v0);
				xv1 = x.inner(v1);
				v0v0 = v0.inner(v0);
				v1v1 = v1.inner(v1);
				v0v1 = v0.inner(v1);
				den = (v0v0 * v1v1 - v0v1 * v0v1);
				s = (xv0 * v1v1 - xv1 * v0v1) / den;
				t = (xv1 * v0v0 - xv0 * v0v1) / den;
				if (s >= 0 && t >= 0 && s + t <= 1) {
					// update blob
					xp = point((i + (1 - s)) * delta_x, (j + (1 - t)) * delta_y);
					trans = xp - blobs[k].location;
					blobs[k].location = blobs[k].origin = xp;
					blobs[k].min += trans; blobs[k].max += trans;
					blob_found = true; break;
				}
				
			}
			if (blob_found) break;
		}
	}
}