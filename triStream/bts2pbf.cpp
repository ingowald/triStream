// ======================================================================== //
// Copyright 2018-2020 Ingo Wald                                            //
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
#include "triStream/MeshBuilderSink.h"

namespace triStream {

  extern "C" int main(int ac, char **av)
  {
    std::string inFileName = "";
    std::string outFileName = "";
    
    for (int i=1;i<ac;i++) {
      const std::string arg =av[i];
      if (arg == "-o") {
        outFileName = av[++i];
      } else if (arg[0] != '-') {
        inFileName = arg;
      } else throw std::runtime_error("unknown cmd line arg "+arg);
    }

    if (inFileName == "")
      throw std::runtime_error("no input filename specified");
    if (outFileName == "")
      throw std::runtime_error("no output filename specified");
    
    FileSource source(inFileName);
    MeshBuilderSink mesh;
    source.streamTo(&mesh);
    
    std::cout << "done building mesh" << std::endl;
    std::cout << " - " << prettyNumber(mesh.index.size()) << " triangles" << std::endl;
    std::cout << " - " << prettyNumber(mesh.vertex.size()) << " vertices" << std::endl;

    pbrt::TriangleMesh::SP pbrtMesh = std::make_shared<pbrt::TriangleMesh>();
    pbrtMesh->material = std::make_shared<pbrt::MatteMaterial>();
    (std::vector<vec3i>&)pbrtMesh->index  = std::move(mesh.index);
    (std::vector<vec3f>&)pbrtMesh->vertex = std::move(mesh.vertex);

    pbrt::Object::SP pbrtObject = std::make_shared<pbrt::Object>();
    pbrtObject->shapes.push_back(pbrtMesh);
    
    pbrt::Scene::SP pbrtScene = std::make_shared<pbrt::Scene>();
    pbrtScene->world = pbrtObject;
    pbrtScene->saveTo(outFileName);
  }
  
}
