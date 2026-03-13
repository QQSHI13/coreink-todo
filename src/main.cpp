/**
 * M5Stack Core Ink - Todo List - Simplified Version
 * 
 * Features:
 * - Display todo items on e-ink screen
 * - Check/uncheck items with buttons
 * - Persistent storage (saves to flash)
 * 
 * Hardware: M5Stack Core Ink
 */

#include <M5CoreInk.h>
#include <Preferences.h>

#define BUTTON_A 37
#define BUTTON_B 38
#define BUTTON_C 39
#define LED_RED 10
#define MAX_TODOS 10

struct TodoItem {
    String text;
    bool done;
};

Preferences prefs;
TodoItem todos[MAX_TODOS];
int todoCount = 0;
int selectedIndex = 0;
bool needsUpdate = true;

enum Mode { VIEW_LIST, ADD_ITEM, DELETE_ITEM };
Mode currentMode = VIEW_LIST;

// Forward declarations
void drawInterface();
void handleButtons();
void addTodo(String text);
void deleteTodo(int index);
void saveTodos();
void loadTodos();
void blinkLED(int pin, int duration);

void setup() {
    M5.begin();
    
    // Initialize LED
    pinMode(LED_RED, OUTPUT);
    digitalWrite(LED_RED, HIGH);
    
    // Initialize buttons
    pinMode(BUTTON_A, INPUT_PULLUP);
    pinMode(BUTTON_B, INPUT_PULLUP);
    pinMode(BUTTON_C, INPUT_PULLUP);
    
    // Initialize display
    M5.M5Ink.begin();
    M5.M5Ink.clear();
    delay(100);
    
    // Load todos
    loadTodos();
    drawInterface();
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
    M5.M5Ink.clear();
    
    // Use simple text drawing
    M5.M5Ink.drawString(10, 5, "TODO LIST");
    
    // Draw line
    for (int x = 0; x < 200; x++) {
        M5.M5Ink.drawPix(x, 22, BLACK);
    }
    
    // Draw mode at bottom
    String modeText;
    switch(currentMode) {
        case VIEW_LIST: modeText = "[VIEW] A:Up B:Check C:Down"; break;
        case ADD_ITEM: modeText = "[ADD] A:Cancel B:Add"; break;
        case DELETE_ITEM: modeText = "[DEL] A:Del B:Cancel"; break;
    }
    M5.M5Ink.drawString(5, 185, modeText.c_str());
    
    // Draw todo items
    int y = 30;
    for (int i = 0; i < todoCount && i < 8; i++) {
        String line;
        if (todos[i].done) {
            line = "[X] " + todos[i].text;
        } else {
            line = "[ ] " + todos[i].text;
        }
        
        // Truncate if too long
        if (line.length() > 25) {
            line = line.substring(0, 22) + "...";
        }
        
        M5.M5Ink.drawString(5, y, line.c_str());
        
        // Highlight selected
        if (i == selectedIndex && currentMode != ADD_ITEM) {
            for (int x = 0; x < 200; x++) {
                M5.M5Ink.drawPix(x, y + 8, BLACK);
            }
        }
        
        y += 18;
    }
    
    // Empty state
    if (todoCount == 0) {
        M5.M5Ink.drawString(60, 80, "No todos!");
        M5.M5Ink.drawString(50, 100, "Press C to add");
    }
    
    // Draw count
    String countStr = String(todoCount) + "/" + String(MAX_TODOS);
    M5.M5Ink.drawString(170, 5, countStr.c_str());
}

void addTodo(String text) {
    if (todoCount >= MAX_TODOS) return;
    
    todos[todoCount].text = text;
    todos[todoCount].done = false;
    todoCount++;
    
    saveTodos();
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
}

void blinkLED(int pin, int duration) {
    digitalWrite(pin, LOW);
    delay(duration);
    digitalWrite(pin, HIGH);
}
