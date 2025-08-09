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

#include "triStream/FileSource.h"

namespace triStream {

  struct PBRTSceneSource : public Source
  {
    PBRTSceneSource(const std::string &fileName);
    void streamTo(Sink *sink) override;

    virtual std::string toString() const
    { return "PBRTSceneSource("+fileName+")"; }

    void saveMeta(const std::string &fileName);
    
    /*! the meta-data we extracted from the scene; basically a single
        object with one dummy shape per input material; the 'meta'
        field of each generated triangle gets set to the index of the
        material in this faked material list */
    pbrt::Scene::SP   metaData;
  private:
    std::map<pbrt::Material::SP,int> materialMap;
    const std::string fileName;
    pbrt::Scene::SP   scene;
  };

}
