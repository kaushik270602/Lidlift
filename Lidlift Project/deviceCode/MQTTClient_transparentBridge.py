from AWSIoTPythonSDK.MQTTLib import AWSIoTMQTTClient
import paho.mqtt.client as mqtt
import json
import time
from datetime import datetime
import signal

def on_message(_client, _userdata, message):
    print('...')
    
    date = datetime.now().strftime('%d-%m-%YT%H:%M:%S')

    print(date + " -Message received: " + str(message.payload))

    payload = json.loads(message.payload)

    new_payload = {
        'timestamp' : date,
        'distance' : int(payload['lastDistance']),
        'status' : str(payload['status'])
    }

    json_payload = json.dumps(new_payload)
    
    topic = MQTT_PUB_TOPIC

    success = AWSIOTClient.publish(topic, json_payload, 0)

    #time.sleep(5)
    if (success):
        print("Message published to topic "+ topic)
    print('-----')

def on_connect(_client, _userdata, _flags, result):

    print('Connected ' + str(result))
    #success = AWSIOTClient.publish(MQTT_PUB_TOPIC, "ciao", 0)
    #if(success):
    #   print("")

    print('Subscribing to ' + MQTT_SUB_TOPIC)
    MQTT_CLIENT.subscribe(MQTT_SUB_TOPIC)

def disconnect_clients(signum, frame):
    MQTT_CLIENT.loop_stop()
    MQTT_CLIENT.disconnect()
    AWSIOTClient.disconnect()
    print("Disconnected from clients")
    exit(0)

signal.signal(signal.SIGINT, disconnect_clients)

BROKER_ADDRESS = "10.211.55.4"
MQTT_BROKER_PORT = 1883
MQTT_BROKER_CLIENT_ID = "broker"

# AWS IoT settings
AWS_IOT_ENDPOINT = "a1vmy40papjlgl-ats.iot.eu-west-3.amazonaws.com"
AWS_IOT_CLIENT_ID = "ESP32"

# Relative path to the AWS IoT Root CA file
AWS_IOT_ROOT_CA = "cert/AmazonRootCA1.pem"

# Relative path to the AWS IoT Private Key file
AWS_IOT_PRIVATE_KEY = "cert/f00a4ca45e241edc6f9e377946dbd36719f0d37769d4c0b1108ae8947a2cba14-private.pem.key"

# Relative path to the AWS IoT Certificate file
AWS_IOT_CERTIFICATE = "cert/f00a4ca45e241edc6f9e377946dbd36719f0d37769d4c0b1108ae8947a2cba14-certificate.pem.crt"

# Configurations

# For certificate based connection
AWSIOTClient = AWSIoTMQTTClient(AWS_IOT_CLIENT_ID)

# For TLS mutual authentication
AWSIOTClient.configureEndpoint(AWS_IOT_ENDPOINT, 8883)
AWSIOTClient.configureCredentials(AWS_IOT_ROOT_CA, AWS_IOT_PRIVATE_KEY, AWS_IOT_CERTIFICATE)

AWSIOTClient.configureOfflinePublishQueueing(-1)  # Infinite offline Publish queueing
AWSIOTClient.configureDrainingFrequency(2)  # Draining: 2 Hz
AWSIOTClient.configureConnectDisconnectTimeout(10)  # 10 sec
AWSIOTClient.configureMQTTOperationTimeout(5)  # 5 sec

#TOPIC
MQTT_SUB_TOPIC = "dustbin"
MQTT_PUB_TOPIC = "dustbin/data"

# Creating a MQTT client instance
MQTT_CLIENT = mqtt.Client(client_id=MQTT_BROKER_CLIENT_ID)

def main():
    MQTT_CLIENT.on_connect = on_connect
    MQTT_CLIENT.on_message = on_message
    MQTT_CLIENT.connect(BROKER_ADDRESS, MQTT_BROKER_PORT)
    print("Trying to connect to AWS IOT CORE")
    AWSIOTClient.connect()
    MQTT_CLIENT.loop_forever()

main()