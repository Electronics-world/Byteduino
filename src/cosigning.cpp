// Byteduino lib - papabyte.com
// MIT License

#include "cosigning.h"

walletCreation newWallet;
extern bufferPackageSent bufferForPackageSent;
extern Byteduino byteduino_device;
waitingConfirmationRoom waitingConfirmationSignature;

cbSignatureToConfirm _cbSignatureToConfirm;

void setCbSignatureToConfirm(cbSignatureToConfirm cbToSet){
	_cbSignatureToConfirm = cbToSet;
}

void removeKeyIfExisting(const char * key, JsonObject& object){
	if (object.containsKey(key)){
		object.remove(key);
#ifdef DEBUG_PRINT
		Serial.println(F("remove key"));
		Serial.println(key);
#endif
	}
	
}

bool confirmSignature(const uint8_t hash[]){
	
	for (int i=0;i<32;i++){
		if (hash[i]!= waitingConfirmationSignature.hash[i]){
			return false;
		}
	}
	
	waitingConfirmationSignature.isConfirmed = true;
	return true;
}

void treatWaitingSignature(){
	if (!waitingConfirmationSignature.isFree && waitingConfirmationSignature.isConfirmed){
		if (bufferForPackageSent.isFree){
			waitingConfirmationSignature.isFree = true;
			char output[200];
			StaticJsonBuffer<400> jsonBuffer;
			JsonObject & message = jsonBuffer.createObject();
			message["from"] = byteduino_device.deviceAddress;
			message["device_hub"] = byteduino_device.hub;
			message["subject"] = "signature";

			JsonObject & objBody = jsonBuffer.createObject();
			char hashB64[45];
			Base64.encode(hashB64,(char *) waitingConfirmationSignature.hash, 32);
			objBody["signed_text"]= hashB64;
			objBody["signature"]= waitingConfirmationSignature.sigb64;
			objBody["signing_path"]= waitingConfirmationSignature.signing_path;
			objBody["address"]= waitingConfirmationSignature.address;

			message["body"]= objBody;

			bufferForPackageSent.isRecipientTempMessengerKeyKnown = false;
			memcpy(bufferForPackageSent.recipientPubkey,waitingConfirmationSignature.recipientPubKey,45);
			memcpy(bufferForPackageSent.recipientHub,byteduino_device.hub,strlen(byteduino_device.hub)+1);
			bufferForPackageSent.isFree = false;
			bufferForPackageSent.isRecipientKeyRequested = false;
			message.printTo(bufferForPackageSent.message);
#ifdef DEBUG_PRINT
			Serial.println(bufferForPackageSent.message);
#endif
		} else {
#ifdef DEBUG_PRINT
			Serial.println(F("Buffer not free to send message"));
#endif
		}
	}
	
}
/*	"unsigned_unit": {
			"version": "1.0t",
			"alt": "2",
			"messages": [{
				"app": "data_feed",
				"payload_location": "inline",
				"payload_hash": "GgIxFXZw0uheXXnmwF0Wr8KEL1C7LcxcfqwrCJdlJ1U=",
				"payload": {
					"test": "1234"
				}
			}, {
				"app": "payment",
				"payload_location": "inline",
				"payload_hash": "eqHXLjxWO3l5dcnxkE/sp54B2nC/bpKHVg108DFUlKY=",
				"payload": {
					"outputs": [{
						"address": "ORH6FKZTWWNSQJETPL5WYRC32KZZZQLC",
						"amount": 48996609
					}],
					"inputs": [{
						"unit": "Drbo4dt05IYwDYE+yFWjc6mYHX7AeidzOaO4UiSGD30=",
						"message_index": 0,
						"output_index": 0
					}]
				}
			}],*/


