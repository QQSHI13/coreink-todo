/**
 * M5Stack Core Ink - Todo List
 * 
 * Features:
 * - Display todo items on e-ink screen
 * - Check/uncheck items with buttons
 * - Persistent storage (saves to flash)
 * - Low power mode (e-ink retains display)
 * 
 * Hardware: M5Stack Core Ink
 * - E-ink display: 200x200
 * - Buttons: A (GPIO 37), B (GPIO 38), C (GPIO 39)
 * - LEDs: RED (GPIO 10)
 */

#include <Arduino.h>  // Must be first!
#include <M5CoreInk.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// Button pins
#define BUTTON_A 37
#define BUTTON_B 38
#define BUTTON_C 39
#define LED_RED 10

// Display dimensions
#define SCREEN_WIDTH 200
#define SCREEN_HEIGHT 200
#define MAX_TODOS 10

// Todo item structure
struct TodoItem {
    String text;
    bool done;
};

// Global variables
Preferences prefs;
TodoItem todos[MAX_TODOS];
int todoCount = 0;
int selectedIndex = 0;
bool needsUpdate = true;

// Modes
enum Mode {
    VIEW_LIST,
    ADD_ITEM,
    DELETE_ITEM
};
Mode currentMode = VIEW_LIST;

// Function declarations (forward declarations)
void drawInterface();
void handleButtons();
void addTodo(String text);
void deleteTodo(int index);
void saveTodos();
void loadTodos();
void blinkLED(int pin, int duration);

void setup() {
    M5.begin();
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("  Core Ink Todo List");
    Serial.println("========================================");
    
    // Initialize LED
    pinMode(LED_RED, OUTPUT);
    digitalWrite(LED_RED, HIGH);  // Off (active low)
    
    // Initialize buttons
    pinMode(BUTTON_A, INPUT_PULLUP);
    pinMode(BUTTON_B, INPUT_PULLUP);
    pinMode(BUTTON_C, INPUT_PULLUP);
    
    // Initialize display
    M5.M5Ink.begin();
    M5.M5Ink.clear();
    delay(100);
    
    // Load todos from flash
    loadTodos();
    
    // Initial display
    drawInterface();
    
    Serial.println("Started!");
    Serial.println("Button A: Previous");
    Serial.println("Button B: Toggle done");
    Serial.println("Button C: Next / Add");
}

void loop() {
    handleButtons();
    
    if (needsUpdate) {
        drawInterface();
        needsUpdate = false;
    }
    
    delay(100);
}

void handleButtons() {
    static unsigned long lastPress = 0;
    if (millis() - lastPress < 300) return;
    
    // Button A - Previous / Cancel / Delete
    if (digitalRead(BUTTON_A) == LOW) {
        lastPress = millis();
        
        if (currentMode == VIEW_LIST) {
            if (selectedIndex > 0) {
                selectedIndex--;
                needsUpdate = true;
                blinkLED(LED_RED, 50);
            }
        } else if (currentMode == ADD_ITEM) {
            currentMode = VIEW_LIST;
            needsUpdate = true;
            blinkLED(LED_RED, 100);
        } else if (currentMode == DELETE_ITEM) {
            deleteTodo(selectedIndex);
            currentMode = VIEW_LIST;
            needsUpdate = true;
            blinkLED(LED_RED, 200);
        }
    }
    
    // Button B - Toggle done / Confirm
    if (digitalRead(BUTTON_B) == LOW) {
        lastPress = millis();
        
        if (currentMode == VIEW_LIST) {
            if (todoCount > 0 && selectedIndex < todoCount) {
                todos[selectedIndex].done = !todos[selectedIndex].done;
                saveTodos();
                needsUpdate = true;
                blinkLED(LED_RED, 50);
            }
        } else if (currentMode == ADD_ITEM) {
            addTodo("Task " + String(todoCount + 1));
            currentMode = VIEW_LIST;
            needsUpdate = true;
            blinkLED(LED_RED, 200);
        } else if (currentMode == DELETE_ITEM) {
            currentMode = VIEW_LIST;
            needsUpdate = true;
        }
    }
    
    // Button C - Next / Add mode
    if (digitalRead(BUTTON_C) == LOW) {
        lastPress = millis();
        
        if (currentMode == VIEW_LIST) {
            if (selectedIndex < todoCount - 1) {
                selectedIndex++;
                blinkLED(LED_RED, 50);
            } else if (todoCount < MAX_TODOS) {
                currentMode = ADD_ITEM;
                blinkLED(LED_RED, 100);
            } else {
                currentMode = DELETE_ITEM;
                blinkLED(LED_RED, 100);
            }
            needsUpdate = true;
        } else if (currentMode == ADD_ITEM) {
            addTodo("Task " + String(todoCount + 1));
            needsUpdate = true;
            blinkLED(LED_RED, 50);
        }
    }
}

