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

#include "triStream/MiniSceneSource.h"
#include "triStream/Stats.h"
#include "owl/common/arrayND/array3D.h"

namespace triStream {

  extern "C" int main(int ac, char **av)
  {
    if (ac < 2) throw std::runtime_error("usage: ./btsInfo infile.bts");

    for (int i=1;i<ac;i++) {
      std::string inFileName = av[i];
      
      FileSource::SP in = std::make_shared<FileSource>(inFileName);
      StatsSink stats;
      in->streamTo(&stats);
      std::cout << av[i] << " -> " << prettyNumber(stats.count) << " bounds = " << stats.bounds << std::endl;
    }
  }
}
