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

#include "triStream/Sink.h"

namespace triStream {

  /*! a filter is a sink that filters out some of the pushed
      triangles, and pushes the rest on to another sink */
  struct BoxFilter : public Sink {
    BoxFilter(Sink *sink, const box3f &stencil);
    BoxFilter(const box3f &stencil, Sink *sink);
    BoxFilter(Sink *sink, const std::vector<box3f> &stencils);
    BoxFilter(Sink::SP sink, const std::vector<box3f> &stencils);
    
    void push(const std::vector<Triangle> &triangles) override;

  private:
    bool active(const Triangle &tri) const;
    
    Sink *const sink;
    Sink::SP sinkHandle;
    const std::vector<box3f> stencils;
  };

}
