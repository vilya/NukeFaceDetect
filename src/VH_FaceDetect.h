/*
 * VH_FaceDetect.h
 *
 *  Created on: 10-Aug-2009
 *      Author: vharvey
 */

#ifndef VH_FACEDETECT_H_
#define VH_FACEDETECT_H_


#include <DDImage/Iop.h>
#include <DDImage/Knobs.h>
#include <DDImage/Row.h>
#include <DDImage/Tile.h>
#include <DDImage/Channel.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>


//
// DECLARATIONS
//

class VH_FaceDetect: public DD::Image::Iop {
public:
    static const DD::Image::Iop::Description desc;

    VH_FaceDetect(Node* node);
    virtual ~VH_FaceDetect();

    virtual const char* Class() const;
    virtual const char* displayName() const;
    virtual const char* node_help() const;
    virtual void knobs(DD::Image::Knob_Callback f);

protected:
    virtual void _validate(bool for_real);
    virtual void _request(int x, int y, int r, int t, DD::Image::ChannelMask channels, int count);
    virtual void _open();
    virtual void engine(int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& row);
    virtual void _close();

private:
    IplImage *build_opencv_image() const;
    void detect_faces(IplImage *img);
    float border_color(DD::Image::Channel chan) const;

    const char *m_cascadeFile;
    float m_borderColor[3];

    CvMemStorage *m_storage;
    CvHaarClassifierCascade *m_cascade;
    CvSeq *m_faces;
};


#endif /* VH_FACEDETECT_H_ */
