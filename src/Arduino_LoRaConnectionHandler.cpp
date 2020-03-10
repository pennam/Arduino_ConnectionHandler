/*
   This file is part of ArduinoIoTCloud.

   Copyright 2019 ARDUINO SA (http://www.arduino.cc/)

   This software is released under the GNU General Public License version 3,
   which covers the main part of arduino-cli.
   The terms of this license can be found at:
   https://www.gnu.org/licenses/gpl-3.0.en.html

   You can be released from the requirements of the above licenses by purchasing
   a commercial license. Buying such a license is mandatory if you want to modify or
   otherwise use the software for commercial activities involving the Arduino
   software without disclosing the source code of your own applications. To purchase
   a commercial license, send an email to license@arduino.cc.
*/

/******************************************************************************
   INCLUDE
 ******************************************************************************/

#if defined(ARDUINO_SAMD_MKRWAN1300) || defined(ARDUINO_SAMD_MKRWAN1310) /* Only compile if the board has LoRa */

#include "Arduino_LoRaConnectionHandler.h"

/******************************************************************************
   TYPEDEF
 ******************************************************************************/

typedef enum
{
  LORA_ERROR_ACK_NOT_RECEIVED     = -1,
  LORA_ERROR_GENERIC              = -2,
  LORA_ERROR_WRONG_PARAM          = -3,
  LORA_ERROR_COMMUNICATION_BUSY   = -4,
  LORA_ERROR_MESSAGE_OVERFLOW     = -5,
  LORA_ERROR_NO_NETWORK_AVAILABLE = -6,
  LORA_ERROR_RX_PACKET            = -7,
  LORA_ERROR_REASON_UNKNOWN       = -8,
  LORA_ERROR_MAX_PACKET_SIZE      = -20
} LoRaCommunicationError;

/******************************************************************************
   CONSTANTS
 ******************************************************************************/

static unsigned int const CHECK_INTERVAL_TABLE[] =
{
  /* INIT          */ 100,
  /* CONNECTING    */ 500,
  /* CONNECTED     */ 10000,
  /* GETTIME       */ 100,
  /* DISCONNECTING */ 100,
  /* DISCONNECTED  */ 1000,
  /* CLOSED        */ 1000,
  /* ERROR         */ 1000
};

/******************************************************************************
   CTOR/DTOR
 ******************************************************************************/

LoRaConnectionHandler::LoRaConnectionHandler(char const * appeui, char const * appkey, _lora_band const band, _lora_class const device_class)
: _appeui(appeui)
, _appkey(appkey)
, _band(band)
, _device_class(device_class)
, _lastConnectionTickTime(millis())
, _keep_alive(false)
{

}

/******************************************************************************
   PUBLIC MEMBER FUNCTIONS
 ******************************************************************************/

NetworkConnectionState LoRaConnectionHandler::check()
{
  unsigned long const now = millis();
  unsigned int const connectionTickTimeInterval = CHECK_INTERVAL_TABLE[static_cast<unsigned int>(_netConnectionState)];

  if((now - _lastConnectionTickTime) > connectionTickTimeInterval)
  {
    _lastConnectionTickTime = now;

    switch (_netConnectionState)
    {
      case NetworkConnectionState::INIT:          _netConnectionState = update_handleInit();          break;
      case NetworkConnectionState::CONNECTING:    _netConnectionState = update_handleConnecting();    break;
      case NetworkConnectionState::CONNECTED:     _netConnectionState = update_handleConnected();     break;
      case NetworkConnectionState::DISCONNECTING: _netConnectionState = update_handleDisconnecting(); break;
      case NetworkConnectionState::DISCONNECTED:  _netConnectionState = update_handleDisconnected();  break;
      case NetworkConnectionState::ERROR:                                                            break;
      case NetworkConnectionState::CLOSED:                                                           break;
    }
  }

  return _netConnectionState;
}

