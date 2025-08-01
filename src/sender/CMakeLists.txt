# Sender executable
add_executable(sender
    main.cpp
)

# Link dependencies
target_link_libraries(sender
    PRIVATE 
        Boost::system
        Threads::Threads
)
