add_contract(alaio.bios alaio.bios ${CMAKE_CURRENT_SOURCE_DIR}/src/alaio.bios.cpp)

target_include_directories(alaio.bios
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(alaio.bios
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/alaio.bios.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/alaio.bios.contracts.md @ONLY )

target_compile_options( alaio.bios PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