int LoRaConnectionHandler::write(const uint8_t * buf, size_t size)
{
  _modem.beginPacket();
  _modem.write(buf, size);
  int const err = _modem.endPacket(true);

  if (err != size)
  {
    switch (err)
    {
      case LoRaCommunicationError::LORA_ERROR_ACK_NOT_RECEIVED:     Debug.print(DBG_ERROR, "Message ack was not received, the message could not be delivered"); break;
      case LoRaCommunicationError::LORA_ERROR_GENERIC:              Debug.print(DBG_ERROR, "LoRa generic error (LORA_ERROR)");                                  break;
      case LoRaCommunicationError::LORA_ERROR_WRONG_PARAM:          Debug.print(DBG_ERROR, "LoRa malformed param error (LORA_ERROR_PARAM");                     break;
      case LoRaCommunicationError::LORA_ERROR_COMMUNICATION_BUSY:   Debug.print(DBG_ERROR, "LoRa chip is busy (LORA_ERROR_BUSY)");                              break;
      case LoRaCommunicationError::LORA_ERROR_MESSAGE_OVERFLOW:     Debug.print(DBG_ERROR, "LoRa chip overflow error (LORA_ERROR_OVERFLOW)");                   break;
      case LoRaCommunicationError::LORA_ERROR_NO_NETWORK_AVAILABLE: Debug.print(DBG_ERROR, "LoRa no network error (LORA_ERROR_NO_NETWORK)");                    break;
      case LoRaCommunicationError::LORA_ERROR_RX_PACKET:            Debug.print(DBG_ERROR, "LoRa rx error (LORA_ERROR_RX)");                                    break;
      case LoRaCommunicationError::LORA_ERROR_REASON_UNKNOWN:       Debug.print(DBG_ERROR, "LoRa unknown error (LORA_ERROR_UNKNOWN)");                          break;
      case LoRaCommunicationError::LORA_ERROR_MAX_PACKET_SIZE:      Debug.print(DBG_ERROR, "Message length is bigger than max LoRa packet!");                   break;
    }
  }
  else
  {
    Debug.print(DBG_INFO, "Message sent correctly!");
  }
  return err;
}

int LoRaConnectionHandler::read()
{
  return _modem.read();
}

bool LoRaConnectionHandler::available()
{
  return _modem.available();
}

void LoRaConnectionHandler::connect()
{
  if (_netConnectionState != NetworkConnectionState::INIT && _netConnectionState != NetworkConnectionState::CONNECTING)
  {
    _keep_alive = true;
    _netConnectionState = NetworkConnectionState::INIT;
  }
}
void LoRaConnectionHandler::disconnect()
{
  /* Do nothing */
}

/******************************************************************************
   PRIVATE MEMBER FUNCTIONS
 ******************************************************************************/

NetworkConnectionState LoRaConnectionHandler::update_handleInit()
{
  if (!_modem.begin(_band))
  {
    execCallback(NetworkConnectionEvent::ERROR, 0);
    Debug.print(DBG_ERROR, "Something went wrong; are you indoor? Move near a window, then reset and retry.");
  }
  //A delay is required between _modem.begin(band) and _modem.joinOTAA(appeui, appkey) in order to let the chip to be correctly initialized before the connection attempt
  delay(100);
  _modem.configureClass(_device_class);
  delay(100);
  Debug.print(DBG_INFO, "Connecting to the network");
  return NetworkConnectionState::CONNECTING;
}

NetworkConnectionState LoRaConnectionHandler::update_handleConnecting()
{
  bool const network_status = _modem.joinOTAA(_appeui, _appkey);
  if (network_status != true)
  {
    execCallback(NetworkConnectionEvent::ERROR, 0);
    Debug.print(DBG_ERROR, "Something went wrong; are you indoor? Move near a window, then reset and retry.");
    return NetworkConnectionState::ERROR;
  }

  Debug.print(DBG_INFO, "Connected to the network");
  execCallback(NetworkConnectionEvent::CONNECTED, 0);
  return NetworkConnectionState::CONNECTED;
}

NetworkConnectionState LoRaConnectionHandler::update_handleConnected()
{
  bool const network_status = _modem.connected();
  if (network_status != true) {
    execCallback(NetworkConnectionEvent::DISCONNECTED, 0);

    Debug.print(DBG_ERROR, "Connection to the network lost.");
    if (_keep_alive)
    {
      Debug.print(DBG_ERROR, "Attempting reconnection");
    }
    return NetworkConnectionState::DISCONNECTED;
  }
  return NetworkConnectionState::CONNECTED;
}

NetworkConnectionState LoRaConnectionHandler::update_handleDisconnecting()
{
  execCallback(NetworkConnectionEvent::DISCONNECTED, 0);

  Debug.print(DBG_ERROR, "Connection to the network lost.");
  if (_keep_alive)
  {
    Debug.print(DBG_ERROR, "Attempting reconnection");
  }
  return NetworkConnectionState::DISCONNECTED;
}

NetworkConnectionState LoRaConnectionHandler::update_handleDisconnected()
{
  if (_keep_alive)
  {
    return NetworkConnectionState::INIT;
  }
  else
  {
    return NetworkConnectionState::CLOSED;
  }
}

#endif
