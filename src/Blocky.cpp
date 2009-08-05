// Blocky.C

// Copyright (c) 2007 The Foundry Visionmongers Ltd.
// Permission is granted to reuse portions or all of this code for the
// purpose of implementing Nuke plugins, or to demonstrate or document
// the methods needed to implemente Nuke plugins.

// This is the name that Nuke will use to store this operator in the
// scripts. So that nuke can locate the plugin, this must also be the
// name of the compiled plugin (with .machine added to the end):
static const char* const CLASS = "Blocky";

// This is the text displayed by the [?] button in the control panel:
static const char* const HELP =
"This is a demonstration of a Nuke plugin that moves pixels by "
"use of Tile. In this case blocks of pixels are averaged together "
"to produce the result. Notice that this implementation is much "
"slower than necessary as it does not reuse calculations done for "
"adjacent lines."
"\n\n"
"The source code has a lot of comments "
"inserted into it to demonstrate how to write a plugin.";

////////////////////////////////////////////////////////////////

// Most plugins will need these include files. The include files are in
// /usr/dd/include/DDImage:

#include <DDImage/Iop.h>
#include <DDImage/Row.h>
#include <DDImage/Tile.h>
#include <DDImage/Knobs.h>

using namespace DD::Image;

// Define the C++ class that is the new operator. This may be a subclass
// of Iop, or of some subclass of Iop such as Blur:

class Blocky : public Iop {
  // These are the locations the user interface will store into:
  double width, height;
public:
  // You must implement these functions:
  Blocky(Node *);
  void _validate(bool);
  void _request(int,int,int,int,ChannelMask,int);
  void engine(int y,int x,int r, ChannelMask, Row&);
  virtual void knobs(Knob_Callback);
  const char* Class() const {return CLASS;}
  const char* node_help() const {return HELP;}
  static const Description d;
  // You may need to implement the destructor if you allocate memory:
  //~Blocky();
};

// The constructor must initialize the user controls to their default
// values:
Blocky::Blocky(Node *node) : Iop(node) {
  width = 10;
  height = 10;
}

// The knobs function describes each user control so the user interface
// can be built. The possible controls are listed in DDImage/Knobs.h:
void Blocky::knobs(Knob_Callback f) {
  WH_knob(f, &width, "size");
}

// This is a function that creates an instance of the operator, and is
// needed for the Iop::Description to work:
static Iop* Blocky_c(Node *node) {return new Blocky(node);}

// The Iop::Description is how Nuke knows what the name of the operator is,
// how to create one, and the menu item to show the user. The menu item
// is ignored in recent versions of Nuke, but you might want to set it anyway.
const Iop::Description Blocky::d(CLASS, "Filter/Blocky", Blocky_c);

// When the operator runs, "open" is called first. This function
// copies the "info" data from the input operator(s), modifies it
// according to what this operator does, and saves this information so
// that operators connected to this one can see it. The boolean flag
// is so that Nuke can call a "fast" and "slow" version. The fast
// version should avoid opening any files and just make the best
// guess. The slow (argument==true) version will always be called
// before request() or engine() are called:
void Blocky::_validate(bool for_real)
{
  // Get the size and other information from the input image:
  copy_info();

  int w = int(width);
  int h = int(height);

  // If operators do nothing it is much faster to indicate that no
  // channels are changed. If you do this, open(true), request(),
  // and engine() will not be called, so the code in those does not have
  // to worry about these cases:
  if (w*h <= 1 || w < 1 || h < 1 || info_.is_constant()) {
    set_out_channels(Mask_None);
    return;
  }
  set_out_channels(Mask_All);

  // The bounding box is going to expand to the block size:
  info_.set(info_.x()/w * w,
            info_.y()/h * h,
            (info_.r()+w-1)/w * w,
            (info_.t()+h-1)/h * h);
}

// After open is done, "request" is called. This is passed a "viewport"
// which describes which channels and a rectangular "bounding box" that will
// be requested. The operator must translate this to the area that will
// be requested from it's inputs and call request() on them. If you don't
// implement this then a default version requests the entire area of
// pixels and channels available from the input. If possible you should
// override this so that the resulting caches are smaller, this also helps
// Nuke display what portions of the tree are being used.
void Blocky::_request(int x,int y,int r,int t, ChannelMask channels, int count)
{
  // For this we need to go to the edges of any blocks that the output
  // rectangle touches:
  int w = int(width);
  int h = int(height);
  x = x/w * w;
  r = (r+w-1)/w * w;
  y = y/h * h;
  t = (t+h-1)/h * h;
  input0().request(x,y,r,t, channels, count);
}

// This is the operator that does all the work. For each line in the area
// passed to request(), this will be called. It must calculate the image data
// for a region at vertical position y, and between horizontal positions
// x and r, and write it to the passed row structure. Usually this works
// by asking the input for data, and modifying it:
void Blocky::engine(int y, int x, int r, ChannelMask channels, Row& out)
{
  // Figure out a rectangle we want from the input:
  int w = int(width);
  int h = int(height);
  int tx = x/w * w;
  int tr = (r+w-1)/w * w;
  int ty = y/h * h;
  int tt = ty+h;

  // Lock an area into the cache:
  // This should be an interest!!
  Tile tile(input0(), tx, ty, tr, tt, channels);
  // You must always check for aborted after creating a tile. If the
  // operation was aborted, the tile contains bad data:
  if (Op::aborted()) return;

  for (int Y = ty; Y < tt; Y++) {
    // Retrieve a row from the tile. This is much faster than random
    // access of the tile and should be used if at all possible. See
    // the documentation for Interest, Tile, at at() for other ways of
    // getting the data.
    Row in(tx, tr);
    in.get(input0(), Y, tx, tr, channels);
    // For each channel that is needed:
    foreach (z, channels) {
      // For each horizontal block:
      for (int X = tx; X < tr; X += w) {
        // add up all the incoming pixels:
        float sum = 0;
        for (int X1 = 0; X1 < w; X1++) sum += in[z][X+X1];
        // divide by the total size of a block:
        sum /= (w*h);
        // add it to the output pixels (for first row replace the output):
        int outX = X; int E = X+w;
        if (outX < x) outX = x;
        if (E > r) E = r;
        float* TO = out.writable(z)+outX;
        float* END = TO+(E-outX);
        if (Y == ty) {
          while (TO < END) *TO++ = sum;
        } else {
          while (TO < END) *TO++ += sum;
        }
      }
    }
  }
}

// End of Blocky.C
// Copyright (c) 2007 The Foundry Visionmongers Ltd..
