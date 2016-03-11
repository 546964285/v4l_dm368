/*
 **  V4L2 video capture example
 **
 **  This program can be used and distributed without restrictions.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>		// getopt_long()

#include <fcntl.h>		// for open()
#include <unistd.h>     // for close()
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>		// for videodev2.h

#include <linux/videodev.h>
#include <linux/videodev2.h>
#include <media/davinci/dm365_ccdc.h>
#include <media/davinci/vpfe_capture.h>
#include <media/davinci/imp_previewer.h>
#include <media/davinci/imp_resizer.h>
#include <media/davinci/dm365_ipipe.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define CAPTURE_DEVICE	"/dev/video0"
#define VID0_DEVICE "/dev/video2"

char *dev_name_prev = "/dev/davinci_previewer";
char *dev_name_rsz = "/dev/davinci_resizer";

typedef enum
{
  IO_METHOD_READ,
  IO_METHOD_MMAP,
  IO_METHOD_USERPTR,
} io_method;

struct buffer
{
  void *start;
  size_t length;
};

static int linearization_en;
static int csc_en;
static int vldfc_en;
static int en_culling;

static char *dev_name = NULL;
static io_method io = IO_METHOD_MMAP;
static int fd = -1;
static int fd_vid0, fd_osd0, fd_osd1;
static int preview_fd, resizer_fd;
struct buffer *buffers = NULL;
static unsigned int n_buffers = 0;
static struct buffer *vid0Buf;
static struct rsz_channel_config rsz_chan_config; // resizer channel config
static struct prev_channel_config prev_chan_config;
static unsigned long oper_mode_1,user_mode_1;
struct rsz_single_shot_config rsz_ss_config;
struct rsz_continuous_config rsz_ctn_config;
struct prev_single_shot_config prev_ss_config;
struct prev_continuous_config prev_ctn_config;

static void
errno_exit (const char *s)
{
  fprintf (stderr, "%s error %d, %s\n", s, errno, strerror (errno));

  exit (EXIT_FAILURE);
}

static int
xioctl (int fd, int request, void *arg)
{
  int r;

  do
	  r = ioctl (fd, request, arg);
  while (-1 == r && EINTR == errno);

  return r;
}

static int configCCDCraw(int capt_fd)
{
	struct ccdc_config_params_raw raw_params;
	/* Change these values for testing Gain - Offsets */
	struct ccdc_float_16 r = {0, 511};
	struct ccdc_float_16 gr = {0, 511};
	struct ccdc_float_16 gb = {0, 511};
	struct ccdc_float_16 b = {0, 511};
	struct ccdc_float_8 csc_coef_val = { 1, 0 };
	int i;

	//bzero(&raw_params, sizeof(raw_params));
	CLEAR(raw_params);

	/* First get the parameters */
	if (-1 == ioctl(capt_fd, VPFE_CMD_G_CCDC_RAW_PARAMS, &raw_params)) {
		printf("InitDevice:ioctl:VPFE_CMD_G_CCDC_PARAMS, %p", &raw_params);
		return -1;
	}

	raw_params.compress.alg = CCDC_NO_COMPRESSION;
	raw_params.gain_offset.gain.r_ye = r; 
	raw_params.gain_offset.gain.gr_cy = gr;
	raw_params.gain_offset.gain.gb_g = gb;
	raw_params.gain_offset.gain.b_mg = b;
	raw_params.gain_offset.gain_sdram_en = 1;
	raw_params.gain_offset.gain_ipipe_en = 1;
	raw_params.gain_offset.offset = 0;
	raw_params.gain_offset.offset_sdram_en = 1;
	/* To test linearization, set this to 1, and update the
	 * linearization table with correct data
	 */
	if (linearization_en) {
		raw_params.linearize.en = 1;
		raw_params.linearize.corr_shft = CCDC_1BIT_SHIFT;
		raw_params.linearize.scale_fact.integer = 0;
		raw_params.linearize.scale_fact.decimal = 10;

		for (i = 0; i < CCDC_LINEAR_TAB_SIZE; i++)
			raw_params.linearize.table[i] = i;	
	} else {
		raw_params.linearize.en = 0;
	}
	
	/* CSC */
	if (csc_en) {
		raw_params.df_csc.df_or_csc = 0;
		raw_params.df_csc.csc.en = 1;
		/* I am hardcoding this here. But this should
		 * really match with that of the capture standard
		 */
		raw_params.df_csc.start_pix = 1;
		raw_params.df_csc.num_pixels = 720;
		raw_params.df_csc.start_line = 1;
		raw_params.df_csc.num_lines = 480;
		/* These are unit test values. For real case, use
		 * correct values in this table
		 */
		raw_params.df_csc.csc.coeff[0] = csc_coef_val;
		raw_params.df_csc.csc.coeff[1].decimal = 1;
		raw_params.df_csc.csc.coeff[2].decimal = 2;
		raw_params.df_csc.csc.coeff[3].decimal = 3;
		raw_params.df_csc.csc.coeff[4].decimal = 4;
		raw_params.df_csc.csc.coeff[5].decimal = 5;
		raw_params.df_csc.csc.coeff[6].decimal = 6;
		raw_params.df_csc.csc.coeff[7].decimal = 7;
		raw_params.df_csc.csc.coeff[8].decimal = 8;
		raw_params.df_csc.csc.coeff[9].decimal = 9;
		raw_params.df_csc.csc.coeff[10].decimal = 10;
		raw_params.df_csc.csc.coeff[11].decimal = 11;
		raw_params.df_csc.csc.coeff[12].decimal = 12;
		raw_params.df_csc.csc.coeff[13].decimal = 13;
		raw_params.df_csc.csc.coeff[14].decimal = 14;
		raw_params.df_csc.csc.coeff[15].decimal = 15;
		
	} else {
		raw_params.df_csc.df_or_csc = 0;
		raw_params.df_csc.csc.en = 0;
	}

	/* vertical line defect correction */
	if (vldfc_en) {
		raw_params.dfc.en = 1;
		// correction method
		raw_params.dfc.corr_mode = CCDC_VDFC_HORZ_INTERPOL_IF_SAT;
		// not pixels upper than the defect corrected
		raw_params.dfc.corr_whole_line = 1;
		raw_params.dfc.def_level_shift = CCDC_VDFC_SHIFT_2;
		raw_params.dfc.def_sat_level = 20;
		raw_params.dfc.num_vdefects = 7;
		for (i = 0; i < raw_params.dfc.num_vdefects; i++) {
			raw_params.dfc.table[i].pos_vert = i;
			raw_params.dfc.table[i].pos_horz = i + 1;
			raw_params.dfc.table[i].level_at_pos = i + 5;
			raw_params.dfc.table[i].level_up_pixels = i + 6;
			raw_params.dfc.table[i].level_low_pixels = i + 7;
		}
		printf("DFC enabled\n");
	} else {
		raw_params.dfc.en = 0;
	}

	if (en_culling) {
		
		printf("Culling enabled\n");
		raw_params.culling.hcpat_odd  = 0xaa;
		raw_params.culling.hcpat_even = 0xaa;
		raw_params.culling.vcpat = 0x55;
		raw_params.culling.en_lpf = 1;
	} else {
		raw_params.culling.hcpat_odd  = 0xFF;
		raw_params.culling.hcpat_even = 0xFF;
		raw_params.culling.vcpat = 0xFF;
	}
	
	raw_params.col_pat_field0.olop = CCDC_GREEN_BLUE;
	raw_params.col_pat_field0.olep = CCDC_BLUE;
	raw_params.col_pat_field0.elop = CCDC_RED;
	raw_params.col_pat_field0.elep = CCDC_GREEN_RED;
	raw_params.col_pat_field1.olop = CCDC_GREEN_BLUE;
	raw_params.col_pat_field1.olep = CCDC_BLUE;
	raw_params.col_pat_field1.elop = CCDC_RED;
	raw_params.col_pat_field1.elep = CCDC_GREEN_RED;
	raw_params.data_size = CCDC_12_BITS;
	raw_params.data_shift = CCDC_NO_SHIFT;

	printf("VPFE_CMD_S_CCDC_RAW_PARAMS, size = %d, address = %p\n", sizeof(raw_params),
				&raw_params);
	if (-1 == ioctl(capt_fd, VPFE_CMD_S_CCDC_RAW_PARAMS, &raw_params)) {
		printf("InitDevice:ioctl:VPFE_CMD_S_CCDC_PARAMS, %p", &raw_params);
		return -1;
	}

	return 0;
}


