#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Globals
static int shared_var = 0;
static SemaphoreHandle_t mutex;

//*****************************************************************
// Tasks

// Increment shared variable (the wrong way)
void incTask(void *paramenters) {

  int local_var;

  // Loop forever
  while (1) {

    // Take mutex prior to critical section
    if (xSemaphoreTake(mutex, 0) == pdTRUE) {
      // Roundabout way to do "shared_var++" randomly and poorly
      local_var = shared_var;
      local_var++;
      vTaskDelay(random(100, 500) / portTICK_PERIOD_MS);
      shared_var = local_var;

      // Give mutex after critical section
      xSemaphoreGive(mutex);
  
      //Print out new shared variable
      Serial.println(shared_var);
    } else {
      // Do something else
    }
  }
}

void setup() {
  
  // Hack to kinda get randomness
  randomSeed(analogRead(0));

  // Configure Serial
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Race Condition Demo---");

  // Create mutex before starting tasks
  mutex = xSemaphoreCreateMutex();

  // Start task 1
  xTaskCreatePinnedToCore(
    incTask,
    "Increment Task 1",
    1024,
    NULL,
    1,
    NULL,
    app_cpu);

  xTaskCreatePinnedToCore(
    incTask,
    "Increment Task 2",
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
