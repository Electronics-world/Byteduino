// Byteduino lib - papabyte.com
// MIT License

#include "ArduinoJson.h"
#include "byteduino.h"

void requestDefinition(const char* address);
void handleDefinition(JsonArray& arr) ;
void requestInputsForAmount(int amount, const char * address, const char * asset);
void handleInputsForAmount(JsonArray& arr);
void getParentsAndLastBallAndWitnesses();
void getPaymentAddressFromPubKey(const char * pubKey, char * paymentAddress);
void handleUnitProps(JsonArray& arr);