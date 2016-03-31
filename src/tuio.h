#ifndef TUIO_H
#define TUIO_H

#include <pthread.h>

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <sys/fcntl.h>

#include "blob.h"

const int PORT = 3333;

/*
typedef short int16;
typedef unsigned short uint16;
*/
//typedef int int32;
typedef unsigned int uint32;
/*
typedef long long __int64;
typedef unsigned long long __uint64;
*/
//typedef char char8;
typedef unsigned char uchar8;

typedef float float32;
//typedef double float64;
/*
union u16 {
	uint16 i;
	uchar8 c[2];
};
*/
union u32 {
	uint32 i;
	uchar8 c[4];
};
/*
union u64 {
	__uint64 i;
	uchar8 c[8];
};

union s64 {
	__int64 i;
	char8 c[8];
};
*/
union f32 {
	float32 f;
	uchar8 c[4];
};



class cTUIO {
  private:
	int sock, n;
	socklen_t clientlen;
	struct sockaddr_in server, client;
	char buf[1024];

	pthread_mutex_t mutex;
	pthread_t thread;
	bool active;
	
	u32 timestamp, timestamp_fraction, element_size;
	f32 x, y;
	
	std::vector<point> points, ret_points;

  protected:

  public:
	cTUIO();
	~cTUIO();
	
	static void* loop(void* obj);

	void process();
	void processMessage(const char* parameters, const char* arguments);

	std::vector<point>& getPoints();
	void lock();
	void unlock();
};

#endif