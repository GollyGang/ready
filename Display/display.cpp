// OpenCV:
#include <cv.h>
#include <highgui.h>

// stdlib
#include <stdio.h>

// local:
#include "display.h"

bool display(float r[X][Y],float g[X][Y],float b[X][Y],
             int iteration,bool auto_brighten,float manual_brighten,
			 int scale,int delay_ms,const char* window_title)
{
    static bool need_init = true;
	static bool write_video = false;

    static IplImage *im,*im2,*im3;
	static int border = 0;
    static CvFont font;
	static CvVideoWriter *video;
	static const CvScalar white = cvScalar(255,255,255);

    if(need_init)
    {
        need_init = false;

        im = cvCreateImage(cvSize(X,Y),IPL_DEPTH_8U,3);
        cvSet(im,cvScalar(0,0,0));
        im2 = cvCreateImage(cvSize(X*scale,Y*scale),IPL_DEPTH_8U,3);
        im3 = cvCreateImage(cvSize(X*scale+border*2,Y*scale+border),IPL_DEPTH_8U,3);
        
        cvNamedWindow(window_title,CV_WINDOW_AUTOSIZE);
        
        double hScale=0.4;
        double vScale=0.4;
        int lineWidth=1;
        cvInitFont(&font,CV_FONT_HERSHEY_COMPLEX,hScale,vScale,0,lineWidth,CV_AA);

		if(write_video)
		{
			video = cvCreateVideoWriter(window_title,CV_FOURCC('D','I','V','X'),25.0,cvGetSize(im3),1);
			border = 20;
		}
    }

    // convert float arrays to IplImage for OpenCV to display
    float val,minR=FLT_MAX,maxR=-FLT_MAX,minG=FLT_MAX,maxG=-FLT_MAX,minB=FLT_MAX,maxB=-FLT_MAX;
    if(auto_brighten)
    {
        for(int i=0;i<X;i++)
        {
            for(int j=0;j<Y;j++)
            {
				if(r) {
					val = r[i][j];
					if(val<minR) minR=val; if(val>maxR) maxR=val;
				}
				if(g) {
					val = g[i][j];
					if(val<minG) minG=val; if(val>maxG) maxG=val;
				}
				if(b) {
					val = b[i][j];
					if(val<minB) minB=val; if(val>maxB) maxB=val;
				}
            }
        }
    }
    for(int i=0;i<X;i++)
    {
        for(int j=0;j<Y;j++)
        {
			if(r) {
				val = r[i][Y-j-1];
				if(auto_brighten) val = 255.0f * (val-minR) / (maxR-minR);
				else val *= manual_brighten;
				if(val<0) val=0; if(val>255) val=255;
				((uchar *)(im->imageData + j*im->widthStep))[i*im->nChannels + 2] = (uchar)val;
			}
			if(g) {
				val = g[i][Y-j-1];
				if(auto_brighten) val = 255.0f * (val-minG) / (maxG-minG);
				else val *= manual_brighten;
				if(val<0) val=0; if(val>255) val=255;
				((uchar *)(im->imageData + j*im->widthStep))[i*im->nChannels + 1] = (uchar)val;
			}
			if(b) {
				val = b[i][Y-j-1];
				if(auto_brighten) val = 255.0f * (val-minB) / (maxB-minB);
				else val *= manual_brighten;
				if(val<0) val=0; if(val>255) val=255;
				((uchar *)(im->imageData + j*im->widthStep))[i*im->nChannels + 0] = (uchar)val;
			}
        }
    }

    cvResize(im,im2);
	cvCopyMakeBorder(im2,im3,cvPoint(border*2,0),IPL_BORDER_CONSTANT);

	char txt[100];
	if(!write_video)
	{
		sprintf(txt,"%d",iteration);
		cvPutText(im3,txt,cvPoint(20,20),&font,white);

		// DEBUG:
		sprintf(txt,"%.4f,%.4f,%.4f",r[0][0],g[0][0],b[0][0]);
		//cvPutText(im3,txt,cvPoint(20,40),&font,white);
	}

	// DEBUG:
	if(write_video)
	{
		cvPutText(im3,"0.06",cvPoint(5,15),&font,white);
		cvPutText(im3,"F",cvPoint(5,im2->height/2),&font,white);
		cvPutText(im3,"0.00",cvPoint(5,im2->height),&font,white);
		cvPutText(im3,"0.03",cvPoint(border*2-10,im2->height+15),&font,white);
		cvPutText(im3,"K",cvPoint(border*2+im2->width/2,im2->height+15),&font,white);
		cvPutText(im3,"0.07",cvPoint(im3->width-35,im2->height+15),&font,white);
	}

	if(write_video)
		cvWriteFrame(video,im3);

    cvShowImage(window_title,im3);
    
    int key = cvWaitKey(delay_ms); // allow time for the image to be drawn
    if(key==27) // did user ask to quit?
    {
        cvDestroyWindow(window_title);
        cvReleaseImage(&im);
        cvReleaseImage(&im2);
		if(write_video)
			cvReleaseVideoWriter(&video);
        return true;
    }
    return false;
}

