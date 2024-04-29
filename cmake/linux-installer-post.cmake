

file(SIZE ${CPACK_PACKAGE_FILES} FILESIZE)
math(EXPR FILESIZE "${FILESIZE}+1")

message(STATUS ${FILESIZE})

execute_process(COMMAND
    sed -i "s/cpack_include_subdir=\"\"/cpack_include_subdir=FALSE/" ${CPACK_PACKAGE_FILES}
)
execute_process(COMMAND
    sed -i "s/toplevel=\"`pwd`\"/toplevel=\${HOME}/" ${CPACK_PACKAGE_FILES}
)
execute_process(COMMAND
    sed -i "s/\\${toplevel}\/NDI Audio IO.*Linux/\\${toplevel}\//" ${CPACK_PACKAGE_FILES}
)
execute_process(COMMAND
    sed -i "s/\"x\${cpack_include_subdir}x\" != \"xx\" -o//" ${CPACK_PACKAGE_FILES}
)
