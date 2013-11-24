/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

*/

// stdlib:
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

// STL:
#include <vector>
using namespace std;

// OpenCV:
#include <cv.h>
#include <highgui.h>

// return a random value between lower and upper
float frand(float lower,float upper)
{
    return lower + rand()*(upper-lower)/RAND_MAX;
}

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

bool display(float *r,float *g,float *b,int S,
             int iteration,bool auto_brighten,float manual_brighten,
             float scale,int delay_ms,const char* window_title)
{
    static bool need_init = true;
    
    static bool write_video = true;
    static bool is_parameter_map = false;

    static IplImage *im,*im2,*im3;
    static int border = 0;
    static CvFont font;
    static CvVideoWriter *video;
    static const CvScalar white = cvScalar(255,255,255);

    int k=S/2;

    if(need_init)
    {
        need_init = false;

        im = cvCreateImage(cvSize(S,S),IPL_DEPTH_8U,3);
        cvSet(im,cvScalar(0,0,0));
        im2 = cvCreateImage(cvSize(int(S*scale),int(S*scale)),IPL_DEPTH_8U,3);
        
        cvNamedWindow(window_title,CV_WINDOW_AUTOSIZE);
        
        double hScale=0.4;
        double vScale=0.4;
        int lineWidth=1;
        cvInitFont(&font,CV_FONT_HERSHEY_COMPLEX,hScale,vScale,0,lineWidth,CV_AA);

        if(is_parameter_map)
        {
            border = 20;
        }

        im3 = cvCreateImage(cvSize(int(S*scale+border*2),int(S*scale+border)),IPL_DEPTH_8U,3);

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

    if(!im) return true;

    // convert float arrays to IplImage for OpenCV to display
    float val,minR=FLT_MAX,maxR=-FLT_MAX,minG=FLT_MAX,maxG=-FLT_MAX,minB=FLT_MAX,maxB=-FLT_MAX;
    if(auto_brighten)
    {
        for(int i=0;i<S;i++)
        {
            for(int j=0;j<S;j++)
            {
                if(r) {
                    val = r[(i*S+j)*S+k];
                    if(val<minR) minR=val; if(val>maxR) maxR=val;
                }
                if(g) {
                    val = g[(i*S+j)*S+k];
                    if(val<minG) minG=val; if(val>maxG) maxG=val;
                }
                if(b) {
                    val = b[(i*S+j)*S+k];
                    if(val<minB) minB=val; if(val>maxB) maxB=val;
                }
            }
        }
    }
    float rangeR = max(0.001f,maxR-minR);
    float rangeG = max(0.001f,maxG-minG);
    float rangeB = max(0.001f,maxB-minB);
    for(int i=0;i<S;i++)
    {
        for(int j=0;j<S;j++)
        {
            if(r) {
                val = r[(i*S+(S-j-1))*S+k];
                if(auto_brighten) val = 255.0f * (val-minR) / rangeR;
                else val *= manual_brighten;
                if(val<0) val=0; if(val>255) val=255;
                ((uchar *)(im->imageData + j*im->widthStep))[i*im->nChannels + 2] = (uchar)val;
            }
            if(g) {
                val = g[(i*S+(S-j-1))*S+k];
                if(auto_brighten) val = 255.0f * (val-minG) / rangeG;
                else val *= manual_brighten;
                if(val<0) val=0; if(val>255) val=255;
                ((uchar *)(im->imageData + j*im->widthStep))[i*im->nChannels + 1] = (uchar)val;
            }
            if(b) {
                val = b[(i*S+(S-j-1))*S+k];
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

    // show the iteration count
    sprintf(txt,"%d",iteration);
    cvPutText(im3,txt,cvPoint(20,20),&font,white);

    if(!write_video)
    {
        if(auto_brighten)
        {
            // show the range of chemical concentrations
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

int main()
{
    // -- parameters --

    // From Hagberg and Meron:
    // http://arxiv.org/pdf/patt-sol/9401002

    // tip-splitting
    float a0 = -0.1f;
    float a1 = 2.0f;
    float epsilon = 0.05f;
    float delta = 4.0f;
    float k1 = 1.0f;
    float k2 = 0.0f;
    float k3 = 1.0f;
    bool spiral_waves = false;

    // for spiral turbulence:
    /*float a0 = -0.1f;
    float a1 = 2.0f;
    float epsilon = 0.014f;
    float delta = 2.8f;
    float k1 = 1.0f;
    float k2 = 0.0f;
    float k3 = 1.0f;
    bool spiral_waves = false;*/

    // for spiral waves: http://thevirtualheart.org/java/2dfhn.html
    /*float a0 = 0.0f;
    float a1 = 1.0f;
    float epsilon = 0.01f;
    float delta = 0.0f;
    float k1 = -0.1f;
    float k2 = -1.1f;
    float k3 = 0.5f;
    bool spiral_waves = true;
    */

    // from Malevanets and Kapral (can't get these to work)

    // for labyrinth:
    /*float a0 = 0.146f;
    float a1 = 3.05f;
    float epsilon = 0.017f;
    float delta = 4.0f;
    float k1 = 1.0f;
    float k2 = 0.0f;
    float k3 = 1.0f;*/
    
    // for bloch fronts:
    /*float a0 = 0.0f;
    float a1 = 4.88f;
    float epsilon = 0.084f;
    float delta = 0.0f;
    float k1 = 1.0f;
    float k2 = 0.0f;
    float k3 = 1.0f;*/
 
    float speed = 0.02f;
    bool parameter_map = false;
    // ----------------

    const int S = 50;

    // these arrays store the chemical concentrations:
    float *a = new float[S*S*S],*b=new float[S*S*S];
    // these arrays store the rate of change of those chemicals:
    float *da = new float[S*S*S],*db=new float[S*S*S];

    // put the initial conditions into each cell
    srand((unsigned int)time(NULL));
    for(int i = 0; i < S; i++) 
    {
        for(int j = 0; j < S; j++) 
        {
            for(int k = 0; k < S; k++) 
            {
                a[(i*S+j)*S+k] = 0.0f;
                b[(i*S+j)*S+k] = 0.0f;
                float x=i,y=j,z=k;
                // rotate
                {
                    const float PI=3.1415926535f;
                    float a1=PI/10,a2=PI/9,a3=PI/12; // angles
                    x = i * cos(a2)*cos(a3) + j * (-cos(a1)*sin(a3)+sin(a1)*sin(a2)*cos(a3)) + k * (sin(a1)*sin(a3)+cos(a1)*sin(a2)*cos(a3));
                    y = i * cos(a2)*sin(a3) + j * (cos(a1)*cos(a3)+sin(a1)*sin(a2)*sin(a3)) + k * (-sin(a1)*cos(a3)+cos(a1)*sin(a2)*sin(a3));
                    z = i*(-sin(a2)) + j * (sin(a1)*cos(a2)) + k * (cos(a1)*cos(a2));
                }
                if( abs(x-S/2)<S/4 && abs(y-S/3)<4 && abs(z-S/4)<4 ) // start with a static line in the centre
                {
                    a[(i*S+j)*S+k] = frand(0.0f,1.0f);
                    b[(i*S+j)*S+k] = 0.0f;
                }
            }
        }
    }
    
    int iteration = 0;
    int output_frame = 0;
    while(true) 
    {
        // compute:
        int iprev,inext,jprev,jnext,kprev,knext;
        bool toroidal = false;

        // compute change in each cell
        for(int i = 0; i < S; i++) 
        {
            if(toroidal) { iprev = (i + S - 1) % S; inext = (i + 1) % S; }
            else { iprev=max(0,i-1); inext=min(S-1,i+1); }

            for(int j = 0; j < S; j++) 
            {
                if(toroidal) { jprev = (j + S - 1) % S; jnext = (j + 1) % S; }
                else { jprev=max(0,j-1); jnext=min(S-1,j+1); }

                for(int k = 0; k < S; k++) 
                {
                    if(toroidal) { kprev = (k + S - 1) % S; knext = (k + 1) % S; }
                    else { kprev=max(0,k-1); knext=min(S-1,k+1); }

                    float aval = a[(i*S+j)*S+k];
                    float bval = b[(i*S+j)*S+k];

                    // compute the Laplacians of a and b
                    float dda = a[(i*S+jprev)*S+k] + a[(i*S+jnext)*S+k] + a[(iprev*S+j)*S+k] + a[(inext*S+j)*S+k] + a[(i*S+j)*S+kprev] + a[(i*S+j)*S+knext] - 6*aval;
                    float ddb = b[(i*S+jprev)*S+k] + b[(i*S+jnext)*S+k] + b[(iprev*S+j)*S+k] + b[(inext*S+j)*S+k] + b[(i*S+j)*S+kprev] + b[(i*S+j)*S+knext] - 6*bval;

                    // compute the new rate of change of a and b
                    da[(i*S+j)*S+k] = k1*aval - k2*aval*aval - aval*aval*aval - bval + dda;
                    db[(i*S+j)*S+k] = epsilon * (k3*aval - a1*bval - a0) + delta*ddb;
                }
            }
        }

        // effect change
        for(int i = 0; i < S; i++) {
            for(int j = 0; j < S; j++) {
                for(int k = 0; k < S; k++) {
                    a[(i*S+j)*S+k] += (speed * da[(i*S+j)*S+k]);
                    b[(i*S+j)*S+k] += (speed * db[(i*S+j)*S+k]);
                }
            }
        }

        // display
        if(iteration%50==0)
        {
            if(display(a,b,b,S,iteration,true,1.0f,3.0f,10,"FHN 3D (Esc to quit)"))
                break;

            // output volume in VTK format
            {
                char txt[100];
                sprintf(txt,"vol_%05d.vtk",output_frame);
                FILE *out = fopen(txt,"wt");
                fprintf(out,"# vtk DataFile Version 2.0\nVolume example\nASCII\nDATASET STRUCTURED_POINTS\nDIMENSIONS %d %d %d\n",S,S,S);
                fprintf(out,"ASPECT_RATIO 1 1 1\nORIGIN 0 0 0\nPOINT_DATA %d\nSCALARS volume_scalars float 1\nLOOKUP_TABLE default\n",S*S*S);
                for(int i=0;i<S*S*S;i++)
                    fprintf(out,"%.3f ",a[i]);
                fclose(out);
                output_frame++;
            }
        }

        iteration++;
    }

    // clean up
    delete []a;
    delete []b;
    delete []da;
    delete []db;
}
