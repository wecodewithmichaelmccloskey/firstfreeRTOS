#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Settings
static const uint8_t buf_len = 255;
static const char command[] = "delay ";
static const uint8_t delay_queue_len = 5;
static const uint8_t msg_queue_len = 5;
static const uint8_t blink_max = 100;

// Pins
static const int led_pin = LED_BUILTIN;

// Message struct: used to wrap stings
typedef struct Message {
  char body[20];
  int count;
} Message;

// Globals
static QueueHandle_t delay_queue;
static QueueHandle_t msg_queue;

//*********************************************************
// Tasks

// Task: command line interface (CLI)
void doCLI(void *parameter) {
  
  Message rcv_msg;
  char c;
  char buf[buf_len];
  uint8_t idx = 0;
  uint8_t cmd_len = strlen(command);
  int led_delay;

  // Clear whole buffer
  memset(buf, 0, buf_len);

  // Loop forever
  while (1) {

    // See if there's a message in the queue
    if (xQueueReceive(msg_queue, (void *)&rcv_msg, 0) == pdTRUE) {
      Serial.print(rcv_msg.body);
      Serial.println(rcv_msg.count);
    }

    // Read characters from serial
    if (Serial.available() > 0) {
      c = Serial.read();

      // Store received character to buffer if not over buffer limit
      if (idx < buf_len - 1) {
        buf[idx] = c;
        idx++;
      }

      // Print newline and check input on 'enter'
      if ((c == '\n') || (c == '\r')) {

        // Print newline to terminal
        Serial.print("\r\n");

        // Check if the first 6 characters are "delay "
        if (memcmp(buf, command, cmd_len) == 0) {

          // Convert last part to postive integer (negative int crashes)
          char* tail = buf + cmd_len;
          led_delay = atoi(tail);
          led_delay = abs(led_delay);

          // Send interger to other task via queue
          if (xQueueSend(delay_queue, (void *)&led_delay, 10) != pdTRUE) {
            Serial.println("ERROR: Could not put item on delay queue.");
          }
        }

        // Reset recieve buffer and index counter
        memset(buf, 0, buf_len);
        idx = 0;

      // Otherwise, echo character back to serial terminal
      } else {
        Serial.print(c);
      }
    }
  }
}

// Task: flash LED based on delay provided, notify other task every 100 blinks
void blinkLED(void *parameters) {
  
  Message msg;
  int led_delay = 500;
  uint8_t counter = 0;

  // Set up pin
  pinMode(LED_BUILTIN, OUTPUT);

  // Loop forever
  while (1) {

    // See if there's a message in the queue (do not block)
    if (xQueueReceive(delay_queue, (void *)&led_delay, 0) == pdTRUE) {

      // Best practice: use only one task to manage serial comms
      strcpy(msg.body, "Message recieved ");
      msg.count = 1;
      xQueueSend(msg_queue, (void *)&msg, 10);
    }

    // Blink
    digitalWrite(led_pin, HIGH);
    vTaskDelay(led_delay / portTICK_PERIOD_MS);
    digitalWrite(led_pin, LOW);
    vTaskDelay(led_delay / portTICK_PERIOD_MS);

    // If we've blinked 100 times, send a message to the other task
    counter++;
    if (counter >= blink_max) {

      // Construct message and send
      strcpy(msg.body, "Blinked: ");
      msg.count = counter;
      xQueueSend(msg_queue, (void *)&msg, 10);

      // Reset counter
      counter = 0;
    }
  }
}

//**********************************************************************
// Main (runs as its own task with priority q on core 1)

void setup() {

  // Configure Serial
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Queue Solution---");
  Serial.println("Enter the command 'delay xxx' where xxx is your desired ");
  Serial.println("LED blink delay time in milliseconds");

  // Create queues
  delay_queue = xQueueCreate(delay_queue_len, sizeof(int));
  msg_queue = xQueueCreate(msg_queue_len, sizeof(Message));

  // Start CLI task
  xTaskCreatePinnedToCore(
    doCLI,
    "CLI",
    1500,
    NULL,
    1,
    NULL,
    app_cpu);

  // Start blink task
  xTaskCreatePinnedToCore(
    blinkLED,
    "Blink LED",
    1024,
    NULL,
    1,
    NULL,
    app_cpu);

  // Delete "setup and loop" task
  vTaskDelete(NULL);
}

void loop() {
  // Execution should never get here
}
