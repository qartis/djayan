class FL_EXPORT Fl_GIF : public Fl_Pixmap {
private:
    unsigned int FrameCount; // Number of frames in (animated) GIF image
    Fl_Widget* _parent; 
    char **new_data;	// Data array containing decoded scan lines of image
    std::vector<char**>* Frame;  // dynamic array to contain decoded image data
    std::vector<unsigned int>* FrameHeight;  // dynamic array to contain decoded image data
    std::vector<unsigned int>* FrameDelay;  // Frame delay in msec
    FILE *GifFile;	// File to read
    int canvasWidth; 
    int canvasHeight;
    int repcount; 
    int read_image_data(int Left,int Top,int Width,int Height,int BitsPerPixel);
    void convert_to_xpm(int Left,int Top,int Width,int Height,int BitsPerPixel);
    int CodeSize;		/* Code size, init from GIF header, increases... */
    char Interlace;
    uchar transparent_pixel;
    char has_transparent;
    uchar* p;
    /* for temporary colormap */
    uchar* Red;
    uchar* Green;
    uchar* Blue;
    uchar* Image;
    short int* Prefix;
    uchar* Suffix;
    int foffset;		/* Current file cursor offset (mostly for debugging purposes) */
    int blocklen;
    int nextbyte();	/* Reads 1 byte from the file, updates offset and optionally prints */
    int getshort();
    int run_animation;   /* true or false */
    int localcolormap;
    static void animate(void* instance); /* Start animating the image */
    unsigned int currframe;

public:
    void parent(Fl_Widget* parent);
    Fl_Widget* parent();
    void animating(int zeroforno_oneforyes); /* False to freeze animation, true to run it. Default is true. */
    int animating(); /* returns current animation status (0=set for not animating, 1=set for animating) */
    void select_frame(unsigned int framenum); /* Used to select a given frame number. */
    unsigned int framecount();
    unsigned int framedelay();
    void framedelay(unsigned int newdelay_msec); 
    unsigned int repeatcount();
    void repeatcount(unsigned int newcount);
    Fl_GIF(const char* filename);
    ~Fl_GIF();
};