static void
process_image (const void *p)
{
  fputc ('.', stdout);
  fflush (stdout);
}

static int
read_frame (void)
{
	struct v4l2_buffer buf;
	unsigned int i;

	switch (io)
    {
		case IO_METHOD_READ:
			if (-1 == read (fd, buffers[0].start, buffers[0].length))
		    {
			  switch (errno)
				{
				case EAGAIN:
				  return 0;

				case EIO:
				  /* Could ignore EIO, see spec. */

				  /* fall through */

				default:
				  errno_exit ("read");
				}
			}
		    process_image (buffers[0].start);
		  break;
		case IO_METHOD_MMAP:
			  CLEAR (buf);

		  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		  buf.memory = V4L2_MEMORY_MMAP;

		  if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf))
		  {
			  switch (errno)
				{
				case EAGAIN:
				  return 0;

				case EIO:
				  /* Could ignore EIO, see spec. */

				  /* fall through */

				default:
				  errno_exit ("VIDIOC_DQBUF");
				}
		  }
		  assert (buf.index < n_buffers);
		  process_image (buffers[buf.index].start);

		  if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
			errno_exit ("VIDIOC_QBUF");
      break;
    case IO_METHOD_USERPTR:
		  CLEAR (buf);

		  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		  buf.memory = V4L2_MEMORY_USERPTR;

		  if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf))
		  {
		  switch (errno)
			{
			case EAGAIN:
			  return 0;

			case EIO:
			  /* Could ignore EIO, see spec. */

			  /* fall through */

			default:
			  errno_exit ("VIDIOC_DQBUF");
			}
		  }
		  for (i = 0; i < n_buffers; ++i)
			if (buf.m.userptr == (unsigned long) buffers[i].start
				&& buf.length == buffers[i].length)
			  break;

		  assert (i < n_buffers);
		  process_image ((void *) buf.m.userptr);

		  if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
				errno_exit ("VIDIOC_QBUF");

		  break;
    }

  return 1;
}

/******************************************************************************/
static int start_display(int fd, int num_of_bufs)
{
	int i = 0, ret;
	struct v4l2_buffer buf;
	enum v4l2_buf_type type;

	bzero(&buf, sizeof(buf));
	/*
	 *      Queue all the buffers for the initial running
	 */
	printf("6. Test enqueuing of buffers - ");
	for (i = 0; i < num_of_bufs; i++) {
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		ret = ioctl(fd, VIDIOC_QBUF, &buf);
		if (ret < 0) {
			printf("fd = %d\n", fd);
			printf("\n\tError: Enqueuing buffer[%d] failed: VID1",
			       i);
			return -1;
		}
	}
	printf("done\n");

	printf("7. Test STREAM_ON\n");
	type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ret = ioctl(fd, VIDIOC_STREAMON, &type);
	if (ret < 0) {
		perror("VIDIOC_STREAMON\n");
		return -1;
	}
	return 0;
}

/*******************************************************************************
 *	Takes the address, finds the appropriate index
 *	of the buffer, and QUEUEs the buffer to display
 *	If this part is done in the main loop,
 *	there is no need of this conversionof address
 *	to index as both are available.
 */
static int put_display_buffer(int vid_win, void *addr)
{
	struct v4l2_buffer buf;
	int i, index = 0;
	int ret;
	if (addr == NULL)
		return -1;
	memset(&buf, 0, sizeof(buf));

	for (i = 0; i < 3; i++) {
		if (addr == vid0Buf[i].start) {
			index = i;
			break;
		}
	}
	buf.m.offset = (unsigned long)addr;
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = index;
//printf("in put_display_buffer();\n");
	ret = ioctl(vid_win, VIDIOC_QBUF, &buf);
	return ret;
}

