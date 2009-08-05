static const char* const CLASS = "BasicOpenCV";
static const char* const CATEGORY = "Image/BasicOpenCV";
static const char* const DISPLAY_NAME = "BasicOpenCV";
static const char* const HELP = "Performs some basic drawing operations.";

#include <stdarg.h>
#include <stdio.h>

#include <DDImage/Iop.h>
#include <DDImage/Row.h>
#include <DDImage/Knobs.h>
#include <DDImage/Tile.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>

using namespace DD::Image;


//
// DECLARATIONS
//

class BasicOpenCV: public Iop {
public:
    static const Description desc;

    BasicOpenCV(Node* node);
    virtual ~BasicOpenCV();

    virtual const char* Class() const;
    virtual const char* displayName() const;
    virtual const char* node_help() const;
    virtual void knobs(Knob_Callback f);

protected:
    virtual void _validate(bool for_real);
    virtual void _request(int x, int y, int r, int t, ChannelMask channels, int count);
    virtual void _open();
    virtual void engine(int y, int x, int r, ChannelMask channels, Row& row);
    virtual void _close();

private:
    IplImage *build_opencv_image() const;
    void print_image_info() const;
    void detect_and_draw(IplImage *img) const;

    const char *m_filename;
    const char *m_cascadeFile;

    IplImage *m_img;
    int m_boxW;
    int m_boxH;

    CvMemStorage *m_storage;
    CvHaarClassifierCascade *m_cascade;
};


//
// PUBLIC METHODS
//

BasicOpenCV::BasicOpenCV(Node *node) : Iop(node) {
    m_filename = NULL;
    m_cascadeFile = NULL;

    m_img = NULL;
    m_storage = cvCreateMemStorage(0);
    m_cascade = NULL;
}


BasicOpenCV::~BasicOpenCV() {
    if (m_img != NULL) {
        cvReleaseImage(&m_img);
        m_img = NULL;
    }
    if (m_cascade != NULL) {
        cvRelease((void **)&m_cascade);
        m_cascade = NULL;
    }
    cvClearMemStorage(m_storage);
    cvReleaseMemStorage(&m_storage);
}


const char* BasicOpenCV::Class() const {
    return desc.name;
}


const char* BasicOpenCV::displayName() const {
    return DISPLAY_NAME;
}


const char* BasicOpenCV::node_help() const {
    return HELP;
}


void BasicOpenCV::knobs(Knob_Callback f) {
    File_knob(f, &m_filename, "imagefile", "imagefile");
    File_knob(f, &m_cascadeFile, "cascadefile", "cascadefile");
}


//
// PROTECTED METHODS
//

void BasicOpenCV::_validate(bool for_real) {
    fprintf(stderr, "_validate called\n");
    copy_info();
    m_boxW = 32;
    m_boxH = 32;
}


void BasicOpenCV::_request(int x, int y, int r, int t,
                           ChannelMask channels, int count)
{
    fprintf(stderr, "_request called\n");

    // We need to request the whole input image here, unfortunately.
    input0().request(0, 0, info_.w(), info_.h(), channels, count);
}


void BasicOpenCV::_open() {
    fprintf(stderr, "_open called\n");

    if (m_img != NULL) {
        cvReleaseImage(&m_img);
        m_img = NULL;
    }

    if (m_cascade != NULL) {
        cvRelease((void **)&m_cascade);
        m_cascade = NULL;
    }

    if (m_filename != NULL && strlen(m_filename) > 0) {
        m_img = cvLoadImage(m_filename, 1);
        if (m_img == NULL) {
            error("Unable to load the source image.");
            return;
        }
    }
    print_image_info();

    if (m_cascadeFile != NULL && strlen(m_cascadeFile) > 0) {
        m_cascade = (CvHaarClassifierCascade *)cvLoad(m_cascadeFile);
        if (m_cascade == NULL) {
            error("Unable to load the Haar classifier cascade file.");
            return;
        }
    }

    if (m_img != NULL && m_cascade != NULL)
        detect_and_draw(m_img);
}


