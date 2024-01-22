#include <painlessMesh.h>
#include <Arduino_JSON.h>

#define MESH_PREFIX     "XirkaMesh"
#define MESH_PASSWORD   "12345678"
#define MESH_PORT       5555

int nodeNumber = 1;
String readings;
Scheduler userScheduler;
painlessMesh  mesh;

void sendMessage();
void sendSerial2();
void receiveSerial2();
String getReadings();

Task taskSendMessage(5000, TASK_FOREVER, &sendMessage);
Task taskSerial2Send(10000, TASK_FOREVER, &sendSerial2);
Task taskSerial2Receive(1000, TASK_FOREVER, &receiveSerial2);
 
String getReadings() {
  JSONVar jsonReadings;
  jsonReadings["node"] = nodeNumber;
  jsonReadings["light"] = random(1024);
  jsonReadings["temp"] = random(100);
  jsonReadings["hum"] = random(100);
  readings = JSON.stringify(jsonReadings);
  return readings;
}

void sendMessage() {
  String msg = getReadings();
  mesh.sendBroadcast(msg);
  Serial.print("Data Sent via Mesh:");
  Serial.println(msg);
  Serial.println();
}

void sendSerial2() {
  String msg = getReadings();
  Serial2.println(msg);
  Serial.print("Data Sent via Serial2:");
  Serial.println(msg);
  Serial.println();
}

void receiveSerial2() {
  while (Serial2.available() > 0) {
    String receivedData = Serial2.readStringUntil('\n');
    Serial.print("Data Received via Serial2:");
    Serial.println(receivedData);
  }
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  JSONVar myObject = JSON.parse(msg.c_str());
  int node = myObject["node"];
  double light = myObject["light"];
  double temp = myObject["temp"];
  double hum = myObject["hum"];
  Serial.printf("Node: %d\tIntensity: %.2f Lux\tTemperature: %.2f C\tHumidity: %.2f \n", node, light, temp, hum);
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 17, 16);
  
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
  userScheduler.addTask(taskSerial2Send);
  taskSerial2Send.enable();
  userScheduler.addTask(taskSerial2Receive);
  taskSerial2Receive.enable();
}

void loop() {
  mesh.update();
  userScheduler.execute();
}
