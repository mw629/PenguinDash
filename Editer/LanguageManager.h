#pragma once
#include <string>

enum class EditorLanguage {
    Japanese,
    English
};

class LanguageManager {
private:
    static EditorLanguage currentLanguage_;

public:
    static void SetLanguage(EditorLanguage lang) { currentLanguage_ = lang; }
    static EditorLanguage GetLanguage() { return currentLanguage_; }

    // Returns the translated string if language is Japanese, otherwise returns the English key
    static const char* Tr(const char* englishText);
};
