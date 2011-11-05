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

#define STBI_HEADER_FILE_ONLY
#include "stb_image.c"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <math.h>
#include <queue>

namespace glaiel {
  //pxProcessFunc functions
  int round(double a){
    return floor(a+.5);
  }

  

  int flagcount(int a){

    int ct = 0;
    if (a & px::neighbor::TOP) ct++;
    if (a & px::neighbor::RIGHT) ct++;
    if (a & px::neighbor::BOTTOM) ct++;
    if (a & px::neighbor::LEFT) ct++;
    if (a & px::neighbor::TOPRIGHT) ct++;
    if (a & px::neighbor::TOPLEFT) ct++;
    if (a & px::neighbor::BOTTOMRIGHT) ct++;
    if (a & px::neighbor::BOTTOMLEFT) ct++;

    return ct;
  }

  bool flagmin (int a){
    return flagcount(a)>=3;
  }


  int clamp(int a){
    if(a < 0) return 0;
    if(a > 255) return 255;
    return a;
  }

  px::color solid_color (unsigned int x, unsigned int y, image* img, replace_mode* param){
    return param->replace_color;
  }

  px::color smear_color (unsigned int x, unsigned int y, image* img, replace_mode* param){
    px::color res = {0};
    double r=0;
    double g=0;
    double b=0;

    px::prop meta = img->meta[x+y*img->width];
    double ct = 0;

    const double gaussian_weight = 2.0;

    if(meta&px::neighbor::TOP){
      px::rgba pix = img->data[x+(y-1)*img->width].components;
      r += pix.r*gaussian_weight;
      g += pix.g*gaussian_weight;
      b += pix.b*gaussian_weight;
      ct+=gaussian_weight;
    }
    if(meta&px::neighbor::BOTTOM){
      px::rgba pix = img->data[x+(y+1)*img->width].components;
      r += pix.r*gaussian_weight;
      g += pix.g*gaussian_weight;
      b += pix.b*gaussian_weight;
      ct+=gaussian_weight;
    }
    if(meta&px::neighbor::LEFT){
      px::rgba pix = img->data[x-1+(y)*img->width].components;
      r += pix.r*gaussian_weight;
      g += pix.g*gaussian_weight;
      b += pix.b*gaussian_weight;
      ct+=gaussian_weight;
    }
    if(meta&px::neighbor::RIGHT){
      px::rgba pix = img->data[x+1+(y)*img->width].components;
      r += pix.r*gaussian_weight;
      g += pix.g*gaussian_weight;
      b += pix.b*gaussian_weight;
      ct+=gaussian_weight;
    }
    if(meta&px::neighbor::TOPLEFT){
      px::rgba pix = img->data[(x-1)+(y-1)*img->width].components;
      r += pix.r;
      g += pix.g;
      b += pix.b;
      ct+=1.0;
    }
    if(meta&px::neighbor::TOPRIGHT){
      px::rgba pix = img->data[(x+1)+(y-1)*img->width].components;
      r += pix.r;
      g += pix.g;
      b += pix.b;
      ct+=1.0;
    }
    if(meta&px::neighbor::BOTTOMLEFT){
      px::rgba pix = img->data[(x-1)+(y+1)*img->width].components;
      r += pix.r;
      g += pix.g;
      b += pix.b;
      ct+=1.0;
    }
    if(meta&px::neighbor::BOTTOMRIGHT){
      px::rgba pix = img->data[(x+1)+(y+1)*img->width].components;
      r += pix.r;
      g += pix.g;
      b += pix.b;
      ct+=1.0;
    }


    res.components.r = clamp(round(double(r)/ct));
    res.components.g = clamp(round(double(g)/ct));
    res.components.b = clamp(round(double(b)/ct));
    res.components.a = 0;

    return res;
  }

  image load_png (const char* filename){
    image temp;
    int component_count;

    temp.data = (px::color*)stbi_load(filename, &temp.width, &temp.height, &component_count, 4);


    if(temp.data == NULL) return temp;
    //preprocess pixel metadata
    temp.meta = new px::prop[temp.width*temp.height];

    for(int i = 0; i<temp.width*temp.height; i++){
      temp.meta[i] = 0;

      if(temp.data[i].components.a == 0){
        temp.meta[i] |= px::meta::INVISIBLE;
      } else {
        temp.meta[i] |= px::meta::SOLID;
      }
    }

    return temp;

  }
  void save_png (const char* filename, image& img){
    stbi_write_png(filename, img.width, img.height, 4, img.data, 4*img.width);
  }
  void destroy_image(image& img){
    delete[] img.data;
    delete[] img.meta;
    img.data = NULL;
  }


  void remove_halo_flat_iteration(image& img, replace_mode& method) {
    for(int i = 0; i<img.width; i++){
      for(int j = 0; j<img.height; j++){
        //img.data[i+j*img.width].components.a = 255;
        if(img.meta[i+j*img.width] & px::meta::INVISIBLE){
          img.data[i+j*img.width] = method.func(i, j, &img, &method);
          img.meta[i+j*img.width] |= px::meta::UPDATED;
        }
      }
    }
  }

  struct point {
    int x;
    int y;

    point(int x_=0, int y_=0):x(x_),y(y_){}
  };

  struct point_sorted {
    static int sindex;
    double distance;
    point pt;
    int created;

    bool operator<(const point_sorted& rhs) const{

      if(distance>rhs.distance) return true; //recently added points get higher priority
      if(distance<rhs.distance) return false;

      if(created > rhs.created) return true;
      if(created < rhs.created) return false;

      return false;
    };

    point_sorted(point p, double d):pt(p), distance(d){
      created = ++sindex;
    }
  };
  int point_sorted::sindex = 0;

