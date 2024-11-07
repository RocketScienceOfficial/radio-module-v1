#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "lib/lora/sx127X_driver.h"
#include <stdio.h>

#define SPI spi0
#define CS 2
#define MISO 4
#define MOSI 3
#define SCK 6
#define RESET 7
#define SPI_FREQUENCY 100000

#define UART uart1
#define RX 9
#define TX 8
#define UART_BAUDRATE 115200

#define LED_UP_TIME_MS 20
#define LED_RX_UART_PIN 28
#define LED_RX_RADIO_PIN 5

int main()
{
    stdio_init_all();
    sleep_ms(1000);

    printf("Initialized board!\n");
    printf("Firmware version: 1.0\n");
    printf("Author: Filip Gawlik\n");

    spi_init(SPI, SPI_FREQUENCY);
    uart_init(UART, UART_BAUDRATE);
    uart_set_fifo_enabled(UART, true);

    gpio_init(MISO);
    gpio_set_dir(MISO, true);
    gpio_set_function(MISO, GPIO_FUNC_SPI);

    gpio_init(MOSI);
    gpio_set_dir(MOSI, true);
    gpio_set_function(MOSI, GPIO_FUNC_SPI);

    gpio_init(SCK);
    gpio_set_dir(SCK, true);
    gpio_set_function(SCK, GPIO_FUNC_SPI);

    gpio_init(RX);
    gpio_set_dir(RX, true);
    gpio_set_function(RX, GPIO_FUNC_UART);

    gpio_init(TX);
    gpio_set_dir(TX, true);
    gpio_set_function(TX, GPIO_FUNC_UART);

    gpio_init(LED_RX_UART_PIN);
    gpio_set_dir(LED_RX_UART_PIN, true);
    gpio_put(LED_RX_UART_PIN, 0);

    gpio_init(LED_RX_RADIO_PIN);
    gpio_set_dir(LED_RX_RADIO_PIN, true);
    gpio_put(LED_RX_RADIO_PIN, 0);

    sx127x_config_t loraData = {};
    sx127x_init(&loraData, SPI, CS, RESET, 433000000);
    sx127x_set_signal_bandwidth(&loraData, 125000);
    sx127x_set_spreading_factor(&loraData, 8);

    uint8_t curSize = 0;
    uint8_t recvBuffer[64];
    uint8_t bufLen = 0;

    while (true)
    {
        while (uart_is_readable(UART))
        {
            uint8_t byte;
            uart_read_blocking(UART, &byte, 1);

            if (curSize == 0)
            {
                if (byte <= sizeof(recvBuffer))
                {
                    curSize = byte;

                    gpio_put(LED_RX_UART_PIN, 1);

                    printf("Received request for %d bytes\n", curSize);
                }
                else
                {
                    printf("Received invalid number of bytes (%d)\n", byte);
                }
            }
            else
            {
                recvBuffer[bufLen++] = byte;

                if (bufLen == curSize)
                {
                    printf("All bytes received!\n");

                    sx127x_write_buffer(&loraData, recvBuffer, bufLen);

                    printf("Packet sent!\n");

                    gpio_put(LED_RX_UART_PIN, 0);

                    curSize = 0;
                    bufLen = 0;
                }
            }
        }

        size_t packetSize = sx127x_parse_packet(&loraData, 0);

        if (packetSize > 0)
        {
            gpio_put(LED_RX_RADIO_PIN, 1);

            printf("Received packet with size: %d\n", packetSize);

            uint8_t buffer[256];
            uint8_t i = 0;

            while (sx127x_available(&loraData))
            {
                if (i == packetSize)
                {
                    printf("Something went wrong while parsing packet!");

                    break;
                }

                if (i < sizeof(buffer))
                {
                    buffer[i++] = sx127x_read(&loraData);
                }
                else
                {
                    printf("Buffer overflow while parsing packet!");

                    break;
                }
            }

            uart_write_blocking(UART, &i, 1);
            uart_write_blocking(UART, buffer, i);

            gpio_put(LED_RX_RADIO_PIN, 0);
        }
    }
}