#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

#include <hardware/gpio.h>
#include <hardware/uart.h>
#include <pico/stdio_usb.h>
#include <stdio.h>
#include "hardware/adc.h"
#include "pico/stdlib.h"

#include "hardware/adc.h"
#include <vector>
const uint LED_PIN = 25;
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "pico/time.h"

/*   GPIO 4 (pin 6)-> SDA on LCD bridge board
   GPIO 5 (pin 7)-> SCL on LCD bridge board
   3.3v (pin 36) -> VCC on LCD bridge board
   GND (pin 38)  -> GND on LCD bridge board
*/
// commands
const int LCD_CLEARDISPLAY = 0x01;
const int LCD_RETURNHOME = 0x02;
const int LCD_ENTRYMODESET = 0x04;
const int LCD_DISPLAYCONTROL = 0x08;
const int LCD_CURSORSHIFT = 0x10;
const int LCD_FUNCTIONSET = 0x20;
const int LCD_SETCGRAMADDR = 0x40;
const int LCD_SETDDRAMADDR = 0x80;
// flags for display entry mode
const int LCD_ENTRYSHIFTINCREMENT = 0x01;
const int LCD_ENTRYLEFT = 0x02;

// flags for display and cursor control
const int LCD_BLINKON = 0x01;
const int LCD_CURSORON = 0x02;
const int LCD_DISPLAYON = 0x04;

// flags for display and cursor shift
const int LCD_MOVERIGHT = 0x04;
const int LCD_DISPLAYMOVE = 0x08;

// flags for function set
const int LCD_5x10DOTS = 0x04;
const int LCD_2LINE = 0x08;
const int LCD_8BITMODE = 0x10;

// flag for backlight control
const int LCD_BACKLIGHT = 0x08;

const int LCD_ENABLE_BIT = 0x04;

// By default these LCD display drivers are on bus address 0x27
static int addr = 0x27;

// Modes for lcd_send_byte
#define LCD_CHARACTER  1
#define LCD_COMMAND    0

#define MAX_LINES      2
#define MAX_CHARS      16

// DATA TYPES
#define NUM_SAMPLES 50
#define NUM_SAMPLE 1000
float features[NUM_SAMPLE];
    uint32_t start_time, end_time, peak_time;
    float bpm;
    int peak1 = 0;
    int peak2 = 0;
    uint32_t peak_times[NUM_SAMPLES];
    uint32_t sum_peak_time = 0;
    float avg_bpm;
/* Quick helper function for single byte transfers */
void i2c_write_byte(uint8_t val) {
#ifdef i2c_default
    i2c_write_blocking(i2c_default, addr, &val, 1, false);
#endif
}

void lcd_toggle_enable(uint8_t val) {
    // Toggle enable pin on LCD display
    // We cannot do this too quickly or things don't work
#define DELAY_US 600
    sleep_us(DELAY_US);
    i2c_write_byte(val | LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
    i2c_write_byte(val & ~LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
}

// The display is sent a byte as two separate nibble transfers
void lcd_send_byte(uint8_t val, int mode) {
    uint8_t high = mode | (val & 0xF0) | LCD_BACKLIGHT;
    uint8_t low = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;

    i2c_write_byte(high);
    lcd_toggle_enable(high);
    i2c_write_byte(low);
    lcd_toggle_enable(low);
}

void lcd_clear(void) {
    lcd_send_byte(LCD_CLEARDISPLAY, LCD_COMMAND);
}

// go to location on LCD
void lcd_set_cursor(int line, int position) {
    int val = (line == 0) ? 0x80 + position : 0xC0 + position;
    lcd_send_byte(val, LCD_COMMAND);
}

static void inline lcd_char(char val) {
    lcd_send_byte(val, LCD_CHARACTER);
}

void lcd_string(const char *s) {
    while (*s) {
        lcd_char(*s++);
    }
}

void lcd_init() {
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x02, LCD_COMMAND);

    lcd_send_byte(LCD_ENTRYMODESET | LCD_ENTRYLEFT, LCD_COMMAND);
    lcd_send_byte(LCD_FUNCTIONSET | LCD_2LINE, LCD_COMMAND);
    lcd_send_byte(LCD_DISPLAYCONTROL | LCD_DISPLAYON, LCD_COMMAND);
    lcd_clear();
}

int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
  memcpy(out_ptr, features + offset, length * sizeof(float));
  return 0;
}

