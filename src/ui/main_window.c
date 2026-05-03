#include <windows.h>

typedef struct {
    HWND hMainWnd;       
    HWND hScintilla;         
    HWND hToolbar;           
    HWND hStatusBar;         
    HWND hRulesPanel;        
    int currentTab;          

} AppState;