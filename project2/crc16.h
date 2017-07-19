#ifndef _CRC16_H
#define _CRC16_H

#define CRC16 0x8005

unsigned short calc_crc16(const char *data, unsigned short size)
{
    unsigned short out = 0;
    int bitsRead = 0;
    int bitFlag;

    while(size > 0)
    {
        bitFlag = out >> 15;

        //ambil bit berikutnya
        out <<= 1;
        out |= (*data >> bitsRead) & 1;

        bitsRead++;
        if(bitsRead > 7)
        {
            bitsRead = 0;
            data++;
            size--;
        }

        //cyclic, xor dengan polynom
        if(bitFlag)
            out ^= CRC16;

    }

    //kembalikan format, bagi lagi
    int i;
    for (i = 0; i < 16; ++i) {
        bitFlag = out >> 15;
        out <<= 1;
        if(bitFlag)
            out ^= CRC16;
    }

    //reverse bitnya
    unsigned short crc = 0;
    i = 0x8000;
    int j = 0x0001;
    for (; i != 0; i >>=1, j <<= 1) {
        if (i & out) crc |= j;
    }

    return crc;
}

#endif
