# -- Project Setup ------------------------------------------------------------

project(novamene)

# -- novamene -------------------------------------------------------------------

set(novamene_sources
  bf/bitvector.cpp
  bf/counter_vector.cpp
  bf/hash.cpp
  bf/bloom_filter/a2.cpp
  bf/bloom_filter/basic.cpp
  bf/bloom_filter/bitwise.cpp
  bf/bloom_filter/counting.cpp
  bf/bloom_filter/stable.cpp
  rapidjson/document.h
  rapidjson/writer.h
  rapidjson/stringbuffer.h
  helpers/primitives.h
  cpp-datetimelite/datetimelite.h
        novamene.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(novamene ${novamene_sources})
