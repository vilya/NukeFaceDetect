static const char* const CLASS = "VH_FaceMask";
static const char* const CATEGORY = "Image/VH_FaceMask";
static const char* const DISPLAY_NAME = "VH_FaceMask";
static const char* const HELP = "Detects faces using the OpenCV library and generates a mask for them.";

#include <stdarg.h>
#include <stdio.h>
#include <vector>

#include "VH_FaceMask.h"

using namespace DD::Image;

//
// DECLARATIONS
//

// A helper structure for rendering.
struct span {
    int left;
    int right;

    span(int _left, int _right) : left(_left), right(_right) {}
};


//
// PUBLIC METHODS
//

VH_FaceMask::VH_FaceMask(Node *node) : DrawIop(node) {
    m_cascadeFile = NULL;

    m_storage = cvCreateMemStorage(0);
    m_cascade = NULL;
    m_faces = NULL;
}


VH_FaceMask::~VH_FaceMask() {
    if (m_cascade != NULL) {
        cvRelease((void **)&m_cascade);
        m_cascade = NULL;
    }
    cvClearMemStorage(m_storage);
    cvReleaseMemStorage(&m_storage);
}


const char* VH_FaceMask::Class() const {
    return desc.name;
}


const char* VH_FaceMask::node_help() const {
    return HELP;
}


void VH_FaceMask::knobs(Knob_Callback f) {
    input_knobs(f);
    File_knob(f, &m_cascadeFile, "cascadefile", "cascadefile");
    output_knobs(f);
}


//
// PROTECTED METHODS
//

void VH_FaceMask::_validate(bool for_real) {
    set_out_channels(Mask_All);
    DrawIop::_validate(for_real);
}


void VH_FaceMask::_request(int x, int y, int r, int t,
                           ChannelMask channels, int count)
{
    // We need to request the whole input image here, unfortunately.
    input0().request(0, 0, info_.w(), info_.h(), channels, count);
}


void VH_FaceMask::_open() {
    if (m_cascade != NULL) {
        cvRelease((void **)&m_cascade);
        m_cascade = NULL;
    }

    IplImage *img = build_opencv_image();
    if (img == NULL) {
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

    if (m_cascade != NULL)
        detect_faces(img);
    cvReleaseImage(&img);
}


bool VH_FaceMask::draw_engine(int y, int left, int right, float *buffer) {
    if (m_faces == NULL)
        return false;

    std::vector<span> spans;
    for (int face = 0; face < (m_faces ? m_faces->total : 0); ++face) {
        CvRect* rect = (CvRect*)cvGetSeqElem(m_faces, face);
        if (rect->y <= y && (rect->y + rect->height) > y) {
            int face_left = rect->x;
            int face_right = rect->y + rect->width;
            if (face_left <= right && face_right > left) {
                if (left > face_left)
                    face_left = left;
                if (right < face_right)
                    face_right = right;
                spans.push_back(span(face_left, face_right));
            }
        }
    }
    if (spans.size() == 0)
        return false;

    for (int x = left; x < right; ++x)
        buffer[x] = 0.0f;

    std::vector<span>::const_iterator s;
    for (s = spans.begin(); s != spans.end(); ++s) {
        for (int x = s->left; x < s->right; ++x)
            buffer[x] = 1.0f;
    }
    return true;
}


//
// PRIVATE METHODS
//

IplImage *VH_FaceMask::build_opencv_image() const {
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


void VH_FaceMask::detect_faces(IplImage *img) {
    if (m_cascade != NULL) {
        IplImage* gray = cvCreateImage(cvSize(img->width,img->height), 8, 1);
        cvCvtColor(img, gray, CV_BGR2GRAY);
        cvEqualizeHist(gray, gray);
        cvClearMemStorage(m_storage);
        m_faces = cvHaarDetectObjects(gray, m_cascade, m_storage,
                                      1.1, 2, 0/*CV_HAAR_DO_CANNY_PRUNING*/,
                                      cvSize(30, 30));
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
    return new VH_FaceMask(node);
}


const Iop::Description VH_FaceMask::desc(CLASS, CATEGORY, build);
