print("Hello Core IOT")
import paho.mqtt.client as mqttclient
import time
import json

BROKER_ADDRESS = "app.coreiot.io"
PORT = 1883
ACCESS_TOKEN = "TXCkvBdJYY5vIrljVmUq"
ACCESS_USERNAME = ""

def subscribed(client, userdata, mid, granted_qos):
    print("Subscribed...")


def recv_message(client, userdata, message):
    print("Received: ", message.payload.decode("utf-8"))
    temp_data = {'value': True}
    try:
        jsonobj = json.loads(message.payload)
        if jsonobj['method'] == "setValue":
            temp_data['value'] = jsonobj['params']
            client.publish('esp/telemetry', json.dumps(temp_data), 1)
    except:
        pass


def connected(client, usedata, flags, rc):
    if rc == 0:
        print("Connected successfully!!")
        client.subscribe("esp/telemetry")
    else:
        print("Connection is failed")


client = mqttclient.Client("Sensor T1")
client.username_pw_set(ACCESS_TOKEN)

client.on_connect = connected
client.connect(BROKER_ADDRESS, 1883)
client.loop_start()

client.on_subscribe = subscribed
client.on_message = recv_message

temp = 30
humi = 50
light_intesity = 100
counter = 0
#HCMUT
long = 106.65789107082472
lat = 10.772175109674038

#H6
long = 106.80633605864662
lat = 10.880018410410052


while True:
    collect_data = {'temperature': temp, 'humidity': humi,
                    'light':light_intesity,
                    'long': long, 'lat': lat}
    temp += 1
    humi += 1
    light_intesity += 1
    client.publish('esp/telemetry', json.dumps(collect_data), 1)
    time.sleep(5)