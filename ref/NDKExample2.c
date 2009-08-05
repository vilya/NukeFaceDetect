/* Acknowledgments
 * Matt Plec of The Foundry and his post on the Nuke Developer list dated 04/27/07.
 * 
 * This example source code demostrates a node with 4 XY_knobs, the viewer then connects
 * the four points each knob already draws in the ui with a line and then fills in the area
 * contained inbetween the four points.  This code also demonstrates the NoIop node.
 */

static const char* const HELP =
"NDKExampleTwo";

#include <DDImage/NoIop.h>
#include <DDImage/Knobs.h>
#include <DDImage/Knob.h>
#include <DDImage/ViewerContext.h>
#include <DDImage/gl.h>

using namespace DD::Image;

class NDKExampleTwo : public NoIop {

	public:
	double point0[2];
	double point1[2];
	double point2[2];
	double point3[2];

	NDKExampleTwo() : NoIop() {
		point0[0] = info_.format().w()/4;
		point0[1] = info_.format().h()/4;
		point1[0] = info_.format().w()/4;
		point1[1] = info_.format().h()-(info_.format().h()/4);
		point2[0] = info_.format().w()-(info_.format().w()/4);
		point2[1] = info_.format().h()-(info_.format().h()/4);
		point3[0] = info_.format().w()-(info_.format().w()/4);
		point3[1] = info_.format().h()/4;
	}

	void knobs(Knob_Callback f);

	void build_handles(ViewerContext* ctx);

	void draw_handle(ViewerContext* ctx);

	static const Iop::Description d;

	const char* Class() const {return d.name;}

	const char* node_help() const {return HELP;}
};

void NDKExampleTwo::knobs(Knob_Callback f) {
	XY_knob(f, point0, "point0", "point0");
	XY_knob(f, point1, "point1", "point1");
	XY_knob(f, point2, "point2", "point2");
	XY_knob(f, point3, "point3", "point3");
}

void NDKExampleTwo::build_handles(ViewerContext* ctx) {

 build_input_handles(ctx);
 build_knob_handles(ctx);

 if (panel_visible() && ctx->transform_mode()==VIEWER_2D)
   add_draw_handle(ctx);
}

void NDKExampleTwo::draw_handle(ViewerContext* ctx) {

	if (!ctx->draw_unpickable_lines()) return;

	glColor4f(0, 0, 1,0.25);
	
	glBegin(GL_POLYGON);
		glVertex2d(point0[0],point0[1]);
		glVertex2d(point1[0],point1[1]);
		glVertex2d(point2[0],point2[1]);
		glVertex2d(point3[0],point3[1]);
	glEnd();

	glColor3f(1, 0, 0);

	glBegin(GL_LINE_STRIP);
		glPointSize(4);
		glVertex2d(point0[0],point0[1]);
		glVertex2d(point1[0],point1[1]);
		glVertex2d(point2[0],point2[1]);
		glVertex2d(point3[0],point3[1]);
		glVertex2d(point0[0],point0[1]);
	glEnd();

	glColor(ctx->node_color());
}

static Iop* build() {return new NDKExampleTwo();}

const Iop::Description NDKExampleTwo::d("NDKExampleTwo", "Color/NDKExampleTwo", build);
