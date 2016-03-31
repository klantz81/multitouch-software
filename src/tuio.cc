#include "tuio.h"

cTUIO::cTUIO() {
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	int flags = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, flags | O_NONBLOCK);

	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(PORT);
	bind(sock, (struct sockaddr *)&server, sizeof(server));

	clientlen = sizeof(struct sockaddr_in);

	pthread_mutex_init(&mutex, 0);

	active = true;
	pthread_create(&thread, 0, &cTUIO::loop, this);
}

cTUIO::~cTUIO() {
	active = false;
	pthread_join(thread, 0);
	
	pthread_mutex_destroy(&mutex);
	
	close(sock);
}

void* cTUIO::loop(void *obj) {
	while (reinterpret_cast<cTUIO *>(obj)->active) reinterpret_cast<cTUIO *>(obj)->process();
}

enum { UNDEFINED, SOURCE, ALIVE, SET, FSEQ };

void cTUIO::processMessage(const char* parameters, const char* arguments) {
	int type = UNDEFINED, k, fcount = 0;
	for (int i = 0, j = 0; i < strlen(parameters); i++) {
		switch (parameters[i]) {
		  case 's':
			if (type == UNDEFINED) {
				if      (strcmp(&arguments[j], "source") == 0) { type = SOURCE; }
				else if (strcmp(&arguments[j], "alive")  == 0) { type = ALIVE; points.clear(); }
				else if (strcmp(&arguments[j], "set")    == 0) { type = SET; }
				else if (strcmp(&arguments[j], "fseq")   == 0) { type = FSEQ; }
				k = strlen(&arguments[j]);
				k = k % 4 == 0 ? k : k + 4 - (k % 4);
				j += k;
			} else {//if (type == SOURCE) {
				// source stored in &arguments[j]
			}
			break;
		  case 'i':
			if        (type == ALIVE) {
				// id
			} else if (type == SET)   {
				// id
			} else if (type == FSEQ)  {
				// frame id
			}
			j += 4;
			break;
		  case 'f':
			//if (type == SET) {
			// xyXYm (position, veloction, motion acceleration)
			if        (fcount == 0) {
				x.c[3] = arguments[j+0];
				x.c[2] = arguments[j+1];
				x.c[1] = arguments[j+2];
				x.c[0] = arguments[j+3];
			} else if (fcount == 1) {
				y.c[3] = arguments[j+0];
				y.c[2] = arguments[j+1];
				y.c[1] = arguments[j+2];
				y.c[0] = arguments[j+3];
				points.push_back(point(x.f, y.f));
			}
			fcount++;
			j += 4;
			break;
		}
	}
}

void cTUIO::process() {
	memset(buf, 0, 1024);
	n = recvfrom(sock, buf, 1024, 0, (struct sockaddr *)&client, &clientlen);

	if (n > 0) {
		int k;
		if (strcmp(&buf[0], "#bundle") == 0) {
			timestamp.c[3] = buf[8]; timestamp.c[2] = buf[9]; timestamp.c[1] = buf[10]; timestamp.c[0] = buf[11];
			timestamp_fraction.c[3] = buf[12]; timestamp_fraction.c[2] = buf[13]; timestamp_fraction.c[1] = buf[14]; timestamp_fraction.c[0] = buf[15];
			for (int i = 16; i < n; i += 4) {
				element_size.c[3] = buf[i]; element_size.c[2] = buf[i+1]; element_size.c[1] = buf[i+2]; element_size.c[0] = buf[i+3];
				
				if (strcmp(&buf[i+4], "/tuio/2Dcur") == 0) {
					k = i + 17 + strlen(&buf[i+17]) + 1;
					k = k % 4 == 0 ? k : k + 4 - (k % 4);
					processMessage(&buf[i+17], &buf[k]);
				}
				
				i+= element_size.i;
			}
			pthread_mutex_lock(&mutex);
			ret_points = points;
			pthread_mutex_unlock(&mutex);
		}
	}
}

std::vector<point>& cTUIO::getPoints() {
	pthread_mutex_lock(&mutex);
	return ret_points;
}

void cTUIO::lock() {
	pthread_mutex_lock(&mutex);
}

void cTUIO::unlock() {
	pthread_mutex_unlock(&mutex);
}