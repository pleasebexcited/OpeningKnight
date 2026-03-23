# Opening Knight

UE 5.6 theatre-style dice roguelike. Dice gameplay, block/counter minigame, curtains, battle flow.

## Setup (for collaborators)

### Prerequisites
- **Unreal Engine 5.6** (install from Epic Games Launcher)
- **Visual Studio 2022** with C++ Game Development workload (Desktop development with C++, etc.)

### Clone and open
1. Clone the repo (use a path without spaces if you run into issues):
   ```
   git clone https://github.com/pleasebexcited/OpeningKnight.git
   cd OpeningKnight
   ```
2. Open the project:
   - Double-click `OpeningKnight/OpeningKnight.uproject`
   - Or launch Unreal Editor and open that file
3. First open will compile C++ and may take a few minutes.
4. For Visual Studio:
   - Right-click `OpeningKnight.uproject` → **Generate Visual Studio project files**
   - Open the generated `OpeningKnight.sln`

### Project layout
- `OpeningKnight/` – Unreal project (Content, Source, Config)
- `images/` – Source PNGs (imported into Content)
- `music/` – Source audio

### Engine path
The project expects UE 5.6. If you use a different install path, generate VS project files from the .uproject and it will use your local paths.