void drawInterface() {
    // Clear screen
    M5.M5Ink.clear();
    
    // Create sprite for drawing
    Ink_Sprite sprite(&M5.M5Ink);
    sprite.clear();
    
    // Draw title
    sprite.drawString("TODO LIST", 10, 5);
    sprite.drawLine(0, 22, 200, 22, BLACK);
    
    // Draw mode indicator
    String modeText;
    switch(currentMode) {
        case VIEW_LIST: modeText = "[VIEW] A:Up B:Check C:Down"; break;
        case ADD_ITEM: modeText = "[ADD] A:Cancel B:Add"; break;
        case DELETE_ITEM: modeText = "[DEL] A:Delete B:Cancel"; break;
    }
    sprite.drawString(modeText, 5, 185);
    
    // Draw todo items
    int y = 30;
    for (int i = 0; i < todoCount && i < 8; i++) {
        // Selection indicator
        if (i == selectedIndex && currentMode != ADD_ITEM) {
            sprite.fillRect(0, y - 2, 200, 18, BLACK);
            sprite.setTextColor(WHITE);
        } else {
            sprite.setTextColor(BLACK);
        }
        
        // Checkbox and text
        String checkbox = todos[i].done ? "[X]" : "[ ]";
        String text = todos[i].text;
        if (text.length() > 18) {
            text = text.substring(0, 15) + "...";
        }
        
        sprite.drawString(checkbox + " " + text, 5, y);
        
        sprite.setTextColor(BLACK);
        y += 18;
    }
    
    // Empty state
    if (todoCount == 0) {
        sprite.drawString("No todos!", 60, 80);
        sprite.drawString("Press C to add", 50, 100);
    }
    
    // Draw count
    sprite.drawString(String(todoCount) + "/" + String(MAX_TODOS), 170, 5);
    
    // Push to display
    sprite.pushSprite();
}

void addTodo(String text) {
    if (todoCount >= MAX_TODOS) return;
    
    todos[todoCount].text = text;
    todos[todoCount].done = false;
    todoCount++;
    
    saveTodos();
    Serial.println("Added: " + text);
}

void deleteTodo(int index) {
    if (index < 0 || index >= todoCount) return;
    
    for (int i = index; i < todoCount - 1; i++) {
        todos[i] = todos[i + 1];
    }
    todoCount--;
    
    if (selectedIndex >= todoCount) {
        selectedIndex = todoCount - 1;
    }
    if (selectedIndex < 0) selectedIndex = 0;
    
    saveTodos();
    Serial.println("Deleted item at index " + String(index));
}

void saveTodos() {
    prefs.begin("todoapp", false);
    prefs.putInt("count", todoCount);
    
    for (int i = 0; i < todoCount; i++) {
        String key = "todo" + String(i);
        prefs.putString((key + "_text").c_str(), todos[i].text);
        prefs.putBool((key + "_done").c_str(), todos[i].done);
    }
    
    prefs.end();
    Serial.println("Todos saved!");
}

void loadTodos() {
    prefs.begin("todoapp", true);
    
    todoCount = prefs.getInt("count", 0);
    if (todoCount > MAX_TODOS) todoCount = MAX_TODOS;
    
    for (int i = 0; i < todoCount; i++) {
        String key = "todo" + String(i);
        todos[i].text = prefs.getString((key + "_text").c_str(), "Task " + String(i + 1));
        todos[i].done = prefs.getBool((key + "_done").c_str(), false);
    }
    
    prefs.end();
    Serial.print("Loaded ");
    Serial.print(todoCount);
    Serial.println(" todos");
}

void blinkLED(int pin, int duration) {
    digitalWrite(pin, LOW);  // On (active low)
    delay(duration);
    digitalWrite(pin, HIGH); // Off
}
