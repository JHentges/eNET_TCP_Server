/*
The TCP Server Message is received in a standardized format.
Write functions to
) Build DataItems with zero or more bytes as the DataItem's `Data`
) Build Messages with zero or more DataItems as the Message `Payload`
) Append DataItems to already built Messages
) Validate a Message is well-formed (which also means all DataItems are well-formed)
) 
) Parse the Message into a queue of Actions
) Execute the queue

A Message is a sequence of bytes received by the TCP Server (across TCP/IP) in this format:
    SCMidLL_C
where
    S       is the START symbol
    C       is the category of the Message
    Mid     is a Message ID#, which may be mnemonic/logical
    LL      is the Length of an optional payload
    _       is zero or more bytes, the optional payload's data
    C       is the checksum for all bytes in MMMM, LL, and _

Given: checksum is calculated on all bytes in Message except the "start sentinel"; the sum, including the checksum, should be zero

Further, the payload is an ordered list of zero or more Data Items, which are themselves a sequence of bytes, in this format:
    LDid_
where
    L       is the length of the data encapsulated in this data item (in bytes) (exclusing L and Did)
    Did     is the Data ID#, which identifies the *type* of data encapsulated in this data item
    _       is the Data Item's payload, or "the data"; `byte[L] Data`
*/

#include "parser.h"
#include "diag.h"

// NYI - validate the payload of a Data Item based on the Data Item ID
int validateDataItemPayload(TDataItemID DataItemID, __u8 *Data, __u32 DataSize)
{
    int result = 0;
    // switch (DataItemID.DataItemID) {
    //     case DIAG_DATA_ITEM_ID_DIAG_MSG:
    //         if (DataSize != sizeof(TDiagMsg))
    //             result = ERR_MSG_DATAITEM_SIZE_INVALID;
    //         break;
    // }
    return result;
}

int validateDataItem(__u8 *DataItem)
{
    print_dataitem(DataItem);
    int result = 0;
    TDataItemLength DataItemSize = DataItem[0];
    TDataItemID dataItemID;
    dataItemID.Did[0] = DataItem[0];
    dataItemID.Did[1] = DataItem[1];
    dataItemID.Did[2] = DataItem[2];

    if (!isValidDataItemID(dataItemID)) {
        return ERR_MSG_DATAITEM_ID_UNKNOWN;
    }

    if (DataItemSize == 0) // no data in this data item so no need to check the payload
        return result;
    else
    {
        __u8 *Data =(__u8 *) malloc(DataItemSize);
        memcpy(Data, DataItem + 4, DataItemSize);
        result = validateDataItemPayload(dataItemID, Data, DataItemSize);
    }
    return result;
}


// "A Message has an optional Payload, which is a sequence of zero or more Data Items"
// Checks the Payload for well-formedness
// returns 0 if the Payload is well-formed
// this means that all the Data Items are well formed and the total size matches the expectation
int validatePayload(__u8 * Payload, __u32 payload_size) 
{
    // NYI - validate Payload, which consists of zero or more Data Items
    int result = 0;
    if (payload_size == 0) // zero-length payload size is a valid payload
        return result;

    //print_payload(Payload, payload_size);
    __u8 * DataItem = Payload;
    
    // DataItemSize is the size of the Data Item, including the size of the Data Item Length, Data Item ID, and the Data Item's payload
    int DataItemSize = sizeof(TDataItemID::Did) + sizeof(TDataItemLength) + DataItem[3];
    if (DataItemSize > payload_size)
        result = ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH;
    else {
        result = validateDataItem(DataItem);
        if (result == 0) // if the Data Item is well-formed, check the next one
        {
            DataItem += DataItemSize;
            payload_size -= DataItemSize;
            if (payload_size > 0)
                result = validatePayload(DataItem, payload_size);
        }
    }

    return result;
}


// Checks the Message for well-formedness
// returns 0 if Message is well-formed
int validateMessage(__u8 *buf, __u32 received_bytes) // "NAK()" is shorthand for return error condition etc
{
    print_message(buf, received_bytes);

    if (received_bytes < minimumMessageLength)
        return ERR_MSG_TOO_SHORT; // NAK(received insufficient data, yet) until more data (the rest of the Message/header) received?

    TMessageHeader *head = (TMessageHeader *) buf;

    if (! isValidMessageID(head->type))
        return ERR_MSG_ID_UNKNOWN; // NAK(invalid MessageID Category byte)

    __u32 statedMessageLength = minimumMessageLength + head->payload_size;
    if (received_bytes < statedMessageLength)
        return ERR_MSG_LEN_MISMATCH; // NAK(received insufficient data, yet) until more data

    TCheckSum checksum = calculateChecksum(buf, statedMessageLength);

    if (__valid_checksum__ !=checksum)
        return ERR_MSG_CHECKSUM; // NAK(invalid checksum)

    __u8 *payload = buf + sizeof(TMessageHeader);
    int validPayload = validatePayload(payload, head->payload_size);
    if (validPayload != 0)
        return validPayload;

    return 0; // valid message
}