/*******************************************************************************
 *	Does a DEQUEUE and gets/returns the address of the
 *	dequeued buffer
 */
static void *get_display_buffer(int vid_win)
{
	int ret;
	struct v4l2_buffer buf;
	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ret = ioctl(vid_win, VIDIOC_DQBUF, &buf);
	if (ret < 0) {
		perror("VIDIOC_DQBUF\n");
		return NULL;
	}
	return vid0Buf[buf.index].start;
}

/*******************************************************************************
 *      Stops Streaming
 */
static int stop_capture(int vid_win)
{
	int ret;
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(vid_win, VIDIOC_STREAMOFF, &type);
	return ret;
}

/******************************************************************************/
static int start_loop(void)
{
	int ret, quit;
	struct v4l2_buffer buf;
	static int captFrmCnt = 0, printfn = 0, stress_test = 1, start_loopCnt = 200;
	unsigned char *displaybuffer = NULL;
	int i;
	char *ptrPlanar = NULL;
	void *src, *dest;

	ptrPlanar = (char *)calloc(1, 1280 * 720 * 2);

	while (!quit) {
		fd_set fds;
		struct timeval tv;
		int r;


    //printf("in start_loop\n");
		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		/* Timeout */
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		r = select(fd + 1, &fds, NULL, NULL, &tv);
		if (-1 == r) {
			if (EINTR == errno)
				continue;
			printf("StartCameraCapture:select\n");
			return -1;
		}
		if (0 == r)
			continue;

		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		/* determine ready buffer */
		if (-1 == ioctl(fd, VIDIOC_DQBUF, &buf)) {
			if (EAGAIN == errno)
				continue;
			printf("StartCameraCaputre:ioctl:VIDIOC_DQBUF\n");
			return -1;
		}

		/******************* V4L2 display ********************/
		displaybuffer = get_display_buffer(fd_vid0);
		if (NULL == displaybuffer) {
			printf("Error in getting the  display buffer:VID1\n");
			return ret;
		}

		src = buffers[buf.index].start;
		dest = displaybuffer;

		/* Display image onto requested video window */
		for(i=0 ; i < 720; i++) {
			memcpy(dest, src, 2560);
			src += 2560;
			dest += 2560;
		}

		ret = put_display_buffer(fd_vid0, displaybuffer);
    //printf("put_display_buffer\n");
		if (ret < 0) {
			printf("Error in putting the display buffer\n");
			return ret;
		}
		/***************** END V4L2 display ******************/

		
		if (printfn)
			printf("time:%lu    frame:%u\n", (unsigned long)time(NULL),
		       		captFrmCnt++);

		/* requeue the buffer */
		if (-1 == ioctl(fd, VIDIOC_QBUF, &buf))
			printf("StartCameraCaputre:ioctl:VIDIOC_QBUF\n");
		if (stress_test) {
			start_loopCnt--;
			if (start_loopCnt == 0) {
				start_loopCnt = 50;
				break;
			}
		}
	}
	ret = stop_capture(fd);
	if (ret < 0)
		printf("Error in VIDIOC_STREAMOFF:capture\n");
	return ret;
}

