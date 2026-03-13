/**
 * M5Stack Core Ink - Todo List
 * 
 * Features:
 * - Display todo items on e-ink screen
 * - Check/uncheck items with buttons
 * - Add new items (simplified)
 * - Persistent storage (saves to flash)
 * - Low power mode (e-ink retains display)
 * 
 * Hardware: M5Stack Core Ink
 * - E-ink display: 200x200
 * - Buttons: A (GPIO 37), B (GPIO 38), C (GPIO 39)
 * - LEDs: RED (GPIO 10), GREEN (GPIO 9)
 */

#include <M5CoreInk.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// Button pins
#define BUTTON_A 37
#define BUTTON_B 38
#define BUTTON_C 39

// LED pins
#define LED_RED 10
#define LED_GREEN 9

// Display dimensions
#define SCREEN_WIDTH 200
#define SCREEN_HEIGHT 200

// Todo item structure
struct TodoItem {
    String text;
    bool done;
    uint32_t createdAt;
};

// Global variables
Preferences prefs;
TodoItem todos[10];  // Max 10 items
int todoCount = 0;
int selectedIndex = 0;
bool needsUpdate = true;

// Modes
enum Mode {
    VIEW_LIST,    // View and navigate list
    ADD_ITEM,     // Add new item (simplified)
    DELETE_ITEM   // Delete selected item
};

Mode currentMode = VIEW_LIST;

void setup() {
    M5.begin();
    Serial.begin(115200);
    
    // Initialize LEDs
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    
    // Initialize buttons
    pinMode(BUTTON_A, INPUT_PULLUP);
    pinMode(BUTTON_B, INPUT_PULLUP);
    pinMode(BUTTON_C, INPUT_PULLUP);
    
    // Load todos from flash
    loadTodos();
    
    // Initial display
    M5.M5Ink.clear();
    delay(100);
    drawInterface();
    
    Serial.println("Core Ink Todo List Started!");
    Serial.println("Button A: Previous / Cancel");
    Serial.println("Button B: Toggle done / Confirm");
    Serial.println("Button C: Next / Add");
    
    // Green LED on = ready
    digitalWrite(LED_GREEN, LOW);
    delay(500);
    digitalWrite(LED_GREEN, HIGH);
}

void loop() {
    handleButtons();
    
    if (needsUpdate) {
        drawInterface();
        needsUpdate = false;
    }
    
    delay(100);  // Small delay to reduce power consumption
}

void handleButtons() {
    static unsigned long lastPress = 0;
    if (millis() - lastPress < 300) return;  // Debounce
    
    // Button A - Previous / Cancel / Delete
    if (digitalRead(BUTTON_A) == LOW) {
        lastPress = millis();
        
        if (currentMode == VIEW_LIST) {
            // Previous item
            if (selectedIndex > 0) {
                selectedIndex--;
                needsUpdate = true;
                blinkLED(LED_RED, 50);
            }
        } else if (currentMode == ADD_ITEM) {
            // Cancel add
            currentMode = VIEW_LIST;
            needsUpdate = true;
            blinkLED(LED_RED, 100);
        } else if (currentMode == DELETE_ITEM) {
            // Delete selected
            deleteTodo(selectedIndex);
            currentMode = VIEW_LIST;
            needsUpdate = true;
            blinkLED(LED_RED, 200);
        }
    }
    
    // Button B - Toggle done / Confirm / Add new
    if (digitalRead(BUTTON_B) == LOW) {
        lastPress = millis();
        
        if (currentMode == VIEW_LIST) {
            // Toggle done status
            if (todoCount > 0 && selectedIndex < todoCount) {
                todos[selectedIndex].done = !todos[selectedIndex].done;
                saveTodos();
                needsUpdate = true;
                blinkLED(LED_GREEN, 50);
            }
        } else if (currentMode == ADD_ITEM) {
            // Add new item (simplified - hardcoded for demo)
            addTodo("New Task " + String(todoCount + 1));
            currentMode = VIEW_LIST;
            needsUpdate = true;
            blinkLED(LED_GREEN, 200);
        } else if (currentMode == DELETE_ITEM) {
            // Cancel delete
            currentMode = VIEW_LIST;
            needsUpdate = true;
        }
    }
    
    // Button C - Next / Add mode / Enter delete mode
    if (digitalRead(BUTTON_C) == LOW) {
        lastPress = millis();
        
        if (currentMode == VIEW_LIST) {
            if (todoCount < 10) {
                // Long press = add, short = next
                unsigned long pressStart = millis();
                while (digitalRead(BUTTON_C) == LOW && millis() - pressStart < 500) {
                    delay(10);
                }
                
                if (millis() - pressStart >= 500) {
                    // Long press - enter add mode
                    currentMode = ADD_ITEM;
                    blinkLED(LED_GREEN, 100);
                } else {
                    // Short press - next item
                    if (selectedIndex < todoCount - 1) {
                        selectedIndex++;
                        blinkLED(LED_GREEN, 50);
                    }
                }
                needsUpdate = true;
            } else {
                // List full, enter delete mode
                currentMode = DELETE_ITEM;
                needsUpdate = true;
                blinkLED(LED_RED, 100);
            }
        } else if (currentMode == ADD_ITEM) {
            // Add another
            addTodo("Task " + String(todoCount + 1));
            needsUpdate = true;
            blinkLED(LED_GREEN, 50);
        }
    }
}

