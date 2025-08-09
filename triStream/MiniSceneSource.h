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

#include "triStream/FileSource.h"
#include "miniScene/Scene.h"

namespace triStream {

  struct MiniSceneSource : public Source
  {
    typedef std::shared_ptr<MiniSceneSource> SP;

    struct IG { int instID, meshID; };

    static SP create(const std::string &fileName)
    { return std::make_shared<MiniSceneSource>(fileName); }
    
    MiniSceneSource(const std::string &fileName);
    void streamTo(Sink *sink) override;

    virtual std::string toString() const
    { return "MiniSceneSource("+fileName+")"; }

    void saveMeta(const std::string &fileName);
    
  private:
    const std::string fileName;

    /*! the actual scene we're streaming out */
    mini::Scene::SP   scene;

    std::vector<IG>   IGs;
    
    /* creates a "dummy" scene with one one instance, one object, and
       exactly one mesh per igID, with that mesh not having any
       triangles, but storing the material for that IGID */
    mini::Scene::SP   meta;
  };

}
