add_library(lora_lib INTERFACE)

target_sources(lora_lib INTERFACE ${CMAKE_CURRENT_LIST_DIR}/src/sx127X_driver.c)

target_include_directories(lora_lib INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(lora_lib INTERFACE pico_stdlib hardware_spi)