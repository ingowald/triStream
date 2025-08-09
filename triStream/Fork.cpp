// ======================================================================== //
// Copyright 2020-2022 Ingo Wald                                            //
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

#include "triStream/Fork.h"

namespace triStream {

  std::vector<Sink *> getPointers(const std::vector<Sink::SP> &sinkHandles)
  {
    std::vector<Sink *> res;
    for (auto sink : sinkHandles) res.push_back(sink.get());
    return res;
  }
  
  ForkSink::ForkSink(const std::vector<Sink::SP> &sinkHandles)
    : sinkHandles(sinkHandles),
      sinks(getPointers(sinkHandles))
  {
  }

  ForkSink::ForkSink(Sink *a, Sink *b)
  {
    sinks.push_back(a);
    sinks.push_back(b);
  }
  
  ForkSink::ForkSink(Sink::SP a, Sink::SP b)
  {
    sinks.push_back(a.get());
    sinks.push_back(b.get());
    sinkHandles.push_back(a);
    sinkHandles.push_back(b);
  }
  
  void ForkSink::push(const std::vector<Triangle> &triangles) 
  {
    parallel_for(sinks.size(),
                 [&](size_t sinkID){
                   sinks[sinkID]->push(triangles);
                 });
  }
    
}
