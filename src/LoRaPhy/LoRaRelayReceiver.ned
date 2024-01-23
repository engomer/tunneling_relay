//
//   LoRaRelayTransmitter.ned
// 
//    Created on: Jan 24, 2024
//        Author: handybald
//

package lpwan.LoRaPhy;

package inet.physicallayer.wireless.common.base.packetlevel;
import inet.physicallayer.wireless.common.base.packetlevel.NarrowbandReceiverBase;

module LoRaRelayReceiver extends NarrowbandReceiverBase
{
    parameters:
        @signal[LoRaRelayReceptionCollision](type=bool)
        @statistic[LoRaRelayReceptionCollision](source=LoRaRelayReceptionCollision, record=count);
        errorModel.typename = default("");
        modulation = default("BPSK");
        bool alohaChannelModel = default(false);
        @class(LoRaRelayReceiver);
        @display("i=block/wrx");
}

