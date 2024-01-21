/*
 * LoRaRelayMac.h
 *
 *  Created on: Nov 21, 2023
 *      Author: handybald
 */

#ifndef LORA_LORARELAYMAC_H_
#define LORA_LORARELAYMAC_H_

namespace lpwan{

class LoRaRelayMac{

public:
    inet::MacAddress getMacAddress();

};
} /* namespace lpwan */



#endif /* LORA_LORARELAYMAC_H_ */
