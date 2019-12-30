add_contract(alaio.token alaio.token ${CMAKE_CURRENT_SOURCE_DIR}/src/alaio.token.cpp)

target_include_directories(alaio.token
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(alaio.token
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/alaio.token.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/alaio.token.contracts.md @ONLY )

target_compile_options( alaio.token PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
