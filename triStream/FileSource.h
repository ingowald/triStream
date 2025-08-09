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

#include "triStream/FileSink.h"

namespace triStream {

  struct Source {
    typedef std::shared_ptr<Source> SP;
    
    virtual std::string toString() const { return "triStream::Source"; }
    virtual void streamTo(Sink *sink) = 0;
  };

  struct FileSource : public Source
  {
    typedef std::shared_ptr<FileSource> SP;

    static SP create(const std::string &fileName)
    { return std::make_shared<FileSource>(fileName); }
    
    static const int bufferSize = 16*1024;
    FileSource(const std::string &fileName);
    
    void streamTo(Sink *sink) override;
    
    std::ifstream file;
  };


  /* source for old-style bts file from before normals and stuff got added */
  struct OldBTSFileSource : public Source
  {
    struct OldBTSTriangle {
      vec3f v0, v1, v2;
      // huh: no clue what that was - vertex colors? rgb material!?
      int meta_ignore[3];
    };
      
    typedef std::shared_ptr<OldBTSFileSource> SP;

    static SP create(const std::string &fileName)
    { return std::make_shared<OldBTSFileSource>(fileName); }
    
    static const int bufferSize = 16*1024;
    OldBTSFileSource(const std::string &fileName);
    
    void streamTo(Sink *sink) override;
    
    std::ifstream file;
  };
  
}
