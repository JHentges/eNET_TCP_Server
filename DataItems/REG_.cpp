#include "REG_.h"

#include "../apci.h"
#include "../eNET-types.h"
#include "../eNET-AIO16-16F.h"
#include "../logging.h"
#include "TDataItem.h"

// extern int apci;

TREG_Read1 &TREG_Read1::Go()
{
	this->Value = 0;
	this->Value = in(offset);
	return *this;
}

std::shared_ptr<void> TREG_Read1::getResultValue()
{
	Trace("returning " + (this->width == 8 ? to_hex<__u8>((__u8)this->Value) : to_hex<__u32>((__u32)this->Value)));
	return this->width == 8
			   ? (std::shared_ptr<void>)std::shared_ptr<__u8>(new __u8(this->Value))
			   : (std::shared_ptr<void>)std::shared_ptr<__u32>(new __u32(this->Value));
}

TError TREG_Read1::validateDataItemPayload(DataItemIds DataItemID, TBytes Data)
{
	TError result = ERR_SUCCESS;
	if (Data.size() != 1)
	{
		Error(err_msg[-ERR_DId_BAD_PARAM]);
		return ERR_DId_BAD_PARAM;
	}
	int offset = Data[0];
	int w = widthFromOffset(offset);
	if (w != 0)
	{
		Error(err_msg[-ERR_DId_BAD_OFFSET]);
		return ERR_DId_BAD_OFFSET;
	}

	return result;
};

TREG_Read1::TREG_Read1(DataItemIds DId, int ofs)
{
	Trace("ENTER. DId: " + to_hex<TDataId>(DId) + ", offset: " + to_hex<__u8>(ofs));
	this->setDId(REG_Read1);
	this->setOffset(ofs);
}

TREG_Read1::TREG_Read1()
{
	this->setDId(REG_Read1);
}

TREG_Read1::TREG_Read1(TBytes data)
{
	Trace("ENTER. TBytes: ", data);
	this->setDId(REG_Read1);

	GUARD(data.size() == 1, ERR_DId_BAD_PARAM, data.size());
	this->offset = data[0];
	int w = widthFromOffset(offset);
	GUARD(w != 0, ERR_DId_BAD_PARAM, REG_Read1);
	this->width = w;
}

TREG_Read1 &TREG_Read1::setOffset(int ofs)
{
	int w = widthFromOffset(ofs);
	if (w == 0)
	{
		Error("Invalid offset");
		throw std::logic_error("Invalid offset passed to TREG_Read1::setOffset");
	}
	this->offset = ofs;
	this->width = w;
	return *this;
}

TBytes TREG_Read1::calcPayload(bool bAsReply)
{
	TBytes bytes;
	stuff<__u8>(bytes, this->offset);

	if (bAsReply)
	{
		auto value = this->getResultValue();
		__u32 v = *((__u32 *)value.get());
		stuff(bytes, v);
	}

	Trace("TREG_Read1::calcPayload built: ", bytes);
	return bytes;
}

std::string TREG_Read1::AsString(bool bAsReply)
{
	std::stringstream dest;

	dest << "REG_Read1(" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(this->offset) << ")";
	if (bAsReply)
	{
		dest << " → ";
		auto value = this->getResultValue();
		__u32 v = *((__u32 *)value.get());
		if (this->width == 8)
		{
			dest << std::hex << std::setw(2) << (v & 0x000000FF);
		}
		else
		{
			dest << std::hex << std::setw(8) << v;
		}
	}
	Trace("Built: " + dest.str());
	return dest.str();
}
#pragma endregion

#pragma region TREG_Writes implementation
TREG_Writes::~TREG_Writes()
{
	Trace("Attempting to free the memory in this->Writes vector");
	this->Writes.clear();
}

// TODO: write WaitUntilBitsMatch(__u8 offset, __u32 bmMask, __u32 bmPattern);
int WaitUntilRegisterBitIsLow(__u8 offset, __u32 bitMask) // TODO: move into utility source file
{
	__u32 value = 0;
	int attempt = 0;
	do
	{
		value = in(offset);
		Trace("SPI Busy Bit at " + std::string(to_hex<__u8>(offset)) + " is " + ((value & bitMask) ? "1" : "0"));
		// if (status < 0)
		// 	return -errno;
		if (++attempt > 100)
		{
			Error("Timeout waiting for SPI to be not busy, at offset: " + to_hex<__u8>(offset));
			return -ETIMEDOUT; // TODO: swap "attempt" with "timeout" RTC if benchmark proves RTC is not too slow
		}
	} while ((value & bitMask));

	return 0;
}