void BasicOpenCV::engine(int y, int x, int r, ChannelMask channels, Row& row) {
    fprintf(stderr, "engine called\n");

    float* p[3];
    p[0] = row.writable(Chan_Blue);
    p[1] = row.writable(Chan_Green);
    p[2] = row.writable(Chan_Red);

    if (m_img != NULL) {
        y = m_img->height - y;
        unsigned char *pixel = (unsigned char *)m_img->imageData + y * m_img->widthStep;
        for (int i = x; i < r; ++i) {
            for (int channel = 0; channel < 3; ++channel)
                p[channel][i] = pixel[channel] / 255.0f;
            pixel += 3;
        }
    } else {
        for (int i = x; i < r; ++i) {
            bool in_box = (((i / m_boxW) + (y / m_boxH)) % 2) == 0;
            if (in_box) {
                p[0][i] = 0.75f;
                p[1][i] = 0.75f;
                p[2][i] = 0.75f;
            }
        }
    }
}


void BasicOpenCV::_close() {
    fprintf(stderr, "_close() called\n");
    if (m_img != NULL) {
        cvReleaseImage(&m_img);
        m_img = NULL;
    }
}


//
// PRIVATE METHODS
//

IplImage *BasicOpenCV::build_opencv_image() const {
    int w = info_.w();
    int h = info_.h();

    // Load the entire source image into the cache.
    Tile tile(input0(), 0, 0, w, h, Mask_RGB);
    if (Op::aborted())
        return NULL;

    IplImage *img = cvCreateImage(cvSize(w, h), 8, 3);
    char *pixel;;
    for (int y = 0; y < h; ++y) {
        // Get the incoming pixel data for this row.
        Row in(0, w);
        in.get(input0(), y, 0, w, Mask_RGB);

        // Write the pixels to the OpenCV image.
        int imgY = h - y;
        pixel = img->imageData + imgY * img->widthStep;
        for (int x = 0; x < w; ++x) {
            pixel[0] = (char)fast_rint(in[Chan_Blue][x] * 255.0);
            pixel[1] = (char)fast_rint(in[Chan_Green][x] * 255.0);
            pixel[2] = (char)fast_rint(in[Chan_Red][x] * 255.0);
            pixel += 3;
        }
    }

    return img;
}


void BasicOpenCV::print_image_info() const {
    if (m_img != NULL) {
        fprintf(stderr, "Loaded image %s:\n", m_filename);
        fprintf(stderr, "    # channels:    %d\n", m_img->nChannels);
        fprintf(stderr, "    alpha channel: %d\n", m_img->alphaChannel);
        fprintf(stderr, "    depth:         %d\n", m_img->depth);
        fprintf(stderr, "    data order:    %d\n", m_img->dataOrder);
        fprintf(stderr, "    origin:        %d\n", m_img->origin);
        fprintf(stderr, "    align:         %d\n", m_img->align);
        fprintf(stderr, "    width:         %d\n", m_img->width);
        fprintf(stderr, "    height:        %d\n", m_img->height);
        fprintf(stderr, "    width step:    %d\n", m_img->widthStep);
    }
}


void BasicOpenCV::detect_and_draw(IplImage *img) const {
    double scale = 1.3;
    IplImage* gray = cvCreateImage(cvSize(img->width,img->height), 8, 1);
    IplImage* small_img = cvCreateImage(
            cvSize(cvRound (img->width/scale), cvRound (img->height/scale)),
            8, 1);

    cvCvtColor(img, gray, CV_BGR2GRAY);
    cvResize(gray, small_img, CV_INTER_LINEAR);
    cvEqualizeHist(small_img, small_img);
    cvClearMemStorage(m_storage);

    if (m_cascade != NULL) {
        double t = (double)cvGetTickCount();
        CvSeq* faces = cvHaarDetectObjects( small_img, m_cascade, m_storage,
                                            1.1, 2, 0/*CV_HAAR_DO_CANNY_PRUNING*/,
                                            cvSize(30, 30) );
        t = (double)cvGetTickCount() - t;
        fprintf(stderr, "detection time = %gms\n", t/((double)cvGetTickFrequency()*1000.0));

        for(int i = 0; i < (faces ? faces->total : 0); i++) {
            CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
            CvPoint center;
            int radius;
            center.x = cvRound((r->x + r->width*0.5)*scale);
            center.y = cvRound((r->y + r->height*0.5)*scale);
            radius = cvRound((r->width + r->height)*0.25*scale);
            cvCircle(img, center, radius, cvScalar(0.75, 0.75, 0.75, 1.0), 3, 8, 0);
        }
    }

    cvReleaseImage( &gray );
    cvReleaseImage( &small_img );
}


//
// STATIC FUNCTIONS
//

static Iop* build(Node *node) {
    return new BasicOpenCV(node);
}


const Iop::Description BasicOpenCV::desc(CLASS, CATEGORY, build);
