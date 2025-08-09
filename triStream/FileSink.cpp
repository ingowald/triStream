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

#include "triStream/FileSink.h"
#include "minilzo.h"

namespace triStream {

  FileSink::FileSink(const std::string &fileName)
    : file(fileName,std::ios::binary), fileName(fileName)
  {
    if (!file.good())
      throw std::runtime_error("could not open filesink "+toString());
    triStream::init();
    header.bounds = box3f();
    header.count  = 0;
    file.write((char*)&header,sizeof(header));
  }

  FileSink::~FileSink()
  {
    flush();

    // now add the header to the front
    file.seekp(0);
    file.write((char*)&header,sizeof(header));
    if (!file.good()) {
      // throw std::runtime_error("some error while writing to filesink "+toString());
      std::cerr << "some error while writing to filesink " << toString() << std::endl;
    }
  }

  void FileSink::flush()
  { 
  }

  void FileSink::push(const std::vector<Triangle> &triangles)
  {
#if TRISTREAM_HAVE_LZO
    box3f thisBounds;
    for (int i=0;i<triangles.size();i++) {
      auto tri = triangles[i];
      thisBounds.extend(tri.getBounds());
    }

    unsigned char workMem[LZO1X_1_MEM_COMPRESS];
    lzo_uint inSize = triangles.size()*sizeof(Triangle);
    // allocate FAR more than we need:
    lzo_uint outSize = 2*inSize+LZO1X_1_MEM_COMPRESS;
    std::vector<char> compressed(outSize);
    
    auto r = lzo1x_1_compress((lzo_bytep)triangles.data(),
                           inSize,
                           (lzo_bytep)compressed.data(),
                           &outSize, workMem);
    if (r != LZO_E_OK || outSize > compressed.size())
      throw std::runtime_error("error in lzo compression");

    // ------------------------------------------------------------------
    // now actually lock and write...
    // ------------------------------------------------------------------
    std::lock_guard<std::mutex> lock(mutex);
    header.bounds.extend(thisBounds);
    
    int numTris = triangles.size();
    header.count += numTris;
    file.write((char*)&numTris,sizeof(numTris));
    file.write((char*)&outSize,sizeof(outSize));
    file.write(compressed.data(),outSize);
#else
    std::lock_guard<std::mutex> lock(mutex);

    for (auto &tri : triangles) {
      // auto &tri = buffer[i];
      header.bounds.extend(tri.bounds());
      header.count++;
    }
    file.write((char*)triangles.data(),triangles.size()*sizeof(Triangle));
#endif
  }

}

