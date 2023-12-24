/*
 * LoRaRelayRadio.cc
 *
 *  Created on: Nov 21, 2023
 *      Author: handybald
 */

#include "LoRaRelayRadio.h"
#include "LoRaPhy/LoRaPhyPreamble_m.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"

namespace lpwan {

Define_Module(LoRaRelayRadio);

simsignal_t LoRaRelayRadio::minSNIRSignal = cComponent::registerSignal("minSNIR");
simsignal_t LoRaRelayRadio::packetErrorRateSignal = cComponent::registerSignal("packetErrorRate");
simsignal_t LoRaRelayRadio::bitErrorRateSignal = cComponent::registerSignal("bitErrorRate");
simsignal_t LoRaRelayRadio::symbolErrorRateSignal = cComponent::registerSignal("symbolErrorRate");
simsignal_t LoRaRelayRadio::droppedPacket = cComponent::registerSignal("droppedPacket");

void LoRaRelayRadio::initialize(int stage)
{
    FlatRadioBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        iAmTransmiting = false;
    }
}

void LoRaRelayRadio::finish()
{
    FlatRadioBase::finish();
}

void LoRaRelayRadio::handleSelfMessage(cMessage *message)
{
    if (isTransmissionTimer(message)) {
        handleTransmissionTimer(message);
    }
    else if (isReceptionTimer(message)) {
        handleReceptionTimer(message);
    }
    else {
        FlatRadioBase::handleSelfMessage(message);
    }
}

void LoRaRelayRadio::handleUpperPacket(Packet *packet)
{
    emit(packetReceivedFromUpperSignal, packet);

    if (isTransmitterMode(radioMode))
    {
        auto tag = packet->removeTag<LoRaTag>();

        const auto &frame = packet->peekAtFront<LoRaRelayMacFrame>();

        auto preamble = makeShared<LoRaPhyPreamble>();

        preamble->setBandwidth(frame->getLoRaBW());
        preamble->setCenterFrequency(frame->getLoRaCF());
        preamble->setCodeRendundance(frame->getLoRaCR());
        preamble->setPower(mW(frame->getLoRaTP()));
        preamble->setSpreadFactor(frame->getLoRaSF());
        preamble->setUseHeader(frame->getLoRaUseHeader());
        preamble->setReceiverAddress(frame->getReceiverAddress());
        const auto & loraHeader =  packet->peekAtFront<LoRaMacFrame>();
        preamble->setReceiverAddress(loraHeader->getReceiverAddress());

        auto signalPowerReq = packet->addTagIfAbsent<SignalPowerReq>();
        signalPowerReq->setPower(mW(frame->getLoRaTP()));

        preamble->setChunkLength(b(16));
        packet->insertAtFront(preamble);

        if(transmissionTimer->isScheduled())
        {
            throw cRuntimeError("Received frame from upper layer while transmitting");
        }
        if (separateTransmissionParts)
        {
            startTransmission(packet, IRadioSignal::SIGNAL_PART_PREAMBLE);
        }
        else
        {
            startTransmission(packet, IRadioSignal::SIGNAL_PART_WHOLE);
        }
    }
    else {
        EV_ERROR << "Radio is not in transmitter or transceiver mode, dropping packet\n";
        delete packet;
    }
}

void LoRaRelayRadio::handleSignal(WirelessSignal *radioFrame)
{
    auto receptionTimer = createReceptionTimer(radioFrame);
    if (separateReceptionParts)
    {
        startReception(receptionTimer, IRadioSignal::SIGNAL_PART_PREAMBLE);
    }
    else
    {
        startReception(receptionTimer, IRadioSignal::SIGNAL_PART_WHOLE);
    }
}

bool LoRaRelayRadio::isTransmissionTimer(const cMessage *message) const
{
    return !strcmp(message->getName(), "transmissionTimer");
}

void LoRaRelayRadio::handleTransmissionTimer(cMessage *message)
{
    if (message->getKind() == IRadioSignal::SIGNAL_PART_WHOLE)
    {
        endTransmission(message);
    }
    else if (message->getKind() == IRadioSignal::SIGNAL_PART_PREAMBLE)
    {
        continueTransmission(message);
    }
    else if (message->getKind() == IRadioSignal::SIGNAL_PART_HEADER)
    {
        continueTransmission(message);
    }
    else if (message->getKind() == IRadioSignal::SIGNAL_PART_DATA)
    {
        endTransmission(message);
    }
    else
    {
        throw cRuntimeError("Unknown signal part");
    }
}

void LoRaRelayRadio::startTransmission(Packet *macFrame, IRadioSignal::SignalPart part)
{
    if (iAmTransmiting == false)
    {
        iAmTransmiting = true;
        
        auto radioFrame = createSignal(macFrame);
        auto transmission = radioFrame->getTransmission();

        cMessage *txTimer = new cMessage("transmissionTimer");
        txTimer->setKind(part);
        txTimer->setContextPointer(radioFrame);
        scheduleAt(transmission->getEndTime(part), txTimer);
        emit(transmissionStartedSignal, check_and_cast<const cObject*>(transmission));
        EV_INFO << "Transmission started: " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << transmission << endl;
        check_and_cast<LoRaMedium *>(medium.get())->emit(IRadioMedium::signalDepartureStartedSignal, check_and_cast<const cObject *>(transmission));
    }
    else
    {
        delete macFrame;
    }
}

