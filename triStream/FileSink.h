// ======================================================================== //
// Copyright 2020-2022 Ingo Wald                                            //
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

#include "triStream/Sink.h"

namespace triStream {

  struct FileSink : public Sink
  {
    typedef std::shared_ptr<FileSink> SP;

    static SP create(const std::string &fileName)
    { return std::make_shared<FileSink>(fileName); }
    
    FileSink(const std::string &fileName);
    ~FileSink();

    std::string toString() const override { return "triStream::FileSink{"+fileName+"}"; }
    
    // void push(const Triangle &triangle) override;
    void push(const std::vector<Triangle> &triangles);

    void flush() override;
    
    struct Header {
      enum { MAGIC = 0x1425021920 };
      box3f  bounds ;
      size_t count = 0;
      size_t magic = MAGIC;
    };
  private:
    
    std::mutex mutex;
    
    /*! the header we'll add to every file after we close it; we store
        this here loally so we can incrementally update it every time
        we're pushing a buffer */
    Header header;
    std::ofstream file;
    const std::string   fileName;
  };

}
