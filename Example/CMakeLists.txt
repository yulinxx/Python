add_executable(SyPythonHostExample
    main.cpp
)

target_compile_features(SyPythonHostExample PRIVATE cxx_std_17)

target_link_libraries(SyPythonHostExample PRIVATE
    PythonHost
)

if(MSVC)
    target_compile_options(SyPythonHostExample PRIVATE /utf-8)
endif()

set_target_properties(SyPythonHostExample PROPERTIES FOLDER "Examples")

message(STATUS "  Example: SyPythonHostExample")