static void
mainloop (void)
{
	unsigned int count;
	count = 100;
	int i = 0, ret = 0;

//	    printf("process in mainloop()\n");
	
//		printf("1. Opening VID0 device\n");
	fd_vid0 = open(VID0_DEVICE, O_RDWR);
	if (-1 == fd_vid0) {
		printf("failed to open VID1 display device\n");
		//return -1;
	}
//		printf("Opening VID0 device SUCCESS!\n");
	
	// Testing IOCTLs
	struct v4l2_capability capability;
	ret = ioctl(fd_vid0, VIDIOC_QUERYCAP, &capability);
	if (ret < 0) {
		printf("FAILED: QUERYCAP\n");
		//return -1;
	}
//		else
//		{
//			printf("fd = %d\n", fd_vid0);
//			if (capability.capabilities & V4L2_CAP_VIDEO_OUTPUT)
//				printf("\tDisplay capability is supported\n");
//			if (capability.capabilities & V4L2_CAP_STREAMING)
//				printf("\tStreaming is supported\n");
//		}
	
	// 查询支持的输出格式
	struct v4l2_fmtdesc format;
	while (1) {
		format.index = i;
		format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		ret = ioctl(fd_vid0, VIDIOC_ENUM_FMT, &format);
		if (ret < 0)
			break;
		printf("description = %s\n", format.description);
		if (format.type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
			printf("Video Display type is ");
		if (format.pixelformat == V4L2_PIX_FMT_UYVY)
			printf("V4L2_PIX_FMT_UYVY\n");
		i++;
	}
	
	// 测试缓冲区
	printf("2. Test request for buffers\n");
	struct v4l2_requestbuffers reqbuf;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	reqbuf.count = 3;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(fd_vid0, VIDIOC_REQBUFS, &reqbuf);
	if (ret < 0) {
		printf("\n\tError: Could not allocate the buffers: VID0\n");
		//return -1;
	}
	else
	{
		printf("\tNumbers of buffers returned - %d\n", reqbuf.count);
	}
	
	// 设置输出格式
	struct v4l2_format setfmt;
	CLEAR(setfmt);
	setfmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	//setfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
	//setfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
	setfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
	//setfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SBGGR16;
	//setfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	setfmt.fmt.pix.width = 1280;
	setfmt.fmt.pix.height = 720;
	setfmt.fmt.pix.field = V4L2_FIELD_NONE;
	ret = ioctl(fd_vid0, VIDIOC_S_FMT, &setfmt);
	if (ret < 0) 
	{
		perror("VIDIOC_S_FMT err\n");
		//close(fd_vid0);
		//return -1;
	} 
	else
	{
		printf(" VIDIOC_S_FMT: PASS\n");
	}
	
	// buffer chacteristics
	printf("3. Test GetFormat\n");
	struct v4l2_format fmt;
	int disppitch, dispheight, dispwidth;
	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ret = ioctl(fd_vid0, VIDIOC_G_FMT, &fmt);
	if (ret < 0) {
		printf("\tError: Get Format failed: VID0\n");
		//return -1;
	}
	else
	{
		dispheight = fmt.fmt.pix.height;
		disppitch = fmt.fmt.pix.bytesperline;
		dispwidth = fmt.fmt.pix.width;
		printf("\tdispheight = %d\n\tdisppitch = %d\n\tdispwidth = %d\n", dispheight, disppitch, dispwidth);
		printf("\timagesize = %d\n", fmt.fmt.pix.sizeimage);
	}
	
	//
	printf("4. Test querying of buffers and mapping them\n");
	
	struct v4l2_buffer buf;
	vid0Buf = (struct buffer *)calloc(reqbuf.count, sizeof(struct buffer));
	if (!vid0Buf) {
		printf("InitDisplayBuffers:calloc:\n");
		//return -1;
	}
	for (i = 0; i < reqbuf.count; i++) 
	{
		buf.index = i;
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(fd_vid0, VIDIOC_QUERYBUF, &buf);
		if (ret < 0) 
		{
			printf("\tError: Querying buffer info failed: VID0\n");
			//return -1;
		}
		else
		{
			printf("\tQuerying buffer info SUCCESS: VID0\n");
		}
		
		vid0Buf[i].length = buf.length;
		vid0Buf[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_vid0, buf.m.offset);
	
		printf("buffer:%d phy:%x mmap:%p length:%d\n",
		       buf.index, buf.m.offset, vid0Buf[i].start, buf.length);
	
		if (MAP_FAILED == vid0Buf[i].start) {
			printf("InitDisplayBuffers:mmap:\n");
			//return -1;
		}
		printf("\tTotal length of the buffer allocated = %d\n",
		       buf.length);
	
		//memset(vid0Buf[i].start, 0x80, buf.length);
		memset(vid0Buf[i].start, 0x00, buf.length);
	}
	
    printf("4+. Test querying of control info\n");
    struct v4l2_control ctrl_r;
    CLEAR(ctrl_r);
    ret = ioctl(fd_vid0, VIDIOC_G_CTRL, &ctrl_r);
    if (ret < 0) 
    {
        printf("\tError: Querying control info failed: VID0\n");
    }
    else
    {
        printf("\tctrl_r.id = %d, ctrl_r.value = %d\n", ctrl_r.id, ctrl_r.value);
        printf("\tQuerying control info SUCCESS: VID0\n");
    }
	
	// Start Display - STREAM_ON
	printf("5. Test initial queueing of buffers\n");
	ret = start_display(fd_vid0, reqbuf.count);
	if (ret < 0) 
	{
		printf("\tError: Starting display failed:VID1\n");
		//return ret;
	}

	start_loop();
	

	while (count-- > 0)
    {
		for (;;)
		{
		  fd_set fds;
		  struct timeval tv;
		  int r;

		  FD_ZERO (&fds);
		  FD_SET (fd, &fds);

		  /* Timeout. */
		  tv.tv_sec = 2;
		  tv.tv_usec = 0;

		  r = select (fd + 1, &fds, NULL, NULL, &tv);

		  if (-1 == r)
		  {
			  if (EINTR == errno)
					continue;
			  errno_exit ("select");
		  }
		  if (0 == r)
		  {
			  fprintf (stderr, "select timeout\n");
			  exit (EXIT_FAILURE);
		  }

		  if (read_frame ())
				break;
		  /* EAGAIN - continue select loop. */
		}
    }
}

static void stop_capturing (void)
{
	enum v4l2_buf_type type;

    switch (io)
    {
		case IO_METHOD_READ:
			  /* Nothing to do. */
			  break;

		case IO_METHOD_MMAP:
		case IO_METHOD_USERPTR:
			  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			  if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
					errno_exit ("VIDIOC_STREAMOFF");
			  break;
    }
}

static void start_capturing (void)
{
	unsigned int i;
    enum v4l2_buf_type type;
//	    printf("in function start_capturing()\n");
    switch (io)
    {
		case IO_METHOD_READ:
		  /* Nothing to do. */
		  break;

		case IO_METHOD_MMAP:
			for (i = 0; i < n_buffers; ++i)
			{
			  struct v4l2_buffer buf;

			  CLEAR (buf);

			  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			  buf.memory = V4L2_MEMORY_MMAP;
			  buf.index = i;

			  if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
				errno_exit ("VIDIOC_QBUF");
//				  else
//					printf("success in VIDIOC_QBUF\n");
			}

			// 打开视频捕获
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		    	if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
				errno_exit ("VIDIOC_STREAMON");
//				else
//					printf("success in VIDIOC_STREAMON\n");

			break;
		case IO_METHOD_USERPTR:
			for (i = 0; i < n_buffers; ++i)
			{
			  struct v4l2_buffer buf;

			  CLEAR (buf);

			  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			  buf.memory = V4L2_MEMORY_USERPTR;
			  buf.index = i;
			  buf.m.userptr = (unsigned long) buffers[i].start;
			  buf.length = buffers[i].length;

			  if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
				errno_exit ("VIDIOC_QBUF");
			}
		    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		    if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
				errno_exit ("VIDIOC_STREAMON");
		  break;
    }
}

static void
uninit_device (void)
{
  unsigned int i;

  switch (io)
    {
    case IO_METHOD_READ:
      free (buffers[0].start);
      break;

    case IO_METHOD_MMAP:
      for (i = 0; i < n_buffers; ++i)
	if (-1 == munmap (buffers[i].start, buffers[i].length))
	  errno_exit ("munmap");
      break;

    case IO_METHOD_USERPTR:
      for (i = 0; i < n_buffers; ++i)
	free (buffers[i].start);
      break;
    }

  free (buffers);
}

static void
init_read (unsigned int buffer_size)
{
  buffers = calloc (1, sizeof (*buffers));

  if (!buffers)
    {
      fprintf (stderr, "Out of memory\n");
      exit (EXIT_FAILURE);
    }

  buffers[0].length = buffer_size;
  buffers[0].start = malloc (buffer_size);

  if (!buffers[0].start)
    {
      fprintf (stderr, "Out of memory\n");
      exit (EXIT_FAILURE);
    }
}

static void
init_mmap (void)
{
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;

    CLEAR (req);
    req.count = 3;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    // 向设备驱动申请缓冲区
    if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req))
    {
		if (EINVAL == errno)
		{
			fprintf (stderr, "%s does not support "
				   "memory mapping\n", dev_name);
		    exit (EXIT_FAILURE);
		}
        else
		{
		    errno_exit ("err: VIDIOC_REQBUFS");
		}
    }
    else
    {
        printf("success in VIDEO_REQBUFS\n");
    }

    if (req.count < 2)
    {
      fprintf (stderr, "Insufficient buffer memory on %s\n", dev_name);
      exit (EXIT_FAILURE);
    }
    
    // 申请缓冲区
    buffers = calloc (req.count, sizeof (*buffers));

    if (!buffers)
    {
        fprintf (stderr, "Out of memory\n");
        exit (EXIT_FAILURE);
    }
