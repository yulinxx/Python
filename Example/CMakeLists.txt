add_executable(AprPythonHostExample
    main.cpp
)

target_compile_features(AprPythonHostExample PRIVATE cxx_std_17)

target_link_libraries(AprPythonHostExample PRIVATE
    PythonHost
)

if(MSVC)
    target_compile_options(AprPythonHostExample PRIVATE /utf-8)
endif()

set_target_properties(AprPythonHostExample PROPERTIES FOLDER "Examples")

message(STATUS "  Example: AprPythonHostExample")
