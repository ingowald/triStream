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

#include "triStream/Filter.h"

namespace triStream {

  BoxFilter::BoxFilter(Sink *sink, const box3f &stencil)
    : sink(sink),
      stencils({stencil})
  {}
  
  BoxFilter::BoxFilter(const box3f &stencil, Sink *sink)
    : sink(sink),
      stencils({stencil})
  {}
  
  BoxFilter::BoxFilter(Sink *sink, const std::vector<box3f> &stencils)
    : sink(sink),
      stencils(stencils)
  {}

  BoxFilter::BoxFilter(const Sink::SP &sinkHandle, const box3f &stencil)
    : sink(sinkHandle.get()),
      sinkHandle(sinkHandle),
      stencils(std::vector<box3f>{stencil})
  {}
  
  BoxFilter::BoxFilter(const Sink::SP &sinkHandle, const std::vector<box3f> &stencils)
    : sink(sinkHandle.get()),
      sinkHandle(sinkHandle),
      stencils(stencils)
  {}
    
  void BoxFilter::push(const std::vector<Triangle> &triangles) 
  {
    WriteBuffer  buffer(sink);
    for (auto &tri : triangles) {
      if (active(tri))
        buffer.push(tri);
    }
    buffer.flush();
  }

  bool BoxFilter::active(const Triangle &tri) const
  {
    const box3f bounds = tri.bounds();
    for (auto box : stencils)
      if (!box.empty() && bounds.overlaps(box))
        return true;
    return false;
  }
        
}
