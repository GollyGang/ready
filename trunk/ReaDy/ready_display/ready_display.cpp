/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See ../../README.txt for more details.

*/

// OpenCV:
#include <cv.h>
#include <highgui.h>

// stdlib
#include <stdio.h>

// local:
#include "ready_display.h"

#ifndef max
# define max(a,b) (((a) > (b)) ? (a) : (b))
# define min(a,b) (((a) < (b)) ? (a) : (b))
# define minmax(v, lo, hi) max(lo, min(v, hi))
#endif

#define INDEX(a,x,y) ((a)[(x)*g_width+(y)])

bool display(int g_width, int g_height, float *r, float *g, float *b,
             double iteration, float model_scale, bool auto_brighten,float manual_brighten,
             int image_scale,int delay_ms,const char* message, bool write_video)
{
  static bool need_init = true;

  static IplImage *im,*im2;
  static int border = 0;
  static CvFont font;
  static CvVideoWriter *video;
  static const CvScalar white = cvScalar(255,255,255);

  const char *title = "ReaDy (ESC to quit)";

  if(need_init)
  {
    need_init = false;

    im = cvCreateImage(cvSize(g_width,g_height),IPL_DEPTH_8U,3);
    cvSet(im,cvScalar(0,0,0));
    im2 = cvCreateImage(cvSize(g_width*image_scale,g_height*image_scale),IPL_DEPTH_8U,3);
        
    cvNamedWindow(title,CV_WINDOW_AUTOSIZE);
        
    double hScale=0.4;
    double vScale=0.4;
    int lineWidth=1;
    cvInitFont(&font,CV_FONT_HERSHEY_COMPLEX,hScale,vScale,0,lineWidth,CV_AA);

    if(write_video)
    {
      video = cvCreateVideoWriter("vid-Gray-Scott.avi",CV_FOURCC('D','I','V','X'),25.0,cvGetSize(im),1);
      if(video == NULL) {
        fprintf(stdout, "NULL from cvCreateVideoWriter\n"); exit(-1);
      }
    }
  }

  // convert float arrays to IplImage for OpenCV to display
  float val,minR=FLT_MAX,maxR=-FLT_MAX,minG=FLT_MAX,maxG=-FLT_MAX,minB=FLT_MAX,maxB=-FLT_MAX;
  for(int i=0;i<g_height;i++) {
    for(int j=0;j<g_width;j++) {
      val = INDEX(r,i,j);
      if(val<minR) minR=val; if(val>maxR) maxR=val;

      val = INDEX(g,i,j);
      if(val<minG) minG=val; if(val>maxG) maxG=val;

      val = INDEX(b,i,j);
      if(val<minB) minB=val; if(val>maxB) maxB=val;
    }
  }
  // I use this to find out the range of the chemicals when adding a new R-D system to ReaDy
  // printf("R:[%f..%f] G:[%f..%f] B:[%f..%f]\n", minR,maxR, minG,maxG, minB,maxB);

  for(int i=0;i<g_height;i++)
  {
    for(int j=0;j<g_width;j++)
    {
      float val;
      val = INDEX(r,i,g_width-j-1);
      if(auto_brighten) val = 255.0f * (val-minR) / (maxR-minR);
      else val = (val * 255.0 / manual_brighten);
      if(val<0) val=0; if(val>255) val=255;
      ((uchar *)(im->imageData + i*im->widthStep))[j*im->nChannels + 2] = (uchar)val;

      val = INDEX(g,i,g_width-j-1);
      if(auto_brighten) val = 255.0f * (val-minG) / (maxG-minG);
      else val = (val * 255.0 / manual_brighten);
      if(val<0) val=0; if(val>255) val=255;
      ((uchar *)(im->imageData + i*im->widthStep))[j*im->nChannels + 1] = (uchar)val;

      val = INDEX(b,i,g_width-j-1);
      if(auto_brighten) val = 255.0f * (val-minB) / (maxB-minB);
      else val = (val * 255.0 / manual_brighten);
      if(val<0) val=0; if(val>255) val=255;
      ((uchar *)(im->imageData + i*im->widthStep))[j*im->nChannels + 0] = (uchar)val;
    }
  }

  cvResize(im,im2);

  char txt[100];
  const char * fmt;

  if (g_width > 170) {
    fmt = "Generation=%7g (model's t=%g)";
  } else if (g_width > 140) {
    fmt = "Gen.=%7g (model's t=%g)";
  } else {
    fmt = "G=%7g t=%g";
  }
  sprintf(txt,fmt,iteration, ((double) iteration) / model_scale);
  cvPutText(im2,txt,cvPoint(5,20),&font,white);
  cvPutText(im2,message,cvPoint(5,40),&font,white);

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

void colorize(float *u, float *v, float *du, float uv_range,
  float *red, float *green, float *blue, long width, long height,
  int color_style, int pastel_mode)
{
  float r, g, b;
  // Step by row
  for(long i = 0; i < height*width; i++) {
    float uval = u[i] / uv_range;
    float vval = v[i] / uv_range;
    float delta_u = du[i] / uv_range;

    color_mapping(uval, vval, delta_u, color_style, &r, &g, &b, pastel_mode);
    red[i] = r;
    green[i] = g;
    blue[i] = b;
  }
}

/*
 These are all the colour schemes from Robert Munafo's PDE3 and PDE4 programs, some of which which date back to 1994.
Included are Pearson's original colours from the 1993 paper, some enhanced versions of Pearson with the derivative shown
in various ways, some monochrome mappings, and the colours used for all the figures and videos on
mrob.com/pub/comp/xmorphia.
 Colormap 22 is the one that used to be selected by "-color" (that option now selects colormap 17).
The rest, including colormap 0, are selected by "-oldcolor N"
 */
void color_mapping(float u0, float v0, float dU, int pm, float *red, float *green, float *blue, int pastel)
{
  float diff, dif2;
  float t1, t2, t3, t4;

  // Old ratio calculation doesn't work any better than a simple linear
  // scaling, so I've returned to that - 20090311
  diff = (dU * 3.0e5) + 0.5; // 3e5
  diff = minmax(diff, 0.0, 1.0);

  dif2 = (dU * 3.0e7) + 0.5; // 3e7
  dif2 = minmax(dif2, 0.0, 1.0);

  t1 = diff;
  t1 = minmax(t1, 0.0, 1.0);
    
  t2 = u0;
  t2 = minmax(t2, 0.0, 1.0);

  t3 = v0;
  t3 = minmax(t3, 0.0, 1.0);

  switch(pm) {
    case 0:
      // Original color scheme from the very very earliest versions of pde1.
      *red = t2; *green = t3; *blue = t2;
      break;
    case 1:
      // Like colormap 6: Red component shows whether U is increasing,
      // decreasing or is unchanged as compared to the (previous/next)
      // generation. (Green and Blue show U and V, like colormap 6).
      // This system dates back to 19940826, when I had not yet eliminated
      // the second frame buffer and the derivative calculation was thus
      // considerably easier.
      *red = t1; *green = t2; *blue = t3;
      break;
    case 2:
      *red = t2; *green = t1; *blue = t3;
      break;
    case 3:
      // The classic version of this colormap was:
      // *red = 1.0-t3; *green = 1.0-t3; *blue = 1.0-t2
      // But this just looks like a washed-out version of colormap 4, so I've
      // replaced it with this which is a nice blue and green variation
      *red = t3; *green = 1.0-t2; *blue = 1.0-t1;
      break;
    case 4:
      // Looks way too much like colormap 6
      *blue = 1.0-t2;
      t4 = 1.0-t3 - (*blue/2.0); if (t4 < 0) t4 = 0;
      *red = t4; *green = t4;
      break;
    case 5:
      t2 = 1.0-t2;
      t4 = 1.0-t3 - (t2/2.0); if (t4 < 0) t4 = 0;
      if (t4 > t2) {
        *red = t4-t2; *green = t4-t2; *blue = t2-t4+1.0;
      } else {
        *red = t2-t4; *green = t4-t2+1.0; *blue = t4-t2+1.0;
      }
      break;
    case 6:
      // This is the color mode I settled with early on; as of 2009
      // I didn't even remember ever having other color modes.
      t2 = 1.0-t2;
      t4 = 1.0-t3 - (t2/2); if (t4 < 0) t4 = 0;
      if (t4 > t2) {
        *red = t4-t2; *green = t4-t2; *blue = t2-t4+1.0;
      } else {
        *red = t2-t4; *green = 0; *blue = t4-t2+1.0;
      }
      break;
    case 7:
      // I added this color mode on 20090116 because I wanted something
      // pretty for the vision board, and for some variety -- all the
      // other color modes had yellow in the large area to the right!
      t2 = 1.0-t2;
      t4 = 1.0-t3 - (t2/2); if (t4 < 0) t4 = 0;
      if (t4 > t2) {
        *red = 1.0-(t4-t2); *green = 1.0-(t4-t2); *blue = (t4-t2)/2.0;
      } else {
        *red = 1.0-(t2-t4); *green = 1.0; *blue = (t2-t4)/2.0;
      }
      break;
    case 8:
      // This color mode is meant to reflect Pearson's description of the
      // two large plain states as "blue state" and "red state" (see the
      // Pearson paper, 9304003.pdf, page 9, caption for figure 3),
      // without actually using the HSV color space (for that use color
      // mode 9)
      *red = (1.0-t3)*0.75; *green = 0; *blue = 1.0-t2;
      break;
    default:
    case 9:
      // This is a reasonable facsimile of Pearson's coloring scheme. He
      // shows only the U value, expressing it as a hue in the HSV color
      // space.
      t2 = 1.0 - t2;
      t2 = t2 * 0.7 + 0.061;
      t3 = 1.0;
      t4 = 1.0;
      if (t2 < 0.02136) {
        t3 = 0.7324;
      } else if (t2 < 0.3052) {
        t3 = 0.7324 + (t2-0.2136) * 2.5;
      }
      if (t2 < 0.1526) {
        t4 = 0.4883;
      } else if (t2 < 0.2441) {
        t4 = 0.4883 + (t2-0.1526) * 5.0;
      }
      go_hsv2rgb(t2, t3, t4, red, green, blue);
      break;

    case 10: // `a'
      // Colorscheme created in 200901 for PDE3.
      // Like colormap 6: Saturation shows whether U is increasing, decreasing
      // or is unchanged as compared to the (previous/next) generation. Hue
      // shows U; value constant at 1.0.
      t1 = 0.5 + t1/2.0;
      go_hsv2rgb(0.8 * (1.0 - t2), t1, 1.0, red, green, blue);
      break;

    case 11: // `b'
      // First new colorscheme devised on 20090311. Rick likes this one.
      t1 = 0.5 + t1/2.0;
      go_hsv2rgb(0.8 * ((float) (1.0 - t2)), (float) t1, t2, red, green, blue);
      break;

    case 12: // `c'
      // This one compresses into MP4 better because it avoids bright red
      t1 = 0.25 + t1 * 0.75;
      go_hsv2rgb(0.8 * ((float) (t2)), t1, t2, red, green, blue);
      break;

    case 13: // `d'
      // Modification of scheme 10, uses a greater range of the color space
      t1 = 0.5 + t1/2.0;
      t4 = 4 * (1.0 - t2) / 5;
      t4 = t4 + 0.75; t4 = t4 - floor(t4); // olive, yellow, red, pink, lavender
      go_hsv2rgb(t4, t1, t2, red, green, blue);
      break;

    case 14: // `e'
      // Modification of scheme 13, limiting use of saturated colors to force
      // better quality out of H264/mp4
      t1 = 0.25 + t1 * 0.4;
      t4 = (1.0 - t2)*1.8; // more than one full turn around the hue wheel
      t4 = t4 + 0.5798; t4 = t4 - floor(t4); // deep purple, deep blue, green, yellow, red, pink, blue, aqua
      go_hsv2rgb(t4, t1, t2, red, green, blue);
      break;

    case 15: // `f'
      // Another one that compresses well, with orange in the "desert"
      t1 = 0.25 + t1 * 0.4;
      t4 = 6 * (1.0 - t2) / 5; // more than one full turn around the hue wheel
      t4 = t4 + 1.0681; t4 = t4 - floor(t4); // dark brown, deep purple, deep blue, green, yellow, orange
      go_hsv2rgb(t4, t1, t2, red, green, blue);
      break;

    case 16: // `g'
      // Modification of scheme 13, limiting use of saturated colors to force
      // better quality out of H264/mp4
      t1 = 0.25 + t1 * 0.4;
      t4 = (1.0 - t2) * 1.4;
      t4 = t4 + 0.6409; t4 = t4 - floor(t4); // deep purple, deep blue, teal, green, yellow, red, pink, blue
      go_hsv2rgb(t4, t1, t2, red, green, blue);
      break;

    case 17: // colormode 17, originally letter `h'
      // Back to colorscheme 11, but with subdued brightness and saturation
      t1 = 0.25 + t1 * 0.6;
      t4 = (1.0 - t2) * 1.0;
      t4 = t4 + 1.9836; t4 = t4 - floor(t4); // deep purple, deep blue, teal, green yellow, red
      go_hsv2rgb(t4, t1, 0.3815 + t2/2.0, red, green, blue);
      break;

    case 18: // `i'
      // Created on 20090404 specifically for monochrome figures in uskate
      // world. The first version of this was:
      //   t2 = minmax((t2 * 2) - 0.6104, 0.0, 1.0)
      // Then I did 5 shades of gray on thresholds 0.40, 0.418, 0.422, 0.55.
      // Finally, on 20090405 I optimized it for the B&W illustrations in the
      // paper I'm writing and came up with this combination of 3 linear
      // sclings.
      diff = (u0 - 0.4) * 16.28;
      if (diff > 1.22) {
        diff = (diff / 10.0) + 0.5;
      } else if (diff > 0.4578) {
        diff = (diff / 5.0) + 0.3662;
      }
      t2 = minmax(diff, 0.0, 1.0);
      go_hsv2rgb(0, 0, t2, red, green, blue);
      break;

    case 19: // `j'
      // Created on 20090404 specifically for monochrome figures in uskate world
      go_hsv2rgb(0, 0, (1.0 - t1), red, green, blue);
      break;

    case 20:
      // Created on 20090507, higher-contrast derivative
      t4 = 1.0 * dif2;
      t4 = minmax(t4, 0, 1.0);
      go_hsv2rgb(0, 0, (1.0 - t4), red, green, blue);
      break;

    case 21:
      // 20101122: Monochrome without contrast adjustment
      go_hsv2rgb(0, 0, u0, red, green, blue);
      break;

    case 22:
      // Created by Robert Munafo for Tim Hutton's reaction-diffusion project
      // Something simple to start (-:
      // different colour schemes result if you reorder these, or replace
      // "x" with "1.0f-x" for any of the 3 variables
      diff = dU * 1000.0f + 0.5f;
      diff = minmax(diff, 0.0, 1.0);
      *red = diff; // increasing U will look pink
      *green = 1.0-u0; *blue = 1.0-v0;

  }

  // Pastel mode transformation simply reduces all three components by half --
  // either as additive primaries (making it darker) or as subtractive
  // primaries (making it lighter)
  if (pastel == 1) {
    *red = 0.5 + *red/2.0;
    *green = 0.5 + *green/2.0;
    *blue = 0.5 + *blue/2.0;
  } else if (pastel == 2) {
    *red = *red/2.0;
    *green = *green/2.0;
    *blue = *blue/2.0;
  }
}

void go_hsv2rgb(float h, float s, float v, float *red, float *green, float *blue)
{
  float p16 = 65536.0;
  float i, f, p, q, t, r, g, b;
  int ii;

  if (s <= 0.0) {
    // Ignore hue
    r = v;
    g = v;
    b = v;
  } else {
    h = h * 6.0;
    ii = ((int) h);
    i = (float) ii;
    f = h - i;
    p = v*(1.0 - s);
    q = v*(1.0 - (s*f));
    t = v*(1.0 - (s*(1.0 - f)));
    switch(ii) {
      case 0:
        r = v; g = t; b = p;
        break;
      case 1:
        r = q; g = v; b = p;
        break;
      case 2:
        r = p; g = v; b = t;
        break;
      case 3:
        r = p; g = q; b = v;
        break;
      case 4:
        r = t; g = p; b = v;
        break;
      case 5:
      default:
        r = v; g = p; b = q;
        break;
    }
  }
  *red = r; *green = g; *blue = b;
}