//	    else
//	    {
//	        printf("success in calloc\n");
//	    }

    // 获取缓冲帧的地址及长度
    //struct v4l2_buffer ：代表驱动中的一帧
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers)
    {
//		printf("process n_buffers %d\n", n_buffers);
        CLEAR (buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
			errno_exit ("VIDIOC_QUERYBUF");
        else
        {
            printf("success in VIDEO_QUERYBUF\n");
//	            printf("buffer.length = %d\n",buf.length);
//	            printf("buffer.bytesused = %d\n",buf.bytesused);
        }

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap (NULL /* start anywhere */ ,
				       buf.length,
				       PROT_READ | PROT_WRITE /* required */ ,
				       MAP_SHARED /* recommended */ ,
				       fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start)
			errno_exit ("mmap");
//	        else
//	            printf("success in mmap\n");
    }
}

static void
init_userp (unsigned int buffer_size)
{
  struct v4l2_requestbuffers req;
  unsigned int page_size;

  page_size = getpagesize ();
  buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

  CLEAR (req);

  req.count = 4;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_USERPTR;

  if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req))
    {
      if (EINVAL == errno)
	{
	  fprintf (stderr, "%s does not support "
		   "user pointer i/o\n", dev_name);
	  exit (EXIT_FAILURE);
	}
      else
	{
	  errno_exit ("VIDIOC_REQBUFS");
	}
    }

  buffers = calloc (4, sizeof (*buffers));

  if (!buffers)
    {
      fprintf (stderr, "Out of memory\n");
      exit (EXIT_FAILURE);
    }

  for (n_buffers = 0; n_buffers < 4; ++n_buffers)
    {
      buffers[n_buffers].length = buffer_size;
      buffers[n_buffers].start = memalign ( /* boundary */ page_size,
					   buffer_size);

      if (!buffers[n_buffers].start)
	{
	  fprintf (stderr, "Out of memory\n");
	  exit (EXIT_FAILURE);
	}
    }
}

static void init_device (void)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    struct v4l2_fmtdesc fmtdesc; 
    struct v4l2_streamparm parm;
    v4l2_std_id std; 
    u_char ret;
    unsigned int min;

    // 查询设备属性： VIDIOC_QUERYCAP
    if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap))
    {
		if (EINVAL == errno)
		{
			fprintf (stderr, "%s is no V4L2 device\n", dev_name);
		    exit (EXIT_FAILURE);
		}
//			else
//			{
//			    errno_exit ("VIDIOC_QUERYCAP");
//			}
    }
//	    else
//	    {
//	        printf("%s is a V4L2 device\n", dev_name);
//	    }
    
#if 0

    fprintf(stdout, "DriverName:%s\nCard Name:%s\nBus info:%s\nDriverVersion:%u.%u.%u\n", cap.driver, cap.card, cap.bus_info, (cap.version>>16)&255, (cap.version>>8)&255, (cap.version)&255);  
   
    // 是否支持图像获取  
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
		fprintf (stderr, "%s is no video capture device\n", dev_name);
        exit (EXIT_FAILURE);
    }
    else
    {
        printf("%s is a video capture device\n", dev_name);
    }

//	    switch (io)
//	    {
//			case IO_METHOD_READ:
//				if (!(cap.capabilities & V4L2_CAP_READWRITE))
//				{
//				  fprintf (stderr, "%s does not support read i/o\n", dev_name);
//				  exit (EXIT_FAILURE);
//				}
//			    break;
//			case IO_METHOD_MMAP:
//			case IO_METHOD_USERPTR:
//				if (!(cap.capabilities & V4L2_CAP_STREAMING))
//				{
//				  fprintf (stderr, "%s does not support streaming i/o\n", dev_name);
//				  exit (EXIT_FAILURE);
//				}
//			    break;
//	    }

    if (!(cap.capabilities & V4L2_CAP_READWRITE))
    {
        printf("It can not be read\n");
    }
    else
    {
        printf("It can be read\n");
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        printf("It can not streaming\n");
    }
    else
    {
        printf("It can streaming\n");
    }
    
  /* Select video input, video standard and tune here. */

    // 图像的修剪缩放
    CLEAR (cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(fd, VIDIOC_CROPCAP, &cropcap)) 
    {
        printf("InitDevice:ioctl:VIDIOC_CROPCAP\n");
    }
    else
    {
        printf("VIDIOC_CROPCAP SUCCESS!\n");
        if (-1 == ioctl(fd, VIDIOC_G_CROP, &cropcap))
        {
            printf("VIDIOC_G_CROP err\n");
        }
        else
        {
            printf("cropcap.type = %d\n", cropcap.type);
            printf("cropcap.bounds.height = %d\n", cropcap.bounds.height);
            printf("cropcap.bounds.width = %d\n", cropcap.bounds.width);
            printf("cropcap.bounds.left = %d\n", cropcap.bounds.left);
            printf("cropcap.bounds.top = %d\n", cropcap.bounds.top);
            printf("cropcap.defrect.height = %d\n", cropcap.defrect.height);
            printf("cropcap.defrect.width = %d\n", cropcap.defrect.width);
            printf("cropcap.defrect.left = %d\n", cropcap.defrect.left);
            printf("cropcap.defrect.top = %d\n", cropcap.defrect.top);
        }

        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.bounds;
        if (-1 == ioctl(fd, VIDIOC_S_CROP, &crop))
        {
            printf("VIDIOC_S_CROP err\n");
        }
        else
        {
            printf("VIDIOC_S_CROP SUCCESS!\n");
        }
    }
    
