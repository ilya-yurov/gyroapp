#include <linux/i2c-dev.h> //заговочный файл определяет макросы для i2c-устройств
#include <unistd.h>        //read/write
#include <string.h>        //strerror
#include <sys/ioctl.h>     //заголовочный файл описывает функции, взаимодействующие с файлами устройств (ioctl() и др.)
#include <fcntl.h>         //заголовочный файл описывает параметры управления файлом (функция open и др.) 
#include <stdlib.h>        //заголовочный файл определяет функцию exit, и тип данных size_t
#include <stdio.h>         //загаловочный файл определяет стандартные функции ввода/вывода
#include <stdint.h>        //заголовочный файл описывает целочисленные типы данных с установленными диапазонами представления чисел
#include <errno.h>         //заголовочный файл определяет макрос errno (значение типа int, которое интерпретируется как код последней ошибки)
#include "gyroapp.h"       //подключает заголовочный файл с адресами устройства и регистров устройства

int file = -1; //устанаваем переменную дескриптора файла

size_t i2c_write_configs (uint8_t reg_address, uint8_t value) //функция записи конфигураций в регистры гироскопа
{
	uint8_t buf[2] = {0};
	size_t result = 0;
	buf[0] = reg_address;
	buf[1] = value;
	result = write(file, buf, sizeof(buf));
	return result;
}
size_t i2c_write_byte (uint8_t reg_address) //функция записи байта в регистр оси устройства
{
	size_t result = 0;
	result = write(file, &reg_address, sizeof(reg_address));
	return result;
}
size_t i2c_read_byte(char *reg_value) //функция чтения байта из регистра оси устройства
{
	size_t result = 0;
	result = read(file, &reg_value, sizeof(reg_value));
	return result;
}

uint16_t merge_bytes( uint8_t LSB, uint8_t MSB)          //функция объединения младшего и страшего байтов
{                                                        //побитовый оператор & (И) обнуляет все разряды, кроме младших 8 
	return (uint16_t) (((LSB & 0xFF) << 8) | MSB);   //побитовый оператор << (сдвиг влево) сдвигает младшие биты на 8 разрядов влево (заполняет освободившиеся разряды нулями) 
}                                                        //побитовый оператор | (ИЛИ) устанавливает единицы в разрядах в соответствии со значением в MSB

int16_t two_complement_to_int( uint8_t LSB, uint8_t MSB) //функция приведения дополнительного кода к знаковому значению int16_t
{
	int16_t signed_int = 0;
	uint16_t word;

	word = merge_bytes(LSB, MSB);                    //сперва объединяем байты при помощи функции merge_bytes; на выходе получаем беззнаковое значение

	if((word & 0x8000) == 0x8000)                    //выполняем проверку на знак (если первый бит = 1, то число отрицательное, в противном случае - положительное)
	{
		signed_int = (int16_t) -(~word);         //явно присваиваем через унарный оператор (приведение) переменной тип int16_t
	}                                                //унарный оператор - устанавливает, что значение переменной отрицательное
	else                                             //побитовый унарный оператор ~ (побитовое отрицание) инвертирует биты беззнаковой переменной word,
	{                                                //что справедливо для отрицательного значения дополнительного кода
		signed_int = (int16_t) (word & 0x7fff);  //в противном случае число положительное; обнуляем все разряды, кроме необходимых младших пятнадцати
	}
	return signed_int;                               //возвращаем знаковую переменную, содержащую объединенные байты (старший и младший)
}

