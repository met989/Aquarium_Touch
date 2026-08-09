#include "language_manager.h"
#include "embedded_languages.h"

LanguageManager langManager;

LanguageManager::LanguageManager() {}



void LanguageManager::init() {
  m_langCount = 0;
  scanAvailableLanguages();
  loadLanguage(m_activeLangFile);
}

void LanguageManager::scanAvailableLanguages() {
  m_langCount = 0;
  for (int i = 0; i < EMBEDDED_LANGUAGES_COUNT && i < 10; i++) {
    m_availableLangs[m_langCount++] = {
      EMBEDDED_LANGUAGES[i].filename,
      EMBEDDED_LANGUAGES[i].name
    };
  }
}

bool LanguageManager::loadLanguage(const String &langNameOrPath) {
  String pureName = langNameOrPath;
  if (pureName.startsWith("/languages/"))
    pureName = pureName.substring(11);
  else if (pureName.startsWith("/"))
    pureName = pureName.substring(1);

  const char* content = nullptr;
  for (int i = 0; i < EMBEDDED_LANGUAGES_COUNT; i++) {
    if (String(EMBEDDED_LANGUAGES[i].filename) == pureName) {
      content = EMBEDDED_LANGUAGES[i].content;
      break;
    }
  }

  if (content == nullptr) {
    if (EMBEDDED_LANGUAGES_COUNT > 0) {
      content = EMBEDDED_LANGUAGES[0].content;
      pureName = EMBEDDED_LANGUAGES[0].filename;
    } else {
      return false;
    }
  }

  m_dictionary.clear();
  m_activeLangFile = pureName;

  const char* p = content;
  while (*p != '\0') {
    String line = "";
    while (*p != '\0' && *p != '\n') {
      if (*p != '\r') {
        line += *p;
      }
      p++;
    }
    if (*p == '\n') {
      p++;
    }

    line.trim();
    if (line.length() == 0 || line.startsWith("#"))
      continue;
    int eq = line.indexOf('=');
    if (eq > 0) {
      String k = line.substring(0, eq);
      String v = line.substring(eq + 1);
      k.trim();
      v.trim();
      m_dictionary[k] = v;
    }
  }

  if (m_dictionary.find("LANG_NAME") != m_dictionary.end()) {
    m_activeLangName = m_dictionary["LANG_NAME"];
  } else {
    m_activeLangName = pureName;
  }
  return true;
}

LangItem LanguageManager::getAvailableLanguage(int idx) const {
  if (idx >= 0 && idx < m_langCount) {
    return m_availableLangs[idx];
  }
  return {"", ""};
}

const char *LanguageManager::getText(const char *key, const char *fallback) {
  if (key == nullptr)
    return fallback ? fallback : "";
  auto it = m_dictionary.find(String(key));
  if (it != m_dictionary.end()) {
    return it->second.c_str();
  }
  return fallback ? fallback : key;
}
