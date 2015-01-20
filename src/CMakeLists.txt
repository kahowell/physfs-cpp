add_library(physfs++
			${LIB_TYPE}
            physfs.cpp)

include_directories("../include"
                    ${PHYSFS_INCLUDE_DIR})

target_link_libraries(physfs++
                      ${PHYSFS_LIBRARY})