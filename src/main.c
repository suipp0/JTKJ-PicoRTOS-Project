// Miikka Rautio, Santeri Alapudas
// Ryhmän kolmas jäsen (Markku Koivumaa) ei suorittanut lopputyötä tänä vuonna
// Pisteiden jako: Miikka: 4p , Santeri: 2p
// Miikka toteutti molemmat tehtävät (sensor_task ja print_task), asentojen tunnistuksen, globaalin muuttujan current_symbol, sekä main funktion toteutusta
// Santeri toteutti keskeytyskäsittelijän, kaksi globaalia muuttujaa (button1_pressed, button2_pressed), sekä main funktion toteutusta
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"


#define DEFAULT_STACK_SIZE 2048
#define CDC_ITF_TX      1

enum state { WAITING=1};
enum state programState = WAITING;

// Globaalit muuttujat:
static volatile char current_symbol = '\0';
static volatile uint8_t button1_pressed = 0;
static volatile uint8_t button2_pressed = 0;

static void btn_fxn(uint gpio, uint32_t eventMask) {
    // Napin 1 painallus välitetään print_task-tehtävälle
    if (gpio  == BUTTON1) {
        button1_pressed = true;
    }
    // Napin 2 painallus välitetään print_task-tehtävälle
    else if (gpio == BUTTON2) {
        button2_pressed = true;
    }
}

static void sensor_task(void *arg) {
    (void)arg;
    float ax, ay, az, gx, gy, gz, t;
 
    for(;;){

        // Imu anturin sensoridatan lukeminen ja mittaaminen
        if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
            // Testituloksia varten printf-funktio
            //printf("Accel: X=%f, Y=%f, Z=%f | Gyro: X=%f, Y=%f, Z=%f| Temp: %2.2f°C\n", ax, ay, az, gx, gy, gz, t);

            // Tallentaa globaaliin muuttujaan pisteen, kun pico on vaakatasossa
            if (ax <= 0.5 && ay >= -0.5 && az >= 0.5) {
                current_symbol = '.';
            }
            // Tallentaa globaaliin muuttujaan viivan, kun pico on pystysuorassa
            else if (ax <= 0.5 && ay <= -0.5 && az <= 0.5) {
                current_symbol = '-';
            }
            // Tallentaa globaaliin muuttujaan välilyönnin, kun pico on kallistettu 90 astetta oikeaan
            else if (ax >= 0.5 && ay <= 0.5 && az <= 0.5) {
                current_symbol = ' ';
            }
            // Tallentaa globaaliin muuttujaan välilyönnin, kun pico on kallistettu 90 astetta vasempaan
            else if (ax <= -0.5 && ay <= 0.5 && az <= 0.5) {
                current_symbol = ' ';
           }
        }
        // Jos IMU:ta ei pystytä lukemaan, tulostetaan virheviesti
        else {
            printf("Failed to read imu data\n");
        }

        // Do not remove this
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void print_task(void *arg) {
    (void)arg;
    
    while(1) {
        // Tulostaa merkin riippuen napin 1 painalluksen aikaisesta asennosta
        if (button1_pressed == true) {
            printf("%c", current_symbol);
            button1_pressed = false;
        }
        // Nappia 2 painettaessa viesti päätetään kahteen välilyöntiin ja rivinvaihtomerkkiin
        else if (button2_pressed == true) {
            printf("  \n");
            button2_pressed = false;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
        // Do not remove this
        vTaskDelay(pdMS_TO_TICKS(1000));
}

int main() {
    // Alustetaan ohjelmaan liitetyt syöttö- ja tulostuskanavat.
    stdio_init_all();

    // Uncomment this lines if you want to wait till the serial monitor is connected
    /*while (!stdio_usb_connected()){
        sleep_ms(10);
    }*/ 
    
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.

    // Otetaan käyttöön painikkeiden nousevan reunan keskeytykset:
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_RISE, true, btn_fxn);
    gpio_set_irq_enabled_with_callback(BUTTON2, GPIO_IRQ_EDGE_RISE, true, btn_fxn);

    // Alustetaan ja käynnistetään IMU-anturi:
    init_ICM42670();
    ICM42670_start_with_default_values();
    
    // Alustetaan painikkeet:
    init_sw1();
    init_sw2();

    TaskHandle_t hSensorTask, hPrintTask, hUSB = NULL;

    // Exercise 4: Uncomment this xTaskCreate to create the task that enables dual USB communication.
    // Tehtävä 4: Poista tämän xTaskCreate-rivin kommentointi luodaksesi tehtävän,
    // joka mahdollistaa kaksikanavaisen USB-viestinnän.

    /*
    xTaskCreate(usbTask, "usb", 2048, NULL, 3, &hUSB);
    #if (configNUMBER_OF_CORES > 1)
        vTaskCoreAffinitySet(hUSB, 1u << 0);
    #endif
    */


    // Create the tasks with xTaskCreate
    BaseType_t result = xTaskCreate(sensor_task, // (en) Task function
                "sensor",                        // (en) Name of the task 
                DEFAULT_STACK_SIZE,              // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                            // (en) Arguments of the task 
                2,                               // (en) Priority of this task
                &hSensorTask);                   // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("Sensor task creation failed\n");
        return 0;
    }
    result = xTaskCreate(print_task,  // (en) Task function
                "print",              // (en) Name of the task 
                DEFAULT_STACK_SIZE,   // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                 // (en) Arguments of the task 
                2,                    // (en) Priority of this task
                &hPrintTask);         // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("Print Task creation failed\n");
        return 0;
    }
    // Start the scheduler (never returns)
    vTaskStartScheduler();
    
    // Never reach this line.
    return 0;
}
