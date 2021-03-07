/* Decode beacon trame balise
	*  Set outil/partition schem/No OTA 2MB APP/2MB SPIFFS
	*  Receive data from wifi beacon from emetter
	*  Send data over BLE to Android application
	*  Version 1/11/2020
*/

#include <Arduino.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <nvs_flash.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define DEFAULT_SCAN_LIST_SIZE 20
String trame;
BLECharacteristic * pCharacteristic;

/**
	* Enumeration des types de données à  envoyer
*/
enum DATA_TYPE: uint8_t {
	RESERVED = 0,
	PROTOCOL_VERSION = 1,
	ID_FR = 2,
	ID_ANSI_CTA = 3,
	LATITUDE = 4,        // In WS84 in degree * 1e5
	LONGITUDE = 5,       // In WS84 in degree * 1e5
	ALTITUDE = 6,        // In MSL in m
	HEIGTH = 7,          // From Home in m
	HOME_LATITUDE = 8,   // In WS84 in degree * 1e5
	HOME_LONGITUDE = 9,  // In WS84 in degree * 1e5
	GROUND_SPEED = 10,   // In m/s
	HEADING = 11,        // Heading in degree from north 0 to 359.
	NOT_DEFINED_END = 12,
};

/**
	* Tableau TLV (TYPE, LENGTH, VALUE) avec les tailles attendu des diffÃ©rents type données.
***/
static constexpr uint8_t TLV_LENGTH[] {
	0,  // [DATA_TYPE::RESERVED]
	1,  // [DATA_TYPE::PROTOCOL_VERSION]
	30, // [DATA_TYPE::ID_FR]
	0,  // [DATA_TYPE::ID_ANSI_CTA]
	4,  // [DATA_TYPE::LATITUDE]
	4,  // [DATA_TYPE::LONGITUDE]
	2,  // [DATA_TYPE::ALTITUDE]
	2,  // [DATA_TYPE::HEIGTH]
	4,  // [DATA_TYPE::HOME_LATITUDE]
	4,  // [DATA_TYPE::HOME_LONGITUDE]
	1,  // [DATA_TYPE::GROUND_SPEED]
	2,  // [DATA_TYPE::HEADING]
};


