check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(examples_as_tests_sources)
if(${ENABLE_EXAMPLES})
    set(examples_as_tests_sources
        #test/new-wifi-examples-test-suite.cc
        )
endif()

build_lib(
    LIBNAME new-wifi
    SOURCE_FILES model/new-wifi.cc
                 helper/new-wifi-helper.cc
    HEADER_FILES model/new-wifi.h
                 helper/new-wifi-helper.h
    LIBRARIES_TO_LINK ${libcore}
    TEST_SOURCES test/new-wifi-test-suite.cc
                 ${examples_as_tests_sources}
)
