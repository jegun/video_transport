# Receiver executable
add_executable(receiver main.cpp)

target_include_directories(receiver
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(receiver
    PRIVATE 
        core
)