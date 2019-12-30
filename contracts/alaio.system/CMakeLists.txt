add_contract(alaio.system alaio.system ${CMAKE_CURRENT_SOURCE_DIR}/src/alaio.system.cpp)

target_include_directories(alaio.system
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../alaio.token/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../dapp_registry/include
)

set_target_properties(alaio.system
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

add_contract(rex.results rex.results ${CMAKE_CURRENT_SOURCE_DIR}/src/rex.results.cpp)

target_include_directories(rex.results
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(rex.results
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.rex")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/alaio.system.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/alaio.system.contracts.md @ONLY )

target_compile_options( alaio.system PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
