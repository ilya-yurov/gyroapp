#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include "gyroapp.h"

int file = -1;

size_t i2c_write_configs (uint8_t reg_address, uint8_t value)
{
	uint8_t buf[2] = {0};
	size_t result = 0;
	buf[0] = reg_address;
	buf[1] = value;
	result = write(file,buf,sizeof(buf));
	return result;
}
size_t i2c_write_byte (uint8_t reg_address)
{
	size_t result = 0;
	result = write(file,&reg_address,sizeof(reg_address));
	return result;
}
size_t i2c_read_byte(char *reg_value)
{
	size_t result = 0;
	result = read(file,&reg_value,sizeof(reg_value));
	return result;
}

uint16_t merge_bytes( uint8_t LSB, uint8_t MSB)
{
	return (uint16_t) (((LSB & 0xFF) << 8) | MSB);
}

int16_t two_complement_to_int( uint8_t LSB, uint8_t MSB)
{
	int16_t signed_int = 0;
	uint16_t word;

	word = merge_bytes(LSB, MSB);

	if((word & 0x8000) == 0x8000)
	{
		signed_int = (int16_t) -(~word);
	}
	else
	{
		signed_int = (int16_t) (word & 0x7fff);
	}
	return signed_int;
}

int main()
{
	const char * devName = "/dev/i2c-1";
	char gyro_x_h, gyro_x_l, gyro_y_h, gyro_y_l, gyro_z_h, gyro_z_l;
	gyro_x_h = 0;
	gyro_x_l = 0;
        gyro_y_h = 0;
        gyro_y_l = 0;
        gyro_z_h = 0;
        gyro_z_l = 0;
	int16_t x_gyro_int = 0;
	int16_t y_gyro_int = 0;
	int16_t z_gyro_int = 0;
	float x_gyro, y_gyro, z_gyro;

	file = open(devName, O_RDWR);
	if (file == -1)
	{
		perror(devName);
		exit(1);
	}

	if (ioctl(file, I2C_SLAVE, MPU6050_I2C_ADDR) < 0)
	{
		printf("Failed to acquire bus access and/or talk to slave!\n");
		exit(1);
	}

	if (i2c_write_configs(PWR_MGMT_1, 0x01) == -1)
	{
		printf("Failed to write() byte to power management! %s\n", strerror(errno));
		exit(1);
	}
	if (i2c_write_configs(GYRO_CONFIG, 0x00) == -1)
	{
		printf("Failed to write byte to gyro configuration! %s\n", strerror(errno));
                exit(1);
	}

	while(1)
	{
		if (i2c_write_byte(REG_GYRO_XOUT_H) == -1)
		{
			printf("Something went wrong with write(xout_h)! %s\n", strerror(errno));
                        exit(1);
                }
		if (i2c_read_byte(&gyro_x_h) == -1)
		{
			printf("Something went wrong with read(xout_h)! %s\n", strerror(errno));
			exit(1);
		}
                if (i2c_write_byte(REG_GYRO_XOUT_L) == -1)
		{
                        printf("Something went wrong with write(xout_l)! %s\n", strerror(errno));
                        exit(1);
                }
                if (i2c_read_byte(&gyro_x_l) == -1)
		{
                        printf("Something went wrong with read(xout_l)! %s\n", strerror(errno));
                        exit(1);
                }
                if (i2c_write_byte(REG_GYRO_YOUT_H) == -1)
		{
                        printf("Something went wrong with write(yout_h)! %s\n", strerror(errno));
                        exit(1);
                }
                if (i2c_read_byte(&gyro_y_h) == -1)
		{
                        printf("Something went wrong with read(yout_h)! %s\n", strerror(errno));
                        exit(1);
                }
                if (i2c_write_byte(REG_GYRO_YOUT_L) == -1)
		{
                        printf("Something went wrong with write(yout_l)! %s\n", strerror(errno));
                        exit(1);
                }
                if (i2c_read_byte(&gyro_y_l) == -1)
		{
                        printf("Something went wrong with read(yout_l)! %s\n", strerror(errno));
                        exit(1);
                }
		if (i2c_write_byte(REG_GYRO_ZOUT_H) == -1)
		{
                        printf("Something went wrong with write(zout_h)! %s\n", strerror(errno));
                        exit(1);
                }
                if (i2c_read_byte(&gyro_z_h) == -1)
		{
                        printf("Something went wrong with read(zout_h)! %s\n", strerror(errno));
                        exit(1);
                }
                if (i2c_write_byte(REG_GYRO_ZOUT_L) == -1)
		{
                        printf("Something went wrong with write(zout_l)! %s\n", strerror(errno));
                        exit(1);
                }
                if (i2c_read_byte(&gyro_z_l) == -1)
		{
                        printf("Something went wrong with read(zout_l)! %s\n", strerror(errno));
                        exit(1);
                }

		x_gyro_int = two_complement_to_int(gyro_x_h, gyro_x_l);
		y_gyro_int = two_complement_to_int(gyro_y_h, gyro_y_l);
		z_gyro_int = two_complement_to_int(gyro_z_h, gyro_z_l);

		x_gyro = ((float) x_gyro_int)/131;
		y_gyro = ((float) y_gyro_int)/131;
		z_gyro = ((float) z_gyro_int)/131;

		printf("x_gyro = %.3f d/s   y_gyro = %.3f d/s   z_gyro = %.3f d/s \r", x_gyro, y_gyro, z_gyro);
		usleep(10000);

	}

	return 0;
}
