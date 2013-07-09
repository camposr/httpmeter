#include "httpmelt.h"

double getTime(void)
{
	struct timeval currentTime;
	double returnTime;
	int rc = 0;

	rc = gettimeofday(&currentTime, NULL);

	if (rc != 0)
		fprintf(stderr, "%s: Error while getting time: %s\n", __FUNCTION__, strerror(errno));
	
	returnTime = (double)currentTime.tv_sec + (double)currentTime.tv_usec/1000000;

	return(returnTime);
}

double getDelta(double lastTime)
{
	struct timeval currentTime;
	double delta;
	int rc = 0;

	rc = gettimeofday(&currentTime, NULL);

	if (rc != 0)
		fprintf(stderr, "%s: Error while getting time: %s\n", __FUNCTION__, strerror(errno));
	
	delta = ((double)currentTime.tv_sec + (double)currentTime.tv_usec/1000000) -
	        lastTime;

	return(delta);
}
