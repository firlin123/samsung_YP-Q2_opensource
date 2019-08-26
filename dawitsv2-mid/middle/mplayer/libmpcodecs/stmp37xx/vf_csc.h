/*
 */

#define MAX_YUV_BUFFER			4

/* private data */
struct vf_priv_s
{
	char *y[MAX_YUV_BUFFER];
	char *u[MAX_YUV_BUFFER];
	char *v[MAX_YUV_BUFFER];
	ulong buf_size;

	int fd;
	int bUseHWCSC;
	int brightness, contrast;
};


