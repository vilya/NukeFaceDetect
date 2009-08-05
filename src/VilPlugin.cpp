static const char* const HELP = "Vil's first attempt at a Nuke pugin.";

#include <stdarg.h>

#include <DDImage/Iop.h>
#include <DDImage/Row.h>
#include <DDImage/Knobs.h>
#include <DDImage/NukeWrapper.h>

#include <opencv/cv.h>

using namespace DD::Image;

//
// DECLARATIONS
//

class VilPlugin: public Iop {
public:
    static const Description d;

    VilPlugin(Node *node);
    virtual ~VilPlugin();

    virtual void in_channels(int input, ChannelSet& mask) const;
    virtual void knobs(Knob_Callback f);

    bool pass_transform() const;
    const char* Class() const;
    const char* node_help() const;

protected:
    virtual void _validate(bool for_real);
    virtual void _request(int x, int y, int r, int t, ChannelMask channels, int count);
    virtual void engine(int y, int x, int r, ChannelMask channels, Row& out);

    virtual IplImage *buildOpenCVImage();

private:
    const char *m_haarCascadeFile;
    CvMemStorage *m_storage;
    CvHaarClassifierCascade *m_cascade;
};


//
// IMPLEMENTATIONS
//

VilPlugin::VilPlugin(Node *node) : Iop(node) {
    m_haarCascadeFile = NULL;
    m_storage = cvCreateMemStorage(0);
    m_cascade = NULL;
}


VilPlugin::~VilPlugin() {
    cvReleaseMemStorage(&m_storage);
    if (m_cascade != NULL) {
        cvRelease((void **)&m_cascade);
    }
}


void VilPlugin::in_channels(int input, ChannelSet& mask) const {
    // We use RGB input.
    mask = Mask_RGB;
}


void VilPlugin::engine(int y, int x, int r, ChannelMask channels, Row& out) {
    // TODO: conversion between Nuke and OpenCV image data formats.
//    static CvScalar colors[] = {
//        {{0,0,255}},
//        {{0,128,255}},
//        {{0,255,255}},
//        {{0,255,0}},
//        {{255,128,0}},
//        {{255,255,0}},
//        {{255,0,0}},
//        {{255,0,255}}
//    };
//
//    double scale = 1.3;
//
//    Tile tile(input0(), Mask_RGB);
//    if (Op::aborted())
//        return;
//
//    int w = info().w();
//    int h = info().h();
//
//    IplImage *img = cvCreateImage(cvSize(w, h), 8, 3);
//    for (int y = 0; y < h; ++y) {
//        Row in(0, w);
//        in.get(input0(), y, 0, w, channels);
//        for (int x = 0; x < w; ++x) {
//            img->imageData
//        }
//    }
//
//    IplImage *gray = cvCreateImage(cvSize(w, h), 8, 1);
//    IplImage* small_img = cvCreateImage(
//            cvSize(cvRound(img->width / scale), cvRound(img->height / scale)),
//            8, 1);
//
//    cvResize( gray, small_img, CV_INTER_LINEAR );
//    cvEqualizeHist( small_img, small_img );
//    cvClearMemStorage( m_storage );
//
//    if (m_cascade) {
//        CvSeq* faces = cvHaarDetectObjects( small_img, m_cascade, m_storage,
//                1.1, 2, 0/*CV_HAAR_DO_CANNY_PRUNING*/, cvSize(30, 30) );
//        for (int i = 0; i < (faces ? faces->total : 0); i++) {
//            CvRect* r = (CvRect*)cvGetSeqElem(faces, i);
//
//            CvPoint center;
//            center.x = cvRound((r->x + r->width*0.5)*scale);
//            center.y = cvRound((r->y + r->height*0.5)*scale);
//
//            int radius = cvRound((r->width + r->height)*0.25*scale);
//
//            // TODO: replace this with code to draw a circle on the output
//            // channels. Do each in a different colour.
//
//            cvCircle(img, center, radius, colors[i%8], 3, 8, 0);
//        }
//    }
//
//    // At this point, we can write the output
//    //cvShowImage( "result", img );
//    cvReleaseImage( &gray );
//    cvReleaseImage( &small_img );


}


void VilPlugin::_validate(bool for_real) {
    input0().validate(for_real);
    copy_info();

    if (for_real) {
        m_cascade = (CvHaarClassifierCascade*)cvLoad(m_haarCascadeFile, NULL, NULL, NULL);
        if (m_cascade != NULL) {
            set_out_channels(Mask_RGB);
        } else {
            error("Unable to load Haar classifier cascade from %s.", m_haarCascadeFile);
        }
    }

    if (m_cascade == NULL) {
        set_out_channels(Mask_None);
    }
}


void VilPlugin::_request(int x, int y, int r, int t,
                         ChannelMask channels, int count)
{
    input0().request(Mask_RGB, count);
}


void VilPlugin::knobs(Knob_Callback f) {
    File_knob(f, &m_haarCascadeFile, "Haar cascade file");
}


inline const char* VilPlugin::Class() const {
    return d.name;
}


inline const char* VilPlugin::node_help() const {
    return HELP;
}


static Iop* build(Node *node) {
    return new NukeWrapper(new VilPlugin(node));
}


const Iop::Description VilPlugin::d("VilPlugin", "Filter/VilPlugin", build);
