cmake_minimum_required(VERSION 3.16)
project(IntelliEditor C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- Flags de compilation ---
add_compile_options(-Wall -Wextra -Wpedantic)

# --- Includes partagés ---
include_directories(include)

# --- llama.cpp ---
add_subdirectory(llama.cpp)

# --- Hunspell ---
find_library(HUNSPELL_LIB hunspell-1.7 hunspell REQUIRED)
find_path(HUNSPELL_INCLUDE hunspell/hunspell.h REQUIRED)
include_directories(${HUNSPELL_INCLUDE})

# --- Threads ---
find_package(Threads REQUIRED)

# --- Module DEV-C : LLM ---
add_library(llm_module STATIC
    src/llm/llm_interface.c
    src/llm/llm_thread.c
    src/llm/prompts.c
)
target_link_libraries(llm_module
    llama
    Threads::Threads
)

# --- Module DEV-C : NLP ---
add_library(nlp_module STATIC
    src/nlp/nlp_engine.c
    src/nlp/hunspell_wrap.c
    src/nlp/tokenizer.c
)
target_link_libraries(nlp_module
    llm_module
    ${HUNSPELL_LIB}
)

# --- Executable de test DEV-C ---
add_executable(test_nlp tests/test_nlp.c)
target_link_libraries(test_nlp
    nlp_module
    llm_module
)
