#ifndef LANGUAGE_MANAGER_H
#define LANGUAGE_MANAGER_H

#include <Arduino.h>
#include <SD.h>
#include <map>
#include "config.h"

struct LangItem {
  String filename; // e.g. "english.lng" or "/languages/english.lng"
  String name;     // e.g. "English"
};

class LanguageManager {
public:
  LanguageManager();
  void init();
  void scanAvailableLanguages();
  bool loadLanguage(const String& filePath);

  String getActiveLanguageName() const { return m_activeLangName; }
  String getActiveLanguageFile() const { return m_activeLangFile; }

  int getAvailableLanguageCount() const { return m_langCount; }
  LangItem getAvailableLanguage(int idx) const;

  const char* getText(const char* key, const char* fallback = nullptr);

private:
  String m_activeLangFile = DEFAULT_LANGUAGE_FILE;
  String m_activeLangName = DEFAULT_LANGUAGE_NAME;

  int m_langCount = 0;
  LangItem m_availableLangs[10];

  std::map<String, String> m_dictionary;
};

extern LanguageManager langManager;

#endif // LANGUAGE_MANAGER_H