void LoRaRelayRadio::continueTransmission(cMessage *timer)
{
    auto previousPart = (IRadioSignal::SignalPart)timer->getKind();
    auto nextPart = (IRadioSignal::SignalPart)(previousPart + 1);
    auto radioFrame = static_cast<WirelessSignal *>(timer->getContextPointer());
    auto transmission = radioFrame->getTransmission();
    EV_INFO << "Transmission ended: " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(previousPart) << " as " << radioFrame->getTransmission() << endl;
    timer->setKind(nextPart);
    scheduleAt(transmission->getEndTime(nextPart), timer);
    EV_INFO << "Transmission started: " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(nextPart) << " as " << transmission << endl;
}

void LoRaRelayRadio::endTransmission(cMessage *timer)
{
    iAmTransmiting = false;
    auto part = (IRadioSignal::SignalPart)timer->getKind();
    auto signal = static_cast<WirelessSignal *>(timer->getContextPointer());
    auto transmission = signal->getTransmission();
    timer->setContextPointer(nullptr);

    EV_INFO << "Transmission ended: " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << transmission << endl;
    emit(transmissionEndedSignal, check_and_cast<const cObject *>(transmission));
    check_and_cast<LoRaMedium *>(medium.get())->emit(IRadioMedium::signalDepartureEndedSignal, check_and_cast<const cObject *>(transmission));
    delete(timer);
}

bool LoRaRelayRadio::isReceptionTimer(const cMessage *message) const
{
    return !strcmp(message->getName(), "receptionTimer");
}

// void LoRaRelayRadio::handleReceptionTimer(cMessage *message)
// {
//     if (message->getKind() == IRadioSignal::SIGNAL_PART_WHOLE)
//         endReception(message);
//     else if (message->getKind() == IRadioSignal::SIGNAL_PART_PREAMBLE)
//         continueReception(message);
//     else if (message->getKind() == IRadioSignal::SIGNAL_PART_HEADER)
//         continueReception(message);
//     else if (message->getKind() == IRadioSignal::SIGNAL_PART_DATA)
//         endReception(message);
//     else
//         throw cRuntimeError("Unknown self message");
// }

