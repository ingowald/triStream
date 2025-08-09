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

  std::string computeHashFor(mini::Mesh::SP mesh)
  {
    mini::Material::SP mat = mesh->material;
#if 1
    std::stringstream ss;
    ss << (int*)mat.get();
    return ss.str();
#else
    DisneyMaterial::SP disney = mat->as<DisneyMaterial>();
    if (disney) {
      ss << "DisneyMaterial:";
      ss << "emission " << disney->emission;
      ss << "baseColor " << disney->baseColor;
      ss << "roughness " << disney->roughness;
      ss << "transmission " << disney->transmission;
      ss << "ior " << disney->ior;
      // we use actual mem addresses here, which is OK because we only
      // uset hsi for a hash, this will never get stored anywhere
      // beyond the lifetime of this process.
      ss << "colorTexture " << (size_t)disney->colorTexture.get();
      ss << "alphaTexture " << (size_t)disney->alphaTexture.get();
      return ss.str();
    }
    return mat->toString();
#endif
  }
  
  MeshBuilderSink::SP MetaMeshBuilderSink::getSinkFor(int meta)
  {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = meshByMeta.find(meta);
    if (it == meshByMeta.end()) {
      if (metaMeshes) {
        // merge metas that refer to the same material ...
        const std::string hash = computeHashFor((*metaMeshes)[meta]);
        if (uniqueMetaByMaterial.find(hash) != uniqueMetaByMaterial.end()) {
          meshByMeta[meta] = meshByMeta[uniqueMetaByMaterial[hash]];
        } else {
          MeshBuilderSink::SP newSink = MeshBuilderSink::create();
          meshByMeta[meta] = newSink;
          uniqueMetaByMaterial[hash] = meta;
        }
        return meshByMeta[meta];
      } else {
        // NO material merging - every used meta gets its own output mesh
        MeshBuilderSink::SP newSink = MeshBuilderSink::create();
        meshByMeta[meta] = newSink;
        return newSink;
      }
    } else
      return it->second;
  }
    
}
