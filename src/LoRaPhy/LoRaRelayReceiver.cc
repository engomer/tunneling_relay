/*
 * LoRaRelayReceiver.cc
 *
 *  Created on: Dec 24, 2023
 *      Author: handybald
 */

#include "LoRaRelayReceiver.h"
#include "LoRaReception.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarNoise.h"
#include "LoRaPhyPreamble_m.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"

namespace lpwan {

Define_Module(LoRaRelayReceiver);

LoRaRelayReceiver::LoRaRelayReceiver() :
    snirThreshold(NaN)
{

}

void LoRaRelayReceiver::initialize(int stage)
{
    FlatReceiverBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
    {
        snirThreshold = math::dB2fraction(par("snirThreshold"));
        energyDetection = mW(math::dBmW2mW(par("energyDetection")));
        alohaChannelModel = par("alohaChannelModel");
        LoRaRelayReceptionCollision = registerSignal("LoRaRelayReceptionCollision");
        numCollisions = 0;
        rcvBelowSensitivity = 0;
    }
}

void LoRaRelayReceiver::finish()
{
    FlatReceiverBase::finish();
    recordScalar("numCollisions", numCollisions);
    recordScalar("rcvBelowSensitivity", rcvBelowSensitivity);
}

bool LoRaRelayReceiver::computeIsReceptionPossible(const IListening *listening, const ITransmission *transmission) const
{
    //TODO: if we you write LoRaRelayTransmission, change the cast below.
    const LoRaTransmission *loRaTransmission = check_and_cast<const LoRaTransmission *>(transmission);
    auto *loRaRadio = check_and_cast<const LoRaRelayRadio *>(loRaTransmission->getTransmitter()->getRadio());
}

bool LoRaRelayReceiver::computeIsReceptionPossible(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part) const
{
    //TODO: if you implement LoRaRelayBandListening, change cast below.
    const LoRaBandListening *loRaListening = check_and_cast<const LoRaBandListening *>(listening);
    const LoRaReception *loRaReception = check_and_cast<const LoRaReception*>(reception);
    W minReceptionPower = loRaReception->computeMinPower(reception->getStartTime(part), reception->getEndTime(part));
    W sensitivity = getSensitivity(loRaReception);
    bool isReceptionPossible = minReceptionPower >= sensitivity;
    EV_DEBUG << "Computing whether reception is possible: minimum reception power = " >> minReceptionPower << ", sensitivity = " << sensitivity << " -> reception is " << (isReceptionPossible ? "possible" : "impossible") << endl;
    if (isReceptionPossible == false)
    {
        const_cast<LoRaRelayReceiver*>(this)->rcvBelowSensitivity++;
    }
    return isReceptionPossible;
}

bool LoRaRelayReceiver::computeIsReceptionAttempted(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference) const
{
    if (isPacketCollided(reception, part, interference))
    {
        auto packet = reception->getTransmission()->getPacket();
        const auto &chunk = packet->peekAtFront<FieldsChunk>();
        auto loRaMac = dynamicPtrCast<const LoRaMacFrame>(chunk);
        auto LoRaPreamble = dynamicPtrCast<const LoRaPhyPreamble>(chunk);
        MacAddress rec;
        if (loRaPreamble)
        {
            rec = loRaPreamble->getReceiverAddress();
        }
        else if (loRaMac)
        {
            rec = loRaMac->getReceiverAddress();
        }

        auto *macLayer = check_and_cast<LoRaRelayMac*>(getParentModule()->getParentModule()->getSubmodule("mac"));
        if (rec == macLayer->getMacAddress())
        {
            const_cast<LoRaRelayReceiver*>(this)->numCollisions++;
            emit(LoRaRelayReceptionCollision, packet);
        }
        return false;
    }
    else{
        return true;
    }
}

bool LoRaRelayReceiver::isPacketCollided(const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference) const
{
    auto interferingReceptions = interference->getInterferingReceptions();
    const LoRaReception *loRaReception = check_and_cast<const LoRaReception*>(reception);
    simtime_t m_x = (loRaReception->getStartTime() + loRaReception->getEndTime())/2;
    simtime_t d_x = (loRaReception->getEndTime() - loRaReception->getStartTime())/2;

    double P_threshold = 6;
    W signalRSSI_w = loRaReception->getPower();
    double signalRSSI_mw = signalRSSI_w.get()*1000;
    double signalRSSI_dBm = math::mW2dBmW(signalRSSI_mw);
    EV << signalRSSI_mw << endl;
    EV << signalRSSI_dBm << endl;
    int receptionSF = loRaReception->getLoRaSF();

    for (auto interferingReception : *interferingReceptions)
    {
        bool overlap = false;
        bool frequencyCollision = false;
        bool captureEffect = false;
        bool timingCollision = false;
        const LoRaReception *loRaInterference = check_and_cast<const LoRaReception*>(interferingReception);

        simtime_t m_y = (loRaInterference->getStartTime() + loRaInterference->getEndTime())/2;
        simtime_t d_y = (loRaInterference->getEndTime() - loRaInterference->getStartTime())/2;
        if (omnetpp::fabs(m_x - m_y) < d_x + d_y)
        {
            overlap = true;
        }

        if (loRaReception->getLoRaCF() == loRaInterference->getLoRaCF())
        {
            frequencyCollision = true;
        }

        W interferenceRSSI_w = loRaInterference->getPower();
        double interferenceRSSI_mw = interferenceRSSI_w.get()*1000;
        double interferenceRSSI_dBm = math::mW2dBmW(interferenceRSSI_mw);
        int interferenceSF = loRaInterference->getLoRaSF();

        if (signalRSSI_dBm - interferenceRSSI_dBm >= nonOrthDelta[receptionSF-7][interferenceSF-7])
        {
            captureEffect = true;
        }

        EV << "[MSDEBUG] Received packet at SF: " << receptionSF << " with power " << signalRSSI_dBm << endl;
        EV << "[MSDEBUG] Received interference at SF: " << interferenceSF << " with power " << interferenceRSSI_dBm << endl;
        EV << "[MSDEBUG] Acceptable diff is equal " << nonOrthDelta[receptionSF-7][interferenceSF-7] << endl;
        EV << "[MSDEBUG] Diff is equal " << signalRSSI_dBm - interferenceRSSI_dBm << endl;

        if (captureEffect == false)
        {
                        EV << "[MSDEBUG] Packet is discarded" << endl;
        } else
            EV << "[MSDEBUG] Packet is not discarded" << endl;

        double nPreamble = 8; //from the paper "Do Lora networks..."
        simtime_t Tsym = (pow(2, loRaReception->getLoRaSF()))/(loRaReception->getLoRaBW().get()/1000)/1000;
        simtime_t csBegin = loRaReception->getPreambleStartTime() + Tsym * (nPreamble - 6);
        if(csBegin < loRaInterference->getEndTime())
        {
            timingCollision = true;
        }

        if (overlap && frequencyCollision)
        {
            if(alohaChannelModel == true)
            {
                if(iAmGateway && (part == IRadioSignal::SIGNAL_PART_DATA || part == IRadioSignal::SIGNAL_PART_WHOLE)) const_cast<LoRaReceiver* >(this)->emit(LoRaReceptionCollision, true);
                return true;
            }
            if(alohaChannelModel == false)
            {
                if(captureEffect == false && timingCollision)
                {
                    if(iAmGateway && (part == IRadioSignal::SIGNAL_PART_DATA || part == IRadioSignal::SIGNAL_PART_WHOLE)) const_cast<LoRaReceiver* >(this)->emit(LoRaReceptionCollision, true);
                    return true;
                }
            }

        }
    }
    return false;

}

}