void print_inference_result(ei_impulse_result_t result) {
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    lcd_init();
    lcd_clear();
    // Print how long it took to perform inference
lcd_set_cursor(1, 0); // Move to the second line
ei_printf("Timing: DSP %d ms, inference %d ms \r\n",
            result.timing.dsp,
            result.timing.classification);
    // Print the prediction results (classification)
    ei_printf("Predictions:\r\n");
    lcd_string("Predictions \r\n");
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        ei_printf("  %s: ", ei_classifier_inferencing_categories[i]);
        ei_printf("%.5f\r\n", result.classification[i].value);
    }
    if (result.classification[0].value > .50)
    {
      ei_printf("You need to Consult Your Doctor...");
      lcd_clear();
      lcd_string("\nConsult Doctor");
        lcd_set_cursor(1, 0); // Move to the second line
        char str_bpm[16]; // Max number of digits in a float is 7, plus the null terminator
        sprintf(str_bpm, "heart rate: %.2f", avg_bpm); // Format the heart rate value with 2 decimal places
        lcd_string(str_bpm);
    }
    else
    {
      ei_printf("Everything is fine.....No worries");
      lcd_clear();
      lcd_string("\nNo worries");
        lcd_set_cursor(1, 0); // Move to the second line
        char str_bpm[16]; // Max number of digits in a float is 7, plus the null terminator
        sprintf(str_bpm, "heart rate: %.2f", avg_bpm); // Format the heart rate value with 2 decimal places
        lcd_string(str_bpm);
    }
    
}

int main() {
    printf("Calculating average peak time and beats per minute...\n");
    while(1)
 {
    stdio_usb_init();
 // data from ADC
    stdio_init_all();
    printf("ADC Example, measuring GPIO26\n");
    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(26);
    // Select ADC input 0 (GPIO26)
    adc_select_input(0);
    for (int i = 0; i < 1000; i++) {
            const float conversion_factor = 3.3f / (1 << 12);
        uint16_t result = adc_read();
        const float Voltage = result * conversion_factor;
        printf("%d\n", result);
        features[i]=Voltage;
        sleep_ms(1);
    }
    for (int i = 0; i < NUM_SAMPLES; i++) {
        start_time = to_ms_since_boot(get_absolute_time());
        while (peak1 <= 80) {
            peak1 = adc_read();
            sleep_ms(1);
        }
        sleep_ms(1);
        while (peak2 <= 80) {
            peak2 = adc_read();
            sleep_ms(1);
        }
        end_time = to_ms_since_boot(get_absolute_time());
        peak_time = (end_time - start_time);
        peak_times[i] = peak_time;
        sum_peak_time += peak_time;
        //Reset
        peak1 = 0;
        peak2 = 0;
    }
        peak_time = (end_time - start_time);
        printf("Peak time %.2f\n", peak_time);
        
          printf("Peak time values:\n");
    for (int i = 0; i < NUM_SAMPLES; i++) {
        printf("%d\n", peak_times[i]);
    }

    float avg_peak_time = (float)sum_peak_time / NUM_SAMPLES;
    printf("Average peak time: %.2f ms\n", avg_peak_time);

    avg_bpm = 60000.0 / avg_peak_time; // avg_bpm = 60/peak time in ms OR 60000/peak time 
                                      // multiplying by 60 is because to convert beat per sec to beats per minute
    printf("Average beats per minute: %.2f\n", avg_bpm);
    // END ADC
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    ei_impulse_result_t result = {nullptr};

    ei_printf("Edge Impulse standalone inferencing (Raspberry Pi Pico)\n");

    if (sizeof(features) / sizeof(float) != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        ei_printf("The size of your 'features' array is not correct. Expected %d items, but had %u\n",
        EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, sizeof(features) / sizeof(float));
        return 1;
    }
        // blink LED
        gpio_put(LED_PIN, !gpio_get(LED_PIN));
        // the features are stored into flash, and we don't want to load everything into RAM
        signal_t features_signal;
        features_signal.total_length = sizeof(features) / sizeof(features[0]);
        features_signal.get_data = &raw_feature_get_data;

        // invoke the impulse
        EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false /* debug */);
        if (res != EI_IMPULSE_OK) {
            ei_printf("ERR: Failed to run classifier (%d)\n", res);
            return res;
        }
       
    // This example will use I2C0 on the default SDA and SCL pins (4, 5 on a Pico)

        print_inference_result(result);
        ei_sleep(2000);
}
}