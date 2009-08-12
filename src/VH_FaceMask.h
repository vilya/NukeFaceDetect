/*
 * VH_FaceDetect.h
 *
 *  Created on: 10-Aug-2009
 *      Author: vharvey
 */

#ifndef VH_FACEMASK_H_
#define VH_FACEMASK_H_


#include <DDImage/DrawIop.h>
#include <DDImage/Knobs.h>
#include <DDImage/Row.h>
#include <DDImage/Tile.h>
#include <DDImage/Channel.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>


//
// DECLARATIONS
//

class VH_FaceMask : public DD::Image::DrawIop {
public:
    static const DD::Image::Iop::Description desc;

    VH_FaceMask(Node* node);
    virtual ~VH_FaceMask();

    virtual const char* Class() const;
    virtual const char* node_help() const;
    virtual void knobs(DD::Image::Knob_Callback f);

protected:
    virtual void _validate(bool for_real);
    virtual void _request(int x, int y, int r, int t, DD::Image::ChannelMask channels, int count);
    virtual void _open();
    virtual bool draw_engine(int y, int left, int right, float *buffer);

private:
    IplImage *build_opencv_image() const;
    void detect_faces(IplImage *img);

    const char *m_cascadeFile;

    CvMemStorage *m_storage;
    CvHaarClassifierCascade *m_cascade;
    CvSeq *m_faces;
};


#endif /* VH_FACEDETECT_H_ */