void drawInterface() {
    InkPageSprite* sprite = &M5.M5Ink.getPageSprite();
    sprite->clear();
    
    // Draw title
    sprite->drawString("✓ TODO LIST", 10, 5, &AsciiFont8x16);
    sprite->drawLine(0, 22, 200, 22, BLACK);
    
    // Draw mode indicator
    String modeText;
    switch(currentMode) {
        case VIEW_LIST: modeText = "[VIEW] A:↑ B:✓ C:↓(hold:+)"; break;
        case ADD_ITEM: modeText = "[ADD] A:Cancel B:Add"; break;
        case DELETE_ITEM: modeText = "[DEL] A:Del B:Cancel"; break;
    }
    sprite->drawString(modeText, 5, 185, &AsciiFont4x8);
    
    // Draw todo items
    int y = 30;
    for (int i = 0; i < todoCount && i < 8; i++) {
        // Selection indicator
        if (i == selectedIndex && currentMode != ADD_ITEM) {
            sprite->fillRect(0, y - 2, 200, 18, BLACK);
            sprite->setTextColor(WHITE);
        } else {
            sprite->setTextColor(BLACK);
        }
        
        // Checkbox
        String checkbox = todos[i].done ? "[✓]" : "[ ]";
        sprite->drawString(checkbox, 5, y, &AsciiFont8x16);
        
        // Text (truncate if too long)
        String text = todos[i].text;
        if (text.length() > 18) {
            text = text.substring(0, 15) + "...";
        }
        if (todos[i].done) {
            // Strikethrough effect (draw line through text)
            sprite->drawString(text, 35, y, &AsciiFont8x16);
            sprite->drawLine(35, y + 8, 35 + text.length() * 8, y + 8, 
                             (i == selectedIndex) ? WHITE : BLACK);
        } else {
            sprite->drawString(text, 35, y, &AsciiFont8x16);
        }
        
        sprite->setTextColor(BLACK);
        y += 18;
    }
    
    // Draw empty state
    if (todoCount == 0) {
        sprite->drawString("No todos!", 60, 80, &AsciiFont8x16);
        sprite->drawString("Hold C to add", 50, 100, &AsciiFont8x16);
    }
    
    // Draw count
    String countStr = String(todoCount) + "/10";
    sprite->drawString(countStr, 170, 5, &AsciiFont8x16);
    
    // Push to display
    M5.M5Ink.pushPageSprite();
}

void addTodo(String text) {
    if (todoCount >= 10) return;
    
    todos[todoCount].text = text;
    todos[todoCount].done = false;
    todos[todoCount].createdAt = millis();
    todoCount++;
    
    saveTodos();
}

void deleteTodo(int index) {
    if (index < 0 || index >= todoCount) return;
    
    // Shift items
    for (int i = index; i < todoCount - 1; i++) {
        todos[i] = todos[i + 1];
    }
    todoCount--;
    
    // Adjust selection
    if (selectedIndex >= todoCount) {
        selectedIndex = todoCount - 1;
    }
    if (selectedIndex < 0) selectedIndex = 0;
    
    saveTodos();
}

void saveTodos() {
    prefs.begin("todoapp", false);
    
    // Save count
    prefs.putInt("count", todoCount);
    
    // Save each todo as JSON
    for (int i = 0; i < todoCount; i++) {
        StaticJsonDocument<256> doc;
        doc["text"] = todos[i].text;
        doc["done"] = todos[i].done;
        doc["created"] = todos[i].createdAt;
        
        String key = "todo" + String(i);
        String jsonStr;
        serializeJson(doc, jsonStr);
        prefs.putString(key.c_str(), jsonStr);
    }
    
    prefs.end();
    Serial.println("Todos saved!");
}

void loadTodos() {
    prefs.begin("todoapp", true);  // Read-only
    
    todoCount = prefs.getInt("count", 0);
    if (todoCount > 10) todoCount = 10;
    
    for (int i = 0; i < todoCount; i++) {
        String key = "todo" + String(i);
        String jsonStr = prefs.getString(key.c_str(), "");
        
        if (jsonStr.length() > 0) {
            StaticJsonDocument<256> doc;
            DeserializationError error = deserializeJson(doc, jsonStr);
            
            if (!error) {
                todos[i].text = doc["text"].as<String>();
                todos[i].done = doc["done"].as<bool>();
                todos[i].createdAt = doc["created"].as<uint32_t>();
            }
        }
    }
    
    prefs.end();
    Serial.print("Loaded ");
    Serial.print(todoCount);
    Serial.println(" todos");
}

void blinkLED(int pin, int duration) {
    digitalWrite(pin, LOW);
    delay(duration);
    digitalWrite(pin, HIGH);
}
