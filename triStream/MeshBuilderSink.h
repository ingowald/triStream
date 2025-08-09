// ======================================================================== //
// Copyright 2020-2021 Ingo Wald                                            //
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

#include "Sink.h"

namespace triStream {

  /*! a sink that builds a triangle *mesh* from the triangles pushed
      into it */
  struct MeshBuilderSink : public Sink {
    typedef std::shared_ptr<MeshBuilderSink> SP;

    static SP create() { return std::make_shared<MeshBuilderSink>(); }
    
    
    void push(const std::vector<Triangle> &triangles) override;

    std::vector<vec3i> indices;

    int getVertexID(const vec3f &v, const vec3f &n, const vec2f &t);
    
    std::mutex vertexMutex;
    std::mutex indexMutex;
    std::map<std::tuple<vec3f,vec3f,vec2f>,int> vertexMap;
  };

  struct MetaMeshBuilderSink : public Sink {
    typedef std::shared_ptr<MetaMeshBuilderSink> SP;

    static SP create() { return std::make_shared<MetaMeshBuilderSink>(); }
    
    void push(const std::vector<Triangle> &triangles) override;

    MeshBuilderSink::SP getSinkFor(int meta);
    
    std::mutex mutex;
    std::map<int,MeshBuilderSink::SP> meshByMeta;
  };
  
}
