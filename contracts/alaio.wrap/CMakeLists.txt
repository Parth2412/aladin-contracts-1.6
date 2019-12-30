add_contract(alaio.wrap alaio.wrap ${CMAKE_CURRENT_SOURCE_DIR}/src/alaio.wrap.cpp)

target_include_directories(alaio.wrap
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(alaio.wrap
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/alaio.wrap.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/alaio.wrap.contracts.md @ONLY )

target_compile_options( alaio.wrap PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
