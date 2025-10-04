// ESP32 Reddit Display - Shows Reddit posts on LCD screen
#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

// Colors (Reddit style)
#define REDDIT_ORANGE    0xFA20
#define REDDIT_BLACK     0x0000  
#define REDDIT_WHITE     0xFFFF
#define REDDIT_GRAY      0x8410
#define REDDIT_UPVOTE    0xFA20
#define REDDIT_DOWNVOTE  0x8010

#define HEADER_HEIGHT    35
#define PADDING          8

// Button setup
const int BUTTON_PIN = 0;  // Boot button
bool lastButtonState = HIGH;
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// Two display modes
enum DisplayMode {
  MODE_LATEST,    // Show post summary
  MODE_FULLTEXT   // Show full post with scrolling
};
DisplayMode currentMode = MODE_LATEST;

// Post data storage
String postSubreddit = "";
String postTitle = "";
String postAuthor = "";
int postScore = 0;
String postFullText = "";
String serialBuffer = "";

// Auto-refresh timer (30 seconds)
unsigned long lastRequestTime = 0;
const unsigned long REQUEST_INTERVAL = 30000;

// Scrolling for long posts
int scrollOffset = 0;
int maxScrollOffset = 0;
int contentHeight = 0;
const int SCROLL_STEP = 3;

// Function declarations
void setupDisplay();
void setupButton();
void drawLatestPost();
void drawFullTextScrollable();
void drawWelcomeScreen();
int calculateContentHeight(String text, int textSize);
void renderTextToSprite(String text, int textSize);
bool checkButtonPress();
void handleSerialData();
void parseLatestPost(String data);
void parsePlainText(String data);
void requestLatestPost();
void requestPlainText();

void setup() {
  Serial.begin(115200);
  delay(100);
  
  setupButton();
  setupDisplay();
  drawWelcomeScreen();
  
  requestLatestPost();
  lastRequestTime = millis();
}

void loop() {
  if (checkButtonPress()) {
    if (currentMode == MODE_LATEST) {
      // Switch to fulltext mode
      currentMode = MODE_FULLTEXT;
      scrollOffset = 0;  // Reset scroll position
      requestPlainText();
    } else {
      // In fulltext mode, scroll down
      if (scrollOffset < maxScrollOffset) {
        scrollOffset += SCROLL_STEP;
        if (scrollOffset > maxScrollOffset) {
          scrollOffset = maxScrollOffset;
        }
        drawFullTextScrollable();
      } else {
        // Reached bottom, go back to latest mode
        currentMode = MODE_LATEST;
        scrollOffset = 0;
        requestLatestPost();
      }
    }
  }
  
  if (currentMode == MODE_LATEST) {
    unsigned long now = millis();
    if (now - lastRequestTime >= REQUEST_INTERVAL) {
      requestLatestPost();
      lastRequestTime = now;
    }
  }
  
  handleSerialData();
  delay(10);
}

void setupDisplay() {
  pinMode(32, OUTPUT);
  digitalWrite(32, HIGH);
  
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(REDDIT_BLACK);
  
  // Create sprite for scrollable content area
  // Width = screen width, Height = content area (screen - header)
  sprite.createSprite(tft.width(), tft.height() - HEADER_HEIGHT);
  sprite.setTextWrap(true);
}

void setupButton() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

bool checkButtonPress() {
  bool currentState = digitalRead(BUTTON_PIN);
  bool wasPressed = false;
  
  unsigned long now = millis();
  
  if (lastButtonState == HIGH && currentState == LOW) {
    if (now - lastButtonPress > DEBOUNCE_DELAY) {
      wasPressed = true;
      lastButtonPress = now;
    }
  }
  
  lastButtonState = currentState;
  return wasPressed;
}

void requestLatestPost() {
  Serial.println("REQUEST_LATEST");
}

void requestPlainText() {
  Serial.println("REQUEST_PLAINTEXT");
}

