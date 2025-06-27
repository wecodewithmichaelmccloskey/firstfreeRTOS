#if CONFIG_FREE_RTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Settings
#define buf_len 255

// Global
static volatile bool flag = false;
static char *msg = NULL;

//***************************************************************
// Tasks

// Task A
void readSerial(void *parameter) {
  char c;
  char buf[buf_len];
  uint8_t idx = 0;

  // Clear whole buffer
  memset(buf, 0, buf_len);

  while (1) {
    // Read characters from serial
    if (Serial.available() > 0) {
      c = Serial.read();

      // On newline character, store characters in heap, notify task B
      if (c == '\n') {
        if (flag == 0) {
          msg = (char*)pvPortMalloc(buf_len * sizeof(char));
  
          // prevent heap overflow
          if (msg == NULL) {
            Serial.println("Not enough heap");
          } else {
            // copy msg from buffer to heap
            for (int i = 0; i < buf_len; i++) {
              msg[i] = buf[i];
            }
          }
          // Notify other task that the message is ready
          flag = true;
        }
        
        memset(buf, 0, buf_len);
        idx = 0;
      } else {
        // Only append if index is not over message limit
        if (idx < buf_len - 1) {
          buf[idx] = c;
          idx++;
        }
      }
    }
  }
}

// Task B
void printSerial(void *parameter) {
  while (1) {
    // wait for flag to be set to print message
    if (flag) {
      Serial.println(msg);
      vPortFree(msg);
      msg = NULL;
      flag = false;
    }
  }
}

void setup() {
  
  // Configure serial and wait a second
  Serial.begin(115200);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println("Pass a Message Challenge");

  // Start listen task
  xTaskCreatePinnedToCore(
    readSerial,
    "Read Serial",
    1024,
    NULL,
    1,
    NULL,
    app_cpu);

  // Start print task
  xTaskCreatePinnedToCore(
    printSerial,
    "Print Serial",
    1024,
    NULL,
    1,
    NULL,
    app_cpu);
 
  // Delete "setup and loop" task
  vTaskDelete(NULL);
}

void loop() {
  // Execution never gets here
}
