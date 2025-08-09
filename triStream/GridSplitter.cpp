// ======================================================================== //
// Copyright 2018-2020 Ingo Wald                                            //
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

#include "triStream/GridSplitter.h"

namespace triStream {
  
  GridSplitter::GridSplitter(const box3f &domain,
                             const vec3i &dims,
                             const std::vector<Sink::SP> &cells)
    : domain(domain),
      dims(dims),
      cells(cells),
      filters(volume(dims))
  {
    owl::common::array3D::for_each
      (dims,[&](const vec3i &idx){
              const size_t linearIdx = owl::common::array3D::linear(idx,dims);
              const box3f cellBounds = getCellBounds(idx);
              // std::cout << "gridsplitter cell " << idx << " = " << cellBounds << std::endl;
              filters[linearIdx]
                = std::make_shared<BoxFilter>(cellBounds,
                                              cells[linearIdx].get());
            });
  }
  
  // Sink::SP GridSplitter::getCell(const vec3i &idx)
  // {
  //   return cells[owl::common::array3D::linear(idx,dims)]; 
  // }
  
  // void GridSplitter::setCell(const vec3i &idx, Sink::SP sink)
  // {
  //   cells[owl::common::array3D::linear(idx,dims)] = sink;
  // }
  
  box3f GridSplitter::getCellBounds(const vec3i &idx) const
  {
    const box3f cellBounds
      (domain.lower+vec3f(idx)*domain.size()/vec3f(dims),
       domain.lower+vec3f(idx+vec3i(1))*domain.size()/vec3f(dims));
    return cellBounds;
  }
  
  void GridSplitter::push(const std::vector<Triangle> &triangles)
  {
    parallel_for(filters.size(),[&](size_t filterID){
                                  filters[filterID]->push(triangles);
                                });
  }
  
  
}