void handleSerialData() {
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n') {
      String message = serialBuffer;
      message.trim();
      serialBuffer = "";
      
      if (message.length() > 0) {
        if (message.startsWith("LATEST:")) {
          parseLatestPost(message.substring(7));
        } 
        else if (message.startsWith("PLAINTEXT:")) {
          parsePlainText(message.substring(10));
        }
      }
    } 
    else if (c >= 32) {  // Accept printable characters
      serialBuffer += c;
    }
  }
}

void parseLatestPost(String data) {
  // Format: subreddit|title|author|score
  int pipe1 = data.indexOf('|');
  int pipe2 = data.indexOf('|', pipe1 + 1);
  int pipe3 = data.indexOf('|', pipe2 + 1);
  
  if (pipe1 > 0 && pipe2 > 0 && pipe3 > 0) {
    postSubreddit = data.substring(0, pipe1);
    postTitle = data.substring(pipe1 + 1, pipe2);
    postAuthor = data.substring(pipe2 + 1, pipe3);
    postScore = data.substring(pipe3 + 1).toInt();
    
    drawLatestPost();
  }
}

void parsePlainText(String data) {
  postFullText = data;
  postFullText.replace(" [NL] ", "\n");  // Convert back to newlines
  
  scrollOffset = 0;
  contentHeight = 0;
  drawFullTextScrollable();
}

void drawWelcomeScreen() {
  tft.fillScreen(REDDIT_BLACK);
  
  tft.fillRect(0, 0, tft.width(), HEADER_HEIGHT, REDDIT_ORANGE);
  tft.setTextColor(REDDIT_WHITE, REDDIT_ORANGE);
  tft.setTextSize(2);
  tft.setCursor(10, 8);
  tft.println("Reddit Display");
  
  tft.setTextColor(REDDIT_WHITE, REDDIT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, 60);
  tft.println("Connecting...");
  
  tft.setTextColor(REDDIT_GRAY, REDDIT_BLACK);
  tft.setCursor(10, 80);
  tft.println("Waiting for posts from Go");
}

