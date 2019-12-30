# dapp registry contract
add_contract(dapp_registry dapp_registry ${CMAKE_CURRENT_SOURCE_DIR}/src/dapp_registry.cpp)

target_include_directories(dapp_registry
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../alaio.token/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../alaio.system/include
)

set_target_properties(dapp_registry
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( dapp_registry PUBLIC )

# dummy app contract
add_contract(dummy_app dummy_app ${CMAKE_CURRENT_SOURCE_DIR}/src/dummy_app.cpp)

target_include_directories(dummy_app
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(dummy_app
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( dummy_app PUBLIC )
