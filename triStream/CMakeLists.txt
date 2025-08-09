# ======================================================================== #
# Copyright 2018-2020 Ingo Wald                                            #
#                                                                          #
# Licensed under the Apache License, Version 2.0 (the "License");          #
# you may not use this file except in compliance with the License.         #
# You may obtain a copy of the License at                                  #
#                                                                          #
#     http://www.apache.org/licenses/LICENSE-2.0                           #
#                                                                          #
# Unless required by applicable law or agreed to in writing, software      #
# distributed under the License is distributed on an "AS IS" BASIS,        #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. #
# See the License for the specific language governing permissions and      #
# limitations under the License.                                           #
# ======================================================================== #

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# core tri-stream library
add_library(bts STATIC
  TriStream.h
  TriStream.cpp
  Filter.h
  Filter.cpp
#  PBRTSceneSource.h
#  PBRTSceneSource.cpp
  MiniSceneSource.h
  MiniSceneSource.cpp

  FileSource.h
  FileSource.cpp
  FileSink.h
  FileSink.cpp

  MeshBuilderSink.h
  MeshBuilderSink.cpp

  Fork.h
  Fork.cpp

  Stats.h
  Stats.cpp
  
  GridSplitter.h
  GridSplitter.cpp
  )
target_include_directories(bts PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(bts PUBLIC
  ${PROJECT_SOURCE_DIR}/submodules/minilzo/include/lzo
  ${PROJECT_SOURCE_DIR}/submodules/minilzo/include/
  ${PROJECT_SOURCE_DIR}/submodules/minilzo/minilzo
)
target_link_libraries(bts
  miniScene
  lzo_static_lib
  owl
  tbb
  )

#add_executable(pbf2bts
#  pbf2bts.cpp
#  )
#target_link_libraries(pbf2bts
#  bts
#  )
add_executable(mini2bts
  mini2bts.cpp
  )
target_link_libraries(mini2bts
  bts
  )

# ==================================================================
# given a set of partitioned streams (the actual per-rank triangles),
# a meta 9for the materials), and a summary (to know the spatial
# bounds of the streams), this builds a brix::PartialScene for each
# rank, and a MasterScene for the master
# ==================================================================
#add_executable(btsMakePartialScenes
#  makePartialScenes.cpp
#  )
#target_link_libraries(btsMakePartialScenes
#  bts
#  brix_scene
#  )


add_executable(bts2mini
  bts2mini.cpp
  )
target_link_libraries(bts2mini
  bts
  )

# ================================================================== a
# partitioner that uses binned-sah to evaluate where to split,
# repeatedly splitting biggest sutree until certina number of nodes is
# reached
# ==================================================================
add_executable(btsSAHPartitioner
  sahPartitioner.cpp
  )
target_link_libraries(btsSAHPartitioner
  bts
  )
#add_executable(bts2pbf
#  bts2pbf.cpp
#  )
#target_link_libraries(bts2pbf
#  bts
#  )


#add_executable(btsGridder
#  btsGridder.cpp
#  )
#target_link_libraries(btsGridder
#  bts
#  )

# rawtobts: take a raw volume file, extract iso-surface, and put all
# triangles into a stream
#add_executable(raw2bts
#  raw2bts.cpp
#  )
#target_link_libraries(raw2bts
#  bts
#  )


# rawtobts: print tri count and bounding box of bts file
add_executable(btsInfo
  btsInfo.cpp
  )
target_link_libraries(btsInfo
  bts
  )

