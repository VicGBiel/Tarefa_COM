# Projeto: Controle de LED e Display utilizando interfaces de comunicação serial (UART e I2C)

Este projeto implementa o controle de uma matriz de LEDs WS2812 e um display OLED SSD1306 utilizando um Raspberry Pi Pico W. A comunicação com os dispositivos é feita via UART (Serial Monitor) e I2C, permitindo a entrada de caracteres e números para exibição no display e na matriz de LEDs.

## Funcionalidades

- Exibição de caracteres no display SSD1306 via comunicação I2C.
- Controle de uma matriz de LEDs WS2812 via barramento de comunicação PIO.
- Resposta a entradas via comunicação serial UART no Serial Monitor do VS Code.
- Resposta a botões físicos para controle de LEDs indicadores.
- Reinício do Raspberry Pi Pico W via botão do joystick.

## Componentes Utilizados

- Raspberry Pi Pico
- Matriz de LEDs WS2812 (5x5)
- Display OLED SSD1306
- Botões físicos para interação
- Joystick com botão integrado

## Ligações

- **Matriz WS2812**: Conectada ao pino GPIO7
- **Display SSD1306 (I2C)**: SDA no GPIO14, SCL no GPIO15
- **Botão A**: GPIO5
- **Botão B**: GPIO6
- **Joystick (Botão)**: GPIO22
- **LED Verde**: GPIO11
- **LED Azul**: GPIO12

## Instalação e Uso

1. Compile e carregue o código na BitDogLab.
2. Conecte ao Serial Monitor do VS Code.
3. Digite um número de 0 a 9 para exibição na matriz WS2812.
4. Digite um caractere para exibição no display SSD1306.
5. Utilize os botões físicos para alternar os LEDs indicadores.
6. Pressione o botão do joystick para reiniciar no modo bootloader.

Autor: Victor Gabriel Guimarães Lopes

