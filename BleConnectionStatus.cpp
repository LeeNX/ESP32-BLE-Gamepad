#include "BleConnectionStatus.h"

BleConnectionStatus::BleConnectionStatus(void)
{
}

void BleConnectionStatus::onConnect(NimBLEServer *pServer, ble_gap_conn_desc* desc)
{
    this->connected = true;

    Serial.print("Client address: ");
    Serial.println(NimBLEAddress(desc->peer_id_addr).toString().c_str());
    Serial.println(NimBLEAddress(desc->peer_ota_addr).toString().c_str());

    //Serial.println(connInfo.getAddress().toString().c_str());
    /** We can use the connection handle here to ask for different connection parameters.
        *  Args: connection handle, min connection interval, max connection interval
        *  latency, supervision timeout.
        *  Units; Min/Max Intervals: 1.25 millisecond increments.
        *  Latency: number of intervals allowed to skip.
        *  Timeout: 10 millisecond increments, try for 5x interval time for best results.
        */
    //pServer->updateConnParams(connInfo.getConnHandle(), 24, 48, 0, 60);
    pServer->updateConnParams(desc->conn_handle, 6, 7, 0, 600);
}

void BleConnectionStatus::onDisconnect(NimBLEServer *pServer, ble_gap_conn_desc* desc)
{
    this->connected = false;
    Serial.println("Client disconnected:");
    //Serial.print(pServer->getPeerAddress().toString().c_str());
    Serial.print(NimBLEAddress(desc->peer_ota_addr).toString().c_str());
    //Serial.println("Start advertising ...");
    //NimBLEDevice::startAdvertising();

}

/*
void BleConnectionStatus::onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo)
{
    Serial.printf("MTU updated: %u for connection ID: %u\n", MTU, connInfo.getConnHandle());
};
*/

/*
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");
      }
    }
};
*/