// ======================================================================== //
// Copyright 2018-2021 Ingo Wald                                            //
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

#include "triStream/FileSource.h"
#include "miniScene/Scene.h"
#include "triStream/MeshBuilderSink.h"

namespace triStream {

  void fillMesh(mini::Mesh::SP mesh,
                MeshBuilderSink::SP sink)
  {
    mesh->vertices.resize(sink->vertexMap.size());
    for (auto it : sink->vertexMap) {
      auto vtx = it.first;
      const vec3f v  = std::get<0>(vtx);
      const vec3f n  = std::get<1>(vtx);
      const vec2f uv = std::get<2>(vtx);
      const int vtxID = it.second;

      mesh->vertices[vtxID] = v;
      
      if (n != vec3f(0.f)) {
        if (mesh->normals.empty())
          mesh->normals.resize(mesh->vertices.size());
        mesh->normals[vtxID] = n;
      }
      
      if (uv != vec2f(INFINITY)) {
        if (mesh->texcoords.empty())
          mesh->texcoords.resize(mesh->vertices.size());
        mesh->texcoords[vtxID] = uv;
      }
    }
    mesh->indices = sink->indices;
  }
  
  extern "C" int main(int ac, char **av)
  {
    std::string inFileName = "";
    std::string outFileName = "";
    std::string metaFileName = "";
    
    for (int i=1;i<ac;i++) {
      const std::string arg =av[i];
      if (arg == "-o") {
        outFileName = av[++i];
      } else if (arg == "-m" || arg == "--meta") {
        metaFileName = av[++i];
      } else if (arg[0] != '-') {
        inFileName = arg;
      } else throw std::runtime_error("unknown cmd line arg "+arg);
    }

    if (inFileName == "")
      throw std::runtime_error("no input filename specified");
    if (metaFileName == "")
      throw std::runtime_error("no meta filename specified (-m)");
    if (outFileName == "")
      throw std::runtime_error("no output filename specified (-o)");

    mini::Scene::SP metaScene = Scene::load(metaFileName);
    if (metaScene->instances.size() != 1 ||
        !metaScene->instances[0] ||
        !metaScene->instances[0]->object)
      throw std::runtime_error("this doesn't look like a valid mini meta scene !?");
    
    auto &meta = metaScene->instances[0]->object->meshes;

    std::cout << "creating file source from " << inFileName << std::endl;
    FileSource source(inFileName);
    MetaMeshBuilderSink::SP meshBuilder = MetaMeshBuilderSink::create();
    source.streamTo(meshBuilder.get());
    
    std::cout << "done building meshes: found "
              << prettyNumber(meshBuilder->meshByMeta.size())
              << " meshes total" << std::endl;
    
    std::cout << "now merging with meta and building scene" << std::endl;
    
    mini::Object::SP outObject = mini::Object::create();
    for (auto it : meshBuilder->meshByMeta) {
      int igID = it.first;
      if (igID < 0 || igID >= meta.size() || !meta[igID] || !meta[igID]->material)
        throw std::runtime_error("invalid igID for this meta scene !?");
      mini::Material::SP mat = meta[igID]->material;
      mini::Mesh::SP mesh = mini::Mesh::create(mat);

      fillMesh(mesh,it.second);
      outObject->meshes.push_back(mesh);
    }
    
    mini::Scene::SP outScene = mini::Scene::create();
    outScene->instances.push_back(mini::Instance::create(outObject));

    outScene->save(outFileName);
    // mini::Mesh::SP miniMesh = std::make_shared<mini::TriangleMesh>();
    // miniMesh->material = std::make_shared<mini::MatteMaterial>();
    // (std::vector<vec3i>&)miniMesh->index  = std::move(mesh.index);
    // (std::vector<vec3f>&)miniMesh->vertex = std::move(mesh.vertex);

    // mini::Object::SP miniObject = std::make_shared<mini::Object>();
    // miniObject->shapes.push_back(miniMesh);
    
    // mini::Scene::SP miniScene = std::make_shared<mini::Scene>();
    // miniScene->world = miniObject;
    // miniScene->saveTo(outFileName);
  }
  
}
