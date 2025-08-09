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

#include "triStream/Filter.h"
#include "triStream/Stats.h"

namespace triStream {

  /*! splits a given stream up into NxMxK substreams relative to a
      given world bounding box. traignels overlapping multiple 'cells'
      of the output will get replicated */
  struct GridSplitter : public Sink
  {
    typedef std::shared_ptr<GridSplitter> SP;
    
    GridSplitter(const box3f &domain,
                 const vec3i &dims,
                 const std::vector<Sink::SP> &cells);
    // Sink::SP getCell(const vec3i &idx);
    // void setCell(const vec3i &idx, Sink::SP sink);

    box3f getCellBounds(const vec3i &cellIdx) const;
    void push(const std::vector<Triangle> &triangles) override;
    // void push(const Triangle &triangle) override;
    // void flush() override;
    
    std::vector<Sink::SP>   cells;
    std::vector<BoxFilter::SP> filters;
    const box3f domain;
    const vec3i dims;
  };
  
}
