add_library(physfs++
			${LIB_TYPE}
            physfs.cpp)

include_directories("../include"
                    ${PHYSFS_INCLUDE_DIR})

target_link_libraries(physfs++
                      ${PHYSFS_LIBRARY})

install(TARGETS physfs++
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib${LIB_SUFFIX}
        ARCHIVE DESTINATION lib${LIB_SUFFIX})