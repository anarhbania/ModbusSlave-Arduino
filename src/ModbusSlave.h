#ifndef MODBUS_SLAVE_H
#define MODBUS_SLAVE_H

#include "Arduino.h"

class ModbusSlave
{
	public:

	ModbusSlave(HardwareSerial *_port, uint32_t _baud, uint8_t _slaveID, uint16_t _registersAddress, uint16_t *_registers, uint16_t _registersSize);
	void REDE(uint8_t _pinREDE);
	uint8_t Update(void);
	float ConversionToFloat(uint16_t variable1, uint16_t variable0);

	private:

	#define FRAME_SIZE 128

	HardwareSerial *port;

	bool enableREDE;

	uint8_t exception;
	uint8_t slaveID;
	uint8_t frame[FRAME_SIZE];

	uint16_t *registers;
	uint16_t registersAddress;
	uint16_t registersSize;
	uint8_t pinREDE = -1;

	uint16_t t1_5;
	uint16_t t3_5;

	void SendAnswer(uint8_t length);
	void SendException(uint8_t function, uint8_t exception);
	uint16_t CalculateCRC16(uint8_t length);
};

#endif
