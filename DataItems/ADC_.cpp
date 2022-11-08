#include "ADC_.h"
#include "../apcilib.h"
#include "../apci.h"
#include "../logging.h"
#include "../eNET-AIO16-16F.h"
#include "../adc.h"

extern int apci;

TADC_BaseClock::TADC_BaseClock(TBytes buf)
{
	this->setDId(ADC_BaseClock);
	GUARD((buf.size() == 0) || (buf.size() == 4), ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH, 0);
}

TBytes TADC_BaseClock::calcPayload(bool bAsReply)
{
	TBytes bytes;
	stuff(bytes, this->baseClock);
	Trace("TADC_BaseClock::calcPayload built: ", bytes);
	return bytes;
};

TADC_BaseClock &TADC_BaseClock::Go()
{
	Trace("ADC_BaseClock Go()");
	// apci_read32(apci, 0, BAR_REGISTER, ofsAdcBaseClock, &this->baseClock);
	this->baseClock = in(ofsAdcBaseClock);
	return *this;
}

std::string TADC_BaseClock::AsString(bool bAsReply)
{
	std::stringstream dest;

	dest << "ADC_BaseClock()";
	if (bAsReply)
	{
		dest << " → " << this->baseClock;
	}
	Trace("Built: " + dest.str());
	return dest.str();
}

TADC_StreamStart::TADC_StreamStart(TBytes buf)
{
	this->setDId(ADC_StreamStart);
	GUARD((buf.size() == 0) || (buf.size() == 4), ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH, 0);

	if (buf.size() == 4)
	{
		if (-1 == AdcStreamingConnection)
		{
			this->argConnectionID = (int)*(__u32 *)buf.data();
			AdcStreamingConnection = this->argConnectionID;
		}
		else
		{
			Error("ADC Busy");
			throw std::logic_error("ADC Busy already, on Connection: "+std::to_string(AdcStreamingConnection));
		}
	}
	Trace("AdcStreamingConnection: "+std::to_string(AdcStreamingConnection));
}

TBytes TADC_StreamStart::calcPayload(bool bAsReply)
{
	TBytes bytes;
	stuff(bytes, this->argConnectionID);
	Trace("TADC_StreamStart::calcPayload built: ", bytes);
	return bytes;
};

TADC_StreamStart &TADC_StreamStart::Go()
{
	Trace("ADC_StreamStart::Go(), ADC Streaming Data will be sent on ConnectionID: "+std::to_string(AdcStreamingConnection));
	auto status = apci_dma_transfer_size(apci, 1, RING_BUFFER_SLOTS, BYTES_PER_TRANSFER);
	if (status)
	{
		Error("Error setting apci_dma_transfer_size: "+std::to_string(status));
		throw std::logic_error(err_msg[-status]);
	}

	AdcStreamTerminate = 0;
	if (AdcWorkerThreadID == -1)
	{
		AdcWorkerThreadID = pthread_create(&worker_thread, NULL, &worker_main, &AdcStreamingConnection);
	}
	apci_start_dma(apci);
	return *this;
};

std::string TADC_StreamStart::AsString(bool bAsReply)
{
	std::string msg = this->getDIdDesc();
	if (bAsReply)
	{
		msg += ", ConnectionID = " + to_hex<int>(this->argConnectionID);
	}
	return msg;
};



TADC_StreamStop::TADC_StreamStop(TBytes buf)
{
	this->setDId(ADC_StreamStop);
	GUARD(buf.size() == 0, ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH, 0);
}

TBytes TADC_StreamStop::calcPayload(bool bAsReply)
{
	TBytes bytes;
	return bytes;
};

TADC_StreamStop &TADC_StreamStop::Go()
{
	Trace("ADC_StreamStop::Go(): terminating ADC Streaming");
	AdcStreamTerminate = 1;
	apci_cancel_irq(apci, 1);
	AdcStreamingConnection = -1;
	AdcWorkerThreadID = -1;
	// pthread_cancel(worker_thread);
	// pthread_join(worker_thread, NULL);
	Trace("ADC_StreamStop::Go() exiting");
	return *this;
};

std::string TADC_StreamStop::AsString(bool bAsReply)
{
	return this->getDIdDesc();
}