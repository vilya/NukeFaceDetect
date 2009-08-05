/* Acknowledgments
 * John Haddon of Image Engine and his post on the Nuke Developer list dated 05/22/07.
 * 
 * This example source code demostrates a node with a Animation_knob, the curves
 * of the Animation_knob are used to create an example lookup curve node for modifying
 * the incoming rgb values.
 */

static const char* const HELP =
"NDKExampleOne";

#include <DDImage/Iop.h>
#include <DDImage/Row.h>
#include <DDImage/Animation.h>
#include <DDImage/Knobs.h>
#include <DDImage/Knob.h>

using namespace DD::Image;

class NDKExampleOne : public Iop {

	public:
	const DD::Image::Animation *lookuptest[3];

	void knobs(Knob_Callback f);

	void _validate(bool for_real);

	void engine(int y, int x, int r, ChannelMask channels, Row& out);

	static const Iop::Description d;

	const char* Class() const {return d.name;}

	const char* node_help() const {return HELP;}
};

void NDKExampleOne::knobs(Knob_Callback f) {
	static const char *anim1[] = { "red", "y L 0 s0 1 s0", "green", "y L 0 s0 1 s0", "blue", "y L 0 s0 1 s0", 0, 0 };
	Animation_knob( f, lookuptest, anim1, "lookup", "lookup" );
}

void NDKExampleOne::_validate(bool for_real) {
	input(0)->validate(for_real);
	copy_info();
	set_out_channels(Mask_RGB);
}

void NDKExampleOne::engine(int y, int x, int r, ChannelMask channels, Row& out) {
	Row in(x, r);
	in.get(*input(0), y, x, r, channels);

	const float* rIn = in[Chan_Red]+x;
	const float* gIn = in[Chan_Green]+x;
	const float* bIn = in[Chan_Blue]+x;

	float *rOut = out.writable(Chan_Red)+x;
	float *gOut = out.writable(Chan_Green)+x;
	float *bOut = out.writable(Chan_Blue)+x;

	for (int n = x; n < r; n++) {
		*rOut++ = lookuptest[0]->evaluate(*rIn++);
		*gOut++ = lookuptest[1]->evaluate(*gIn++);
		*bOut++ = lookuptest[2]->evaluate(*bIn++);
	}
}

static Iop* build() {return new NDKExampleOne();}

const Iop::Description NDKExampleOne::d("NDKExampleOne", "Color/NDKExampleOne", build);
