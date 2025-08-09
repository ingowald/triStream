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
#include "triStream/MeshBuilderSink.h"
#include "brix/scene/PartialScene.h"
#include "brix/scene/MasterScene.h"

#define FORCE_INSTANCE_PER_META 1

namespace triStream {

  using brix::scene::PartialScene;
  using brix::scene::MasterScene;
  
  void usage(const std::string &error)
  {
    if (!error.empty())
      std::cerr << "Error: " << error << "\n\n";
    std::cout << "Usage: ./btsMakePartialScene inFilePathNoExt -m metaScene.mini -o outBasePath\n";
    exit(!error.empty());
  }


  struct Summary {
    Summary(const std::string &inFileBase,
            const std::string &metaFileName)
      : inFileBase(inFileBase)
    {
      const std::string summaryFileName = inFileBase+".summary";
      std::cout << OWL_TERMINAL_BLUE
                << "trying to read summary from " << summaryFileName
                << OWL_TERMINAL_DEFAULT << std::endl;
      std::ifstream summary(summaryFileName,std::ios::binary);
      summary.read((char*)&numParts,sizeof(numParts));
      partBoxes.resize(numParts);
      summary.read((char*)partBoxes.data(),partBoxes.size()*sizeof(partBoxes[0]));
      if (!summary.good())
        throw std::runtime_error("error in reading summary from "+summaryFileName);

      std::cout << OWL_TERMINAL_GREEN
                << "read summary, found " << numParts << " parts"
                << OWL_TERMINAL_DEFAULT << std::endl;
      if (metaFileName.empty()) {
        std::cout << "creating single-material dummy meta ...  " << std::endl;
        meta = createDummyMeta();
      } else {
        std::cout << "reading meta from " << metaFileName << std::endl;
        meta = mini::Scene::load(metaFileName);
      }
      std::cout << OWL_TERMINAL_GREEN
                << "read summary, found " << numParts << " parts"
                << OWL_TERMINAL_DEFAULT << std::endl;
    }
    std::string getStreamName(int partID)
    { return inFileBase+"_part"+std::to_string(partID)+".bts"; }

    static Scene::SP createDummyMeta()
    {
      Material::SP metaMat = Material::create();
      Mesh::SP metaMesh = Mesh::create(metaMat);
      Object::SP metaObj = Object::create({metaMesh});
      Scene::SP meta = Scene::create();
      meta->instances.push_back(Instance::create(metaObj));
      return meta;
    }
    
    int numParts = -1;
    std::vector<box3f> partBoxes;
    std::vector<int>   meshesOnPart;
    mini::Scene::SP    meta;
    const std::string inFileBase;

    void countMeshesPerPart()
    {
      meshesOnPart.resize(numParts);
      for (int partID=0;partID<numParts;partID++) {
        std::string inFileName = getStreamName(partID);
        FileSource::SP in = FileSource::create(inFileName);
        MetaMeshBuilderSink::SP sink = MetaMeshBuilderSink::create();
        std::cout << "... going to read (used) IGs from stream " << inFileName << std::endl;
        in->streamTo(sink.get());
        int numMeshesThisPart = sink->meshByMeta.size();
        std::cout << "    -> found " << numMeshesThisPart << " meshes in this stream..." << std::endl;
        meshesOnPart[partID] = numMeshesThisPart;
      }
    }
  };

  Material::SP getMaterialByMeta(Scene::SP metaScene, uint64_t matID)
  {
    if (matID < 0)
      throw std::runtime_error("negative material/IGID!?");

    if (metaScene->instances.size() == 1 && metaScene->instances[0]->object->meshes.size() == 1)
      return metaScene->instances[0]->object->meshes[0]->material;
    
    uint64_t remaining = matID;
    for (int instID=0;instID<metaScene->instances.size();instID++) {
      auto inst = metaScene->instances[instID];
      assert(inst);
      auto obj = inst->object;
      assert(obj);
      if (obj->meshes.size() <= remaining) {
        remaining -= obj->meshes.size();
        continue;
      }
      Mesh::SP mesh = obj->meshes[remaining];
      assert(mesh);
      assert(mesh->material);
      return mesh->material;
    }
    throw std::runtime_error("did not find this material/IGID!?");
  }

  int makeTextureID(PartialScene::SP scene,
                    std::map<Texture::SP,int> &knownTextures,
                    Texture::SP texture)
  {
    if (!texture) return -1;
    if (knownTextures.find(texture) == knownTextures.end()) {
      knownTextures[texture] = scene->textures.size();
      scene->textures.push_back(texture);
    }
    return knownTextures[texture];
  }
  
