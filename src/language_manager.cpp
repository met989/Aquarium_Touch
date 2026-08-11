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

  m_activeLang = nullptr;
  for (int i = 0; i < EMBEDDED_LANGUAGES_COUNT; i++) {
    if (String(EMBEDDED_LANGUAGES[i].filename) == pureName) {
      m_activeLang = &EMBEDDED_LANGUAGES[i];
      break;
    }
  }

  if (m_activeLang == nullptr) {
    if (EMBEDDED_LANGUAGES_COUNT > 0) {
      m_activeLang = &EMBEDDED_LANGUAGES[0];
      pureName = EMBEDDED_LANGUAGES[0].filename;
    } else {
      return false;
    }
  }

  m_activeLangFile = pureName;
  m_activeLangName = m_activeLang->name;

  return true;
}

LangItem LanguageManager::getAvailableLanguage(int idx) const {
  if (idx >= 0 && idx < m_langCount) {
    return m_availableLangs[idx];
  }
  return {"", ""};
}

const char *LanguageManager::getText(const char *key, const char *fallback) {
  if (key == nullptr) return fallback ? fallback : "";
  if (m_activeLang != nullptr) {
    for (int i = 0; i < m_activeLang->num_entries; i++) {
      if (strcmp(m_activeLang->entries[i].key, key) == 0) {
        return m_activeLang->entries[i].value;
      }
    }
  }
  return fallback ? fallback : key;
}


