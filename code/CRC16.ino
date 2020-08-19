unsigned int CRC16(unsigned int crc, char *buf, int len) {
  for (int pos = 0; pos < len; pos++) {
    crc ^= (unsigned int)buf[pos];    // XOR byte into least sig. byte of crc

    for (int i = 8; i != 0; i--) {    // Loop over each bit
      if ((crc & 0x0001) != 0) {      // If the LSB is set
        crc >>= 1;                    // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                            // Else LSB is not set
        crc >>= 1;                    // Just shift right
    }
  }

  return crc;
}

unsigned int p1_crc16(String buffer) {
  int CRC_POLYNOM = 0xA001;
  int CRC_PRESET = 0x0000;
  unsigned int crc = CRC_PRESET;

  for (int i = 0; i < buffer.length(); i++) {
    crc ^= buffer[i];

    for (int j = 0; j < 8; j++) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= CRC_POLYNOM;
      } else {
        crc >>= 1;
      }
    }
  }

  return crc;
}

uint16_t _crc16_update(uint16_t crc, uint8_t data) {
  unsigned int i;

  crc ^= data;
  for (i = 0; i < 8; ++i) {
    if (crc & 1) {
      crc = (crc >> 1) ^ 0xA001;
    } else {
      crc = (crc >> 1);
    }
  }
  return crc;
}