  void remove_halo_smear_iteration(image& img, replace_mode& method) {
    point_sorted::sindex = 0;
    //scan for solids
    //max iter is 8*pixelcount (each pixel can be added by a maximum of 4 others before it is never added again)
    //rather than deal with flexible containers i'll just allocate a buffer large enough to store the max amount
    //and shift a pointer

    std::priority_queue<point_sorted> pqueue;

    for(int i = 0; i<img.width; i++){
      for(int j = 0; j<img.height; j++){
        //img.data[i+j*img.width].components.a = 255;
        if(img.meta[i+j*img.width] & px::meta::SOLID){
          pqueue.push(point_sorted(point(i,j),0));
        }
      }
    }

    while(!pqueue.empty()){
      //std::cout << pqueue.top().priority << " " << pqueue.size() << std::endl;
     // if(pqueue.size()>50000) break;

      point p = pqueue.top().pt;
      double dex_adj = pqueue.top().distance+1.0;
      double dex_diag = pqueue.top().distance+1.41421356;
      pqueue.pop();

      if(!(img.meta[p.x+p.y*img.width] & px::meta::UPDATED)){
        img.meta[p.x+p.y*img.width] |= px::meta::UPDATED;

        if(!(img.meta[p.x+p.y*img.width] & px::meta::SOLID)){
          img.data[p.x+p.y*img.width] = method.func(p.x, p.y, &img, &method);
        }

        //test to the left
        if(p.x>0){
          point pt = point(p.x-1, p.y);
          if(!(img.meta[pt.x+pt.y*img.width] & (px::meta::SOLID|px::meta::UPDATED))){
            img.meta[pt.x+pt.y*img.width] |= px::neighbor::RIGHT;
            if(flagmin(img.meta[pt.x+pt.y*img.width]) || pqueue.empty()) pqueue.push(point_sorted(pt,dex_adj));
          }
        }
        //test to the right
        if(p.x<img.width-1){
          point pt = point(p.x+1, p.y);
          if(!(img.meta[pt.x+pt.y*img.width] & (px::meta::SOLID|px::meta::UPDATED))){
            img.meta[pt.x+pt.y*img.width] |= px::neighbor::LEFT;
            if(flagmin(img.meta[pt.x+pt.y*img.width]) || pqueue.empty()) pqueue.push(point_sorted(pt,dex_adj));
          }
        }
        //test to the top
        if(p.y>0){
          point pt = point(p.x, p.y-1);
          if(!(img.meta[pt.x+pt.y*img.width] & (px::meta::SOLID|px::meta::UPDATED))){
            img.meta[pt.x+pt.y*img.width] |= px::neighbor::BOTTOM;
            if(flagmin(img.meta[pt.x+pt.y*img.width]) || pqueue.empty()) pqueue.push(point_sorted(pt,dex_adj));
          }
        }
        //test to the bottom
        if(p.y<img.height-1){
          point pt = point(p.x, p.y+1);
          if(!(img.meta[pt.x+pt.y*img.width] & (px::meta::SOLID|px::meta::UPDATED))){
            img.meta[pt.x+pt.y*img.width] |= px::neighbor::TOP;
            if(flagmin(img.meta[pt.x+pt.y*img.width]) || pqueue.empty()) pqueue.push(point_sorted(pt,dex_adj));
          }
        }
        
        //hmm guess i need to test diagonals too
        if(p.x>0 && p.y>0){
          point pt = point(p.x-1, p.y-1);
          if(!(img.meta[pt.x+pt.y*img.width] & (px::meta::SOLID|px::meta::UPDATED))){
            img.meta[pt.x+pt.y*img.width] |= px::neighbor::BOTTOMRIGHT;
            if(flagmin(img.meta[pt.x+pt.y*img.width]) || pqueue.empty()) pqueue.push(point_sorted(pt,dex_diag));
          }
        }

        if(p.x<img.width-1 && p.y>0){
          point pt = point(p.x+1, p.y-1);
          if(!(img.meta[pt.x+pt.y*img.width] & (px::meta::SOLID|px::meta::UPDATED))){
            img.meta[pt.x+pt.y*img.width] |= px::neighbor::BOTTOMLEFT;
            if(flagmin(img.meta[pt.x+pt.y*img.width]) || pqueue.empty()) pqueue.push(point_sorted(pt,dex_diag));
          }
        }

        if(p.x>0 && p.y<img.height-1){
          point pt = point(p.x-1, p.y+1);
          if(!(img.meta[pt.x+pt.y*img.width] & (px::meta::SOLID|px::meta::UPDATED))){
            img.meta[pt.x+pt.y*img.width] |= px::neighbor::TOPRIGHT;
            if(flagmin(img.meta[pt.x+pt.y*img.width]) || pqueue.empty()) pqueue.push(point_sorted(pt,dex_diag));
          }
        }

        if(p.x<img.width-1 && p.y<img.height-1){
          point pt = point(p.x+1, p.y+1);
          if(!(img.meta[pt.x+pt.y*img.width] & (px::meta::SOLID|px::meta::UPDATED))){
            img.meta[pt.x+pt.y*img.width] |= px::neighbor::TOPLEFT;
            if(flagmin(img.meta[pt.x+pt.y*img.width]) || pqueue.empty()) pqueue.push(point_sorted(pt,dex_diag));
          }
        }
      }
    }

  }

  void remove_halo (image& img, replace_mode& method){
    if(method.iteration_type == replace_mode::FLAT_ITERATION){
      remove_halo_flat_iteration(img, method);
    } else {
      remove_halo_smear_iteration(img, method);
    }
  }

  namespace mode {
    replace_mode solid = {replace_mode::FLAT_ITERATION, {0}, NULL, solid_color};
    replace_mode smear = {replace_mode::SMEAR_ITEATION, {0}, NULL, smear_color};
  }

};
  