cmake_minimum_required(VERSION 3.10)
project(IntelliEditor C)

# Définir le standard C
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Inclure les dossiers d'en-têtes
include_directories(include)

# Ajouter les sous-dossiers source
add_subdirectory(src)

# Créer l'exécutable principal
add_executable(intellieditor src/ui/main_window.c)

# Lier les bibliothèques (à ajouter plus tard)
# target_link_libraries(intellieditor llm nlp rules editor utils)