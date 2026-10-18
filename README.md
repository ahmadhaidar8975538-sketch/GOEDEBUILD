# Auto Design (Geode mod for Geometry Dash)

Select blocks in the editor, press the black AUTO box, get instant extreme-demon-style decoration.

## Build locally
1. Install the Geode SDK + CLI: https://docs.geode-sdk.org/getting-started/
2. `geode build`  (or: `cmake -B build && cmake --build build --config RelWithDebInfo`)
3. The `.geode` file appears in `build/`. Drop it into your GD `geode/mods` folder.

## Build with no setup (GitHub Actions)
1. Make a new GitHub repo and upload this folder's contents.
2. Go to Actions -> "Build Geode Mod" -> Run workflow.
3. Download the `.geode` file from the "Build Output" artifact.

## Before you build
Edit `mod.json`: change `"id"` (format `yourname.autodesign`) and `"developer"`.
