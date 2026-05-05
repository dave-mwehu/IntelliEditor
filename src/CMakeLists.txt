# CMakeLists.txt pour src/

# Créer des bibliothèques statiques pour chaque module

# Module LLM
add_library(llm STATIC
    llm/llm_interface.c
    llm/llm_thread.c
    llm/prompts.c
)

# Module NLP
add_library(nlp STATIC
    nlp/nlp_engine.c
    nlp/tokenizer.c
    nlp/hunspell_wrap.c
)

# Module Rules
add_library(rules STATIC
    rules/rule_engine.c
    rules/rule_parser.c
)

# Module Editor
add_library(editor STATIC
    editor/gap_buffer.c
    editor/undo_redo.c
    editor/exporter.c
)

# Module Utils
add_library(utils STATIC
    utils/config.c
    utils/encoding.c
)

# Module UI (dépend des autres)
add_library(ui STATIC
    ui/main_window.c
    ui/rules_panel.c
    ui/scintilla_wrap.c
)

# Dépendances (à ajuster selon les besoins)
target_link_libraries(llm utils)
target_link_libraries(nlp utils)
target_link_libraries(rules utils)
target_link_libraries(editor utils)
target_link_libraries(ui editor llm nlp rules utils)