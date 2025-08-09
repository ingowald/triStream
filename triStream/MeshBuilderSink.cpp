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

#include "MeshBuilderSink.h"

namespace triStream {

  void MeshBuilderSink::push(const std::vector<Triangle> &triangles) 
  {
    std::vector<vec3i> indices;
    for (auto tri : triangles) {
      indices.push_back(vec3i(getVertexID(tri.v[0],tri.n[0],tri.uv[0]),
                              getVertexID(tri.v[1],tri.n[1],tri.uv[1]),
                              getVertexID(tri.v[2],tri.n[2],tri.uv[2])));
    }
    std::lock_guard<std::mutex> lock(indexMutex);
    for (auto idx : indices)
      this->indices.push_back(idx);
  }

  int MeshBuilderSink::getVertexID(const vec3f &v, const vec3f &n, const vec2f &uv)
  {
    std::lock_guard<std::mutex> lock(vertexMutex);
    std::tuple<vec3f,vec3f,vec2f> key{v,n,uv};
    
    auto it = vertexMap.find(key);
    if (it != vertexMap.end()) return it->second;

    int idx = vertexMap.size();
    vertexMap[key] = idx;
    return idx;
  }
  
  void MetaMeshBuilderSink::push(const std::vector<Triangle> &triangles)
  {
    std::vector<Triangle> stillToDo = triangles;
    std::vector<Triangle> matching, other;
    while (!stillToDo.empty()) {
      int thisMeta = stillToDo[0].meta;
      for (auto &tri : stillToDo)
        if (tri.meta == thisMeta)
          matching.push_back(tri);
        else
          other.push_back(tri);
      MeshBuilderSink::SP thisSink = getSinkFor(thisMeta);
      thisSink->push(matching);
      stillToDo = std::move(other);
    }
  }
  
  MeshBuilderSink::SP MetaMeshBuilderSink::getSinkFor(int meta)
  {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = meshByMeta.find(meta);
    if (it == meshByMeta.end()) {
      MeshBuilderSink::SP newSink = MeshBuilderSink::create();
      meshByMeta[meta] = newSink;
      return newSink;
    } else
      return it->second;
  }
    
}
