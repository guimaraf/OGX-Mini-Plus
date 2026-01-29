function(get_pico_sdk EXTERNAL_DIR VERSION_TAG)
    set(LOCAL_SDK_PATH "${EXTERNAL_DIR}/pico-sdk")
    
    # Check if local SDK exists first
    if(EXISTS "${LOCAL_SDK_PATH}/pico_sdk_init.cmake")
        message(STATUS "Using local pico-sdk: ${LOCAL_SDK_PATH}")
        set(PICO_SDK_PATH "${LOCAL_SDK_PATH}" PARENT_SCOPE)
    elseif(DEFINED ENV{PICO_SDK_PATH} AND EXISTS "$ENV{PICO_SDK_PATH}/pico_sdk_init.cmake")
        message(STATUS "Using PICO_SDK_PATH from environment: $ENV{PICO_SDK_PATH}")
        set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}" PARENT_SCOPE)
    else()
        message(STATUS "Cloning pico-sdk to ${LOCAL_SDK_PATH}")
        execute_process(
            COMMAND git clone --recursive --branch patched-2.1.1 https://github.com/guimaraf/pico-sdk.git pico-sdk
            WORKING_DIRECTORY ${EXTERNAL_DIR}
        )

        execute_process(
            COMMAND git fetch --tags
            WORKING_DIRECTORY ${LOCAL_SDK_PATH}
        )
    
        execute_process(
            COMMAND git checkout tags/${VERSION_TAG}
            WORKING_DIRECTORY ${LOCAL_SDK_PATH}
        )
    
        execute_process(
            COMMAND git submodule update --init --recursive 
            WORKING_DIRECTORY ${LOCAL_SDK_PATH}
        )

        set(PICO_SDK_PATH "${LOCAL_SDK_PATH}" PARENT_SCOPE)
    endif()

    set(PICOTOOL_FETCH_FROM_GIT_PATH ${EXTERNAL_DIR}/picotool PARENT_SCOPE)
endfunction()