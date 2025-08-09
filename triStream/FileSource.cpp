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

#include "triStream/FileSource.h"
#include "minilzo.h"

namespace triStream {

  FileSource::FileSource(const std::string &fileName)
    : file(fileName,std::ios::binary)
  {
    triStream::init();
    if (!file.good())
      throw std::runtime_error("could not open file "+fileName);
  }
  
  void FileSource::streamTo(Sink *sink) 
  {
    file.seekg(0);

    FileSink::Header header;
    file.read((char*)&header,sizeof(header));
    if (header.magic != FileSink::Header::MAGIC)
      throw std::runtime_error("wrong magic number in bts file ....");

    std::cout << "#bts: starting to stream " << prettyNumber(header.count) << " triangles..." << std::endl;
    
#if TRISTREAM_HAVE_LZO
    int numLeft = header.count;
    while (numLeft) {
      int thisCount;
      file.read((char*)&thisCount,sizeof(thisCount));
      std::vector<Triangle> triangles(thisCount);
      
      lzo_uint compressedSize;
      file.read((char*)&compressedSize,sizeof(compressedSize));
      std::vector<char> compressed(compressedSize);
      file.read(compressed.data(),compressed.size());

      lzo_uint destLen = triangles.size()*sizeof(triangles[0]);
      unsigned char workMem[LZO1X_MEM_DECOMPRESS];
      int rc = lzo1x_decompress((lzo_bytep)compressed.data(),
                                compressed.size(),
                                (lzo_bytep)triangles.data(),
                                &destLen,workMem);
      if (rc != LZO_E_OK
          ||
          destLen != triangles.size()*sizeof(triangles[0]))
        throw std::runtime_error("error in minilzo decompression!?");
      numLeft -= triangles.size();

      sink->push(triangles);
    }
#else
    WriteBuffer buffer(sink);

    for (size_t i=0;i<header.count;i++) {
      Triangle t;
      file.read((char*)&t,sizeof(t));
      buffer.push(t);
    };
    buffer.flush();
#endif
    sink->flush();
  }







  OldBTSFileSource::OldBTSFileSource(const std::string &fileName)
    : file(fileName,std::ios::binary)
  {
    triStream::init();
    if (!file.good())
      throw std::runtime_error("could not open file "+fileName);
  }
  
  void OldBTSFileSource::streamTo(Sink *sink) 
  {
    file.seekg(0);

    FileSink::Header header;
    file.read((char*)&header,sizeof(header));
    if (header.magic != FileSink::Header::MAGIC)
      throw std::runtime_error("wrong magic number in bts file ....");

    std::cout << "#bts: starting to stream " << prettyNumber(header.count) << " triangles..." << std::endl;
    
    WriteBuffer buffer(sink);

    for (size_t i=0;i<header.count;i++) {
      OldBTSTriangle t;
      file.read((char*)&t,sizeof(t));
      // PING;
      // PRINT(t.v0);
      // PRINT(t.v1);
      // PRINT(t.v2);
      // PRINT(t.meta_ignore[0]);
      // PRINT(t.meta_ignore[1]);
      // PRINT(t.meta_ignore[2]);
      Triangle tNew(t.v0,t.v1,t.v2,i/(16*1024*1024));
      buffer.push(tNew);
    };
    buffer.flush();
    sink->flush();
  }

}

