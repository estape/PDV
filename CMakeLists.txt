cmake_minimum_required(VERSION 3.20)
project(ZanthusReplicaPDV C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Inclui diretórios essenciais
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite3
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/httplib
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/photino/Photino.Native
)

# ----------------------------------------------------------------------------
# 1. CORE DE NEGÓCIO ZANTHUS EM C PURO + SQLITE3
# ----------------------------------------------------------------------------
set(ZAN_CORE_SOURCES
    src/core/zan_barcode.c
    src/core/zan_pricing.c
    src/core/zan_records.c
    src/core/zan_fiscal.c
    src/core/zan_tef.c
    src/core/zan_db.c
    src/core/zan_engine.c
    third_party/sqlite3/sqlite3.c
)

add_library(pdv_core STATIC ${ZAN_CORE_SOURCES})
target_compile_definitions(pdv_core PRIVATE SQLITE_THREADSAFE=1 SQLITE_ENABLE_FTS5=0)

if(UNIX)
    target_link_libraries(pdv_core PRIVATE pthread dl)
endif()

# ----------------------------------------------------------------------------
# 2. TESTE AUTOMATIZADO DO CORE (test_core)
# ----------------------------------------------------------------------------
add_executable(test_core src/core/test_core.c)
target_link_libraries(test_core PRIVATE pdv_core)

# ----------------------------------------------------------------------------
# 3. APLICAÇÃO DESKTOP COM PHOTINO.NATIVE (pdv_app)
# ----------------------------------------------------------------------------
if(WIN32)
    add_compile_definitions(
        _WIN32
        UNICODE
        _UNICODE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )

    set(WEBVIEW2_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/packages/Microsoft.Web.WebView2/build/native)
    set(WIL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/packages/Microsoft.Windows.ImplementationLibrary)

    include_directories(
        ${WEBVIEW2_ROOT}/include
        ${WIL_ROOT}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/photino/Photino.Native/Dependencies
    )

    set(PHOTINO_WIN_SOURCES
        third_party/photino/Photino.Native/Photino.Windows.cpp
        third_party/photino/Photino.Native/Photino.Windows.DarkMode.cpp
        third_party/photino/Photino.Native/Photino.Windows.Dialog.cpp
        third_party/photino/Photino.Native/Dependencies/wintoastlib.cpp
        third_party/photino/Photino.Native/Exports.cpp
    )

    add_executable(pdv_app
        src/main.cpp
        ${PHOTINO_WIN_SOURCES}
    )

    # Detecta arquitetura do build
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCH_DIR x64)
    else()
        set(ARCH_DIR x86)
    endif()

    target_link_libraries(pdv_app PRIVATE
        pdv_core
        ${WEBVIEW2_ROOT}/${ARCH_DIR}/WebView2Loader.dll.lib
        shlwapi
        version
        ole32
        shell32
        user32
        gdi32
        advapi32
        ws2_32
    )

    # Copia a DLL do WebView2Loader para a pasta de saída do executável
    add_custom_command(TARGET pdv_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${WEBVIEW2_ROOT}/${ARCH_DIR}/WebView2Loader.dll"
            "$<TARGET_FILE_DIR:pdv_app>/WebView2Loader.dll"
        COMMENT "Copiando WebView2Loader.dll para pasta de execucao..."
    )

elseif(UNIX AND NOT APPLE)
    add_compile_definitions(__linux__ OS_LINUX)

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.1)
    if(NOT WEBKIT_FOUND)
        pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.0)
    endif()
    pkg_check_modules(LIBNOTIFY REQUIRED libnotify)

    include_directories(
        ${GTK3_INCLUDE_DIRS}
        ${WEBKIT_INCLUDE_DIRS}
        ${LIBNOTIFY_INCLUDE_DIRS}
    )

    set(PHOTINO_LINUX_SOURCES
        third_party/photino/Photino.Native/Photino.Linux.cpp
        third_party/photino/Photino.Native/Photino.Linux.Dialog.cpp
        third_party/photino/Photino.Native/Exports.cpp
    )

    add_executable(pdv_app
        src/main.cpp
        ${PHOTINO_LINUX_SOURCES}
    )

    target_link_libraries(pdv_app PRIVATE
        pdv_core
        ${GTK3_LIBRARIES}
        ${WEBKIT_LIBRARIES}
        ${LIBNOTIFY_LIBRARIES}
        pthread
        dl
    )
endif()

# Copia pasta de assets da UI multiplataforma
add_custom_command(TARGET pdv_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/ui"
        "$<TARGET_FILE_DIR:pdv_app>/ui"
    COMMENT "Copiando pasta ui/ para o diretorio do binario..."
)
