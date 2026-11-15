<div align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="Presentation/Brand/Wolcan%20Logo%20-%20Dark%20Background.png">
    <img src="Presentation/Brand/Wolcan%20Logo%20-%20Light%20Background.png" width="420" alt="Wolcan pixel-art wordmark in front of an erupting volcano">
  </picture>
  <h1>WOLCAN</h1>
  <p><strong>Escape the maze. Survive the volcano.</strong></p>
</div>

<div align="center">

[![C++20](https://img.shields.io/badge/C%2B%2B-20-315c8c?logo=cplusplus)](https://isocpp.org/)
[![raylib 5.5](https://img.shields.io/badge/raylib-5.5-D94326?logo=raylib)](https://github.com/raysan5/raylib/tree/5.5)



[Game Modes](#choose-your-challenge) · [Build and Run](#installation--setup) · [QA](#quality-assurance) · [Team](#meet-the-team)

</div>

---

## About Wolcan

**Wolcan** is a 2D pixel-art maze survival game built in C++ with raylib. Find a route through procedurally generated volcanic corridors, avoid lava and wall spikes, and reach the exit as quickly as you can. Hidden passages lead to collectibles, while the timer and step counter give you a reason to replay a seed and refine your route.

## Choose Your Challenge

| Mode | What you face | What you earn |
| --- | --- | --- |
| **Classic Maze** | Four difficulty tiers, false walls, hidden finds, and hazards in Extra Hard. The game code calls this mode Expedition. | Personal best times, Gold, Silver, or Bronze medals, and Classic Maze achievements. |
| **The Floor Is Lava** | The floor collapses and lava spreads across a 25 × 25 maze. Collect Speed Boost Potions to reach the exit faster. | A mode-specific time record and lava challenges such as finishing without damage. |
| **Spike Rush** | Faster, staggered wall spikes guard a 25 × 25 maze. Watch the warning and cross during a safe window. | A separate time record and challenges for speed, spike avoidance, and damage-free runs. |



## Difficulty System

| Tier | Preset | Maze design |
| --- | --- | --- |
| **Easy** | 15 × 15 | More connecting loops and a closer exit. |
| **Normal** | 25 × 25 | Fewer shortcuts, a longer route, and up to one boulder. |
| **Hard** | 41 × 41 | A distant exit, fewer loops, and up to three boulders. |
| **Extra Hard** | 49 × 49 | Long routes with fragile floors, lava traps, and timed wall spikes. |

Use the left and right arrows in the main menu to set a board size from **10 × 10** to **50 × 50** while keeping the selected tier's generation rules.

## Survive the Volcano

| Mechanic | Rules |
| --- | --- |
| ❤️ **Health and respawn** | Start with **100 HP** and recover **4 HP every 4 seconds** of safe play. At zero HP, the character respawns at the start of the current maze with full health. |
| 🌋 **Lava traps** | Fragile tiles crack before they collapse. Linked tiles can fall in sequence. Lava deals **7 HP per second**, slows movement, and suspends regeneration while it deals damage. |
| ⚠️ **Wall spikes** | Traps warn, extend, and retract at staggered intervals. An eligible hit during the active phase deals **12 HP**. |
| 🧪 **Speed Boost Potions** | In The Floor Is Lava, potions increase movement speed by **40% for 10 seconds** of active play. A second pickup refreshes the timer. |
| 🔨 **Magic Hammer** | Discover the hidden Konami Code in Classic Maze to receive **10 wall breaks**. Breaking an eligible wall marks the run as assisted and excludes it from timed records and medals. |

## Unlock Your Legacy

Earn **Gold** or **Silver** for meeting both time and step targets. Other completed, unassisted Classic Maze runs earn **Bronze**. Explore false walls for runes, crystals, and relics, or take on speed and no-damage challenges.

| Achievement | How to earn it |
| --- | --- |
| **Untouchable** | Finish Extra Hard without taking damage. |
| **Perfect Run** | Earn Gold in Extra Hard without taking damage. |
| **Lava Runner** | Finish The Floor Is Lava. |
| **Spike Survivor** | Finish Spike Rush. |
| **Cheater!** | Break five walls with the Magic Hammer. |
| **Extreme ball knowledge!** | Discover the secret tool. |

## Choose Your Survivor

Explorer is available from the start. Open **Character Customization** to preview each skin and equip an unlocked one. Wolcan keeps your selection in the local profile.



| Lava Walker | Obsidian Knight | Magma Spirit | Ash Wanderer | Crimson Guardian |
| --- | --- | --- | --- | --- |
| First maze clear | A clear without damage | Three unique collectibles | Five distinct maze clears | Perfect Run |

## Installation & Setup

### Requirements

- **Windows x64** and **Visual Studio 2026** with **Desktop development with C++**, a Windows SDK, and **C++ CMake tools for Windows**.
- **CMake 4.2 or later** with the Visual Studio 2026 generator. The build script checks Visual Studio's bundled CMake first.
- Internet access for the first raylib dependency download if the dependency has yet to be cached.

### Build with Visual Studio

1. Clone the repository.

   ~~~bash
   git clone https://github.com/NIIvanov24/Wolcan.git
   cd Wolcan
   ~~~

2. Open [Wolcan.slnx](Wolcan/Wolcan.slnx) in Visual Studio.
3. Select **Release | x64** or **Debug | x64** and set **PL** as the startup project.
4. Build the solution. The PL project prepares **raylib 5.5** automatically and copies runtime resources alongside the executable.
5. Press <kbd>F5</kbd> to start the game. The Release solution build outputs to <code>build\native\x64\Release\Wolcan.exe</code>.

<details>
<summary><strong>Build and run from PowerShell</strong></summary>

Run these commands from the folder containing this README.

~~~powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build.ps1 -Configuration Release -Architecture x64
.\build\x64\Release\Wolcan.exe
~~~

The [build script](scripts/Build.ps1) configures CMake, which builds raylib from a checksum-verified source archive. To reproduce a particular maze, launch with <code>.\build\x64\Release\Wolcan.exe --seed 42</code> and keep the same mode, size, and difficulty.

</details>

## Controls & Settings

| Default key | Action |
| --- | --- |
| <kbd>↑</kbd> <kbd>↓</kbd> <kbd>←</kbd> <kbd>→</kbd> | Move one cell per step. Hold a direction to keep moving. You can bind movement to **WASD** in Settings. |
| <kbd>P</kbd> | Pause or resume a run. |
| <kbd>Esc</kbd> | Return to the menu or back out of a submenu. From the main menu, quit the game. |
| <kbd>F11</kbd> | Toggle fullscreen. |
| <kbd>H</kbd> | Use the Magic Hammer after unlocking it. |
| <kbd>R</kbd> / <kbd>T</kbd> | Restart with a new seed / replay the same seed. |
| <kbd>N</kbd> | Advance after finishing Classic Maze. |
| <kbd>M</kbd> | Open Mini-Games from the main menu. |

Open **Settings** to rebind keys, adjust music and sound effects separately, mute either channel, or restore default bindings. Wolcan saves settings locally.

## Tech Stack

| Area | Tools | Use |
| --- | --- | --- |
| 🎮 Programming and graphics | **C++20**, **raylib 5.5** | Game rules, rendering, input, and audio. |
| 🛠️ Development | **Visual Studio**, **CMake**, **Git**, **GitHub** | Build the game and track project changes. |
| 💬 Team communication | **Microsoft Teams** | Coordinate work and reviews. |
| 📑 Documentation | **Microsoft Word**, **Microsoft PowerPoint**, **Microsoft Excel** | Technical report, jury presentation, and QA workbook. |
| 💻 AI-assisted development | **ChatGPT**, **OpenAI Codex** | Assist with implementation, debugging, and documentation. |

## Project Structure

~~~text
Wolcan/
├── include/        C++ headers grouped by PL, BLL, and DAL
├── src/            C++ implementations grouped by the same three layers
├── assets/         Runtime sprites, textures, fonts, icons, and audio
├── QA/             QA test scope and demonstration workbook
├── Presentation/   PowerPoint, Word documentation, and README media
├── scripts/        PowerShell build entry point
├── Wolcan/         Visual Studio solution and project settings
└── CMakeLists.txt  CMake build configuration
~~~

The presentation layer (**PL**) handles input, menus, rendering, and sound. The business logic layer (**BLL**) generates mazes and applies movement, hazards, and progression rules. The data access layer (**DAL**) handles local profiles and best times. Automated test sources are kept in the separate <code>Wolcan-support/tests/</code> workspace. The tracked QA workbook and presentation materials are linked below.

## Behind the Volcano

| Phase | Milestone |
| --- | --- |
| 1 · Planning and concept | Define the escape objective and the volcanic setting. |
| 2 · Game architecture | Establish the three layers and the build workflow. |
| 3 · Maze generation | Implement seeded layouts, four difficulties, and connected routes. |
| 4 · Visual design | Draw the pixel-art interface, characters, and volcanic tiles. |
| 5 · Gameplay mechanics | Add health, traps, respawn, and the Magic Hammer. |
| 6 · Mini-games and achievements | Add Lava and Spike Rush challenges, potions, and progression. |
| 7 · Testing and debugging | Check generation, collisions, replay, and saving. |
| 8 · Documentation and presentation | Prepare the technical report, QA workbook, and live demo. |

## Quality Assurance

| Test type | Examples |
| --- | --- |
| Functional | Navigate menus, move the character, finish runs, unlock rewards, and change settings. |
| Integration | Verify that controls, game rules, UI, and persistent profiles agree. |
| Boundary and negative | Check maze size limits, timing thresholds, invalid settings, and blocked movement. |
| Regression | Repeat seeded runs and verify replay, records, and known fixes. |
| Performance and stability | Check behavior at different frame rates, viewport sizes, and during repeated transitions. |

**Verified Windows x64 Release run** · **4 of 4** CTest suites passed. The suites cover game rules, record persistence, progression, and the presentation controller.

**Demonstration workbook** · **39 test cases and 141 checks**. Its 100% sample results illustrate the reporting format. See the [QA Test Scope workbook](QA/Wolcan%20QA%20Test%20Scope.xlsx).

<details>
<summary><strong>How results are counted</strong></summary>

- **PASSED** means the observed result matches the expected result for the executed check.
- **Test coverage** = in-scope requirements with at least one test ÷ total in-scope requirements × 100.
- **Execution rate** = (passed + failed) ÷ total planned checks × 100.
- **Pass rate** = passed ÷ (passed + failed) × 100.

</details>

## Documentation & Project Resources

| Resource | Contents |
| --- | --- |
| [Project presentation](Presentation/Wolcan%20Presentation.pptx) | Game concept, team, modes, development timeline, and live demo. |
| [Technical documentation](Presentation/Wolcan%20Documentation.docx) | Architecture, gameplay systems, core components, and verification results. |
| [QA test report](QA/Wolcan%20QA%20Test%20Scope.xlsx) | Test scenarios by system and a formula-driven demonstration summary. |
| [Asset credits](assets/credits.txt) | Art, fonts, icons, and audio attribution. |


<h2 align="center">👾 MEET THE TEAM · WOLCAN DEVELOPERS</h2>

<table align="center">
  <tr>
    <td align="center" width="25%">
      <br>
      <a href="https://github.com/NIIvanov24">
        <img src="https://github.com/NIIvanov24.png"
             width="120" alt="Nikolay Ivanov">
        <br><b>Nikolay Ivanov</b>
      </a>
      <br><sub>Scrum Trainer</sub>
    </td>
    <td align="center" width="25%">
     <br>
      <a href="https://github.com/VAZarakyn24">
        <img src="https://github.com/VAZarakyn24.png"
             width="120" alt="Vasil Zarakyan">
        <br><b>Vasil Zarakyan</b>
      </a>
      <br><sub>QA</sub>
    </td>
    <td align="center" width="25%">
     <br>
      <a href="https://github.com/FYFilipov24">
        <img src="https://github.com/FYFilipov24.png"
             width="120" alt="Filip Filipov">
        <br><b>Filip Filipov</b>
      </a>
      <br><sub>Front-End Developer</sub>
    </td>
    <td align="center" width="25%">
      <br>
      <a href="https://github.com/GGGakev24">
        <img src="https://github.com/GGGakev24.png"
             width="120" alt="Georgi Gakev">
        <br><b>Georgi Gakev</b>
      </a>
      <br><sub>Back-End Developer</sub>
    </td>
  </tr>
</table>

---

<div align="center">

### Ready to Enter the Volcano?

[Build Wolcan](#installation--setup) · [Explore the repository](https://github.com/NIIvanov24/Wolcan)

</div>
