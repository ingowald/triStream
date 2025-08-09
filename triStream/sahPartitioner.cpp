// ======================================================================== //
// Copyright 2022-2022 Ingo Wald                                            //
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

#include "triStream/Fork.h"
#include "triStream/Sink.h"
#include "triStream/Stats.h"
#include "triStream/Filter.h"
#include "triStream/FileSource.h"
#include "triStream/MiniSceneSource.h"

namespace triStream {

  void usage(const std::string &error)
  {
    if (!error.empty())
      std::cerr << "Error: " << error << "\n\n";
    std::cout << "Usage: ./btsSAHPartitioner inFile.bts -o outBasePath -n numParts\n";
    exit(!error.empty());
  }
  
  /*! a sink that creates one sink for each left and right side of an
      N-binned sah binner (with a stats sink in each such bin), and
      simply replicates all triangles into these bins; this mean that
      after the end of stream process we can simply look at all the
      differet bins and select the one with least sah */
  struct SAHSink : public Sink {
    SAHSink(const box3f &domain,
            const float surfaceWeight = 1.f,
            const int numBins = 8)
      : domain(domain),
        surfaceWeight(surfaceWeight),
        numBins(numBins)
    {
      std::vector<Sink::SP> allFilters;
      for (int dim=0;dim<3;dim++)
        for (int plane=1;plane<numBins;plane++) {
          box3f lDomain = domain;
          box3f rDomain = domain;
          lDomain.upper[dim] = rDomain.lower[dim]
            = domain.lower[dim] + (plane)/float(numBins)*domain.size()[dim];
          StatsSink::SP lSink = StatsSink::create();
          StatsSink::SP rSink = StatsSink::create();
          lSideSinks[dim].push_back(lSink);
          rSideSinks[dim].push_back(rSink);

          FilterSink::SP lFilter = FilterSink::create(lSink,lDomain);
          FilterSink::SP rFilter = FilterSink::create(rSink,rDomain);
          lSideFilters[dim].push_back(lFilter);
          rSideFilters[dim].push_back(rFilter);
          
          allFilters.push_back(lFilter);
          allFilters.push_back(rFilter);
        }
      forkSink = ForkSink::create(allFilters);
    }

    inline float makeSurfWeight(float surfaceArea)
    {
      return surfaceWeight * surfaceArea + (1.f-surfaceWeight)*1.f;
    }
    
    void findBestSplit(box3f &lDomain,
                       double &lWeight,
                       box3f &rDomain,
                       double &rWeight)
    {
      std::vector<double> lSideWeights[3];
      std::vector<double> rSideWeights[3];
      for (int dim=0;dim<3;dim++) {
        lSideWeights[dim].resize(lSideSinks[dim].size());
        rSideWeights[dim].resize(rSideSinks[dim].size());
      }
      double bestCost = std::numeric_limits<double>::infinity();
      int bestDim = -1;
      int bestBin = -1;
      for (int dim=0;dim<3;dim++)
        for (int i=0;i<lSideSinks[dim].size();i++) {
          lSideSinks[dim][i]->bounds = owl::common::intersection(lSideSinks[dim][i]->bounds,
                                                lSideFilters[dim][i]->getBox());
          rSideSinks[dim][i]->bounds = owl::common::intersection(rSideSinks[dim][i]->bounds,
                                                rSideFilters[dim][i]->getBox());
          lSideWeights[dim][i]
            = (lSideSinks[dim][i]->count == 0)
            ? std::numeric_limits<double>::infinity()
            : lSideSinks[dim][i]->count * makeSurfWeight(area(lSideSinks[dim][i]->bounds));
          rSideWeights[dim][i]
            = (rSideSinks[dim][i]->count == 0)
            ? std::numeric_limits<double>::infinity()
            : rSideSinks[dim][i]->count * makeSurfWeight(area(rSideSinks[dim][i]->bounds));
          const double cost = lSideWeights[dim][i] + rSideWeights[dim][i];
          if (cost <= bestCost) {
            bestDim = dim;
            bestBin = i;
            bestCost = cost;
          }
        }
      if (bestDim == -1)
        throw std::runtime_error("could not find *any* plane!?");
      lDomain = lSideSinks[bestDim][bestBin]->bounds;
      rDomain = rSideSinks[bestDim][bestBin]->bounds;
      lWeight = lSideSinks[bestDim][bestBin]->count;
      rWeight = rSideSinks[bestDim][bestBin]->count;
    }
    
    void push(const std::vector<Triangle> &triangles) override
    { forkSink->push(triangles); }
    
    std::vector<StatsSink::SP>  lSideSinks[3];
    std::vector<StatsSink::SP>  rSideSinks[3];
    std::vector<FilterSink::SP> lSideFilters[3];
    std::vector<FilterSink::SP> rSideFilters[3];
    ForkSink::SP forkSink;
    const box3f &domain;
    const int    numBins;

    /*! used to linearly blend between SAH (where each prim count left
        and right is weighted by surface area of the bbox) and pure
        triangle weight. For surfaceWeight=1, surface goes in fully
        into the term (meaning SHA); for surfaceWeight=0 we only
        weight split by num tris left vs right */
    const float surfaceWeight;
  };
  
}

using namespace triStream;
using mini::common::endsWith;

