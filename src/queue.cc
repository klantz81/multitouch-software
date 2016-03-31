#include "queue.h"



cQueue::cQueue() {
}

cQueue::~cQueue() {
}

void cQueue::registerWidget(cWidget& widget) {
	int i;
	for (i = 0; i < widgets.size(); i++) {
		if (widgets[i] == &widget) {
			break;
		}
	}
	if (i == widgets.size()) { widgets.push_back(&widget); widget.bringForward(); }
}

void cQueue::unregisterWidget(cWidget& widget) {
	for (int i = 0; i < widgets.size(); i++) {
		if (widgets[i] == &widget) {
			widgets.erase(widgets.begin() + i);
			break;
		}
	}
}

void cQueue::processEvents(std::vector<cBlob>& blobs) {
	sort(blobs.begin(), blobs.end(), compareBlobID());		// process older blobs first
	sort(widgets.begin(), widgets.end(), compareWidgetsZ());	// process widgets based on zindex ordering

	for (int i = 0; i < widgets.size(); i++) widgets[i]->resetEvents();

	for (int i = 0; i < blobs.size(); i++) {
		if (blobs[i].widget) {
			(static_cast<cWidget *>(blobs[i].widget))->addEvent(blobs[i]);
			blobs[i].tracked = true;
		} else blobs[i].tracked = false;
	}

	for (int i = widgets.size() - 1; i > -1; i--) {
		for (int j = 0; j < blobs.size(); j++) {
			if (blobs[j].tracked == true) continue;

			if (widgets[i]->containsPoint(blobs[j].location)) {
				widgets[i]->addEvent(blobs[j]);
				blobs[j].widget = widgets[i];
				blobs[j].tracked = true;
			}
		}
	}

	for (int i = widgets.size() - 1; i > -1; i--) {
		widgets[i]->update();
	}

	for (int i = 0; i < widgets.size(); i++) {
		widgets[i]->render();
	}
}