void drawLatestPost() {
  tft.fillScreen(REDDIT_BLACK);
  
  // Subreddit header
  tft.fillRect(0, 0, tft.width(), HEADER_HEIGHT, REDDIT_ORANGE);
  tft.setTextColor(REDDIT_WHITE, REDDIT_ORANGE);
  tft.setTextSize(2);
  tft.setCursor(PADDING, 8);
  tft.printf("r/%s", postSubreddit.c_str());
  
  int yPos = HEADER_HEIGHT + PADDING + 5;
  
  // Title
  tft.setTextColor(REDDIT_WHITE, REDDIT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(PADDING, yPos);
  
  // Simple word wrap for title
  int maxChars = 25;  // ~25 chars per line at size 2
  int pos = 0;
  while (pos < postTitle.length()) {
    String line = postTitle.substring(pos, min(pos + maxChars, (int)postTitle.length()));
    tft.println(line);
    pos += maxChars;
    if (pos < postTitle.length()) {
      tft.setCursor(PADDING, tft.getCursorY());
    }
  }
  
  yPos = tft.getCursorY() + 10;
  
  // Author
  tft.setTextColor(REDDIT_GRAY, REDDIT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(PADDING, yPos);
  tft.printf("u/%s", postAuthor.c_str());
  yPos += 18;
  
  // Score
  uint16_t scoreColor = (postScore >= 0) ? REDDIT_UPVOTE : REDDIT_DOWNVOTE;
  tft.setTextColor(scoreColor, REDDIT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(PADDING, yPos);
  
  if (postScore >= 0) {
    tft.printf("^ %d pts", postScore);
  } else {
    tft.printf("v %d pts", abs(postScore));
  }
  
  // Bottom hint
  tft.setTextColor(REDDIT_GRAY, REDDIT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(PADDING, tft.height() - 15);
  tft.print("BOOT: read full text");
}

void drawFullTextScrollable() {
  // Clear main screen and draw header
  tft.fillScreen(REDDIT_BLACK);
  tft.fillRect(0, 0, tft.width(), HEADER_HEIGHT, REDDIT_ORANGE);
  tft.setTextColor(REDDIT_WHITE, REDDIT_ORANGE);
  tft.setTextSize(2);  // Bigger text for header
  tft.setCursor(PADDING, 8);
  tft.printf("r/%s", postSubreddit.c_str());
  
  // Get content
  String content = postFullText;
  if (content.length() == 0) {
    content = postTitle;
  }
  
  // Debug: print content to serial
  Serial.printf("Drawing content (%d chars): %s\n", content.length(), content.substring(0, 50).c_str());
  
  // Calculate total content height if not done yet
  if (contentHeight == 0) {
    contentHeight = calculateContentHeight(content, 2);  // Size 2 text
    int visibleHeight = tft.height() - HEADER_HEIGHT - 15;
    maxScrollOffset = max(0, contentHeight - visibleHeight);
    Serial.printf("Content height: %d, Max scroll: %d\n", contentHeight, maxScrollOffset);
  }
  
  // Draw directly to screen instead of sprite (simpler)
  int yPos = HEADER_HEIGHT + PADDING - scrollOffset;
  
  tft.setTextColor(REDDIT_WHITE, REDDIT_BLACK);
  tft.setTextSize(2);  // Bigger text for readability
  tft.setCursor(PADDING, yPos);
  
  // Simple word wrapping
  int maxChars = 25;  // chars per line for size 2
  int currentPos = 0;
  
  while (currentPos < content.length()) {
    // Find end of current line
    int lineEnd = currentPos + maxChars;
    if (lineEnd >= content.length()) {
      lineEnd = content.length();
    } else {
      // Try to break at space
      while (lineEnd > currentPos && content.charAt(lineEnd) != ' ' && content.charAt(lineEnd) != '\n') {
        lineEnd--;
      }
      if (lineEnd == currentPos) {
        lineEnd = currentPos + maxChars; // Force break
      }
    }
    
    // Extract line
    String line = content.substring(currentPos, lineEnd);
    line.trim();
    
    // Check if we're in visible area
    if (yPos >= HEADER_HEIGHT && yPos < tft.height() - 20) {
      tft.setCursor(PADDING, yPos);
      tft.print(line);
    }
    
    // Handle newlines in content
    if (lineEnd < content.length() && content.charAt(lineEnd) == '\n') {
      yPos += 20; // Extra space for paragraph break
      currentPos = lineEnd + 1;
    } else {
      currentPos = lineEnd + 1;
    }
    
    yPos += 18; // Line height for size 2
  }
  
  // Draw scroll indicator
  if (maxScrollOffset > 0) {
    tft.setTextColor(REDDIT_GRAY, REDDIT_BLACK);
    tft.setTextSize(1);
    tft.fillRect(0, tft.height() - 15, tft.width(), 15, REDDIT_BLACK);
    tft.setCursor(PADDING, tft.height() - 12);
    
    if (scrollOffset < maxScrollOffset) {
      tft.printf("BOOT: scroll (%d%%)", (scrollOffset * 100) / maxScrollOffset);
    } else {
      tft.print("BOOT: back to posts");
    }
  }
}

int calculateContentHeight(String text, int textSize) {
  int charsPerLine = (textSize == 2) ? 25 : 50;
  int lineHeight = (textSize == 2) ? 18 : 12;
  
  int totalChars = text.length();
  int lines = (totalChars / charsPerLine) + 1;
  
  // Add extra lines for actual newlines in text
  for (int i = 0; i < text.length(); i++) {
    if (text.charAt(i) == '\n') {
      lines += 2; // Extra space for paragraph breaks
    }
  }
  
  return lines * lineHeight + PADDING * 2;
}