int main(int ac, char **av)
{
  std::string inFileName;
  std::string outFileBase;
  float surfaceWeight = 1.f;
  int targetNumParts = 0;
  for (int i=1;i<ac;i++) {
    const std::string arg = av[i];
    if (arg[0] != '-')
      inFileName = arg;
    else if (arg == "-o")
      outFileBase = av[++i];
    else if (arg == "-s" || arg == "--surface-weight")
      surfaceWeight = std::stof(av[++i]);
    else if (arg == "-n")
      targetNumParts = std::stoi(av[++i]);
    else
      usage("unknown cmdline arg '"+arg+"'");
  }
  if (inFileName.empty())
    usage("no input file name specified");
  if (outFileBase.empty())
    usage("no output file path specified (-o)");
  if (targetNumParts <= 0)
    usage("no target num parts specified (-t)");

  MiniSceneSource::SP miniScene;
  Source::SP source;
  if (endsWith(inFileName,".mini")) 
    source = miniScene = MiniSceneSource::create(inFileName);
  else if (endsWith(inFileName,".bts"))
    source = FileSource::create(inFileName);
  else if (endsWith(inFileName,".old-bts"))
    source = OldBTSFileSource::create(inFileName);
  else
    throw std::runtime_error("unknown input file type (should be .bts or .mini)");

  std::cout << OWL_TERMINAL_BLUE << std::endl;
  std::cout << "------------------------------------------------------------------" << std::endl;
  std::cout << "btsSAHPartitioner:" << std::endl;
  std::cout << "- input file        : " << inFileName << std::endl;
  std::cout << "- output path       : " << outFileBase << " + ..." << std::endl;
  std::cout << "- num parts (target): " << targetNumParts << std::endl;
  std::cout << "- surface weight    : " << surfaceWeight << std::endl;
  std::cout << "------------------------------------------------------------------" << std::endl;
  std::cout << OWL_TERMINAL_DEFAULT << std::endl;
  
  std::cout << "initialized source, now computing initial stream bounds..." << std::endl;
  
  StatsSink::SP sceneStats = StatsSink::create();
  source->streamTo(sceneStats.get());
  std::cout << OWL_TERMINAL_GREEN
            << "done initial stream stats, got " << std::endl
            << sceneStats->toString()
            << OWL_TERMINAL_DEFAULT << std::endl;
  
  std::priority_queue<std::pair<double,std::shared_ptr<box3f>>> activeParts;
  activeParts.push({sceneStats->count,std::make_shared<box3f>(sceneStats->bounds)});
  
  std::cout << "initialized work queue, getting to work...." << std::endl;
  while (activeParts.size() < targetNumParts) {
    auto biggest = activeParts.top();
    activeParts.pop();
    
    std::cout << "starting split of part with domain = " << *biggest.second << ", numPrims = " << prettyDouble(biggest.first) << std::endl;
    SAHSink thisSplit(*biggest.second, surfaceWeight);
    source->streamTo(&thisSplit);
    
    box3f lBox, rBox;
    double lWeight, rWeight;
    thisSplit.findBestSplit(lBox,lWeight,rBox,rWeight);
    std::cout << " found split into: " << std::endl;
    std::cout << "   left  : " << lBox << " count " << prettyNumber(size_t(lWeight)) << std::endl;
    std::cout << "   right : " << rBox << " count " << prettyNumber(size_t(rWeight)) << std::endl;
    activeParts.push({lWeight,std::make_shared<box3f>(lBox)});
    activeParts.push({rWeight,std::make_shared<box3f>(rBox)});
    
    std::cout << "done one split, now have " << activeParts.size() << " active parts..." << std::endl;
  }

  std::cout << "got all required parts:" << std::endl;

  std::vector<Sink::SP> finalFilters;
  std::ofstream summary(outFileBase+".summary");
  int numParts = activeParts.size();
  summary.write((char *)&numParts,sizeof(numParts));
  for (int partID=0;!activeParts.empty();partID++) {
    auto next = activeParts.top(); activeParts.pop();
    const box3f domain = *next.second;
    std::cout << " got final part #" << partID << " : domain = " << domain << " numTris = " << prettyNumber((size_t)next.first) << std::endl;
    
    summary.write((char*)&domain,sizeof(domain));
    std::string partStreamName = outFileBase+"_part"+std::to_string(partID)+".bts";
    std::cout << " ... -> writing to "
              << OWL_TERMINAL_BLUE
              << partStreamName
              << OWL_TERMINAL_DEFAULT << std::endl;
    FileSink::SP fileSink = FileSink::create(partStreamName);
    finalFilters.push_back(FilterSink::create(fileSink,domain));
  }
  ForkSink::SP finalFork = ForkSink::create(finalFilters);
  std::cout << "done set-up of final stream-out, now doing that ..." << std::endl;
  source->streamTo(finalFork.get());

  if (miniScene) {
    std::string metaFileName = outFileBase+"_meta.mini";
    std::cout << ".... and also saving meta (to "
              << OWL_TERMINAL_BLUE << metaFileName
              << OWL_TERMINAL_DEFAULT << ")" << std::endl;
    miniScene->saveMeta(metaFileName);
  }
  std::cout << "done ..." << std::endl;
  return 0;
}
