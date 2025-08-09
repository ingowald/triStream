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

#include "triStream/PBRTSceneSource.h"
#include "triStream/GridSplitter.h"
#include "triStream/Stats.h"
#include "triStream/BoxFilter.h"
#include "triStream/Fork.h"
#include "triStream/FileSink.h"
#include "triStream/MeshBuilderSink.h"

namespace triStream {

  void splitGrid(Source::SP source,
                 const vec3i &dims,
                 const box3f &sceneBounds,
                 const std::string &outFileBase)
  {
    std::vector<StatsSink::SP> cellStats;
    std::vector<Sink::SP>      cellSinks;
    cellStats.clear();
    int streamID = 0;
    array3D::for_each
      (dims,
       [&](const vec3i &idx) {
         std::string fileName
           = outFileBase + "_" + std::to_string(streamID++)+".bts";
         FileSink::SP  data = std::make_shared<FileSink>(fileName);
         StatsSink::SP stat = std::make_shared<StatsSink>();
         ForkSink::SP  fork = std::make_shared<ForkSink>(data,stat);
         cellStats.push_back(stat);
         cellSinks.push_back(fork);
       });
    
    
    GridSplitter::SP grid
      = std::make_shared<GridSplitter>(sceneBounds,dims,cellSinks);
    source->streamTo(grid.get());
    
    // cellDomains.clear();
    
    std::ofstream out(outFileBase+".bts_partition",std::ios::binary);
    size_t numBins = dims.x*dims.y*dims.z;
    out.write((char*)&numBins,sizeof(numBins));
    streamID = 0;
    array3D::for_each
      (dims,
       [&](const vec3i &idx){
         box3f cellDomain = grid->getCellBounds(idx);
         box3f cellBounds  = cellStats[streamID++]->bounds;
         std::cout << "cell " << idx << " bounds = " << cellBounds << ", domain = " << cellDomain << std::endl;
         out.write((char*)&cellDomain,sizeof(cellDomain));
         out.write((char*)&cellBounds,sizeof(cellBounds));
       });
  }
  
  
  using CellList = std::vector<int>;

  Source::SP createSource(const std::string &fileName)
  {
    if (fileName.substr(fileName.size()-4) == ".pbf")
      return std::make_shared<PBRTSceneSource>(fileName);
    
    if (fileName.substr(fileName.size()-4) == ".bts")
      return std::make_shared<FileSource>(fileName);

    throw std::runtime_error("unknown file format in " + fileName);
  }
  

  extern "C" int main(int ac, char **av)
  {
    std::string inFileName = "";
    std::string outFileBase;
    vec3i dims(0);
    int autoDims = 0;
    int numRanks = 2;
    
    for (int i=1;i<ac;i++) {
      const std::string arg =av[i];
      if (arg == "-o") {
        outFileBase = av[++i];
      } else if (arg == "-o") {
        outFileBase = av[++i];
      } else if (arg == "-dims") {
        dims.x = atoi(av[++i]);
        dims.y = atoi(av[++i]);
        dims.z = atoi(av[++i]);
      } else if (arg[0] != '-') {
        inFileName = arg;
      } else
        throw std::runtime_error("unknown cmd line arg "+arg);
    }
    if (dims == vec3i(0))
      throw std::runtime_error("dims not specified");
    if (outFileBase == "")
      throw std::runtime_error("no output file name specified (-o)");
    if (inFileName == "")
      throw std::runtime_error("no input file specified");

    Source::SP source = createSource(inFileName);

    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "stage 1: computing input stats" << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    
    StatsSink stats;
    source->streamTo(&stats);
    PRINT(stats.count);
    PRINT(stats.bounds);

    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "stage 2: computing bin sizes for "
              << dims << " bins" << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;

    splitGrid(source,dims,stats.bounds,
              outFileBase);
  }    
}
