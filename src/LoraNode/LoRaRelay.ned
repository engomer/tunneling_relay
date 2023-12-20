//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package lpwan.LoraNode;

import inet.node.contract.INetworkNode;
import inet.mobility.static.StationaryMobility;
import inet.networklayer.common.InterfaceTable;
import inet.applications.contract.IApp;
import lpwan.LoRa.LoRaRelayNic;

module LoRaRelay like INetworkNode
{
    parameters:
        @networkNode();
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.energySourceModule = default(exists(energyStorage) ? absPath(".energyStorage") : "");
        
        LoRaRelayNic.radio.antenna.mobilityModule = default("^.^.^.mobility");
        *.mobilityModule = default(exists(mobility) ? absPath(".mobility") : "");
        
        @display("bgb=783.96,587.688;i=device/modem");
        
    submodules:
        interfaceTable: InterfaceTable{
            @display("p=30,26");
        }
        mobility: StationaryMobility {
            @display("p=24,88");
        }
        LoRaNic: LoRaRelayNic {
            @display("p=137,239");
        }
        app[numApps]: <> like IApp {
            parameters:
                @display("p=375,76,row,150");
        }
    
    connections allowunconnected:
        for i=0..numApps-1 {		
        	app[i].socketOut --> LoRaNic.upperLayerIn;
        	LoRaNic.upperLayerOut --> app[i].socketIn;
        }
}