  int makePart(Summary &summary,
               int partID,
               const std::string &outFileBase,
               int numMeshesInPreviousParts)
  {
#if FORCE_INSTANCE_PER_META
    static int numUsedObjects = 0;
    int firstObjectThisPart = numUsedObjects;
#endif
    std::cout << OWL_TERMINAL_BLUE;
    std::string inFileName = summary.getStreamName(partID);
    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << "making part " << partID << " from stream " << inFileName << std::endl;
    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << OWL_TERMINAL_DEFAULT;

    FileSource::SP in = FileSource::create(inFileName);
    MetaMeshBuilderSink::SP sink = MetaMeshBuilderSink::create();
    in->streamTo(sink.get());

    std::cout << "done reading stream, found " << sink->meshByMeta.size()
              << " different IGs" << std::endl;

    std::map<Texture::SP,int> knownTextures;
    PartialScene::SP out = PartialScene::create();
    out->meshes.reserve(sink->meshByMeta.size());
    for (auto it : sink->meshByMeta) {
      int localMeshID = out->meshes.size();
      uint64_t meta = it.first;
      std::cout << " - mesh with meta " << meta << std::endl;

      PartialScene::Mesh outMesh;
      // ------------------------------------------------------------------
      // making vertex array
      // ------------------------------------------------------------------
      MeshBuilderSink::SP inMesh = it.second;
      outMesh.vertices.resize(inMesh->vertexMap.size());
      std::cout << "    - building " << prettyNumber(outMesh.vertices.size())
                << " vertices" << std::endl;
      for (auto it : inMesh->vertexMap) {
        auto vtx = it.first;
        const vec3f v  = std::get<0>(vtx);
        const vec3f n  = std::get<1>(vtx);
        const vec2f uv = std::get<2>(vtx);
        const int vtxID = it.second;

        outMesh.vertices[vtxID] = v;
      
        if (n != vec3f(0.f)) {
          if (outMesh.normals.empty())
            outMesh.normals.resize(outMesh.vertices.size());
          outMesh.normals[vtxID] = n;
        }
      
        if (uv != vec2f(INFINITY)) {
          if (outMesh.texcoords.empty())
            outMesh.texcoords.resize(outMesh.vertices.size());
          outMesh.texcoords[vtxID] = uv;
        }
      }
      
      // ------------------------------------------------------------------
      // making index array
      // ------------------------------------------------------------------
      outMesh.indices = inMesh->indices;
      std::cout << "    - building " << prettyNumber(outMesh.indices.size())
                << " triangles" << std::endl;
      
      // ------------------------------------------------------------------
      // making mesh material and other stuff
      // ------------------------------------------------------------------
      Material::SP inMaterial = getMaterialByMeta(summary.meta,meta);
      outMesh.material.set(*inMaterial);
      outMesh.colorTextureID = makeTextureID(out,knownTextures,inMaterial->colorTexture);
      outMesh.alphaTextureID = makeTextureID(out,knownTextures,inMaterial->alphaTexture);

      outMesh.globalSerializedMeshID = numMeshesInPreviousParts + out->meshes.size();

#if FORCE_INSTANCE_PER_META
      outMesh.globalSubMeshID        = 0;
      
      // ------------------------------------------------------------------
      // making one object for this mesh
      // ------------------------------------------------------------------
      std::cout << " - building partial-scene object" << std::endl;
      PartialScene::Object object;
      object.globalObjID = numUsedObjects++;
      object.firstLocalMeshID = localMeshID;
      object.numLocalMeshes   = 1;
      
      out->objects.push_back(object);
      
      // ------------------------------------------------------------------
      // making (single) instance that has single object
      // ------------------------------------------------------------------
      std::cout << " - building partial-scene instance" << std::endl;
      PartialScene::Instance instance;
      instance.xfm          = affine3f();
      instance.localObjID   = object.globalObjID - firstObjectThisPart;
      instance.globalObjID  = object.globalObjID;
      instance.globalInstID = object.globalObjID;
      instance.baseIGID     = object.globalObjID;
      out->instances.push_back(instance);
#else
      outMesh.globalSubMeshID        = out->meshes.size();
#endif 
      out->meshes.push_back(outMesh);
   }

#if FORCE_INSTANCE_PER_META
#else
    // ------------------------------------------------------------------
    // making (single) object that has all parts
    // ------------------------------------------------------------------
    std::cout << " - building partial-scene object" << std::endl;
    PartialScene::Object object;
    object.globalObjID = partID;
    object.firstLocalMeshID = 0;
    object.numLocalMeshes = out->meshes.size();

    out->objects.push_back(object);
    
    // ------------------------------------------------------------------
    // making (single) instance that has single object
    // ------------------------------------------------------------------
    std::cout << " - building partial-scene instance" << std::endl;
    PartialScene::Instance instance;
    instance.xfm          = affine3f();
    instance.localObjID   = 0;
    instance.globalObjID  = partID;
    instance.globalInstID = partID;
    instance.baseIGID     = numMeshesInPreviousParts;
    out->instances.push_back(instance);
#endif
    // ------------------------------------------------------------------
    // storing textures
    // ------------------------------------------------------------------
    std::cout << " - building partial-scene textures" << std::endl;
    out->textures.resize(knownTextures.size());
    for (auto it : knownTextures)
      out->textures[it.second] = it.first;

    // ------------------------------------------------------------------
    // storing IGs
    // ------------------------------------------------------------------
    std::cout << " - building IG-Specs" << std::endl;
#if FORCE_INSTANCE_PER_META
    for (int pID=0;pID<summary.numParts;pID++) {
      for (int lmID=0;lmID<summary.meshesOnPart[pID];lmID++) {
        PartialScene::IGSpec igSpec;
        igSpec.ownedOn = brix::scene::NodeMask::singleRank(pID);
        if (pID == partID) {
          igSpec.localMeshID = lmID;
          igSpec.localInstID = lmID;
        } else {
          igSpec.localMeshID = -1;
          igSpec.localInstID = -1;
        }
        out->igSpecs.push_back(igSpec);
      }
    }
#else
    for (int pID=0;pID<summary.numParts;pID++) {
      for (int lmID=0;lmID<summary.meshesOnPart[pID];lmID++) {
        PartialScene::IGSpec igSpec;
        igSpec.ownedOn = brix::scene::NodeMask::singleRank(pID);
        if (pID == partID) {
          igSpec.localMeshID = lmID;
          igSpec.localInstID = 0;
        } else {
          igSpec.localMeshID = -1;
          igSpec.localInstID = -1;
        }
        out->igSpecs.push_back(igSpec);
      }
    }
#endif
    
    // ------------------------------------------------------------------
    // building proxies
    // ------------------------------------------------------------------
    std::cout << " - building proxies" << std::endl;
    for (int pID=0;pID<summary.numParts;pID++) {
      brix::scene::Proxy proxy;
      proxy.bounds = summary.partBoxes[pID];
      proxy.ownedBy = brix::scene::NodeMask::singleRank(pID);
      out->allProxies.push_back(proxy);
      // if (pID == partID) {
      //   out->localProxies.push_back(proxy);
      // } else {
      //   out->remoteProxies.push_back(proxy);
      // }
    }
    
    // ------------------------------------------------------------------
    // done - saving
    // ------------------------------------------------------------------
    const std::string outFileName = outFileBase+"_part"+std::to_string(partID)+".mini";
    std::cout << "done full partialscene, saving it..." << std::endl;
    out->saveRank(outFileBase,partID);

    std::cout << OWL_TERMINAL_GREEN
              << "partial scene successfully saved"
              << OWL_TERMINAL_DEFAULT << std::endl;
    return out->meshes.size();
  }
  
  
} // ::triStream

