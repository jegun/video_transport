# Core library
add_library(core
    DataFile.cpp
    DataUnitConverter.cpp
    DataProvider.cpp
    DataAcceptor.cpp
    AsioSender.cpp
    AsioReceiver.cpp
    TimestampWriter.cpp
)

target_include_directories(core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(core
    PUBLIC
        cxx_std_17
)

target_link_libraries(core
    PUBLIC
        Boost::system
) 