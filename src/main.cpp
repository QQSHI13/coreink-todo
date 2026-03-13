/**
 * M5Stack Core Ink - Todo List - Working Version
 * 
 * API Reference:
 * - drawString(const char* string, int32_t x, int32_t y)
 * - fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
 */

#include <M5CoreInk.h>
#include <Preferences.h>

#define BUTTON_A 37
#define BUTTON_B 38
#define BUTTON_C 39
#define LED_RED 10
#define MAX_TODOS 10

struct TodoItem {
    char text[30];
    bool done;
};

Preferences prefs;
TodoItem todos[MAX_TODOS];
int todoCount = 0;
int selectedIndex = 0;
bool needsUpdate = true;

enum Mode { VIEW_LIST, ADD_ITEM, DELETE_ITEM };
Mode currentMode = VIEW_LIST;

void drawInterface();
void handleButtons();
void addTodo(const char* text);
void deleteTodo(int index);
void saveTodos();
void loadTodos();
void blinkLED(int pin, int duration);

void setup() {
    M5.begin();
    
    pinMode(LED_RED, OUTPUT);
    digitalWrite(LED_RED, HIGH);
    
    pinMode(BUTTON_A, INPUT_PULLUP);
    pinMode(BUTTON_B, INPUT_PULLUP);
    pinMode(BUTTON_C, INPUT_PULLUP);
    
    M5.M5Ink.begin();
    M5.M5Ink.clear();
    delay(100);
    
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
            char buf[20];
            sprintf(buf, "Task %d", todoCount + 1);
            addTodo(buf);
            currentMode = VIEW_LIST;
            needsUpdate = true;
            blinkLED(LED_RED, 200);
        } else if (currentMode == DELETE_ITEM) {
            currentMode = VIEW_LIST;
            needsUpdate = true;
        }
    }
    
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
            char buf[20];
            sprintf(buf, "Task %d", todoCount + 1);
            addTodo(buf);
            needsUpdate = true;
            blinkLED(LED_RED, 50);
        }
    }
}

void drawInterface() {
    M5.M5Ink.clear();
    
    // Title: drawString(const char*, x, y)
    M5.M5Ink.drawString("TODO LIST", 10, 5);
    
    // Line under title
    M5.M5Ink.fillRect(0, 22, 200, 1, BLACK);
    
    // Mode indicator at bottom
    const char* modeText;
    switch(currentMode) {
        case VIEW_LIST: modeText = "[VIEW] A:Up B:Check C:Down"; break;
        case ADD_ITEM: modeText = "[ADD] A:Cancel B:Add"; break;
        case DELETE_ITEM: modeText = "[DEL] A:Del B:Cancel"; break;
        default: modeText = "";
    }
    M5.M5Ink.drawString(modeText, 5, 185);
    
    // Todo items
    int y = 30;
    for (int i = 0; i < todoCount && i < 8; i++) {
        char line[35];
        if (todos[i].done) {
            sprintf(line, "[X] %s", todos[i].text);
        } else {
            sprintf(line, "[ ] %s", todos[i].text);
        }
        
        // Truncate if too long
        if (strlen(line) > 28) {
            line[25] = '.';
            line[26] = '.';
            line[27] = '.';
            line[28] = '\0';
        }
        
        M5.M5Ink.drawString(line, 5, y);
        
        // Highlight selected with underline
        if (i == selectedIndex && currentMode != ADD_ITEM) {
            M5.M5Ink.fillRect(0, y + 14, 200, 2, BLACK);
        }
        
        y += 18;
    }
    
    // Empty state
    if (todoCount == 0) {
        M5.M5Ink.drawString("No todos!", 60, 80);
        M5.M5Ink.drawString("Press C to add", 50, 100);
    }
    
    // Count
    char countStr[10];
    sprintf(countStr, "%d/%d", todoCount, MAX_TODOS);
    M5.M5Ink.drawString(countStr, 170, 5);
}

void addTodo(const char* text) {
    if (todoCount >= MAX_TODOS) return;
    
    strncpy(todos[todoCount].text, text, 29);
    todos[todoCount].text[29] = '\0';
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
        char keyT[20], keyD[20];
        sprintf(keyT, "t%d", i);
        sprintf(keyD, "d%d", i);
        prefs.putString(keyT, todos[i].text);
        prefs.putBool(keyD, todos[i].done);
    }
    
    prefs.end();
}

void loadTodos() {
    prefs.begin("todoapp", true);
    
    todoCount = prefs.getInt("count", 0);
    if (todoCount > MAX_TODOS) todoCount = MAX_TODOS;
    
    for (int i = 0; i < todoCount; i++) {
        char keyT[20], keyD[20];
        sprintf(keyT, "t%d", i);
        sprintf(keyD, "d%d", i);
        
        String t = prefs.getString(keyT, "");
        strncpy(todos[i].text, t.c_str(), 29);
        todos[i].text[29] = '\0';
        todos[i].done = prefs.getBool(keyD, false);
    }
    
    prefs.end();
}

void blinkLED(int pin, int duration) {
    digitalWrite(pin, LOW);
    delay(duration);
    digitalWrite(pin, HIGH);
}
