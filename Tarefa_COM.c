#include <stdio.h>   // Funções de entrada e saída padrão (ex.: printf, usado para mensagens no console)
#include "pico/stdlib.h"  // Funções padrão do Raspberry Pi Pico (GPIO, temporização, inicialização)
#include "hardware/pio.h"  // Controle do PIO (Programmable Input/Output, usado para os LEDs WS2812)
#include "pico/bootrom.h"  // Funções relacionadas ao bootloader (ex.: reset_usb_boot para reinício no modo bootloader)
#include "ws2812.pio.h"  // Programa PIO e inicialização para controlar os LEDs WS2812
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"

// Definições de pinos e configurações do hardware
#define btn_a 5 // Definição do botão A
#define btn_b 6 // Definição do botão B
#define led_pin_green 11 //Definição do LED Verde
#define led_pin_blue 12 // Definição do LED Azul
#define IS_RGBW false // Define se os LEDs são RGBW ou apenas RGB
#define NUM_PIXELS 25 // Número de LEDs na matriz
#define WS2812_PIN 7 // Pino onde os LEDs WS2812 estão conectados
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define joy_btn 22

// Variáveis globais para armazenar a cor do LED
uint8_t led_r = 8; // Intensidade do vermelho
uint8_t led_g = 8;  // Intensidade do verde
uint8_t led_b = 8;  // Intensidade do azul

// Buffer para armazenar o estado de cada LED
bool led_buffer[NUM_PIXELS]= {}; // Inicializa o buffer de LEDs com zeros
bool cor = true;
bool g_led_on;
bool b_led_on;
ssd1306_t ssd; // Inicializa a estrutura do display

static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)

// Função para representar a cor em formato RGB
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

// Envia um pixel para o barramento WS2812
static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

// Protótipos das funções
void atualizaFita(uint8_t r, uint8_t g, uint8_t b);
void atualizaEstado(int estado);
void initGPIO();
static void gpio_irq_handler(uint gpio, uint32_t events);
void WS2812_setup();
void SSD1306_setup();
static void toggle_led(uint led_pin, bool *led_state, const char *message_on, const char *message_off);

