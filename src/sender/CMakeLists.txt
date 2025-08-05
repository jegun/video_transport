# Sender executable
add_executable(sender main.cpp)

# Include directories
target_include_directories(sender
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(sender
    PRIVATE 
        core
)
