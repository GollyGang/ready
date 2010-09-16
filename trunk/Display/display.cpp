// OpenCV:
#include <cv.h>
#include <highgui.h>

// stdlib:
#include <stdio.h>

// local:
#include "display.h"

bool display(float r[X][Y],float g[X][Y],float b[X][Y],
             int iteration,bool auto_brighten,float manual_brighten,
             float scale,int delay_ms,const char* window_title)
{
    static bool need_init = true;
    
    static bool write_video = false;
    static bool is_parameter_map = false;

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
        im2 = cvCreateImage(cvSize(int(X*scale),int(Y*scale)),IPL_DEPTH_8U,3);
        
        cvNamedWindow(window_title,CV_WINDOW_AUTOSIZE);
        
        double hScale=0.4;
        double vScale=0.4;
        int lineWidth=1;
        cvInitFont(&font,CV_FONT_HERSHEY_COMPLEX,hScale,vScale,0,lineWidth,CV_AA);

        if(is_parameter_map)
        {
            border = 20;
        }

        im3 = cvCreateImage(cvSize(int(X*scale+border*2),int(Y*scale+border)),IPL_DEPTH_8U,3);

        if(write_video)
        {
            float fps=30.0f;
            // try to find a codec that works
            video = cvCreateVideoWriter("out.avi",CV_FOURCC('D','I','V','X'),fps,cvGetSize(im3));
            if(video==NULL)
                video = cvCreateVideoWriter("out.avi",CV_FOURCC('M','J','P','G'),fps,cvGetSize(im3));
            if(video==NULL)
                video = cvCreateVideoWriter("out.avi",CV_FOURCC('P','I','M','1'),fps,cvGetSize(im3));
            if(video==NULL)
                video = cvCreateVideoWriter("out.avi",CV_FOURCC('P','I','C','1'),fps,cvGetSize(im3));
            if(video==NULL)
                video = cvCreateVideoWriter("out.avi",CV_FOURCC('M','P','4','2'),fps,cvGetSize(im3));
            if(video==NULL)
                video = cvCreateVideoWriter("out.avi",CV_FOURCC('D','I','V','3'),fps,cvGetSize(im3));
            if(video==NULL)
                video = cvCreateVideoWriter("out.avi",CV_FOURCC('V','P','3','1'),fps,cvGetSize(im3));
            if(video==NULL)
                video = cvCreateVideoWriter("out.avi",CV_FOURCC('V','P','3','0'),fps,cvGetSize(im3));
            if(video==NULL)
                video = cvCreateVideoWriter("out.avi",CV_FOURCC('C','V','I','D'),fps,cvGetSize(im3));
            if(video==NULL)
                video = cvCreateVideoWriter("out.avi",CV_FOURCC('f','f','d','s'),fps,cvGetSize(im3));
            if(video==NULL)
                video = cvCreateVideoWriter("out.mpg",CV_FOURCC_DEFAULT,fps,cvGetSize(im3));
            if(video==NULL)
                video = cvCreateVideoWriter("out.avi",-1,fps,cvGetSize(im3)); // ask usr to choose
            if(video==NULL)
                video = cvCreateVideoWriter("im_00001.png",0,fps,cvGetSize(im3)); // fall back on image output
            // to get video output working on linux, you may need to recompile opencv to include ffmpeg,
            // as described here: http://opencv.willowgarage.com/wiki/InstallGuide (I did on Ubuntu 10.04)
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
    float rangeR = max(0.001f,maxR-minR);
    float rangeG = max(0.001f,maxG-minG);
    float rangeB = max(0.001f,maxB-minB);
    for(int i=0;i<X;i++)
    {
        for(int j=0;j<Y;j++)
        {
            if(r) {
                val = r[i][Y-j-1];
                if(auto_brighten) val = 255.0f * (val-minR) / rangeR;
                else val *= manual_brighten;
                if(val<0) val=0; if(val>255) val=255;
                ((uchar *)(im->imageData + j*im->widthStep))[i*im->nChannels + 2] = (uchar)val;
            }
            if(g) {
                val = g[i][Y-j-1];
                if(auto_brighten) val = 255.0f * (val-minG) / rangeG;
                else val *= manual_brighten;
                if(val<0) val=0; if(val>255) val=255;
                ((uchar *)(im->imageData + j*im->widthStep))[i*im->nChannels + 1] = (uchar)val;
            }
            if(b) {
                val = b[i][Y-j-1];
                if(auto_brighten) val = 255.0f * (val-minB) / rangeB;
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

        sprintf(txt,"bottom-left: %.4f,%.4f,%.4f",r[0][0],g[0][0],b[0][0]);
        cvPutText(im3,txt,cvPoint(20,40),&font,white);
        
        if(auto_brighten)
        {
            sprintf(txt,"chemical 1 range: %.4f - %.4f",minR,maxR);
            cvPutText(im3,txt,cvPoint(20,60),&font,white);
            sprintf(txt,"chemical 2 range: %.4f - %.4f",minG,maxG);
            cvPutText(im3,txt,cvPoint(20,80),&font,white);
            sprintf(txt,"chemical 3 range: %.4f - %.4f",minB,maxB);
            cvPutText(im3,txt,cvPoint(20,100),&font,white);
        }
    }

    if(is_parameter_map)
    {
        // you'll need to change these labels to your needs
        cvPutText(im3,"0.14",cvPoint(5,15),&font,white);
        cvPutText(im3,"F",cvPoint(5,im2->height/2),&font,white);
        cvPutText(im3,"0.00",cvPoint(5,im2->height),&font,white);
        cvPutText(im3,"0.045",cvPoint(border*2-10,im2->height+15),&font,white);
        cvPutText(im3,"K",cvPoint(border*2+im2->width/2,im2->height+15),&font,white);
        cvPutText(im3,"0.07",cvPoint(im3->width-40,im2->height+15),&font,white);
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