TREG_Writes &TREG_Writes::addWrite(__u8 w, int ofs, __u32 value)
{
	Trace("ENTER, w:" + std::to_string(w) + ", offset: " + to_hex<__u8>(ofs) + ", value: " + to_hex<__u32>(value));
	REG_Write aWrite;
	aWrite.width = w;
	aWrite.offset = ofs;
	aWrite.value = value;
	this->Writes.emplace_back(aWrite);
	return *this;
}

#define SPI_DELAY_DAC 160000 // 160 µsec in ns
#define SPI_DELAY_DIO 160000 // 160 µsec in ns

__s64 now() // in nanoseconds
{
	timespec Now;
	clock_gettime(CLOCK_BOOTTIME, &Now);
	return Now.tv_sec * 1E9 + Now.tv_nsec;
}

TREG_Writes &TREG_Writes::Go()
{
	static __s64 nextAllowedTimeDioSpi = now(), nextAllowedTimeDacSpi = now();
	__s64 diffTime = 0.0;

	this->resultCode = 0;
	for (auto action : this->Writes)
	{
		// DAC (at offset +30) and DIO (at offsets +3C → +44) are SPI based and must not write while the respective SPI bus is busy
		switch (action.offset)
		{
		case ofsDac: // DAC SPI busy handling
			this->resultCode = WaitUntilRegisterBitIsLow(ofsDacSpiBusy, bmDacSpiBusy);
			break;
		case ofsDioDirections:
		case ofsDioOutputs:
		case ofsDioInputs: // DIO SPI busy handling
			this->resultCode = WaitUntilRegisterBitIsLow(ofsDioSpiBusy, bmDioSpiBusy);
			break;
		default:
			break;
		}
		out(action.offset, action.value);
		if (action.width == 8){
			Trace("out(" + to_hex<__u8>(action.offset) + ") → " + to_hex<__u8>(action.value));
		}
		else {Trace("out(" + to_hex<__u8>(action.offset) + ") → " + to_hex<__u32>(action.value));
		}
	}
	return *this;
}

std::string TREG_Writes::AsString(bool bAsReply)
{
	std::stringstream dest;
	for (auto aWrite : this->Writes)
	{
		__u32 v = aWrite.value;
		dest << "REG_Write1(" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(aWrite.offset) << ", " << std::hex << std::setw(aWrite.width / 8 * 2) << v << ");";
	}
	return dest.str();
}
#pragma endregion TREG_Writes

#pragma region TREG_Write1 implementation
TREG_Write1::TREG_Write1()
{
	this->setDId(REG_Write1);
}
TREG_Write1::~TREG_Write1()
{
	this->Writes.clear();
}

TREG_Write1::TREG_Write1(TBytes buf)
{
	this->setDId(REG_Write1);
	GUARD(buf.size() > 0, ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH, 0);
	__u8 ofs = buf[0];
	int w = widthFromOffset(ofs);
	GUARD(w != 0, ERR_DId_BAD_OFFSET, ofs);
	GUARD(w == 8 ? (buf.size() == 2) : (buf.size() == 5), ERR_DId_BAD_PARAM, buf.size());

	__u32 value = 0;
	if (w == 8)
		value = *(__u8 *)&buf[1];
	else
		value = *(__u32 *)&buf[1];

	this->addWrite(w, ofs, value);
}

TBytes TREG_Write1::calcPayload(bool bAsReply)
{
	TBytes bytes;
	if (this->Writes.size() > 0)
		stuff<__u8>(bytes, this->Writes[0].offset);
	else
		Error("ERROR: nothing in Write[] queue");

	__u32 v = this->Writes[0].value;
	for (int i = 0; i < this->Writes[0].width / 8; i++)
	{
		bytes.push_back(v & 0x000000FF);
		v >>= 8;
	}
	return bytes;
}
