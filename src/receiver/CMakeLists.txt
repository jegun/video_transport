# Receiver executable
add_executable(receiver
    main.cpp
)

target_link_libraries(receiver
    PRIVATE 
        Boost::system
        Threads::Threads
)