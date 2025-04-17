/*
 * Пример Custom HID (Keyboard) на CH32V203F6P6
 * В файле usb_desc.h можно найти основные определения:
 * - USBD_VID - VID устройства
 * - USBD_PID - PID устройства
 * - USBD_LANGID_STRING - языковой идентификатор
 * - USBD_SIZE_REPORT_DESC - размер репорт десктриптора (данный дестриптор
 * описывает геймпад)
 * - CUSTOM_HID_BINTERVAL - интервал, с которым геймпад будет опрашиваться (в
 * мс) В файле usb_desc.c можно найти основные настройки устройства:
 * - const uint8_t USBD_DeviceDescriptor[USBD_SIZE_DEVICE_DESC] - USB Device
 * Descriptors
 * - const uint8_t USBD_ConfigDescriptor[USBD_SIZE_CONFIG_DESC] - USB
 * Configration Descriptors
 * - const uint8_t USBD_HidRepDesc[USBD_SIZE_REPORT_DESC] - HID Report
 * Descriptor(он описывает наш геймпад: сколько кнопок, осей, и т.д.)
 * - uint8_t StringVendor[64] - Кастомный iManufacturer
 * - uint8_t StringProduct[64] - Кастомный iProduct
 * - const uint8_t USBD_StringSerial[USBD_SIZE_STRING_SERIAL] - Серийный номер
 * устройства (см. стр. 53 Keyboard/Keypad
 * table)https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
 *
 */
#include "main.h"
#include "hw_config.h"
#include "stdio.h"
#include "usb_desc.h"
#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_pwr.h"
#include "usbd_compatibility_hid.h"

#define LED_ON  GPIOB->BSHR = GPIO_BSHR_BS1
#define LED_OFF GPIOB->BSHR = GPIO_BSHR_BR1
bool flag_led = false;
uint8_t Reset_cnt = 4;

extern uint8_t USBD_ENDPx_DataUp(uint8_t endp, uint8_t *pbuf, uint16_t len);

/*Структура нашего геймпада. В точности повторяет HID Report Descriptor*/
typedef struct __attribute__((packed)) {
    uint8_t Modifier_keys;
    uint8_t Reserved;
    uint8_t Keycode_1;
    uint8_t Keycode_2;
    uint8_t Keycode_3;
    uint8_t Keycode_4;
    uint8_t Keycode_5;
    uint8_t Keycode_6;

} USB_Custom_HID_Keyboard;

USB_Custom_HID_Keyboard Keyboard_data;

int main(void) {
    RVMSIS_Debug_init();              // Настройка дебага
    RVMSIS_RCC_SystemClock_144MHz();  // Настройка системной частоты
    RVMSIS_SysTick_Timer_init();      // Настройка системного таймера

    // Настройка GPIO
    RVMSIS_GPIO_init(GPIOB, 1, GPIO_GENERAL_PURPOSE_OUTPUT, GPIO_OUTPUT_PUSH_PULL, GPIO_SPEED_50_MHZ);  // LED
    RVMSIS_GPIO_init(GPIOA, 0, GPIO_INPUT, GPIO_INPUT_FLOATING, GPIO_SPEED_RESERVED);                   // KEY1
    RVMSIS_GPIO_init(GPIOA, 1, GPIO_INPUT, GPIO_INPUT_FLOATING, GPIO_SPEED_RESERVED);                   // KEY2
    RVMSIS_GPIO_init(GPIOA, 2, GPIO_INPUT, GPIO_INPUT_FLOATING, GPIO_SPEED_RESERVED);                   // KEY3
    RVMSIS_GPIO_init(GPIOA, 3, GPIO_INPUT, GPIO_INPUT_FLOATING, GPIO_SPEED_RESERVED);                   // KEY4

    Set_USBConfig();
    USB_Init();
    USB_Interrupts_Config();

    /*Инициализация структуры клавиатуры*/
    Keyboard_data.Modifier_keys = 0;
    Keyboard_data.Reserved = 0;
    Keyboard_data.Keycode_1 = 0x0;
    Keyboard_data.Keycode_2 = 0x0;
    Keyboard_data.Keycode_3 = 0x0;
    Keyboard_data.Keycode_4 = 0x0;
    Keyboard_data.Keycode_5 = 0x0;
    Keyboard_data.Keycode_6 = 0x0;

    while (1) {
        if (bDeviceState == CONFIGURED) {
            /*Keycode_1*/
            if (!READ_BIT(GPIOA->INDR, GPIO_INDR_IDR3)) {
                Keyboard_data.Modifier_keys = 0;
                Keyboard_data.Keycode_1 = 0x6;  // C
            } else {
                Keyboard_data.Modifier_keys = 0;
                Keyboard_data.Keycode_1 = 0;
            }

            /*Keycode_2*/
            if (!READ_BIT(GPIOA->INDR, GPIO_INDR_IDR2)) {
                Keyboard_data.Modifier_keys = 0;
                Keyboard_data.Keycode_2 = 0x17;  // T
            } else {
                Keyboard_data.Modifier_keys = 0;
                Keyboard_data.Keycode_2 = 0;
            }

            /*Keycode_3*/
            if (!READ_BIT(GPIOA->INDR, GPIO_INDR_IDR1)) {
                Keyboard_data.Modifier_keys = 0;
                Keyboard_data.Keycode_3 = 0x0C;  // I
            } else {
                Keyboard_data.Modifier_keys = 0;
                Keyboard_data.Keycode_3 = 0;
            }

            /*Keycode_4*/
            if (!READ_BIT(GPIOA->INDR, GPIO_INDR_IDR0)) {
                Keyboard_data.Modifier_keys = 0;
                Keyboard_data.Keycode_4 = 0x13;  // P
            } else {
                Keyboard_data.Modifier_keys = 0;
                Keyboard_data.Keycode_4 = 0;
            }

            /*Моргнем при активности*/
            uint8_t State = (uint8_t)(GPIOA->INDR);
            if (State < 0xF) {
                LED_ON;
            } else {
                Keyboard_data.Modifier_keys = 0;
                Keyboard_data.Reserved = 0;
                Keyboard_data.Keycode_1 = 0x0;
                Keyboard_data.Keycode_2 = 0x0;
                Keyboard_data.Keycode_3 = 0x0;
                Keyboard_data.Keycode_4 = 0x0;
                Keyboard_data.Keycode_5 = 0x0;
                Keyboard_data.Keycode_6 = 0x0;
                LED_OFF;
            }
            // Отправим в репорт сформированный буфер
            USBD_ENDPx_DataUp(ENDP2, (uint8_t *)&Keyboard_data, sizeof(Keyboard_data));
        } else {
            LED_ON;
            Delay_ms(500);
            LED_OFF;
            Delay_ms(500);

            if (Reset_cnt) {
                Reset_cnt--;
            } else {
                NVIC_SystemReset(); //Если ПК уже 5 секунд не видит устройство - перезагрузим МК(Костыль, т.к. после перезагрузки ПК - устройство видится в диспетчере устройств, но ничего не отправляет. Каждый раз передергивать USB надоело. А как исправить этот косяк - я так и не разобрался...)
            }
        }
    }
}
