# ✓ Core Ink Todo List

A minimalist todo list application for M5Stack Core Ink with e-ink display.

![Todo List](demo.png)

## ✨ Features

- 📋 **Display up to 10 todo items** on e-ink screen
- ✅ **Check/uncheck items** with button press
- 💾 **Persistent storage** - survives power off
- 🔋 **Ultra low power** - e-ink retains display without power
- 🌙 **Always-on display** - no backlight needed
- 🔄 **Simple 3-button interface**

## 🛠️ Hardware

**M5Stack Core Ink**
- E-ink display: 200x200 pixels
- 3 Buttons: A (GPIO 37), B (GPIO 38), C (GPIO 39)
- Status LEDs: Red (GPIO 10), Green (GPIO 9)
- ESP32-PICO processor

## 📦 Installation

### Prerequisites

- [PlatformIO](https://platformio.org/) installed
- M5Stack Core Ink device
- USB-C cable

### Setup

1. Clone the repository:
```bash
git clone https://github.com/QQSHI13/coreink-todo.git
cd coreink-todo
```

2. Build and upload:
```bash
pio run --target upload
```

Or use VS Code with PlatformIO extension.

## 🎮 Usage

### Button Functions

| Button | Short Press | Long Press (>500ms) |
|--------|-------------|---------------------|
| **A** | Previous item | Enter delete mode |
| **B** | Toggle done/undone | Confirm add/delete |
| **C** | Next item | Enter add mode |

### Modes

#### 📖 View Mode (Default)
- Navigate with A (up) and C (down)
- Press B to mark item done/undone
- Hold C to add new item
- Hold A to delete selected item

#### ➕ Add Mode
- Press B to add a new todo
- Press A to cancel
- Auto-saves to flash memory

#### 🗑️ Delete Mode
- Press B to cancel
- Press A to confirm deletion
- Deleted items cannot be recovered

## 🖥️ Display Layout

```
┌────────────────────┐
│ ✓ TODO LIST     3/10│  ← Title + count
├────────────────────┤
│ ▶ [✓] Buy milk     │  ← Selected item
│   [ ] Walk dog     │  ← Unchecked item
│   [✓] Read book    │  ← Done item (strikethrough)
│   ...              │
├────────────────────┤
│ [VIEW] A:↑ B:✓ C:↓ │  ← Mode indicator
└────────────────────┘
```

## 🔧 Customization

### Change Max Items

```cpp
// In main.cpp
#define MAX_TODOS 20  // Default is 10
```

### Customize Add Items

Currently adds hardcoded items for demo. To add custom input:

```cpp
// In addTodo() function
// Replace this:
addTodo("New Task " + String(todoCount + 1));

// With serial input or web interface
String customText = getInputFromSerial();
addTodo(customText);
```

### Web Sync (Advanced)

Add WiFi sync with your todo app:

```cpp
#include <WiFi.h>
#include <HTTPClient.h>

// Sync with tools-suite or other service
void syncWithWeb() {
    // Upload todos to server
    // Download new todos
}
```

## 💾 Data Persistence

Todos are saved to ESP32's flash memory using Preferences library:
- Survives power cycles
- No battery needed to retain data
- JSON format for easy parsing

## 🔋 Power Consumption

| Mode | Current | Battery Life* |
|------|---------|---------------|
| Active (updating) | ~50mA | ~4 hours |
| Sleep (display static) | ~0.01mA | **Months** |

*Based on 500mAh battery

**E-ink magic:** Once display is updated, it stays visible without any power!

## 📁 Project Structure

```
coreink-todo/
├── platformio.ini      # PlatformIO configuration
├── src/
│   └── main.cpp        # Main application code
└── README.md           # This file
```

## 📝 Code Architecture

### Key Functions

| Function | Description |
|----------|-------------|
| `drawInterface()` | Renders todo list on e-ink display |
| `handleButtons()` | Processes button inputs with debounce |
| `addTodo()` | Adds new item and saves to flash |
| `deleteTodo()` | Removes item and shifts array |
| `saveTodos()` | Persists data to Preferences |
| `loadTodos()` | Loads data from Preferences |

### Libraries Used

- [M5CoreInk](https://github.com/m5stack/M5CoreInk): Core Ink hardware support
- [ArduinoJson](https://arduinojson.org/): JSON serialization for storage
- Preferences: ESP32 flash storage

## 🎨 Display Features

- **Strikethrough** for completed items
- **Highlight bar** for selected item
- **Scroll indicator** (shows position)
- **Count badge** (current/max)
- **Mode footer** (current operation)

## 🔌 Wiring

No external wiring needed! Core Ink is self-contained.

For development/debugging:
- USB-C for power and programming
- Serial monitor at 115200 baud

## 🚀 Future Enhancements

- [ ] WiFi sync with web interface
- [ ] Due dates and reminders
- [ ] Categories/tags
- [ ] Priority levels
- [ ] Voice input (with mic module)
- [ ] Barcode scanning for shopping lists

## 📄 License

GPL-3.0 License - See [LICENSE](LICENSE) for details.

## 🙏 Credits

Built by [QQ](https://github.com/QQSHI13) with [Nova ☄️](https://github.com/QQSHI13/nova)

Inspired by minimalist productivity tools.

---

**Stay organized with Core Ink! ✓**

Note: E-ink displays have slow refresh rates. Be patient when updating.
