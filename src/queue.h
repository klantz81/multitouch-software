#ifndef QUEUE_H
#define QUEUE_H

#include <algorithm>
#include "blob.h"
#include "widget.h"
#include "functors.h"



class cQueue {
  private:
	std::vector<cWidget *> widgets;
    
  protected:
    
  public:
	cQueue();
	~cQueue();

	void registerWidget(cWidget& widget);
	void unregisterWidget(cWidget& widget);
	
	void processEvents(std::vector<cBlob>& blobs);
};

#endif