cmake_minimum_required(VERSION 3.10)
project(motis)

file(GLOB_RECURSE motis-ris-files src/*.cc)
add_library(motis-ris STATIC ${motis-ris-files})
target_include_directories(motis-ris PUBLIC include)
target_compile_features(motis-ris PUBLIC cxx_std_17)
target_compile_definitions(motis-ris PUBLIC PROTOBUF_LINKED=1)
target_link_libraries(motis-ris
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_FILESYSTEM}
  protobuf
  motis-module
  motis-core
  miniz
  pugixml
  lmdb
  tar
  motis-ris-gtfsrtpb
)
target_compile_options(motis-ris PRIVATE ${MOTIS_CXX_FLAGS})
