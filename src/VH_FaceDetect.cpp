static const char* const CLASS = "VH_FaceDetect";
static const char* const CATEGORY = "Image/VH_FaceDetect";
static const char* const DISPLAY_NAME = "VH_FaceDetect";
static const char* const HELP = "Detects faces using the OpenCV library.";

#include <stdarg.h>
#include <stdio.h>
#include <vector>

#include "VH_FaceDetect.h"

using namespace DD::Image;


//
// PUBLIC METHODS
//

VH_FaceDetect::VH_FaceDetect(Node *node) : Iop(node) {
    m_cascadeFile = NULL;
    m_circleColor[0] = m_circleColor[1] = m_circleColor[2] = 0.8;

    m_img = NULL;
    m_storage = cvCreateMemStorage(0);
    m_cascade = NULL;
    m_faces = NULL;
}


VH_FaceDetect::~VH_FaceDetect() {
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


const char* VH_FaceDetect::Class() const {
    return desc.name;
}


const char* VH_FaceDetect::displayName() const {
    return DISPLAY_NAME;
}


const char* VH_FaceDetect::node_help() const {
    return HELP;
}


void VH_FaceDetect::knobs(Knob_Callback f) {
    File_knob(f, &m_cascadeFile, "cascadefile", "cascadefile");
    Color_knob(f, m_circleColor, "circleColor", "circle color");
}


//
// PROTECTED METHODS
//

void VH_FaceDetect::_validate(bool for_real) {
    copy_info();
}


void VH_FaceDetect::_request(int x, int y, int r, int t,
                           ChannelMask channels, int count)
{
    // We need to request the whole input image here, unfortunately.
    input0().request(0, 0, info_.w(), info_.h(), channels, count);
}


void VH_FaceDetect::_open() {
    if (m_img != NULL) {
        cvReleaseImage(&m_img);
        m_img = NULL;
    }

    if (m_cascade != NULL) {
        cvRelease((void **)&m_cascade);
        m_cascade = NULL;
    }

    m_img = build_opencv_image();
    if (m_img == NULL) {
        error("Unable to load the source image.");
        return;
    }
    //print_image_info();

    if (m_cascadeFile != NULL && strlen(m_cascadeFile) > 0) {
        m_cascade = (CvHaarClassifierCascade *)cvLoad(m_cascadeFile, m_storage, 0, 0);
        if (m_cascade == NULL) {
            error("Unable to load the Haar classifier cascade file '%s'.",
                    m_cascadeFile);
            return;
        }
    }

    if (m_img != NULL && m_cascade != NULL) {
        detect_and_draw(m_img);
    }
}


struct span {
    int left;
    int right;

    span(int _left, int _right) : left(_left), right(_right) {}
};


void VH_FaceDetect::engine(int y, int x, int r, ChannelMask channels, Row& row) {
    Row in(x, r);
    in.get(input0(), y, x, r, channels);

    if (m_faces != NULL) {
        std::vector<span> spans;
        for (int f = 0; f < (m_faces ? m_faces->total : 0); ++f) {
            CvRect* rect = (CvRect*)cvGetSeqElem(m_faces, f);
            if (rect->y <= y && (rect->y + rect->height) > y)
                if (rect->x <= r && (rect->x + rect->width) > x)
                    spans.push_back(span(rect->x, rect->x + rect->width));
        }

        foreach(chan, channels) {
            float *out = row.writable(chan);
            for (int i = x; i < r; ++i) {
                std::vector<span>::const_iterator s;
                bool faceCount = 0;
                for (s = spans.begin(); s != spans.end(); s++) {
                    if (s->left <= i && i < s->right)
                        ++faceCount;
                }
                double faceScale = (faceCount + 1.0) / (m_faces->total + 1.0);
                out[i] = in[chan][i] * faceScale;
            }
        }
    } else {
        foreach(chan, channels) {
            float *out = row.writable(chan);
            for (int i = x; i < r; ++i)
                out[i] = in[chan][i];
        }
    }
}


void VH_FaceDetect::_close() {
    if (m_img != NULL) {
        cvReleaseImage(&m_img);
        m_img = NULL;
    }
}


//
// PRIVATE METHODS
//

IplImage *VH_FaceDetect::build_opencv_image() const {
    int w = info_.w();
    int h = info_.h();

    // Load the entire source image into the cache.
    Tile tile(input0(), 0, 0, w, h, Mask_RGB);
    if (Op::aborted())
        return NULL;

    IplImage *img = cvCreateImage(cvSize(w, h), 8, 3);
    char *pixel;
    char *min_pixel = img->imageData;
    char *max_pixel = img->imageData + img->imageSize;

    for (int y = 0; y < h; ++y) {
        // Get the incoming pixel data for this row.
        Row in(0, w);
        in.get(input0(), y, 0, w, Mask_RGB);

        // Write the pixels to the OpenCV image.
        int imgY = h - y - 1;
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


void VH_FaceDetect::print_image_info() const {
    if (m_img != NULL) {
        fprintf(stderr, "Loaded image:\n");
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


void VH_FaceDetect::detect_and_draw(IplImage *img) {
    if (m_cascade != NULL) {
        double scale = 1.3;
        IplImage* gray = cvCreateImage(cvSize(img->width,img->height), 8, 1);
        IplImage* small_img = cvCreateImage(
                cvSize(cvRound (img->width/scale), cvRound (img->height/scale)),
                8, 1);

        cvCvtColor(img, gray, CV_BGR2GRAY);
        cvResize(gray, small_img, CV_INTER_LINEAR);
        cvEqualizeHist(small_img, small_img);
        cvClearMemStorage(m_storage);
        m_faces = NULL;

        m_faces = cvHaarDetectObjects( small_img, m_cascade, m_storage,
                                       1.1, 2, 0/*CV_HAAR_DO_CANNY_PRUNING*/,
                                       cvSize(30, 30) );
        for(int i = 0; i < (m_faces ? m_faces->total : 0); i++) {
            CvRect* r = (CvRect*)cvGetSeqElem(m_faces, i );
            r->x *= scale;
            r->y *= scale;
            r->width *= scale;
            r->height *= scale;
            r->y = img->height - r->y - r->height;
        }

        cvReleaseImage( &gray );
        cvReleaseImage( &small_img );
    }
}


//
// STATIC FUNCTIONS
//

static Iop* build(Node *node) {
    return new VH_FaceDetect(node);
}


const Iop::Description VH_FaceDetect::desc(CLASS, CATEGORY, build);
