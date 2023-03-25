

file(GLOB JSONCPP_CXX_C_FILES jsoncpp/include/json/*.h jsoncpp/src/lib_json/*.cpp jsoncpp/src/lib_json/*.h)
add_library(jsoncpp ${JSONCPP_CXX_C_FILES})
target_include_directories(jsoncpp
PUBLIC jsoncpp/include/
)
set_property(TARGET jsoncpp PROPERTY C_STANDARD 11)
set_property(TARGET jsoncpp PROPERTY CXX_STANDARD 20)

