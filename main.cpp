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


#include "png_halo_remove.h"
#include <iostream>

//params:
//pngRemoveHalo.exe inputfilename outputfilename


int main(int argc, char* argv[]){

  if(argc < 3){
    std::cout << "error, not enough parameters" << std::endl;
    return 0;
  }

  const char * infile = argv[1];
  const char * outfile = argv[2];

  glaiel::image image = glaiel::load_png(infile);
  if(image.data == NULL) {
    std::cout << "ERROR READING PNG" << std::endl;
    return 0;
  }

  glaiel::replace_mode mode = glaiel::mode::smear;
  //mode.replace_color.hex = 0x00000000;

  glaiel::remove_halo(image, mode);

  glaiel::save_png(outfile, image);

  glaiel::destroy_image(image);

  return 0;
}