#endif

    // 显示所有支持的格式
    CLEAR (fmtdesc);
    fmtdesc.index=0;
    fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE; 
//	    printf("Supportformat:\n");
    while(ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc)!=-1) 
    {
//	        printf("\tfmt_desc.index = %d\n", fmtdesc.index);
//	        printf(format,args...)("\tfmt_desc.type = %d\n", fmtdesc.type);
//	        printf("\tfmt_desc.description = %s\n", fmtdesc.description);
//	        printf("\tfmt_desc.pixelformat = %x\n", fmtdesc.pixelformat); 
//	        //printf("\t%d.%s\n",fmtdesc.index+1,fmtdesc.description); 
        fmtdesc.index++; 
    }

    // 检查是否支持某种帧格式
    CLEAR (fmt);
    fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE; 
    //fmt.fmt.pix.pixelformat=V4L2_PIX_FMT_SBGGR8;
    fmt.fmt.pix.pixelformat=V4L2_PIX_FMT_NV12;
    fmt.fmt.pix.width=1;
    fmt.fmt.pix.height=1;
    if(ioctl(fd,VIDIOC_TRY_FMT,&fmt)==-1)  
    {
        if(errno==EINVAL)  
        {
            //printf("not support format SBGGR8!\n"); 
        }
    }
    fmt.fmt.pix.width=32768;
    fmt.fmt.pix.height=32768;
    if(ioctl(fd,VIDIOC_TRY_FMT,&fmt)==-1)  
    {
        if(errno==EINVAL)  
        {
            //printf("not support format SBGGR8!\n"); 
        }
    }
//	    else
//	    {
//	         printf("support format SBGGR8\n");
//	    }

    // 检查当前视频设备支持的标准
//	    do 
//	    { 
//	        ret = ioctl(fd, VIDIOC_QUERYSTD, &std); 
//	    } while (ret == -1 && errno == EAGAIN); 
//	    switch (std) 
//	    { 
//	        case V4L2_STD_NTSC: //…… 
//	        case V4L2_STD_PAL: //…… 
//	    }
//    printf("std = 0x%lx\n", std);

    CLEAR (fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_G_FMT, &fmt);


    // 设置当前格式
    CLEAR (fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 1280;
    fmt.fmt.pix.height = 720;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
    //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SBGGR8;
    //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SBGGR16;
    //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    //fmt.fmt.pix.bytesperline=2560;
    printf("pixelformat = %x",fmt.fmt.pix.pixelformat);
    if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt))
		errno_exit ("VIDIOC_S_FMT");


//	    // 查询当前格式
//	    CLEAR (fmt);
//	    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//	    ioctl(fd, VIDIOC_G_FMT, &fmt);
//	    printf("Currentdata format information:\n\twidth:%d\n\theight:%d\n",fmt.fmt.pix.width,fmt.fmt.pix.height);
//	    if(fmt.fmt.pix.pixelformat==V4L2_PIX_FMT_SBGGR16)
//	    {
//	        printf("current format is V4L2_PIX_FMT_SBGGR16\n");
//	    }
//	    if(fmt.fmt.pix.pixelformat==V4L2_PIX_FMT_SBGGR8)
//	    {
//	        printf("current format is V4L2_PIX_FMT_SBGGR8\n");
//	    }
//	//	    switch(fmt.fmt.pix.field)
//	//	    {
//	//	
//	//	    }
//	    printf("V4L2_FIELD = %d\n", fmt.fmt.pix.field);

    // 设置视频采集的帧率及采集模式
    // 一般不需要设置
    CLEAR (parm);
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = 60;
    parm.parm.capture.capturemode = 0;

    if (-1 == ioctl(fd, VIDIOC_S_PARM, &parm))
    {
        //printf("VIDIOC_S_PARM failed ");
    }
//	    else
//	    {
//	        printf("VIDIOC_S_PARM SUCCESS!\n");
//	    }
    
    if (-1 == ioctl(fd, VIDIOC_G_PARM, &parm))
    {
        printf("VIDIOC_G_PARM failed ");
    }
//	    else
//	    {
//	        printf("streamparm:\n\tnumerator =%d\n\tdenominator=%d\n\tcapturemode=%d\n\n",
//	            parm.parm.capture.timeperframe.numerator,
//	            parm.parm.capture.timeperframe.denominator,
//	            parm.parm.capture.capturemode);
//	    }
    
        
    
//	    // 显示所有支持的格式
//	    CLEAR (fmtdesc);
//	    fmtdesc.index=0; 
//	    fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE; 
//	    printf("Supportformat:\n");
//	    while(ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc)!=-1) 
//	    {
//	        printf("\t%d.%s\n",fmtdesc.index+1,fmtdesc.description); 
//	        fmtdesc.index++; 
//	    }


  /* Note VIDIOC_S_FMT may change width and height. */

  /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

    switch (io)
    {
		case IO_METHOD_READ:
		  init_read (fmt.fmt.pix.sizeimage);
		  break;

		case IO_METHOD_MMAP:
		  init_mmap ();
		  break;

		case IO_METHOD_USERPTR:
		  init_userp (fmt.fmt.pix.sizeimage);
		  break;
    }


    // 有哪些输入设备
    struct v4l2_input input;
    input.type = V4L2_INPUT_TYPE_CAMERA;
    input.index = 0;
    while ((ret = ioctl(fd,VIDIOC_ENUMINPUT, &input) == 0))
    {
        printf("input.name = %s at input.index = %d\n", input.name, input.index);
        input.index++;
    }
	if (ret < 0) {
		printf("Couldn't find the input\n");
		//return -1;
	}

	// 设置输入设备
	input.index = 0;