void stripSignAndAddToConfirmationRoom(const char recipientPubKey[45], JsonObject& body){

	JsonObject& unsignedUnit = body["unsigned_unit"];
	int authorsSize = unsignedUnit["authors"].size();

	for (int i = 0;i<authorsSize;i++){
		JsonObject& objAuthentifier = unsignedUnit["authors"][i];
		objAuthentifier.remove("authentifiers");
#ifdef DEBUG_PRINT
		Serial.println(F("remove authentifier"));
#endif
	}

	//these unit properties aren't to be signed
	removeKeyIfExisting("unit", unsignedUnit);
	removeKeyIfExisting("headers_commission", unsignedUnit);
	removeKeyIfExisting("payload_commission", unsignedUnit);
	removeKeyIfExisting("main_chain_index", unsignedUnit);
	removeKeyIfExisting("timestamp", unsignedUnit);
	size_t messagesSize = unsignedUnit["messages"].size();
	
	//we create a JSON digest about what will signed
	StaticJsonBuffer<400> jsonBuffer;
	JsonArray & arrayDigest = jsonBuffer.createArray();
	JsonObject & objPayments= jsonBuffer.createObject();
	for (size_t i = 0;i<messagesSize;i++){
		const char * app = unsignedUnit["messages"][i]["app"];
		if (strcmp(app,"payment") == 0){
			if (unsignedUnit["messages"][i]["payload"]["outputs"].is<JsonArray>()){
				size_t outputsSize = unsignedUnit["messages"][i]["payload"]["outputs"].size();
				for (size_t j = 0; j<outputsSize; j++){
					const char* address = unsignedUnit["messages"][i]["payload"]["outputs"][j]["address"];
					if (address!= nullptr) {
						if (unsignedUnit["messages"][i]["payload"]["outputs"][j]["amount"].is<int>()){
							int amountToAdd = unsignedUnit["messages"][i]["payload"]["outputs"][j]["amount"];
							if (!objPayments.containsKey(address)){
								objPayments[address] = amountToAdd;
							} else {
								int previousAmount = objPayments[address];
								objPayments[address] = previousAmount + amountToAdd;
							}
						}
					}
				}
			}
		} else {
			if (unsignedUnit["messages"][i]["payload"].is<JsonObject>()){
				arrayDigest.add(unsignedUnit["messages"][i]["payload"]);
			}
		}
		
		removeKeyIfExisting("payload", unsignedUnit["messages"][i]);
		removeKeyIfExisting("payload_uri", unsignedUnit["messages"][i]);
	}
	arrayDigest.add(objPayments);
	
	arrayDigest.printTo(waitingConfirmationSignature.JsonDigest);
	Serial.println(waitingConfirmationSignature.JsonDigest);

	uint8_t hash[32];
	getSHA256ForJsonObject(hash, unsignedUnit);
	char sigb64 [89];
	getB64SignatureForHash(sigb64 ,byteduino_device.keys.privateM4400, hash,32);
																
	const char * signing_path= body["signing_path"];
	const char * address= body["address"];
	memcpy(waitingConfirmationSignature.recipientPubKey ,recipientPubKey,45);
	memcpy(waitingConfirmationSignature.hash ,hash,32);
	memcpy(waitingConfirmationSignature.sigb64 ,sigb64,89);
	memcpy(waitingConfirmationSignature.signing_path ,signing_path,strlen(signing_path)+1);
	memcpy(waitingConfirmationSignature.address ,address, 33);
	waitingConfirmationSignature.isConfirmed = false;
	waitingConfirmationSignature.isFree =false;
	if (_cbSignatureToConfirm){
		_cbSignatureToConfirm(waitingConfirmationSignature.hash, waitingConfirmationSignature.JsonDigest);
	}
}


void handleSignatureRequest(const char senderPubkey[45],JsonObject& receivedPackage){
	if (receivedPackage["body"]["unsigned_unit"].is<JsonObject>()){

		if (receivedPackage["body"]["unsigned_unit"]["messages"].is<JsonArray>()){

			int arraySize = receivedPackage["body"]["unsigned_unit"]["messages"].size();
			if (arraySize > 0){
				for (int i = 0;i<arraySize;i++){
					if(receivedPackage["body"]["unsigned_unit"]["messages"][i].is<JsonObject>()){
						if(receivedPackage["body"]["unsigned_unit"]["messages"][i]["payload"].is<JsonObject>()){
							const char * payloadHash = receivedPackage["body"]["unsigned_unit"]["messages"][i]["payload_hash"];
							char hashB64[45];
							getBase64HashForJsonObject (hashB64, receivedPackage["body"]["unsigned_unit"]["messages"][i]["payload"]);
							if (strcmp(hashB64,payloadHash) == 0){
								stripSignAndAddToConfirmationRoom(senderPubkey,receivedPackage["body"]);
							}else{
#ifdef DEBUG_PRINT
							Serial.println(F("payload hash does not match"));
							Serial.println(hashB64);
							Serial.println(payloadHash);
#endif
							}

						}else{
#ifdef DEBUG_PRINT
							Serial.println(F("payload must be an object"));
#endif
							return;
						}
					}else{
#ifdef DEBUG_PRINT
						 Serial.println(F("message must be an object"));
#endif
						return;
					}

				}

			} else {
#ifdef DEBUG_PRINT
				Serial.println(F("arraySize must be >0"));
#endif
			}

		} else {
#ifdef DEBUG_PRINT
			Serial.println(F("messages must be an array"));
#endif
		}
	} else {
#ifdef DEBUG_PRINT
	Serial.println(F("unsigned_unit must be object"));
#endif
	}

}

