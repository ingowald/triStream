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

#include "triStream/Stats.h"
#include <sstream>

namespace triStream {

  std::string StatsSink::toString() const
  {
    std::stringstream ss;
    ss << "Stats{box=" << bounds
       << ", count=" << count
       << " (" << prettyNumber(count) << ")}";
    return ss.str();
  }
  
  void StatsSink::print()
  {
    std::cout << toString() << std::endl;
    // PRINT(count);
    // PRINT(bounds);
  }
  
  void StatsSink::push(const std::vector<Triangle> &triangles) 
  {
    if (triangles.empty()) return;
    
    box3f bounds;
    for (auto &triangle : triangles) {
      bounds.extend(triangle.getBounds());
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    this->bounds.extend(bounds);
    this->count += triangles.size();
  }

}
