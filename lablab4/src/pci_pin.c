#include <stdio.h>
#include <sys/io.h>
#include "pci_c_header.h"
#include <stdbool.h>

#define PCI_COUNT 256
#define DEVICES_COUNT 32
#define FUNCTION_COUNT 8
#define CONFIGURATION_PORT 0xCF8
#define DATA_PORT 0xCFC
#define BAR_REGISTERS_COUNT 6

// Функция для проверки, является ли устройство мостом
bool isBridge(const unsigned int address)
{
    unsigned int temp;
    outl(address + 0x0C, CONFIGURATION_PORT);
    temp = inl(DATA_PORT);
    temp = (temp >> 16);
    return temp > 0;
}

// Функция для отображения Interrupt Pin
void showInterruptPin(const unsigned int address)
{
    unsigned int temp;
    outl(address + 0x3C, CONFIGURATION_PORT);
    temp = inl(DATA_PORT);
    temp = (temp >> 8) & 0xFF;  // Извлечение значения Interrupt pin
    switch(temp)
    {
        case 0:
            puts("Interrupt pin: not used");
            break;
        case 1:
            puts("Interrupt pin: INTA#");
            break;
        case 2:
            puts("Interrupt pin: INTB#");
            break;
        case 3:
            puts("Interrupt pin: INTC#");
            break;
        case 4:
            puts("Interrupt pin: INTD#");
            break;
        default:
            puts("Interrupt pin: reserved");
            break;
    }
}

// Функция для отображения Interrupt Line
void showInterruptLine(const unsigned int address)
{
    unsigned int temp;
    outl(address + 0x3C, CONFIGURATION_PORT);
    temp = inl(DATA_PORT);
    temp = (temp >> 16) & 0xFF;  // Извлечение значения Interrupt line
    if(temp == 0)
    {
        puts("Interrupt line: not used");
    }
    else
    {
        printf("Interrupt line: %d\n", temp);
    }
}

// Функция для отображения базовых регистров ROM памяти
void showROMMemoryRegisters(const unsigned int address)
{
    unsigned int temp;
    // Предполагается, что регистр ROM располагается по смещению 0x30
    outl(address + 0x30, CONFIGURATION_PORT);
    temp = inl(DATA_PORT);
    printf("Базовый регистр ROM: 0x%X\n", temp);
    // Добавьте дополнительные расшифровки при необходимости
}

// Функция для отображения базовых регистров памяти
void showAddresses(const unsigned int address)
{
    unsigned int temp;
    for(int i = 0; i < BAR_REGISTERS_COUNT; i++)
    {
        unsigned int bar_address = address + (0x10 + i * 4);
        outl(bar_address, CONFIGURATION_PORT);
        temp = inl(DATA_PORT);
        if ((temp & 1) > 0)
        {
            temp = temp & ~0b11;
            printf("I/O Address: 0x%X\n", temp);
        }
        else
        {
            temp = temp & ~0b1111;
            if(temp > 0)
            {
                printf("Memory Address: 0x%X\n", temp);
            }
        }
    }
}

// Функция для отображения информации о производителе
void showVendor(unsigned int deviceRegister)
{
    const unsigned short vendorId = deviceRegister & 0xFFFF;
    for (int i = 0; i < PCI_VENTABLE_LEN; i++)
    {
        if (PciVenTable[i].VenId == vendorId)
        {
            printf("Vendor: %s\n", PciVenTable[i].VenFull);
            return;
        }
    }
    printf("Unknown Vendor\n");
}

// Функция для отображения информации об устройстве
void showDevice(const unsigned int deviceRegister)
{
    unsigned short deviceId = deviceRegister >> 16;
    for(int i = 0; i < PCI_DEVTABLE_LEN; i++)
    {
        if(PciDevTable[i].DevId == deviceId && PciDevTable[i].VenId == (deviceRegister & 0xFFFF))
        {
            printf("Device: %s\n", PciDevTable[i].ChipDesc);
            printf("Chip: %s\n", PciDevTable[i].Chip);
            return;
        }
    }
    printf("Unknown Device\n");
}