//		printf("Calling S_INPUT with index = %d\n", input.index);
  	if (-1 == ioctl (fd, VIDIOC_S_INPUT, &input.index))
  	{
		//perror("Error:VIDIOC_S_INPUT\n");
      		//return -1;
  	}
//		else
//		{
//			printf("ioctl:VIDIOC_S_INPUT SUCCESS! selected input index = %d\n", input.index);
//		}

    ret = configCCDCraw(fd);
    if (ret < 0) {
        perror("configCCDCraw error");
//        return -1;
    } else {
        printf("\nconfigCCDCraw Done\n");
    }

	
//		// 检查是否设置成功
//		int temp_input;
//	
//		if (-1 == ioctl (fd, VIDIOC_G_INPUT, &temp_input))
//		{
//			perror("Error:InitDevice:ioctl:VIDIOC_G_INPUT\n");
//			//return -1;
//		}
//		else
//		{
//			printf("ioctl:VIDIOC_G_INPUT input index %d SUCCESS!\n", temp_input);
//		}


	// 检查输入视频标准
//		printf("Following standards available at the input\n");
//		struct v4l2_standard standard;
//		standard.index = 0;
//		while (0 == (ret = ioctl (fd, VIDIOC_ENUMSTD, &standard)))
//		{
//			printf("standard.index = %d\n", standard.index);
//			printf("\tstandard.id = %llx\n", standard.id);
//			printf("\tstandard.frameperiod.numerator = %d\n", standard.frameperiod.numerator);
//			printf("\tstandard.frameperiod.denominator = %d\n", standard.frameperiod.denominator);
//			printf("\tstandard.framelines = %d\n", standard.framelines);
//			standard.index++;
//		}
//		if(ret!=0)
//		{
//			printf("\tstandard not found!\n");
//		}

	
}

static void
close_device (void)
{
  if (-1 == close (fd))
    errno_exit ("close");

  fd = -1;
}