void LoRaRelayRadio::startReception(cMessage *timer, IRadioSignal::SignalPart part)
{
    auto radioFrame = static_cast<WirelessSignal *>(timer->getContextPointer());
    auto arrival = radioFrame->getArrival();
    auto reception = radioFrame->getReception();

    emit(LoRaRelayRadioReceptionStarted, true);

    if (simTime() >= getSimulation()->getWarmupPeriod())
    {
        LoRaRelayRadioReceptionStarted_counter++;
    }
    if (isReceiverMode(radioMode) && arrival->getStartTime() == simTime() && iAmTransmiting == false)
    {
        auto transmission = radioFrame->getTransmission();
        auto isReceptionAttempted = medium->isReceptionAttempted(this, transmission, part);
        EV_INFO << "LoRaRelayRadio ReceptionStarted: " << (isReceptionAttempted ? "attempting" : "not attempting") << " " << (WirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
        if (isReceptionAttempted)
        {
            concurrentReceptions.push_back(timer);
            receptionTimer = timer;
        }
    }
    else
    {
        EV_INFO << "LoRaGWRadio Reception started: ignoring " << (WirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
    }
    timer->setKind(part);
    scheduleAt(reception->getEndTime(part), timer);
    radioMode = RADIO_MODE_TRANSCEIVER;
    check_and_cast<LoRaMedium*>(medium.get())->emit(IRadioMedium::signalArrivalStartedSignal, check_and_cast<const cObject*>(reception));
    //TODO: understand this queue type and manage the packets at upper layer whether they are gw frames or ed frames
    EV << "[MSDebug] start reception, size : " << concurrentReceptions.size() << endl;
}

void LoRaRelayRadio::continueReception(cMessage *timer)
{
    auto previousPart = (IRadioSignal::SignalPart)timer->getKind();
    auto nextPart = (IRadioSignal::SignalPart)(previousPart + 1);
    auto radioFrame = static_cast<WirelessSignal *>(timer->getControlInfo());
    auto arrival = radioFrame->getArrival();
    auto reception = radioFrame->getReception();

    std::list<cMessage *>::iterator it;
    for (it=concurrentReceptions.begin(); it!=concurrentReceptions.end(); it++) {
        if(*it == timer) receptionTimer = timer;
    }

    if (timer == receptionTimer && isReceiverMode(radioMode) && arrival->getEndTime(previousPart) == simTime() && iAmTransmiting == false) {
        auto transmission = radioFrame->getTransmission();
        bool isReceptionSuccessful = medium->isReceptionSuccessful(this, transmission, previousPart);
        EV_INFO << "LoRaRelayRadio Reception ended: " << (isReceptionSuccessful ? "successfully" : "unsuccessfully") << " for " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(previousPart) << " as " << reception << endl;
        if (!isReceptionSuccessful) {
            receptionTimer = nullptr;
            concurrentReceptions.remove(timer);
        }
        auto isReceptionAttempted = medium->isReceptionAttempted(this, transmission, nextPart);
        EV_INFO << "LoRaRelayRadio Reception started: " << (isReceptionAttempted ? "attempting" : "not attempting") << " " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(nextPart) << " as " << reception << endl;
        if (!isReceptionAttempted) {
            receptionTimer = nullptr;
            concurrentReceptions.remove(timer);
        }
    }
    else {
        EV_INFO << "LoRaRelayRadio Reception ended: ignoring " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(previousPart) << " as " << reception << endl;
        EV_INFO << "LoRaRelayRadio Reception started: ignoring " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(nextPart) << " as " << reception << endl;
    }
    timer->setKind(nextPart);
    scheduleAt(arrival->getEndTime(nextPart), timer);
    //updateTransceiverState();
    //updateTransceiverPart();
    radioMode = RADIO_MODE_TRANSCEIVER;
}

void LoRaRelayRadio::endReception(cMessage *timer)
{
    auto part = (IRadioSignal::SignalPart)timer->getKind();
    auto radioFrame = static_cast<WirelessSignal *>(timer->getControlInfo());
    auto arrival = radioFrame->getArrival();
    auto reception = radioFrame->getReception();
    std::list<cMessage *>::iterator it;
    for (it=concurrentReceptions.begin(); it!=concurrentReceptions.end(); it++) {
        if(*it == timer) receptionTimer = timer;
    }
    if (timer == receptionTimer && isReceiverMode(radioMode) && arrival->getEndTime() == simTime() && iAmTransmiting == false) 
    {
        auto transmission = radioFrame->getTransmission();
        // TODO: this would draw twice from the random number generator in isReceptionSuccessful: auto isReceptionSuccessful = medium->isReceptionSuccessful(this, transmission, part);
        auto isReceptionSuccessful = medium->getReceptionDecision(this, radioFrame->getListening(), transmission, part)->isReceptionSuccessful();
        EV_INFO << "LoRaRelayRadio Reception ended: " << (isReceptionSuccessful ? "successfully" : "unsuccessfully") << " for " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
        if(isReceptionSuccessful) {
            auto macFrame = medium->receivePacket(this, radioFrame);
            take(macFrame);
            emit(packetSentToUpperSignal, macFrame);
            emit(LoRaRelayRadioReceptionFinishedCorrect, true);
            if (simTime() >= getSimulation()->getWarmupPeriod())
                LoRaRelayRadioReceptionFinishedCorrect_counter++;
            EV << macFrame->getCompleteStringRepresentation(evFlags) << endl;
            sendUp(macFrame);
        }
        receptionTimer = nullptr;
        concurrentReceptions.remove(timer);
    }
    else
        EV_INFO << "LoRaRelayRadio Reception ended: ignoring " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
    //updateTransceiverState();
    //updateTransceiverPart();
    radioMode = RADIO_MODE_TRANSCEIVER;
    check_and_cast<LoRaMedium *>(medium.get())->emit(IRadioMedium::signalArrivalEndedSignal, check_and_cast<const cObject *>(reception));
    delete timer;
}

void LoRaRelayRadio::abortReception(cMessage *timer)
{
    auto radioFrame = static_cast<WirelessSignal *>(timer->getControlInfo());
    auto part = (IRadioSignal::SignalPart)timer->getKind();
    auto reception = radioFrame->getReception();
    EV_INFO << "LoRaRelayRadio Reception aborted: for " << (IWirelessSignal*)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
    if (timer == receptionTimer) {
        concurrentReceptions.remove(timer);
        receptionTimer = nullptr;
    }
    updateTransceiverState();
    updateTransceiverPart();
}

void LoRaRelayRadio::sendUp(Packet *macFrame)
{
    auto signalPowerInd = macFrame->findTag<SignalPowerInd>();
    if (signalPowerInd == nullptr)
        throw cRuntimeError("signal Power indication not present");
    auto snirInd =  macFrame->findTag<SnirInd>();
    if (snirInd == nullptr)
        throw cRuntimeError("snir indication not present");

    auto errorTag = macFrame->findTag<ErrorRateInd>();

    emit(minSNIRSignal, snirInd->getMinimumSnir());
    if (errorTag && !std::isnan(errorTag->getPacketErrorRate()))
        emit(packetErrorRateSignal, errorTag->getPacketErrorRate());
    if (errorTag && !std::isnan(errorTag->getBitErrorRate()))
        emit(bitErrorRateSignal, errorTag->getBitErrorRate());
    if (errorTag && !std::isnan(errorTag->getSymbolErrorRate()))
        emit(symbolErrorRateSignal, errorTag->getSymbolErrorRate());
    EV_INFO << "Sending up " << macFrame << endl;
    NarrowbandRadioBase::sendUp(macFrame);
    //send(macFrame, upperLayerOut);
}

} /* namespace lpwan */


