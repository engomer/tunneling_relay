/*
 * LoRaRelayReceiver.h
 *
 *  Created on: Dec 24, 2023
 *      Author: handybald
 */

#ifndef LORAPHY_LORARELAYRECEIVER_H_
#define LORAPHY_LORARELAYRECEIVER_H_

#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadioMedium.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/ReceptionResult.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/BandListening.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/ListeningDecision.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/ReceptionDecision.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/NarrowbandNoiseBase.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarSnir.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/FlatReceiverBase.h"

namespace lpwan{

class LoRaRelayReceiver : public inet::physicallayer::FlatReceiverBase
{

};

}

#endif /* LORAPHY_LORARELAYRECEIVER_H_ */
