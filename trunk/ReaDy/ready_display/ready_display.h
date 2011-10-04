bool display(int width, int height, float *r, float *g, float *b,
             double iteration, float model_scale, bool auto_brighten,float manual_brighten,
			 int scale,int delay_ms,const char* message, bool write_video);

void colorize(float *u, float *v, float *du,
             float *red, float *green, float *blue,
             long width, long height, int color_style, int pastel_mode);
void color_mapping(float u0, float v0, float dU, int pm, float *red, float *green, float *blue, int pastel);
void go_hsv2rgb(float h, float s, float v, float *red, float *green, float *blue);

