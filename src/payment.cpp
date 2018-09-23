// Byteduino lib - papabyte.com
// MIT License

#include "payment.h"

extern WebSocketsClient webSocketForHub;
extern Byteduino byteduino_device;

void requestDefinition(const char* address){

	char output[130];
	const size_t bufferSize = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(3);
	StaticJsonBuffer<bufferSize> jsonBuffer;
	JsonArray & mainArray = jsonBuffer.createArray();

	mainArray.add("request");
	JsonObject & objRequest= jsonBuffer.createObject();

	objRequest["command"] = "light/get_definition";
	objRequest["params"] = address;
	
	char tag[12];
	getTag(tag, GET_ADDRESS_DEFINITION);
	objRequest["tag"] = (const char*) tag;
	
	mainArray.add(objRequest);
	mainArray.printTo(output);
#ifdef DEBUG_PRINT
	Serial.println(output);
#endif
	webSocketForHub.sendTXT(output);


}


void handleDefinition(JsonArray& arr) {

	if (arr["response"] == nullptr){
		byteduino_device.requireDefinition = true;
	} else {
		byteduino_device.requireDefinition = false;
	}

	byteduino_device.isDefinitionReceived = true;
}




void requestInputsForAmount(int amount, const char * address, const char * asset){

	char output[260];
	const size_t bufferSize = JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4);
	StaticJsonBuffer<bufferSize> jsonBuffer;
	JsonArray & mainArray = jsonBuffer.createArray();

	mainArray.add("request");
	JsonObject & objRequest= jsonBuffer.createObject();
	JsonObject & objParams = jsonBuffer.createObject();
	JsonArray & arrAddresses = jsonBuffer.createArray();

	objParams["asset"] = asset;

	objParams["last_ball_mci"] = 1000000000;
	objParams["amount"] = amount;

	arrAddresses.add(address);
	objParams["addresses"] = arrAddresses;

	objRequest["command"] = "light/pick_divisible_coins_for_amount";
	
	objRequest["params"] = objParams;

	char tag[12];
	getTag(tag, GET_INPUTS_FOR_AMOUNT);
	objRequest["tag"] = (const char*) tag;

	mainArray.add(objRequest);
	mainArray.printTo(output);
#ifdef DEBUG_PRINT
	Serial.println(output);
#endif
	webSocketForHub.sendTXT(output);

}

void handleInputsForAmount(JsonArray& arr) {



}


void getParentsAndLastBallAndWitnesses(){


	char output[200];
	const size_t bufferSize = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(4);
	StaticJsonBuffer<bufferSize> jsonBuffer;
	JsonArray & mainArray = jsonBuffer.createArray();
	JsonObject & objParams = jsonBuffer.createObject();

	mainArray.add("request");
	JsonObject & objRequest= jsonBuffer.createObject();

	objRequest["command"] = "light/get_parents_and_last_ball_and_witness_list_unit";
	objRequest["params"] = objParams;

	char tag[12];
	getTag(tag, GET_PARENTS_BALL_WITNESSES);
	objRequest["tag"] = (const char*) tag;
	
	mainArray.add(objRequest);
	mainArray.printTo(output);
#ifdef DEBUG_PRINT
	Serial.println(output);
#endif
	webSocketForHub.sendTXT(output);

}

void handleUnitProps(JsonArray& arr){

	


}

void getPaymentAddressFromPubKey(const char * pubKey, char * paymentAddress) {
	char output[200];
	const size_t bufferSize = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(4);
	StaticJsonBuffer<bufferSize> jsonBuffer;
	JsonArray & mainArray = jsonBuffer.createArray();
	JsonObject & objSig = jsonBuffer.createObject();
	objSig["pubkey"] = pubKey;
	mainArray.add("sig");
	mainArray.add(objSig);
	getChash160ForArray (mainArray,paymentAddress);
}
