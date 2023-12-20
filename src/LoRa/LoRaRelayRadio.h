/*
 * LoRaRelayRadio.h
 *
 *  Created on: Nov 21, 2023
 *      Author: handybald
 */

#ifndef LORA_LORARELAYRADIO_H_
#define LORA_LORARELAYRADIO_H_

#include "inet/physicallayer/wireless/common/base/packetlevel/FlatRadioBase.h"

#include "LoRaPhy/LoRaRelayTransmitter.h"
#include "LoRaPhy/LoRaRelayReceiver.h"
#include "LoRaPhy/LoRaReception.h"
#include "LoRaPhy/LoRaTransmission.h"
#include "LoRaMacFrame_m.h"
#include "LoRaRelayMacFrame_m.h"

#include "inet/physicallayer/wireless/common//medium/RadioMedium.h"
#include "LoRaPhy/LoRaMedium.h"
#include "inet/common/LayeredProtocolBase.h"

namespace lpwan {

class LoRaRelayRadio : public FlatRadioBase {

private:

    void completeRadioModeSwitch(RadioMode newRadioMode);

protected:

    void initialize(int stage) override;
    virtual void finish() override;
    virtual void handleSelfMessage(cMessage *message) override;
    virtual void handleUpperPacket(Packet *packet) override;
    virtual void handleSignal(WirelessSignal *radioFrame) override;

    bool iAmTransmiting;
    virtual bool isTransmissionTimer(const cMessage *message) const;
    virtual void handleTransmissionTimer(cMessage *message) override;
    virtual void startTransmission(Packet *macFrame, IRadioSignal::SignalPart part) override;
    virtual void continueTransmission(cMessage *timer);
    virtual void endTransmission(cMessage *timer);

    virtual bool isReceptionTimer(const cMessage *message) const override;
    virtual void startReception(cMessage *timer, IRadioSignal::SignalPart part) override;
    virtual void continueReception(cMessage *timer) override;
    virtual void endReception(cMessage *timer) override;
    virtual void abortReception(cMessage *timer) override;

    virtual void sendUp(Packet *macFrame) override;

public:

    LoRaRelayRadio();
    virtual ~LoRaRelayRadio();

    virtual int getId() const override { return id; }

    virtual std::ostream& printToStream(std::ostream& stream, int level, int evFlags = 0) const override;

    virtual const IAntenna *getAntenna() const override { return antenna; }
    virtual const ITransmitter *getTransmitter() const override { return transmitter; }
    virtual const IReceiver *getReceiver() const override { return receiver; }
    virtual const IRadioMedium *getMedium() const override { return medium; }

    virtual const cGate *getRadioGate() const override { return radioIn; }

    virtual RadioMode getRadioMode() const override { return radioMode; }

    virtual ReceptionState getReceptionState() const override { return receptionState; }
    virtual TransmissionState getTransmissionState() const override { return transmissionState; }

    virtual const ITransmission *getTransmissionInProgress() const override;
    virtual const ITransmission *getReceptionInProgress() const override;

    virtual IRadioSignal::SignalPart getTransmittedSignalPart() const override;
    virtual IRadioSignal::SignalPart getReceivedSignalPart() const override;
    
    virtual void encapsulate(Packet *macFrame) const override;
    virtual void decapsulate(Packet *macFrame) const override;

public:

    double LoRaTP;
    inet::units::values::Hz LoRaCF;
    int LoRaSF;
    inet::units::values::Hz LoRaBW;
    int LoRaCR;
    bool LoRaUseHeader;

    std::list<cMessage *>concurrentReceptions;
    std::list<cMessage *>concurrentTransmissions;

    long LoRaRelayRadioReceptionStarted_counter;
    long LoRaRelayRadioReceptionFinishedCorrect_counter;
    simsignal_t LoRaRelayRadioReceptionStarted;
    simsignal_t LoRaRelayRadioReceptionFinishedCorrect;
};

}

#endif /* LORA_LORARELAYRADIO_H_ */