// Функция для обработки Class Code
void handleClassCode(const unsigned int address)
{
    unsigned int temp = address + 0x08;

    outl(temp, CONFIGURATION_PORT);
    temp = inl(DATA_PORT);

    const unsigned int baseClass = temp >> 24;
    const unsigned int subClass = (temp >> 16) & 0xFF;
    const unsigned int specificRegisterLevel = (temp >> 8) & 0xFF;
    printf("baseClass: %02X\n", baseClass);
    printf("subClass: %02X\n", subClass);
    printf("specificRegisterLevel: %02X\n", specificRegisterLevel);
    printf("Class code: 0x%X\n", temp);
}

// Функция для отображения полной информации об устройстве PCI
void showDeviceInfo(unsigned int bus, unsigned int device, unsigned int function, unsigned int vendorID, unsigned int deviceID)
{
    // Поиск производителя
    const char* vendorName = "Unknown Vendor";
    for(int i = 0; i < PCI_VENTABLE_LEN; i++)
    {
        if(PciVenTable[i].VenId == (unsigned short)vendorID)
        {
            vendorName = PciVenTable[i].VenFull;
            break;
        }
    }

    // Поиск устройства
    const char* deviceName = "Unknown Device";
    for(int i = 0; i < PCI_DEVTABLE_LEN; i++)
    {
        if(PciDevTable[i].DevId == (unsigned short)deviceID && PciDevTable[i].VenId == (unsigned short)vendorID)
        {
            deviceName = PciDevTable[i].ChipDesc;
            break;
        }
    }

    // Вывод информации
    printf("Адрес устройства: Bus %u, Device %u, Function %u\n", bus, device, function);
    printf("Vendor ID: 0x%04X\n", vendorID);
    printf("Device ID: 0x%04X\n", deviceID);
    printf("Производитель: %s\n", vendorName);
    printf("Устройство: %s\n", deviceName);
    printf("----------------------------------------\n");
}

// Функция для обработки каждого адреса устройства PCI
void handleAddress(const unsigned int bus, const unsigned int device, const unsigned int function)
{
    unsigned int address = 0;
    address += (1 << 31);         // Устанавливаем бит Enable
    address += (bus << 16);       // Номер шины
    address += (device << 11);    // Номер устройства
    address += (function << 8);   // Номер функции

    // Чтение Vendor ID и Device ID
    unsigned int vendorID, deviceID;
    outl(address, CONFIGURATION_PORT);
    unsigned int temp = inl(DATA_PORT);
    vendorID = temp & 0xFFFF;
    deviceID = (temp >> 16) & 0xFFFF;

    if(vendorID == 0xFFFF && deviceID == 0xFFFF)
    {
        // Устройство не существует
        return;
    }

    // Отображение информации о устройстве
    showDeviceInfo(bus, device, function, vendorID, deviceID);

    // Дополнительная обработка для мостов и других функций
    bool bridge = isBridge(address);
    printf("isBridge: %d\n", bridge);
    if (bridge)
    {
        handleClassCode(address);
        // Решение задачи 12
        showInterruptPin(address);
        showInterruptLine(address);
    }
    else
    {
        showAddresses(address);
        // Решение задач 2,4,6,7
        showInterruptPin(address);
        showROMMemoryRegisters(address); // Решение задачи 4
        showInterruptLine(address);      // Решение задачи 7
    }
    printf("======================\n");
}

// Функция для формирования цикла опроса и идентификации устройств PCI
void solveSecond()
{
    if (iopl(3) != 0)  // Устанавливаем I/O привилегии
    {
        printf("Ошибка изменения уровня привилегий I/O\n");
        return;
    }

    unsigned int address;
    for (unsigned int bus = 0; bus < PCI_COUNT; bus++)
    {
        for (unsigned int device = 0; device < DEVICES_COUNT; device++)
        {
            for (unsigned int function = 0; function < FUNCTION_COUNT; function++)
            {
                handleAddress(bus, device, function);
            }
        }
    }
}

// Главная функция
int main()
{
    solveSecond();
    return 0;
}
