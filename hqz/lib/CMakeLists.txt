
# add_subdirectory("rapidjson/")
# get_target_property(libRapidJSON_INCLUDE_DIR RapidJSON INCLUDE_DIRECTORIES)
set(libRapidJSON_INCLUDE_DIR "rapidjson/include")

add_library(libRapidJSON INTERFACE)
target_include_directories(libRapidJSON INTERFACE "${libRapidJSON_INCLUDE_DIR}")
target_compile_definitions(libRapidJSON INTERFACE RAPIDJSON_HAS_STDSTRING)
target_compile_definitions(libRapidJSON INTERFACE RAPIDJSON_ASSERT=\ )
