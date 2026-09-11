Import("env")
import json
import os

version_file = os.path.join(env.get("PROJECT_DIR"), "version.json")
version = "unknown"

try:
    if os.path.exists(version_file):
        with open(version_file, "r") as f:
            data = json.load(f)
            version = data.get("version", "unknown")
except Exception as e:
    print(f"Warning: Failed to read version.json: {e}")

# Inject version as a C++ macro
# In C++ it will become: #define AQUARIUM_OS_VERSION "0.5.2-21082026-beta"
env.Append(CPPDEFINES=[
    ("AQUARIUM_OS_VERSION", f'\\"{version}\\"')
])