static void open_device (void)
{
	struct stat st;

	
//    printf("Configuring resizer in the chain mode\n");
//    printf("Opening resizer device, %s\n",dev_name_rsz);
    resizer_fd = open(dev_name_rsz, O_RDWR);
	
    oper_mode_1 = IMP_MODE_CONTINUOUS;
    //oper_mode_1 = 0;
	
    if (ioctl(resizer_fd, RSZ_S_OPER_MODE, &oper_mode_1) < 0) 
    {
    	perror("Error in setting default configuration for oper mode\n");
        close(resizer_fd);
    }
	
    if (ioctl(resizer_fd, RSZ_G_OPER_MODE, &user_mode_1) < 0) 
    {
    	perror("Error in getting default configuration for oper mode\n");
        close(resizer_fd);
    }
	
#if 1
    if (oper_mode_1 == user_mode_1)
    {
    //    printf("RESIZER: Operating mode changed successfully to Continuous");
    }
    else 
    {
        //printf("RESIZER: Couldn't change operation mode to Continuous");
        close(resizer_fd);
    }
#endif
	
    rsz_chan_config.oper_mode = IMP_MODE_CONTINUOUS;
    rsz_chan_config.chain = 1;
    rsz_chan_config.len = 0;
    rsz_chan_config.config = NULL; /* to set defaults in driver */
    if (ioctl(resizer_fd, RSZ_S_CONFIG, &rsz_chan_config) < 0) {
    	perror("Error in setting default configuration for continuous mode\n");
        close(resizer_fd);
    }
	
    CLEAR (rsz_ctn_config);
    rsz_chan_config.oper_mode = IMP_MODE_CONTINUOUS;
	rsz_chan_config.chain = 1;
	rsz_chan_config.len = sizeof(struct rsz_continuous_config);
	rsz_chan_config.config = &rsz_ctn_config;
    if (ioctl(resizer_fd, RSZ_G_CONFIG, &rsz_chan_config) < 0) {
    	perror("Error in getting default configuration for continuous mode\n");
        close(resizer_fd);
    }
	
#if 0
    printf("rsz_ctn_config.chroma_sample_even = %d\n", rsz_ctn_config.chroma_sample_even);
    printf("rsz_ctn_config.yuv_c_max = %d\n", rsz_ctn_config.yuv_c_max);
    printf("rsz_ctn_config.yuv_c_min = %d\n", rsz_ctn_config.yuv_c_min);
    printf("rsz_ctn_config.yuv_y_max = %d\n", rsz_ctn_config.yuv_y_max);
    printf("rsz_ctn_config.yuv_y_min = %d\n", rsz_ctn_config.yuv_y_min);
#endif
    rsz_ctn_config.output1.enable = 1;
    rsz_ctn_config.output2.enable = 0;
    rsz_chan_config.oper_mode = IMP_MODE_CONTINUOUS;
    rsz_chan_config.chain = 1;
    rsz_chan_config.len = sizeof(struct rsz_continuous_config);
    rsz_chan_config.config = &rsz_ctn_config; /* to set defaults in driver */
    if (ioctl(resizer_fd, RSZ_S_CONFIG, &rsz_chan_config) < 0) {
    	perror("Error in setting default configuration for continuous mode\n");
        close(resizer_fd);
    }


    
    preview_fd = open(dev_name_prev, O_RDWR);
    if(preview_fd <= 0)
    {
		printf("Cannot open previewer device\n");
        close(preview_fd);
	}
	
    if (ioctl(preview_fd, PREV_S_OPER_MODE, &oper_mode_1) < 0) 
    {
        perror("Can't set operation mode\n");
        close(preview_fd);
    }
	
	if (ioctl(preview_fd, PREV_G_OPER_MODE, &user_mode_1) < 0) 
    {
		perror("Can't get operation mode\n");
        close(preview_fd);
	}
	
#if 1
	if (oper_mode_1 == user_mode_1) 
    {   
		//printf("Operating mode changed successfully to continuous in previewer");
    }
	else 
    {
		//printf("failed to set mode to continuous in previewer\n");
		close(preview_fd);
	}
#endif
	
    prev_chan_config.oper_mode = IMP_MODE_CONTINUOUS;
    prev_chan_config.len = 0;
    prev_chan_config.config = NULL; /* to set defaults in driver */
    if (ioctl(preview_fd, PREV_S_CONFIG, &prev_chan_config) < 0) 
    {
        perror("Error in setting default configuration\n");
        close(preview_fd);
    }
	
#if 1
    CLEAR (prev_ctn_config);
    prev_chan_config.oper_mode = IMP_MODE_CONTINUOUS;
	prev_chan_config.len = sizeof(struct prev_continuous_config);
	prev_chan_config.config = &prev_ctn_config;
	if (ioctl(preview_fd, PREV_G_CONFIG, &prev_chan_config) < 0) 
    {
		perror("Error in getting configuration from driver\n");
        close(preview_fd);
	}

    prev_chan_config.oper_mode = IMP_MODE_CONTINUOUS;
    prev_chan_config.len = sizeof(struct prev_continuous_config);
	prev_chan_config.config = &prev_ctn_config;
    
#if 0
    /*
        Gb B
        R Gr
    */
        prev_ctn_config.input.colp_elep= IPIPE_RED;
        prev_ctn_config.input.colp_elop= IPIPE_GREEN_RED;
        prev_ctn_config.input.colp_olep= IPIPE_GREEN_BLUE;
        prev_ctn_config.input.colp_olop= IPIPE_BLUE;
#endif


#if 0
    /* 
        B Gb
        Gr R
    */
        prev_ctn_config.input.colp_elep= IPIPE_GREEN_RED;
        prev_ctn_config.input.colp_elop= IPIPE_RED;
        prev_ctn_config.input.colp_olep= IPIPE_BLUE;
        prev_ctn_config.input.colp_olop= IPIPE_GREEN_BLUE;
#endif

#if 1
    /*
        Gr R
        B Gb
    */
            prev_ctn_config.input.colp_elep= IPIPE_BLUE;
            prev_ctn_config.input.colp_elop= IPIPE_GREEN_BLUE;
            prev_ctn_config.input.colp_olep= IPIPE_GREEN_RED;
            prev_ctn_config.input.colp_olop= IPIPE_RED;
#endif

#if 0
    /*
        R Gr
        Gb B
    */

            prev_ctn_config.input.colp_elep= IPIPE_GREEN_BLUE;
            prev_ctn_config.input.colp_elop= IPIPE_BLUE;
            prev_ctn_config.input.colp_olep= IPIPE_RED;
            prev_ctn_config.input.colp_olop= IPIPE_GREEN_RED;
#endif
   
	if (ioctl(preview_fd, PREV_S_CONFIG, &prev_chan_config) < 0) 
    {
		perror("Error in setting default configuration\n");
        close(preview_fd);
	}

    
#endif

    if (-1 == stat (dev_name, &st))
    {
		fprintf (stderr, "Cannot identify '%s': %d, %s\n", dev_name, errno, strerror (errno));
		exit (EXIT_FAILURE);
    } 

    if (!S_ISCHR (st.st_mode))
    {
		fprintf (stderr, "%s is no device\n", dev_name);
        exit (EXIT_FAILURE);
    }

    
    
    // 用非阻塞模式打开摄像头设备 
    fd = open (dev_name, O_RDWR | O_NONBLOCK, 0);
    // 如果用阻塞模式打开摄像头设备，上述代码变为：
    // fd = open (dev_name, O_RDWR, 0);
    
    // tips:
    // 用非阻塞模式调用视频设备
    // 即使尚未捕获到信息，驱动依旧会把缓存（DQBUFF）
    // 里的东西返回给应用程序

    if (-1 == fd)
    {
		fprintf (stderr, "Cannot open '%s': %d, %s\n", dev_name, errno, strerror (errno));
        exit (EXIT_FAILURE);
    }
}

static void
usage (FILE * fp, int argc, char **argv)
{
  fprintf (fp,
	   "Usage: %s [options]\n\n"
	   "Options:\n"
	   "-d | --device name   Video device name [/dev/video]\n"
	   "-h | --help          Print this message\n"
	   "-m | --mmap          Use memory mapped buffers\n"
	   "-r | --read          Use read() calls\n"
	   "-u | --userp         Use application allocated buffers\n"
	   "", argv[0]);
}

static const char short_options[] = "d:hmru";

static const struct option long_options[] = {
  {"device", required_argument, NULL, 'd'},
  {"help", no_argument, NULL, 'h'},
  {"mmap", no_argument, NULL, 'm'},
  {"read", no_argument, NULL, 'r'},
  {"userp", no_argument, NULL, 'u'},
  {0, 0, 0, 0}
};

int main (int argc, char **argv)
{
	dev_name = "/dev/video0";

	for (;;)
    {
      int index;
      int c;

      c = getopt_long (argc, argv, short_options, long_options, &index);

      if (-1 == c)
		break;
	  switch (c)
	  {
			case 0:		/* getopt_long() flag */
				  break;
			case 'd':
				  dev_name = optarg;
				  break;
			case 'h':
				  usage (stdout, argc, argv);
				  exit (EXIT_SUCCESS);
			case 'm':
				  io = IO_METHOD_MMAP;
				  break;
			case 'r':
				  io = IO_METHOD_READ;
				  break;
			case 'u':
				  io = IO_METHOD_USERPTR;
				  break;
			default:
				  usage (stderr, argc, argv);
				  exit (EXIT_FAILURE);
	  }
    }

  open_device ();
  init_device ();

  start_capturing ();
  mainloop ();
  stop_capturing ();

  uninit_device ();

  close_device ();
  exit (EXIT_SUCCESS);

  return 0;
}



