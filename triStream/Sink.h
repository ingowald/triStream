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

#include "triStream/TriStream.h"

namespace triStream {

  struct Sink;
  
  struct WriteBuffer {
    WriteBuffer(Sink *sink, int autoFlushThreshold=16*1024)
      : sink(sink),
        autoFlushThreshold(autoFlushThreshold)
    {}
    
    ~WriteBuffer() { flush(); }
    
    void push(const Triangle &triangle);
    void flush();
    
    std::vector<Triangle> triangles;
    Sink *const sink;
    const int autoFlushThreshold;
  };
  
  struct Sink {
    typedef std::shared_ptr<Sink> SP;
    
    virtual std::string toString() const { return "triStream::Sink"; }
    // virtual void push(const Triangle &triangle) = 0;
    
    virtual void push(const std::vector<triStream::Triangle> &triangles) = 0;

    virtual void flush() {}
  };

  inline void WriteBuffer::push(const Triangle &triangle)
  {
    triangles.push_back(triangle);
    if (triangles.size() >= autoFlushThreshold)
      flush();
  }
  
  inline void WriteBuffer::flush()
  {
    if (triangles.empty())
      return;
    sink->push(triangles);
    triangles.clear();
  }

}
