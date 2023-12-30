/*
 * LoRaRelayTransmitter.cc
 *
 *  Created on: Dec 24, 2023
 *      Author: handybald
 */

#include "LoRaRelayTransmitter.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarTransmission.h"
#include "inet/mobility/contract/IMobility.h"
#include "LoRaPhyPreamble_m.h"
#include <algorithm>

namespace lpwan {

Define_Module(LoRaRelayTransmitter);

LoRaRelayTransmitter::LoRaRelayTransmitter() :
    FlatTransmitterBase()
{
}

void LoRaRelayTransmitter::initialize(int stage)
{
    TransmitterBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
    {
        preambleDuration = 0.001; //par("preambleDuration");
        headerLength = b(par("headerLength"));
        bitrate = bps(par("bitrate"));
        power = W(par("power"));
        centerFrequency = Hz(par("centerFrequency"));
        bandwidth = Hz(par("bandwidth"));
        LoRaRelayTransmissionCreated = registerSignal("LoRaRelayTransmissionCreated");


    }
}

std::ostream& LoRaRelayTransmitter::printToStream(std::ostream& stream, int level, int evFlags) const
{
    stream << "LoRaRelayTransmitter";
    return FlatTransmitterBase::printToStream(stream, level, evFlags);
}

const ITransmission *LoRaRelayTransmitter::createTransmission(const IRadio *transmitter, const Packet *macFrame , const simtime_t startTime) const
{
    const_cast<LoRaRelayTransmitter*>(this)->emit(LoRaRelayTransmissionCreated, true);
    EV << macFrame->getDetailStringRepresentation(evFlags) << endl;
    const auto &frame = macFrame->peekAtFront<LoRaPhyPreamble>();

    int nPreamble = 8;
    simtime_t Tsym = (pow(2, frame->getSpreadFactor()))/(frame->getBandwidth().get()/1000);
    simtime_t Tpreamble = (nPreamble + 4.25) * Tsym / 1000;

    int payloadBytes = 0;
    //FIXME: payload bytes assignment is weird, what to do?
    payloadBytes = 20;
    int payloadSymNb = 8;
    payloadSymNb += std::ceil((8*payloadBytes - 4*frame->getSpreadFactor() + 28 + 16 - 20*0)/(4*(frame->getSpreadFactor()-2*0)))*(frame->getCodeRendundance() + 4);
    if (payloadSymNb < 8) payloadSymNb = 8;
    simtime_t Theader = 0.5 * (8 + payloadSymNb) * Tsym / 1000;
    simtime_t Tpayload = 0.5 * (8 + payloadSymNb) * Tsym / 1000;

    const simtime_t duration = Tpreamble + Theader + Tpayload;
    const simtime_t endTime = startTime + duration;
    inet::IMobility *mobility = transmitter->getAntenna()->getMobility();
    const Coord startPosition = mobility->getCurrentPosition();
    const Coord endPosition = mobility->getCurrentPosition();
    const Quaternion startOrientation = mobility->getCurrentAngularPosition();
    const Quaternion endOrientation = mobility->getCurrentAngularPosition();
    W transmissionPower = computeTransmissionPower(macFrame);

    transmissionPower = mW(math::dBmW2mW(14));

    EV << "[MSDebug] I am sending packet with TP: " << transmissionPower << endl;
    EV << "[MSDebug] I am sending packet with SF: " << frame->getSpreadFactor() << endl;


    return new LoRaTransmission(transmitter,
            macFrame,
            startTime,
            endTime,
            Tpreamble,
            Theader,
            Tpayload,
            startPosition,
            endPosition,
            startOrientation,
            endOrientation,
            transmissionPower,
            frame->getCenterFrequency(),
            frame->getSpreadFactor(),
            frame->getBandwidth(),
            frame->getCodeRendundance());
}

} /* namespace lpwan */