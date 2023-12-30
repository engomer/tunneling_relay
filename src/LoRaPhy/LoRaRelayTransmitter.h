/*
 * LoRaRelayTransmitter.h
 *
 *  Created on: Dec 24, 2023
 *      Author: handybald
 */

#ifndef LORAPHY_LORARELAYTRANSMITTER_H_
#define LORAPHY_LORARELAYTRANSMITTER_H_

#include "inet/physicallayer/wireless/common/base/packetlevel/FlatTransmitterBase.h"
#include "LoRaModulation.h"
#include "LoRaTransmission.h"
#include "LoRa/LoRaRelayRadio.h"
#include "LoRa/LoRaRelayMacFrame_m.h"


namespace lpwan{

class LoRaRelayTransmitter : public inet::physicallayer::FlatTransmitterBase
{
    public:
        LoRaRelayTransmitter();
        virtual void initialize(int stage) override;
        virtual std::ostream& printToStream(std::ostream& stream, int level, int evFlags = 0) const override;
        virtual const ITransmission *createTransmission(const IRadio *radio, const Packet *packet, const simtime_t startTime) const override;

    private:
        simsignal_t LoRaRelayTransmissionCreated;

};

}



#endif /* LORAPHY_LORARELAYTRANSMITTER_H_ */
