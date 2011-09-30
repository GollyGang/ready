// OpenCV:
#include <cv.h>
#include <highgui.h>

// stdlib
#include <stdio.h>

// local:
#include "display_hwiv.h"

#define INDEX(a,x,y) ((a)[(x)*g_width+(y)])

bool display(int g_width, int g_height, float *r, float *g, float *b,
             int iteration,bool auto_brighten,float manual_brighten,
			 int scale,int delay_ms,const char* message, bool write_video)
{
    static bool need_init = true;

    static IplImage *im,*im2; // ,*im3;
	static int border = 0;
    static CvFont font;
	static CvVideoWriter *video;
	static const CvScalar white = cvScalar(255,255,255);

	const char *title = "Press ESC to quit";

    if(need_init)
    {
        need_init = false;

        im = cvCreateImage(cvSize(g_width,g_height),IPL_DEPTH_8U,3);
        cvSet(im,cvScalar(0,0,0));
        im2 = cvCreateImage(cvSize(g_width*scale,g_height*scale),IPL_DEPTH_8U,3);
//        im3 = cvCreateImage(cvSize(g_width*scale+border*2,g_height*scale+border),IPL_DEPTH_8U,3);
        
        cvNamedWindow(title,CV_WINDOW_AUTOSIZE);
        
        double hScale=0.4;
        double vScale=0.4;
        int lineWidth=1;
        cvInitFont(&font,CV_FONT_HERSHEY_COMPLEX,hScale,vScale,0,lineWidth,CV_AA);

		if(write_video)
		{
//			video = cvCreateVideoWriter(title,CV_FOURCC('D','I','V','X'),25.0,cvGetSize(im3),1);
			video = cvCreateVideoWriter("vid-Gray-Scott.avi",CV_FOURCC('D','I','V','X'),25.0,cvGetSize(im),1);
//			video = cvCreateVideoWriter("vid-Gray-Scott.avi",CV_FOURCC('P','I','M','1'),25.0,cvGetSize(im),1);
			if(video == NULL) {
				fprintf(stdout, "NULL from cvCreateVideoWriter\n"); exit(-1);
			}
//			border = 20;
		}
    }

    // convert float arrays to IplImage for OpenCV to display
    float val,minR=FLT_MAX,maxR=-FLT_MAX,minG=FLT_MAX,maxG=-FLT_MAX,minB=FLT_MAX,maxB=-FLT_MAX;
    if(auto_brighten)
    {
        for(int i=0;i<g_height;i++)
        {
            for(int j=0;j<g_width;j++)
            {
				val = INDEX(r,i,j);
				if(val<minR) minR=val; if(val>maxR) maxR=val;

				val = INDEX(g,i,j);
				if(val<minG) minG=val; if(val>maxG) maxG=val;

				val = INDEX(b,i,j);
				if(val<minB) minB=val; if(val>maxB) maxB=val;
            }
        }
    }
    for(int i=0;i<g_height;i++)
    {
        for(int j=0;j<g_width;j++)
        {
			float val;
			val = INDEX(r,i,g_width-j-1);
			if(auto_brighten) val = 255.0f * (val-minR) / (maxR-minR);
			else val *= manual_brighten;
			if(val<0) val=0; if(val>255) val=255;
			((uchar *)(im->imageData + i*im->widthStep))[j*im->nChannels + 2] = (uchar)val;

			val = INDEX(g,i,g_width-j-1);
			if(auto_brighten) val = 255.0f * (val-minG) / (maxG-minG);
			else val *= manual_brighten;
			if(val<0) val=0; if(val>255) val=255;
			((uchar *)(im->imageData + i*im->widthStep))[j*im->nChannels + 1] = (uchar)val;

			val = INDEX(b,i,g_width-j-1);
			if(auto_brighten) val = 255.0f * (val-minB) / (maxB-minB);
			else val *= manual_brighten;
			if(val<0) val=0; if(val>255) val=255;
			((uchar *)(im->imageData + i*im->widthStep))[j*im->nChannels + 0] = (uchar)val;
        }
    }

    cvResize(im,im2);

// Border wasn't doing anything for me
//		cvCopyMakeBorder(im2,im3,cvPoint(border*2,0),IPL_BORDER_CONSTANT);

	char txt[100];

	sprintf(txt,"%d",iteration);
	cvPutText(im2,txt,cvPoint(20,20),&font,white);
	cvPutText(im2,message,cvPoint(20,40),&font,white);

	if(write_video)
		cvWriteFrame(video,im);

    cvShowImage(title,im2);
    
    int key = cvWaitKey(delay_ms); // allow time for the image to be drawn
    if(key==27) // did user ask to quit?
    {
		if(write_video)
			cvReleaseVideoWriter(&video);
        cvDestroyWindow(title);
        cvReleaseImage(&im);
        cvReleaseImage(&im2);
        return true;
    }
    return false;
}

