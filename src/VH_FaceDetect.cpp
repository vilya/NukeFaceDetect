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
// DECLARATIONS
//

struct span {
    int left;
    int right;

    span(int _left, int _right) : left(_left), right(_right) {}
};


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

    if (m_cascadeFile != NULL && strlen(m_cascadeFile) > 0) {
        m_cascade = (CvHaarClassifierCascade *)cvLoad(m_cascadeFile, m_storage, 0, 0);
        if (m_cascade == NULL) {
            error("Unable to load the Haar classifier cascade file '%s'.",
                    m_cascadeFile);
            return;
        }
    }

    if (m_img != NULL && m_cascade != NULL)
        detect_and_draw(m_img);
}


void VH_FaceDetect::engine(int y, int left, int right, ChannelMask channels, Row& row) {
    Row in(left, right);
    in.get(input0(), y, left, right, channels);

    if (m_faces != NULL) {
        std::vector<span> spans;
        for (int face = 0; face < (m_faces ? m_faces->total : 0); ++face) {
            CvRect* rect = (CvRect*)cvGetSeqElem(m_faces, face);
            if (rect->y <= y && (rect->y + rect->height) > y)
                if (rect->x <= right && (rect->x + rect->width) > left)
                    spans.push_back(span(rect->x, rect->x + rect->width));
        }

        foreach(chan, channels) {
            float *out = row.writable(chan);
            for (int x = left; x < right; ++x) {
                std::vector<span>::const_iterator s;
                bool faceCount = 0;
                for (s = spans.begin(); s != spans.end(); s++) {
                    if (s->left <= x && x < s->right)
                        ++faceCount;
                }
                double faceScale = (faceCount + 1.0) / (m_faces->total + 1.0);
                out[x] = in[chan][x] * faceScale;
            }
        }
    } else {
        foreach(chan, channels) {
            float *out = row.writable(chan);
            for (int x = left; x < right; ++x)
                out[x] = in[chan][x];
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


void VH_FaceDetect::detect_and_draw(IplImage *img) {
    if (m_cascade != NULL) {
        IplImage* gray = cvCreateImage(cvSize(img->width,img->height), 8, 1);

        cvCvtColor(img, gray, CV_BGR2GRAY);
        cvEqualizeHist(gray, gray);
        cvEqualizeHist(gray, gray);
        cvClearMemStorage(m_storage);
        m_faces = NULL;

        m_faces = cvHaarDetectObjects( gray, m_cascade, m_storage,
                                       1.1, 2, 0/*CV_HAAR_DO_CANNY_PRUNING*/,
                                       cvSize(30, 30) );
        for(int i = 0; i < (m_faces ? m_faces->total : 0); i++) {
            CvRect* r = (CvRect*)cvGetSeqElem(m_faces, i);
            r->y = img->height - r->y - r->height;
        }

        cvReleaseImage( &gray );
    }
}


//
// STATIC FUNCTIONS
//

static Iop* build(Node *node) {
    return new VH_FaceDetect(node);
}


const Iop::Description VH_FaceDetect::desc(CLASS, CATEGORY, build);
