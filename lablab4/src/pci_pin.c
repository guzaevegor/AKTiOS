#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#define ROM_BASE_ADDRESS_REGISTER 0x30
#define INTERRUPT_LINE_REGISTER 0x3C
#define INTERRUPT_PIN_REGISTER 0x3D
#define HEADER_TYPE_REGISTER 0x0E

// Функция чтения 32-битного регистра конфигурационного пространства
uint32_t pci_read_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC);
    outl(address, PCI_CONFIG_ADDRESS);
    return inl(PCI_CONFIG_DATA);
}

// Функция чтения 8-битного регистра конфигурационного пространства
uint8_t pci_read_config_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC);
    outl(address, PCI_CONFIG_ADDRESS);
    return inb(PCI_CONFIG_DATA + (offset & 3));
}

int main() {
    // Устанавливаем доступ к портам ввода-вывода 0xCF8 и 0xCFC (доступ на 8 байт)
    if (ioperm(PCI_CONFIG_ADDRESS, 8, 1)) {
        printf("Ошибка доступа к портам: %s\n", strerror(errno));
        return 1;
    }

    printf("Сканирование PCI шин...\n");
    
    for (uint8_t bus = 0; bus < 256; ++bus) {
        for (uint8_t device = 0; device < 32; ++device) {
            for (uint8_t function = 0; function < 8; ++function) {
                // Читаем Header Type для определения типа устройства
                uint8_t header_type = pci_read_config_byte(bus, device, function, HEADER_TYPE_REGISTER);
                
                // Проверка, что устройство существует
                if (header_type == 0xFF) {
                    continue; // Устройство отсутствует
                }
                
                // Задание 4: Работа с ROM для устройств, которые не являются мостами
                if ((header_type & 0x80) == 0) { // Устройство не является мостом
                    uint32_t rom_address = pci_read_config(bus, device, function, ROM_BASE_ADDRESS_REGISTER);
                    if (rom_address & 0x1) { // ROM активен
                        printf("Шина %d, устройство %d, функция %d: ROM базовый адрес = 0x%x\n", bus, device, function, rom_address & ~0x7FF);
                    } else {
                        printf("Шина %d, устройство %d, функция %d: ROM не активен\n", bus, device, function);
                    }

                    // Задание 7: Чтение и вывод Interrupt Line
                    uint8_t interrupt_line = pci_read_config_byte(bus, device, function, INTERRUPT_LINE_REGISTER);
                    printf("Interrupt Line: %d\n", interrupt_line);
                }

                // Задание 12: Работа с мостами
                if ((header_type & 0x80) != 0) { // Устройство является мостом
                    uint8_t interrupt_pin = pci_read_config_byte(bus, device, function, INTERRUPT_PIN_REGISTER);
                    uint8_t interrupt_line = pci_read_config_byte(bus, device, function, INTERRUPT_LINE_REGISTER);
                    
                    printf("Шина %d, устройство %d, функция %d: Interrupt Pin = %d, Interrupt Line = %d\n", bus, device, function, interrupt_pin, interrupt_line);
                }
            }
        }
    }

    // Отключаем доступ к портам
    if (ioperm(PCI_CONFIG_ADDRESS, 8, 0)) {
        printf("Ошибка отключения доступа к портам: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}
