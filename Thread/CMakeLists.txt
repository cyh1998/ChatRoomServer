aux_source_directory(. THREAD_DIR_SRCS)

find_package(Threads)

add_library(Thread ${THREAD_DIR_SRCS})

target_link_libraries (Thread ${CMAKE_THREAD_LIBS_INIT})
