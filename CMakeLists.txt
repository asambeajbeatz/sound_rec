cmake_minimum_required(VERSION 3.16)
project(sound_rec LANGUAGES C CXX)

# Явно указываем стандарт C++ 
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Включает статическую линковку CRT (/MT и /MTd), 
# чтобы EXE не зависел от MSVCP140.dll и работал на Windows XP. 
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")


# Макросы для совместимости с Windows XP и Unicode
# WINVER и _WIN32_WINNT в hex для 5.01 
add_compile_definitions(
	WINVER=0x0501 
    _WIN32_WINNT=0x0501 
    UNICODE 
    _UNICODE
)


# Автоматически собираем все .cpp, .h, .rc (добавил res/ .ico в поиск)
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS 
    "*.cpp" 
    "*.h" 
    "*.rc"
    "res/*.ico"
)


# Создаём GUI приложение WIN32 чтобы подсистема была Windows
add_executable(sound_rec WIN32 ${SRC})


# Группировка файлов в папки для Visual Studio (Filters)
source_group("Generated Files" REGULAR_EXPRESSION "cmake_pch.*")

source_group("CMake" FILES "CMakeLists.txt")

source_group("Resource Files" REGULAR_EXPRESSION ".*\\.rc$|.*\\.ico$")

source_group("Source Files" REGULAR_EXPRESSION ".*\\.cpp$")
source_group("Header Files" REGULAR_EXPRESSION ".*\\.h$")

# Получаем список ВСЕХ файлов, которые CMake считает частью проекта
get_target_property(ALL_FILES ${PROJECT_NAME} SOURCES)

foreach(FILE_PATH ${ALL_FILES})
    # Если в пути файла есть папка CMakeFiles то меняем его свойства
    if(FILE_PATH MATCHES "CMakeFiles")
        set_source_files_properties(${FILE_PATH} PROPERTIES 
            HEADER_FILE_ONLY TRUE
        )
    endif()
endforeach()


# Добавляем пути поиска заголовочных файлов, если нужно
# Это позволяет компилятору находить файлы, подключенные через #include
target_include_directories(sound_rec PRIVATE 
	${CMAKE_SOURCE_DIR} 
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

	
# Линкуем системные библиотеки 
target_link_libraries(sound_rec PRIVATE 
	opengl32 
	glu32 
	user32 
	gdi32 
	comdlg32 
	winmm 
)


# Для Visual Studio - устанавливаем флаги линкера: 
if(MSVC) 
	target_link_options(sound_rec PRIVATE 
        "/LARGEADDRESSAWARE"      # Доступ к 3-4 ГБ RAM для Win32 приложения
        "/SUBSYSTEM:WINDOWS,5.01" # Жесткая привязка к XP
    )
endif()


# Опционально: включить предкомпилированный заголовок если используешь stdafx.h 
# Требует CMake 3.16+ 
if (EXISTS "${CMAKE_SOURCE_DIR}/stdafx.h") 
    target_precompile_headers(sound_rec PRIVATE "stdafx.h")
endif()


# Установить sound_rec как проект по умолчанию в Visual Studio 
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT sound_rec)