using namespace triStream;
using mini::common::endsWith;
  
int main(int ac, char **av)
{
  std::string inFileBase;
  std::string metaFileName;
  std::string outFileBase;
  bool createDummyMaterial = false;
  for (int i=1;i<ac;i++) {
    const std::string arg = av[i];
    if (arg[0] != '-')
      inFileBase = arg;
    else if (arg == "-o")
      outFileBase = av[++i];
    else if (arg == "-m")
      metaFileName = av[++i];
    else if (arg == "--create-material")
      createDummyMaterial = true;
    else
      usage("unknown cmdline arg '"+arg+"'");
  }
  if (inFileBase.empty())
    usage("no input file path specified");
  if (metaFileName.empty() && !createDummyMaterial)
    usage("no input meta file specified");
  if (outFileBase.empty())
    usage("no output file path specified (-o)");

  Summary summary(inFileBase,
                  metaFileName);

  // num meshes written across all parts; required for each part to
  // know its IGIDs in reconstitured scene (these GIIDs will *NOT*
  // match those of the original scene before partitioning, but *WILL*
  // be consistent
  int numMeshesWrittenYet = 0;
  std::cout << OWL_TERMINAL_LIGHT_BLUE
            << "##################################################################" << std::endl
            << "PASS NUMBER ONE : counting meshes per part" << std::endl
            << "##################################################################"
            << OWL_TERMINAL_DEFAULT << std::endl;
  summary.countMeshesPerPart();
  
  std::cout << OWL_TERMINAL_LIGHT_BLUE
            << "##################################################################" << std::endl
            << "PASS NUMBER TWO : building actual parts" << std::endl
            << "##################################################################"
            << OWL_TERMINAL_DEFAULT << std::endl;
  for (int partID=0;partID<summary.numParts;partID++)
    numMeshesWrittenYet += makePart(summary,partID,outFileBase,numMeshesWrittenYet);
  
  std::cout << OWL_TERMINAL_LIGHT_BLUE
            << "##################################################################" << std::endl
            << "almost done ... creating master scene" << std::endl
            << "##################################################################"
            << OWL_TERMINAL_DEFAULT << std::endl;
  MasterScene::SP masterScene = MasterScene::create();
  masterScene->numParts = summary.numParts;
  for (auto box: summary.partBoxes)
    masterScene->bounds.extend(box);
  masterScene->quadLights = summary.meta->quadLights;
  masterScene->dirLights  = summary.meta->dirLights;
  masterScene->envMapLight = summary.meta->envMapLight;
  masterScene->save(outFileBase+"_master.summ");

  std::cout << OWL_TERMINAL_GREEN
            << "aaaand ..... all done!"
            << OWL_TERMINAL_DEFAULT << std::endl;
  return 0;
}