typedef struct {
	unsigned frame_ctrl:16;
	unsigned duration_id:16;
	uint8_t addr1[6]; /* receiver address */
	uint8_t addr2[6]; /* sender address */
	uint8_t addr3[6]; /* filtering address */
	unsigned sequence_ctrl:16;
	uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;


typedef struct  {
	int16_t fctl;
	int16_t duration;
	uint8_t da;
	uint8_t sa;
	uint8_t bssid;
	int16_t seqctl;
	unsigned char payload[];
} __attribute__((packed)) WifiMgmtHdr;

typedef struct {
	WifiMgmtHdr hdr;
	uint8_t payload[0];
} wifi_ieee80211_packet_t;

static esp_err_t event_handler(void *ctx, system_event_t *event);
static void wifi_sniffer_init(void);
static void wifi_sniffer_set_channel(uint8_t channel);

esp_err_t event_handler(void *ctx, system_event_t *event)
{
	return ESP_OK;
}


void wifi_sniffer_set_channel(uint8_t channel)
{
	esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

// Function to extract MAC addr from a packet at given offset
void getMAC(char *addr, uint8_t* data, uint16_t offset) {
	sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x", data[offset+0], data[offset+1], data[offset+2], data[offset+3], data[offset+4], data[offset+5]);
}

static void printDataSpan(uint16_t start, int len, uint16_t size, uint8_t* data) {
	for(uint16_t i = start; i < len && i < start+size; i++) {
		Serial.write(data[i]);
		trame = trame + char(data[i]);
	}
}

static void printCoordinates(uint16_t start, int len, uint16_t size, uint8_t* data) {
	uint8_t count = size-1;
	int data_value = 0;
	//Serial.print(" data_value="); Serial.print(data_value); Serial.print(" neg=");
	bool neg_number = data[start] > 0x7F;
	//Serial.print(neg_number);Serial.print(" ");
	
	for(uint16_t i = start; i < len && i < start+size; i++) {
		//Serial.print(count); Serial.print("-");  
		data_value +=  (data[i]) << (8 * count); 
		count--;
	}
	
	if(neg_number) {
		data_value = (0xFFFFFFFF & ~data_value) + 1; 
		data_value *= -1;
	}
	
	Serial.print(double(data_value) * 0.00001 , 5);
	trame += String(double(data_value) * 0.00001,5);
}

static void printAltitude(uint16_t start, int len, uint16_t size, uint8_t* data) {
	uint8_t count = size-1;
	int data_value = 0;
	bool neg_number = data[start] > 0x7F;
	//Serial.print(neg_number);Serial.print(" ");
	
	for(uint16_t i = start; i < len && i < start+size; i++) {
		//Serial.print(count); Serial.print("-");  
		data_value +=  (data[i]) << (8 * count); 
		count--;
	}
	
	if(neg_number) {
		data_value = (0xFFFF & ~data_value) + 1; 
		data_value *= -1;
	}
	
	Serial.print(data_value);
	trame = trame + String(data_value);
}

void beaconCallback(void* buf, wifi_promiscuous_pkt_type_t type)
{
	wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
	
	int len = snifferPacket->rx_ctrl.sig_len;
	uint8_t SSID_length = (int)snifferPacket->payload[40];
	uint8_t offset_OUI = 42+SSID_length;
	//Organizationally Unique Identifier = 6A:5C:35 = Secrétariat général de la défense et de la sécurité nationale
	const uint8_t FRAME_OUI[3] = {0x6A, 0x5C, 0x35};
	
	//Filter OUI from 6A:5C:35
	if(snifferPacket->payload[offset_OUI+1] != FRAME_OUI[0] || snifferPacket->payload[offset_OUI+2] != FRAME_OUI[1] || snifferPacket->payload[offset_OUI+3] != FRAME_OUI[2])
	return;
	
	len -= 4;
	
		// If we dont the buffer size is not 0, don't write or else we get CORRUPT_HEAP
	if (snifferPacket->payload[0] == 0x80)
	{
		Serial.print("len=");Serial.print(len,DEC);
		// ID balise
		trame += "ID=";
		Serial.print(" ID: ");  printDataSpan(offset_OUI+4+6, len, TLV_LENGTH[ID_FR] , snifferPacket->payload);
		uint16_t offset = offset_OUI+4+6+TLV_LENGTH[ID_FR]+2; // +2 : Type + Length
		// Latitude
		trame += " lat=";
		Serial.print(" LAT: "); printCoordinates(offset, len, TLV_LENGTH[LATITUDE] , snifferPacket->payload); 
		offset += TLV_LENGTH[LATITUDE]+2;
		// Longitude
		trame += " long=";  
		Serial.print(" LON: "); printCoordinates(offset, len, TLV_LENGTH[LONGITUDE] , snifferPacket->payload); 
		offset += TLV_LENGTH[LONGITUDE]+2;
		//Altitude msl
		trame += " alt=";
		Serial.print(" ALT ABS: "); printAltitude(offset, len, TLV_LENGTH[ALTITUDE] , snifferPacket->payload); 
		offset += TLV_LENGTH[ALTITUDE]+2;
		//home altitude
		trame += " Hauteur=";
		Serial.print(" HAUTEUR: "); printAltitude(offset, len, TLV_LENGTH[HEIGTH] , snifferPacket->payload); 
		offset += TLV_LENGTH[HEIGTH]+2;
		//home latitude
		trame += " lat dep=";
		Serial.print(" LAT DEP: "); printCoordinates(offset, len, TLV_LENGTH[HOME_LATITUDE] , snifferPacket->payload); 
		offset += TLV_LENGTH[HOME_LATITUDE]+2;
		//home longitude
		trame += " long dep=";
		Serial.print(" LON DEP: "); printCoordinates(offset, len, TLV_LENGTH[HOME_LONGITUDE] , snifferPacket->payload);
		offset += TLV_LENGTH[HOME_LATITUDE]+2; 
		//ground speed
		trame += " Vitesse=";
		Serial.print(" VITESSE HOR: "); printAltitude(offset, len, TLV_LENGTH[GROUND_SPEED] , snifferPacket->payload); 
		offset += TLV_LENGTH[GROUND_SPEED]+2;
		//heading
		trame += " Dir=";
		Serial.print(" DIR: "); printAltitude(offset, len, TLV_LENGTH[HEADING] , snifferPacket->payload); 
		Serial.println();
		Serial.println(trame);
		pCharacteristic->setValue(trame.c_str());
		pCharacteristic->notify();
		trame="";
		Serial.print("RSSI=");
		Serial.println(snifferPacket->rx_ctrl.rssi);
	}
}

void wifi_sniffer_init(void)
{
	nvs_flash_init();
	tcpip_adapter_init();
	ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
	ESP_ERROR_CHECK( esp_wifi_start() );
	esp_wifi_set_promiscuous(true);
	esp_wifi_set_promiscuous_rx_cb(&beaconCallback);
	
}


// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	delay(1000);
	wifi_sniffer_init();
	wifi_sniffer_set_channel(6);
	//initialisation du BLE
	BLEDevice::setMTU(150);
	BLEDevice::init("balise");
	BLEServer *pServer = BLEDevice::createServer();
	BLEService *pService = pServer->createService(SERVICE_UUID);
	pCharacteristic = pService->createCharacteristic(
		CHARACTERISTIC_UUID,
		BLECharacteristic::PROPERTY_READ |
		BLECharacteristic::PROPERTY_WRITE
	);
	pService->start();
	// BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
	BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
	pAdvertising->addServiceUUID(SERVICE_UUID);
	pAdvertising->setScanResponse(true);
	pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
	pAdvertising->setMinPreferred(0x12);
	BLEDevice::startAdvertising();
}

// the loop function runs over and over again forever
void loop() {
	delay(1000); // wait for a second
}
