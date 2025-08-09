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

#include "PBRTSceneSource.h"

namespace triStream {

  PBRTSceneSource::PBRTSceneSource(const std::string &fileName)
    : fileName(fileName)
  {
    std::cout << toString() << ": loading input..." << std::endl;
    scene = pbrt::Scene::loadFrom(fileName);
    std::cout << toString() << ": converting to single level..." << std::endl;
    std::cout << toString() << ": ready to stream." << std::endl;
    scene->makeSingleLevel();

    // ------------------------------------------------------------------
    // FIND all unique materials
    // ------------------------------------------------------------------
    std::set<pbrt::Material::SP> knownMaterials;
    for (auto inst : scene->world->instances)
      if (inst && inst->object)
        for (auto shape : inst->object->shapes)
          if (shape && shape->material)
            knownMaterials.insert(shape->material);
    
    // ------------------------------------------------------------------
    // create dummy object
    // ------------------------------------------------------------------
    metaData = std::make_shared<pbrt::Scene>();
    metaData->world = std::make_shared<pbrt::Object>();
    for (auto mat : knownMaterials) {
      materialMap[mat] = metaData->world->shapes.size();
      pbrt::Shape::SP shape = std::make_shared<pbrt::TriangleMesh>();
      shape->material = mat;
      metaData->world->shapes.push_back(shape);
    }
    std::cout << "created material map for "
              << metaData->world->shapes.size()
              << " materials" << std::endl;
  }
  
  void PBRTSceneSource::streamTo(Sink *sink)
  {
    assert(scene);
    parallel_for(scene->world->instances.size(),[&](size_t instID){
        auto inst = scene->world->instances[instID];
        // for (auto inst : scene->world->instances) {
        if (!inst) return;
        if (!inst->object) return;
        
        WriteBuffer buffer(sink);
        for (auto shape : inst->object->shapes) {
          if (!shape) return;
          pbrt::TriangleMesh::SP mesh
            = shape->as<pbrt::TriangleMesh>();
          if (!mesh)
            continue;
          const affine3f &xfm = (const affine3f&)inst->xfm;
          for (auto idx : mesh->index) {
            const vec3f A = xfmPoint(xfm,(const vec3f&)mesh->vertex[idx.x]);
            const vec3f B = xfmPoint(xfm,(const vec3f&)mesh->vertex[idx.y]);
            const vec3f C = xfmPoint(xfm,(const vec3f&)mesh->vertex[idx.z]);
            buffer.push(Triangle(A,B,C,0));
          }
        }
      });
    sink->flush();
  }

  void PBRTSceneSource::saveMeta(const std::string &fileName)
  {
    metaData->saveTo(fileName);
    std::cout << "#bts: pbrt scene meta-data saved to " << fileName << std::endl;
  }
  
}
