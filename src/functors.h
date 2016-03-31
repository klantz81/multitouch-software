#ifndef FUNCTORS_H
#define FUNCTORS_H



struct compareBlobID {
	bool operator()(const cBlob& lhs, const cBlob& rhs) const { return (lhs.id < rhs.id); }
};

struct compareBlobIDp {
	bool operator()(const cBlob* lhs, const cBlob* rhs) const { return (lhs->id < rhs->id); }
};

struct compareWidgetsZ {
	bool operator()(const cWidget* lhs, const cWidget* rhs) const { return (lhs->z < rhs->z); }
};



#endif