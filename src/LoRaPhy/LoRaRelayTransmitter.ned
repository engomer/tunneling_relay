//
//   LoRaRelayTransmitter.ned
// 
//    Created on: Jan 24, 2024
//        Author: handybald
//
package lpwan.LoRaPhy;

import inet.physicallayer.wireless.common.base.packetlevel.NarrowbandTransmitterBase;

module LoRaRelayTransmitter extends NarrowbandTransmitterBase
{
    parameters:
        @signal[LoRaRelayTransmissionCreated](type=bool); // optional
        @statistic[LoRaRelayTransmissionCreated](source=LoRaRelayTransmissionCreated; record=count);
        modulation = default("LoRaModulation");
        @class(LoRaRelayTransmitter);
}
