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

#include "MiniSceneSource.h"

namespace triStream {

  MiniSceneSource::MiniSceneSource(const std::string &fileName)
    : fileName(fileName)
  {
    std::cout << "#bts: " << toString() << ": loading input..." << std::endl;
    scene = mini::Scene::load(fileName);
    std::cout << "#bts: " << toString() << ": ready to stream." << std::endl;

    // ------------------------------------------------------------------
    // FIND all unique materials
    // ------------------------------------------------------------------
    mini::Object::SP metaObject = mini::Object::create();
    
    for (int instID=0;instID<scene->instances.size();instID++) {
      auto inst = scene->instances[instID];
      if (!inst) continue;
      if (!inst->object) continue;
      for (int meshID=0;meshID<inst->object->meshes.size();meshID++) {
        auto mesh = inst->object->meshes[meshID];
        if (!mesh) continue;
        if (!mesh->material) continue;

        IGs.push_back({instID,meshID});

        mini::Mesh::SP metaMesh = std::make_shared<mini::Mesh>();
        metaMesh->material = mesh->material;
        metaObject->meshes.push_back(metaMesh);
      }
    }
    
    // ------------------------------------------------------------------
    // create dummy object
    // ------------------------------------------------------------------
    meta = std::make_shared<mini::Scene>();
    meta->instances.push_back(std::make_shared<Instance>(metaObject));
    meta->quadLights  = scene->quadLights;
    meta->dirLights   = scene->dirLights;
    meta->envMapLight = scene->envMapLight;
    std::cout << "#bts: " << toString() << ": created semi-flattened meta-scene with "
              << metaObject->meshes.size()
              << " materials" << std::endl;
  }
    
  void MiniSceneSource::streamTo(Sink *sink)
  {
    assert(scene);
    parallel_for
      (IGs.size(),
       [&](size_t igID){
         auto ig = IGs[igID];
         auto inst = scene->instances[ig.instID];
         auto mesh = inst->object->meshes[ig.meshID];
         
         WriteBuffer buffer(sink);
         const affine3f &xfm = inst->xfm;
         for (auto idx : mesh->indices) {
           Triangle tmp;
           tmp.v[0] = xfmPoint(xfm,mesh->vertices[idx.x]);
           tmp.v[1] = xfmPoint(xfm,mesh->vertices[idx.y]);
           tmp.v[2] = xfmPoint(xfm,mesh->vertices[idx.z]);
           if (!mesh->texcoords.empty()) {
             tmp.uv[0] = mesh->texcoords[idx.x];
             tmp.uv[1] = mesh->texcoords[idx.y];
             tmp.uv[2] = mesh->texcoords[idx.z];
           }
           if (!mesh->normals.empty()) {
             tmp.n[0] = xfmNormal(xfm,mesh->normals[idx.x]);
             tmp.n[1] = xfmNormal(xfm,mesh->normals[idx.y]);
             tmp.n[2] = xfmNormal(xfm,mesh->normals[idx.z]);
           }
           tmp.meta = igID;
           buffer.push(tmp);
         }
       });
    sink->flush();
  }

  void MiniSceneSource::saveMeta(const std::string &fileName)
  {
    meta->save(fileName);
    std::cout << "#bts: " << toString() << ": mini scene meta-data saved to " << fileName << std::endl;
  }
  
}
