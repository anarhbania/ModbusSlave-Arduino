#include "ModbusSlave.h"

ModbusSlave::ModbusSlave(HardwareSerial *_port, uint32_t _baud, uint8_t _slaveID, uint16_t _registersAddress, uint16_t *_registers, uint16_t _registersSize)
{
	port = _port;
	slaveID = _slaveID;
	registersAddress = _registersAddress;
	registers = _registers;
	registersSize = _registersSize;
	
	(*port).begin(_baud);
	
	if(_baud > 19200)
	{
		t1_5 = 750; 
		t3_5 = 1750; 
	}
	else 
	{
		t1_5 = 15000000 / _baud;
		t3_5 = 35000000 / _baud;
	}
} 

void ModbusSlave::REDE(uint8_t _pinREDE)
{
	pinREDE = _pinREDE;
	enableREDE = 1;
		
	pinMode(pinREDE, OUTPUT);
	digitalWrite(pinREDE, LOW);
} 

uint8_t ModbusSlave::Update(void)
{	
	if((*port).available())
	{
		uint8_t frameQuantity = 0;
	
		while((*port).available())
		{
			if(frameQuantity == FRAME_SIZE)
			{
				frameQuantity -= frameQuantity;
			}
		  
			frame[frameQuantity++] = (*port).read();
			delayMicroseconds(t1_5);
		}
	
		if(frameQuantity > 7)
		{
			if(frame[0] == slaveID)
			{
				uint16_t calculateCRC = ModbusSlave::CalculateCRC16(frameQuantity - 2);
				
				if(calculateCRC == (((frame[frameQuantity - 1] << 8) | frame[frameQuantity - 2])))
				{
					uint16_t nextFrame = 0;
					uint16_t startingAddress = ((frame[2] << 8) | frame[3]);
					uint16_t quantityRegisters = ((frame[4] << 8) | frame[5]);
					uint16_t quantityData = 2 * quantityRegisters;

					if(frame[1] == 0x03) // Read Holding Registers
					{
						if(startingAddress >= registersAddress)
						{
							if(quantityRegisters <= registersSize)
							{
								frame[2] = quantityData;

								for(uint16_t i = startingAddress - registersAddress; i < startingAddress - registersAddress + quantityRegisters; i++)
								{
									frame[3 + nextFrame] = registers[i] >> 8; // HIGH
									frame[4 + nextFrame] = registers[i] & 0xFF; // LOW

									nextFrame += 2;
								}

								calculateCRC = ModbusSlave::CalculateCRC16(quantityData + 3);

								frame[3 + quantityData] = calculateCRC & 0xFF; // LOW
								frame[4 + quantityData] = calculateCRC >> 8; // HIGH

								ModbusSlave::SendAnswer(5 + quantityData);
							}
							else
							{
								ModbusSlave::SendException(0x03, 0x03);
							}
						}
						else
						{
							ModbusSlave::SendException(0x03, 0x02);
						}
					}
					else if(frame[1] == 0x06) // Preset Single Register
					{
						if(startingAddress >= registersAddress)
						{
							registers[startingAddress - registersAddress] = ((frame[4] << 8) | frame[5]);

							calculateCRC = ModbusSlave::CalculateCRC16(6);

							frame[6] = calculateCRC & 0xFF; // LOW
							frame[7] = calculateCRC >> 8; // HIGH

							ModbusSlave::SendAnswer(8);
						}
						else
						{
							ModbusSlave::SendException(0x06, 0x02);
						}
					}
					else if(frame[1] == 0x10) // Preset Multiple Registers
					{
						if(frame[6] == (frameQuantity - 9))
						{
							if(startingAddress >= registersAddress)
							{
								if(quantityRegisters <= registersSize)
								{
									for(uint16_t i = startingAddress - registersAddress; i < startingAddress - registersAddress + quantityRegisters; i++)
									{
										registers[i] = ((frame[7 + nextFrame] << 8) | frame[8 + nextFrame]);

										nextFrame += 2;
									}

									calculateCRC = ModbusSlave::CalculateCRC16(6);

									frame[6] = calculateCRC & 0xFF; // LOW
									frame[7] = calculateCRC >> 8; // HIGH

									ModbusSlave::SendAnswer(8);
								}
								else
								{
									ModbusSlave::SendException(0x10, 0x03);
								}
							}
							else
							{
								ModbusSlave::SendException(0x10, 0x02);
							}
						}
					}
				}
			}
		}
	}
	
	return exception;
}

float ModbusSlave::ConversionToFloat(uint16_t variable1, uint16_t variable0)
{
	uint32_t variableInt = ((variable1 << 16) | variable0);
    float variableFloat = *(float*)&variableInt;
	
	return variableFloat;
}

void ModbusSlave::SendAnswer(uint8_t length)
{
	if(enableREDE)
	{
		digitalWrite(pinREDE, HIGH);
	}
	
	for(uint8_t i = 0; i < length; i++)
	{
		(*port).write(frame[i]);
	}

	(*port).flush();

	delayMicroseconds(t3_5);
	
	if(enableREDE)
	{
		digitalWrite(pinREDE, LOW);
	}
}

void ModbusSlave::SendException(uint8_t function, uint8_t exception)
{
	frame[0] = slaveID;
	frame[1] = (0x80 | function);
	frame[2] = exception;

	uint16_t calculateCRC = ModbusSlave::CalculateCRC16(3);
	frame[3] = calculateCRC >> 8;
	frame[4] = calculateCRC & 0xFF;

	ModbusSlave::SendAnswer(5);
}

uint16_t ModbusSlave::CalculateCRC16(uint8_t length)
{
	uint16_t crc16 = 0xFFFF;

	for(uint8_t i = 0; i < length; i++)
	{
		crc16 = crc16 ^ frame[i];

		for(uint8_t j = 0; j < 8; j++)
		{
			if((crc16 & 1) == 1)
			{
				crc16 = (crc16 >> 1) ^ 0xA001;
			}
			else
			{
				crc16 >>= 1;
			}
		}
	}

    return crc16;
}