int main()
{
	const char * devName = "/dev/i2c-1"; //устанавливаем переменную, которую будем использовать в качестве первого аргумента для формирования дескриптора файла (для функции open())
	
	char gyro_x_h = 0; //регистр оси х гироскопа (старший байт)
	char gyro_x_l = 0; //регистр оси х гироскопа (младший байт)
        char gyro_y_h = 0; //регистр оси у гироскопа(старший байт)
        char gyro_y_l = 0; //регистр оси у гироскопа(младший байт)
        char gyro_z_h = 0; //регистр оси z гироскопа(старший байт)
        char gyro_z_l = 0; //регистр оси z гироскопа(младший байт)
	int16_t x_gyro_int = 0; //знаковая переменная, содержащая объединенные байты оси х гироскопа
	int16_t y_gyro_int = 0; //знаковая переменная, содержащая объединенные байты оси у гироскопа
	int16_t z_gyro_int = 0; //знаковая переменная, содержащая объединенные байты оси z гироскопа
	float x_gyro = 0; //переменная с плавающей точкой для определения конечного (фактического) значенния оси х гироскопа
	float y_gyro = 0; //переменная с плавающей точкой для определения конечного (фактического) значенния оси у гироскопа
	float z_gyro = 0; //переменная с плавающей точкой для определения конечного (фактического) значенния оси z гироскопа

	file = open(devName, O_RDWR); //при помощи open() присваиваем переменной file значение дескриптора файла (c флагом READ/WRITE)
	if (file == -1)               //выполняем проверку результата выполнения функции open() 
	{
		perror(devName);
		exit(1);
	}

	if (ioctl(file, I2C_SLAVE, MPU6050_I2C_ADDR) < 0) //при помощи ioctl() устанавливаем гироскоп как slave device (и делаем проверку результата работы)
	{
		printf("Failed to acquire bus access and/or talk to slave!\n");
		exit(1);
	}

	if (i2c_write_configs(PWR_MGMT_1, 0x01) == -1) //записываем в регистр power managment значение 0х01 (чтобы вывести устройство из "спящего режима")
	{                                              //выполняем проверку результата работы функции
		printf("Failed to write() byte to power management! %s\n", strerror(errno));
		exit(1);
	}
	if (i2c_write_configs(GYRO_CONFIG, 0x00) == -1) //записываем в регистр gyro config значение 0х009 (full scale range +/- 250 deg/sec)
	{                                               //выполняем проверку результата работы функции
		printf("Failed to write byte to gyro configuration! %s\n", strerror(errno));
                exit(1);
	}

	while(1) //запускаем цикл считывания показаний осей гироскопа
	{
		if (i2c_write_byte(REG_GYRO_XOUT_H) == -1) //записываем байт в старший регистр оси х гироскопа и выполняем проверку результата работы функции
		{
			printf("Something went wrong with write(xout_h)! %s\n", strerror(errno));
                        exit(1);
                }
		if (i2c_read_byte(&gyro_x_h) == -1)        //читаем байт из старшего регистра оси х гироскопа и выполняем проверку результата работы функции
		{
			printf("Something went wrong with read(xout_h)! %s\n", strerror(errno));
			exit(1);
		}
                if (i2c_write_byte(REG_GYRO_XOUT_L) == -1) //записываем байт в младший регистр оси х гироскопа и выполняем проверку результата работы функции
		{
                        printf("Something went wrong with write(xout_l)! %s\n", strerror(errno));
                        exit(1);
                }
                if (i2c_read_byte(&gyro_x_l) == -1)        //читаем байт из старшего регистра оси х гироскопа и выполняем проверку результата работы функции
		{
                        printf("Something went wrong with read(xout_l)! %s\n", strerror(errno));
                        exit(1);
                }
                if (i2c_write_byte(REG_GYRO_YOUT_H) == -1) //записываем байт в старший регистр оси у гироскопа и выполняем проверку результата работы функции
		{
                        printf("Something went wrong with write(yout_h)! %s\n", strerror(errno));
                        exit(1);
                }
                if (i2c_read_byte(&gyro_y_h) == -1)        //читаем байт из старшего регистра оси х гироскопа и выполняем проверку результата работы функции
		{
                        printf("Something went wrong with read(yout_h)! %s\n", strerror(errno));
                        exit(1);
                }
                if (i2c_write_byte(REG_GYRO_YOUT_L) == -1) //записываем байт в младший регистр оси у гироскопа и выполняем проверку результата работы функции
		{
                        printf("Something went wrong with write(yout_l)! %s\n", strerror(errno));
                        exit(1);
                }
                if (i2c_read_byte(&gyro_y_l) == -1)        //читаем байт из старшего регистра оси х гироскопа и выполняем проверку результата работы функции
		{
                        printf("Something went wrong with read(yout_l)! %s\n", strerror(errno));
                        exit(1);
                }
		if (i2c_write_byte(REG_GYRO_ZOUT_H) == -1) //записываем байт в старший регистр оси z гироскопа и выполняем проверку результата работы функции
		{
                        printf("Something went wrong with write(zout_h)! %s\n", strerror(errno));
                        exit(1);
                }
                if (i2c_read_byte(&gyro_z_h) == -1)        //читаем байт из старшего регистра оси х гироскопа и выполняем проверку результата работы функции
		{
                        printf("Something went wrong with read(zout_h)! %s\n", strerror(errno));
                        exit(1);
                }
                if (i2c_write_byte(REG_GYRO_ZOUT_L) == -1) //записываем байт в младший регистр оси z гироскопа и выполняем проверку результата работы функции
		{
                        printf("Something went wrong with write(zout_l)! %s\n", strerror(errno));
                        exit(1);
                }
                if (i2c_read_byte(&gyro_z_l) == -1)        //читаем байт из старшего регистра оси х гироскопа и выполняем проверку результата работы функции
		{
                        printf("Something went wrong with read(zout_l)! %s\n", strerror(errno));
                        exit(1);
                }

		x_gyro_int = two_complement_to_int(gyro_x_h, gyro_x_l); //объединяем старший и младший байты показаний оси х гироскопа
		y_gyro_int = two_complement_to_int(gyro_y_h, gyro_y_l); //объединяем старший и младший байты показаний оси у гироскопа
		z_gyro_int = two_complement_to_int(gyro_z_h, gyro_z_l); //объединяем старший и младший байты показаний оси z гироскопа

		x_gyro = ((float) x_gyro_int)/131; //приводим показание оси х гироскопа к ее фактическому значению //  для GYRO_CONFIG = 0x00                               //            
		y_gyro = ((float) y_gyro_int)/131; //приводим показание оси у гироскопа к ее фактическому значению //  LSB Sensitivity = 131 LSB/deg/sec                    //
		z_gyro = ((float) z_gyro_int)/131; //приводим показание оси z гироскопа к ее фактическому значению //  Информация из MPU-6050 Register Map and Descriptions //

		printf("x_gyro = %.3f d/s   y_gyro = %.3f d/s   z_gyro = %.3f d/s \r", x_gyro, y_gyro, z_gyro); //выводим показания осей гироскопа при помоще функции printf()
		usleep(10000);

	}

	return 0;
}
