/**
 *
 * @license MIT License
 *
 * Copyright (c) 2022 lewis he
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @file      XPowersCommon.h
 * @author    Lewis He (lewishe@outlook.com)
 * @date      2022-05-07
 *
 */


#pragma once

#ifdef _BV
#undef _BV
#endif
#define _BV(b)                          (1ULL << (b))


#ifndef constrain
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#endif


#define XPOWERS_ATTR_NOT_IMPLEMENTED    __attribute__((error("Not implemented")))
#define IS_BIT_SET(val,mask)            (((val)&(mask)) == (mask))



template <class chipType>
class XPowersCommon
{
    typedef int (*iic_fptr_t)(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len);

public:
    bool begin(void)
    {
        Wire.begin();
        wire = &Wire;
        return thisChip().initImpl();
    }

    bool begin(TwoWire &w, uint8_t addr, int sda, int scl)
    {
        wire = &w;
        wire->setPins(sda, scl);
        wire->begin();
        address = addr;
        return thisChip().initImpl();
    }

    bool begin(uint8_t addr, iic_fptr_t readRegCallback, iic_fptr_t writeRegCallback)
    {
        thisReadRegCallback = readRegCallback;
        thisWriteRegCallback = writeRegCallback;
        address = addr;
        return thisChip().initImpl();
    }

    int readRegister(uint8_t reg)
    {
        uint8_t val = 0;
        if (thisReadRegCallback) {
            return thisReadRegCallback(address, reg, &val, 1);
        }
        if (wire) {
            wire->beginTransmission(address);
            wire->write(reg);
            if (wire->endTransmission() != 0) {
                return -1;
            }
            wire->requestFrom(address, 1U);
            return wire->read();
        }
        return -1;
    }

    int writeRegister(uint8_t reg, uint8_t val)
    {
        if (thisWriteRegCallback) {
            return thisWriteRegCallback(address, reg, &val, 1);
        }
        if (wire) {
            wire->beginTransmission(address);
            wire->write(reg);
            wire->write(val);
            return (wire->endTransmission() == 0) ? 0 : -1;
        }
        return -1;
    }

    int readRegister(uint8_t reg, uint8_t *buf, uint8_t lenght)
    {
        if (thisReadRegCallback) {
            return thisReadRegCallback(address, reg, buf, lenght);
        }
        if (wire) {
            wire->beginTransmission(address);
            wire->write(reg);
            if (wire->endTransmission() != 0) {
                return -1;
            }
            wire->requestFrom(address, lenght);
            return wire->readBytes(buf, lenght) == lenght ? 0 : -1;
        }
        return -1;
    }

    int writeRegister(uint8_t reg, uint8_t *buf, uint8_t lenght)
    {
        if (thisWriteRegCallback) {
            return thisWriteRegCallback(address, reg, buf, lenght);
        }
        if (wire) {
            wire->beginTransmission(address);
            wire->write(reg);
            wire->write(buf, lenght);
            return (wire->endTransmission() == 0) ? 0 : -1;
        }
        return -1;
    }


    bool inline clrRegisterBit(uint8_t registers, uint8_t bit)
    {
        int val = readRegister(registers);
        if (val == -1) {
            return false;
        }
        return  writeRegister(registers, (val & (~_BV(bit)))) == 0;
    }

    bool inline setRegisterBit(uint8_t registers, uint8_t bit)
    {
        int val = readRegister(registers);
        if (val == -1) {
            return false;
        }
        return  writeRegister(registers, (val | (_BV(bit)))) == 0;
    }

    bool inline getRegisterBit(uint8_t registers, uint8_t bit)
    {
        int val = readRegister(registers);
        if (val == -1) {
            return false;
        }
        return val & _BV(bit);
    }

    uint16_t inline readRegisterH8L4(uint8_t highReg, uint8_t lowReg)
    {
        int h8 = readRegister(highReg);
        int l4 = readRegister(lowReg);
        if (h8 == -1 || l4 == -1)return 0;
        return (h8 << 4) | (l4 & 0x0F);
    }

    uint16_t inline readRegisterH8L5(uint8_t highReg, uint8_t lowReg)
    {
        int h8 = readRegister(highReg);
        int l5 = readRegister(lowReg);
        if (h8 == -1 || l5 == -1)return 0;
        return (h8 << 5) | (l5 & 0x1F);
    }

    uint16_t inline readRegisterH6L8(uint8_t highReg, uint8_t lowReg)
    {
        int h6 = readRegister(highReg);
        int l8 = readRegister(lowReg);
        if (h6 == -1 || l8 == -1)return 0;
        return ((h6 & 0x3F) << 8) | l8;
    }

    uint16_t inline readRegisterH5L8(uint8_t highReg, uint8_t lowReg)
    {
        int h5 = readRegister(highReg);
        int l8 = readRegister(lowReg);
        if (h5 == -1 || l8 == -1)return 0;
        return ((h5 & 0x1F) << 8) | l8;
    }
    /*
     * CRTP Helper
     */
protected:
    inline const chipType &thisChip() const
    {
        return static_cast<const chipType &>(*this);
    }
    inline chipType &thisChip()
    {
        return static_cast<chipType &>(*this);
    }

protected:
    uint8_t     address              = 0xFF;
    TwoWire     *wire                = NULL;
    iic_fptr_t  thisReadRegCallback  = NULL;
    iic_fptr_t  thisWriteRegCallback = NULL;
};
