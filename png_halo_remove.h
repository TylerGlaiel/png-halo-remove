/*
Version 0.5

Copyright (c) 2011 Tyler Glaiel

Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
  copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

stb_image and stb_image write taken from Sean Barret, http://nothings.org
*/


#pragma once


////////////////////
namespace glaiel {
  


  //enums for pixel metadata
  namespace px {
    struct neighbor {
      enum {
        TOP = 1,
        RIGHT = 2,
        BOTTOM = 4,
        LEFT = 8,
        TOPRIGHT = 16,
        TOPLEFT = 32,
        BOTTOMRIGHT = 64,
        BOTTOMLEFT = 128
      };
    };
    struct meta {
      enum {
        SOLID = 256,     //original pixel was solid or partially transparent
        INVISIBLE = 512, //original pixel was invisible
        UPDATED = 1024    //pixel has been processed
      };
    };

    struct rgba {
      unsigned char r,g,b,a;
    };
    union color {
      rgba components;
      unsigned int hex;
    };
    typedef unsigned short prop;
  };

  struct image {
      px::color *data;
      px::prop  *meta;
      int width;
      int height;
  };

  struct replace_mode;
  typedef px::color (*pxProcessFunc)(unsigned int x, unsigned int y, image* img, replace_mode* param);


  struct replace_mode {
    enum {
      SOLID_COLOR,
      BORDER_ONLY_SOLID, //not implemented
      SMEAR,
      BORDER_ONLY_SMEAR  //not implemented yet
    };

    enum {
      FLAT_ITERATION, //hits each pixel once left to right top to bottom, used for solid and border modes
      SMEAR_ITEATION, //flood fills outward from solid pixels, placing neighbor search information into image::meta, used for smear mode
    } iteration_type;

    px::color replace_color;
    void* usrdata;

    pxProcessFunc func;
  };

  image load_png (const char* filename);
  void  save_png (const char* filename, image& img);
  void destroy_image(image& img);

  void remove_halo (image& img, replace_mode& method);

  namespace mode {
    extern replace_mode solid;
    extern replace_mode smear;
  }

};
