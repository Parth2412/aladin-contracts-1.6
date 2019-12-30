add_contract(transfer_notifier transfer_notifier ${CMAKE_CURRENT_SOURCE_DIR}/src/transfer_notifier.cpp)

target_include_directories(transfer_notifier
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(transfer_notifier
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( transfer_notifier PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
