import os
import gzip

try:
    from SCons.Script import Import
    Import("env")
except ImportError:
    class MockEnv:
        def get(self, key, default="."):
            return default
    env = MockEnv()

project_dir = env.get("PROJECT_DIR", ".")
web_file = os.path.join(project_dir, "web", "index.html")
header_file = os.path.join(project_dir, "include", "embedded_web.h")

if not os.path.exists(web_file):
    print("web/index.html not found, skipping compression.")
    exit(0)

# Read web/index.html
with open(web_file, "r", encoding="utf-8") as f:
    html_content = f.read()

# Gzip and generate header
gz_data = gzip.compress(html_content.encode("utf-8"))

header = """// WARNING: Auto-generated file. Do not edit.
#ifndef EMBEDDED_WEB_H
#define EMBEDDED_WEB_H

#include <Arduino.h>

static const uint8_t INDEX_HTML_GZ[] PROGMEM = {
"""
header += ", ".join(f"0x{b:02x}" for b in gz_data)
header += """
};
static const size_t INDEX_HTML_GZ_LEN = sizeof(INDEX_HTML_GZ);

#endif
"""

# Ensure include/ exists
os.makedirs(os.path.dirname(header_file), exist_ok=True)

with open(header_file, "w", encoding="utf-8") as f:
    f.write(header)

print(f"Gzip complete! Original HTML: {len(html_content)} bytes -> Gzipped: {len(gz_data)} bytes")