// Função principal
int main() {
    stdio_init_all();
    initGPIO(); // Inicializa os pinos GPIO
    WS2812_setup(); // Configura o controlador de LEDs WS2812
    i2c_init(I2C_PORT, 400 * 1000); //Inicializa o i2c em 400kHz
    SSD1306_setup(); // Configura o display SSD1306

    // Configuração das interrupções dos botões
    gpio_set_irq_enabled_with_callback(btn_a, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(btn_b, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(joy_btn, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    
    while (true) {
        if (stdio_usb_connected()) {
            char c;
            if (scanf(" %c", &c) == 1) { // O espaço antes do %c evita captura de espaços em branco indesejados.
                ssd1306_fill(&ssd, !cor); // Limpa o display
                ssd1306_draw_char(&ssd, c, 8, 10);
                ssd1306_send_data(&ssd); // Atualiza o display
                
                // Se o caractere for um número de 0 a 9, exibir na matriz 5x5 WS2812
                if (c >= '0' && c <= '9') {
                    int num = c - '0'; // Converte o caractere numérico para um inteiro
                    atualizaEstado(num); 
                }
            }
        }
    }
}

// Inicializa os pinos GPIO
void initGPIO() {
    //inicializção do led verde
    gpio_init(led_pin_green);
    gpio_set_dir(led_pin_green, true);
    //inicializção do led azul
    gpio_init(led_pin_blue);
    gpio_set_dir(led_pin_blue, true);
    //inicialização do botão A
    gpio_init(btn_a);
    gpio_set_dir(btn_a, false);
    gpio_pull_up(btn_a);
    //inicialização do botão B
    gpio_init(btn_b);
    gpio_set_dir(btn_b, false);
    gpio_pull_up(btn_b);
    //inicialização do botão do joystick
    gpio_init(joy_btn);
    gpio_set_dir(joy_btn, false);
    gpio_pull_up(joy_btn);
}

// Acende/apaga os LEDS RGB e envia os dados para o display e para o monitor serial
static void toggle_led(uint led_pin, bool *led_state, const char *message_on, const char *message_off) {
    *led_state = !(*led_state);
    gpio_put(led_pin, *led_state);

    ssd1306_fill(&ssd, !cor); // Limpa o display
    ssd1306_draw_string(&ssd, *led_state ? message_on : message_off, 8, 10);
    ssd1306_send_data(&ssd); // Atualiza o display
    printf("%s\n", *led_state ? message_on : message_off);
}

// Tratador de interrupção dos botões
static void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    if (current_time - last_time > 200000) { // 200ms de debounce
        last_time = current_time;

        if (gpio == btn_a) {
            toggle_led(led_pin_green, &g_led_on, "LED Verde: ON", "LED Verde: OFF");
        } 
        else if (gpio == btn_b) {
            toggle_led(led_pin_blue, &b_led_on, "LED Azul: ON", "LED Azul: OFF");
        }
        else if (gpio == joy_btn){
            reset_usb_boot(0, 0);
        }
    }
}

// Atualiza todos os LEDs com base no buffer
void atualizaFita(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = urgb_u32(r, g, b); // Define a cor
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (led_buffer[i]) {
            put_pixel(color); // Liga o LED conforme o buffer
        } else {
            put_pixel(0);  // Desliga os LEDs
        }
    }
}

// Atualiza o buffer com base no número escolhido
void atualizaEstado(int estado) {

    switch(estado){
        case 0:         
            bool led_buffer_0 [] = { 
                1, 1, 1, 0, 0, 
                0, 0, 1, 0, 1, 
                1, 0, 1, 0, 0, 
                0, 0, 1, 0, 1, 
                1, 1, 1, 0, 0
            };
            memcpy(led_buffer, led_buffer_0, sizeof(led_buffer));
            atualizaFita(led_r, led_g, led_b);
            break;

        case 1:         
            bool led_buffer_1 [NUM_PIXELS] = {
                1, 0, 0, 0, 0, 
                0, 0, 0, 0, 1, 
                1, 0, 0, 0, 0, 
                0, 0, 0, 0, 1, 
                1, 0, 0, 0, 0
            };
            memcpy(led_buffer, led_buffer_1, sizeof(led_buffer));
            atualizaFita(led_r, led_g, led_b);
            break;

        case 2:         
            bool led_buffer_2 [NUM_PIXELS] = {
                1, 1, 1, 0, 0, 
                0, 0, 1, 0, 0, 
                1, 1, 1, 0, 0, 
                0, 0, 0, 0, 1, 
                1, 1, 1, 0, 0
            };
            memcpy(led_buffer, led_buffer_2, sizeof(led_buffer));
            atualizaFita(led_r, led_g, led_b);
            break;

        case 3:         
            bool led_buffer_3 [NUM_PIXELS] = {
                1, 1, 1, 0, 0, 
                0, 0, 0, 0, 1, 
                1, 1, 1, 0, 0, 
                0, 0, 0, 0, 1, 
                1, 1, 1, 0, 0
            };
            memcpy(led_buffer, led_buffer_3, sizeof(led_buffer));
            atualizaFita(led_r, led_g, led_b);
            break;

        case 4:         
            bool led_buffer_4 [NUM_PIXELS] = {
                1, 0, 0, 0, 0, 
                0, 0, 0, 0, 1, 
                1, 1, 1, 0, 0, 
                0, 0, 1, 0, 1, 
                1, 0, 1, 0, 0
            };
            memcpy(led_buffer, led_buffer_4, sizeof(led_buffer));
            atualizaFita(led_r, led_g, led_b);
            break;

        case 5:         
            bool led_buffer_5 [NUM_PIXELS] = {
                1, 1, 1, 0, 0, 
                0, 0, 0, 0, 1, 
                1, 1, 1, 0, 0, 
                0, 0, 1, 0, 0, 
                1, 1, 1, 0, 0
            };
            memcpy(led_buffer, led_buffer_5, sizeof(led_buffer));
            atualizaFita(led_r, led_g, led_b);
            break;

        case 6:         
            bool led_buffer_6 [NUM_PIXELS] = {
                1, 1, 1, 0, 0, 
                0, 0, 1, 0, 1, 
                1, 1, 1, 0, 0, 
                0, 0, 1, 0, 0, 
                1, 1, 1, 0, 0
            };
            memcpy(led_buffer, led_buffer_6, sizeof(led_buffer));
            atualizaFita(led_r, led_g, led_b);
            break;
        
        case 7:         
            bool led_buffer_7 [NUM_PIXELS] = {
                1, 0, 0, 0, 0, 
                0, 0, 0, 0, 1, 
                1, 0, 0, 0, 0, 
                0, 0, 0, 0, 1, 
                1, 1, 1, 0, 0
            };
            memcpy(led_buffer, led_buffer_7, sizeof(led_buffer));
            atualizaFita(led_r, led_g, led_b);
            break;

        case 8:         
            bool led_buffer_8 [NUM_PIXELS] = {
                1, 1, 1, 0, 0, 
                0, 0, 1, 0, 1, 
                1, 1, 1, 0, 0, 
                0, 0, 1, 0, 1, 
                1, 1, 1, 0, 0
            };
            memcpy(led_buffer, led_buffer_8, sizeof(led_buffer));
            atualizaFita(led_r, led_g, led_b);
            break;
        
        case 9:         
            bool led_buffer_9 [NUM_PIXELS] = {
                1, 1, 1, 0, 0, 
                0, 0, 0, 0, 1, 
                1, 1, 1, 0, 0, 
                0, 0, 1, 0, 1, 
                1, 1, 1, 0, 0
            };
            memcpy(led_buffer, led_buffer_9, sizeof(led_buffer));
            atualizaFita(led_r, led_g, led_b);
            break;
    }
}

// Configura a matriz de LEDS
void WS2812_setup(){
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
    atualizaFita(led_r, led_g, led_b);
}

// Configura o display SSD1306
void SSD1306_setup(){
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, 0x3C, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}