add_contract(alaio.msig alaio.msig ${CMAKE_CURRENT_SOURCE_DIR}/src/alaio.msig.cpp)

target_include_directories(alaio.msig
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(alaio.msig
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/alaio.msig.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/alaio.msig.contracts.md @ONLY )

target_compile_options( alaio.msig PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
