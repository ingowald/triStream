// ======================================================================== //
// Copyright 2020 Ingo Wald                                                 //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "owl/common/math/vec.h"
#include "owl/common/arrayND/array3D.h"
#define MINI_HAVE_OWL_COMMON 1
#include "miniScene/common.h"
// #include "pbrtParser/Scene.h"
#include <fstream>
#include <memory>
#include <limits>

namespace triStream {

  using namespace mini;
  
#define TRISTREAM_HAVE_LZO 1

  void init();
  
  struct Triangle {
    Triangle(const vec3f &_a,
             const vec3f &_b,
             const vec3f &_c,
             const vec3f &_na,
             const vec3f &_nb,
             const vec3f &_nc,
             const vec2f &_ta,
             const vec2f &_tb,
             const vec2f &_tc,
             uint64_t     _meta=0)
      : v{_a,_b,_c},
        n{_na,_nb,_nc},
        uv{_ta,_tb,_tc},
        meta(_meta)
    {}
    Triangle(const vec3f &_a,
             const vec3f &_b,
             const vec3f &_c,
             uint64_t     _meta)
      : v{_a,_b,_c},
        // n{vec3f(0.f),vec3f(0.f),vec3f(0.f)},
        // texCoord{vec2f(INFINITY),vec2f(INFINITY)},
        meta(_meta)
    {}
    Triangle() = default;
    Triangle(const Triangle &other) = default;
    Triangle &operator=(const Triangle &other) = default;

    inline box3f bounds() const { return box3f(v[0]).including(v[1]).including(v[2]); }
    inline box3f getBounds() const { return bounds(); }
    
    vec3f v[3];
    vec3f n[3]  = {vec3f(0.f),vec3f(0.f),vec3f(0.f)};
    vec2f uv[3] = {vec2f(INFINITY),vec2f(INFINITY)};
        

    //! normals, may be 0 if not present
    // vec3f na,nb,nc;
    uint64_t meta = 0;
  };

}
