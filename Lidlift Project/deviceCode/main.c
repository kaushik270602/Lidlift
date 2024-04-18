#include "stdio.h"
#include "stdlib.h"
#include "paho_mqtt.h"
#include "MQTTClient.h"
#include "xtimer.h"
#include "ztimer.h"
#include "time.h"
#include "thread.h"
#include "srf04.h"
#include "servo.h"

#define BUF_SIZE 1024

#define MQTT_VERSION_v311               4     
#define COMMAND_TIMEOUT_MS              5000

#define BROKER_ADDRESS "10.211.55.4"
#define DEFAULT_MQTT_PORT               1883
#define DEFAULT_KEEPALIVE_SEC           15
#define TOPIC "dustbin"

#define IS_CLEAN_SESSION                1
#define IS_RETAINED_MSG                 0

#define TRIGGER_PIN GPIO_PIN(0, 1)
#define ECHO_PIN GPIO_PIN(0, 2)

#define D_THRESHOLD 150

#define DEV              PWM_DEV(0)
#define CHANNEL          0
#define PWM_FREQ         100U
#define PWM_RES          200U
#define SERVO_MIN        1000U
#define SERVO_MAX        2000U


static MQTTClient client;
static Network network;
static unsigned char buf[BUF_SIZE];
static unsigned char readbuf[BUF_SIZE];

static srf04_t dev;
static srf04_params_t params = {
        .trigger = TRIGGER_PIN,
        .echo = ECHO_PIN
    };

static servo_t servo;
servo_pwm_params_t pwm_par = {
        .pwm = DEV,
        .freq = PWM_FREQ,
        .res = PWM_RES
    };

static servo_params_t par = {
        .pwm = &pwm_par,
        .min_us = SERVO_MIN,
        .max_us = SERVO_MAX
    };


int mqtt_init(void){

    xtimer_sleep(5);

    NetworkInit(&network);
    MQTTClientInit(&client, &network, COMMAND_TIMEOUT_MS, buf, BUF_SIZE, readbuf, BUF_SIZE);
    MQTTStartTask(&client);

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = MQTT_VERSION_v311;
    data.clientID.cstring = "";
    data.username.cstring = "";
    data.password.cstring = "";
    data.keepAliveInterval = 60;
    data.cleansession = 1;

    printf("MQTT: Connecting to MQTT Broker from %s %d\n",
            BROKER_ADDRESS, DEFAULT_MQTT_PORT);
    printf("MQTT: Trying to connect to %s, port: %d\n",
            BROKER_ADDRESS, DEFAULT_MQTT_PORT);
    
    int res = NetworkConnect(&network, BROKER_ADDRESS, DEFAULT_MQTT_PORT);

    if(res){
        printf("MQTT unable to connect: Error %d\n", res);
        return res;
    }
    printf("user:%s clientId:%s password:%s\n", data.username.cstring,
             data.clientID.cstring, data.password.cstring);
    res = MQTTConnect(&client, &data);

    if (res < 0) {
        printf("MQTT: Unable to connect client %d\n", res);
        int res = MQTTDisconnect(&client);
        if (res < 0) {
            printf("MQTT: Unable to disconnect\n");
        }
        else {
            printf("MQTT: Disconnect successful\n");
        }
        NetworkDisconnect(&network);
        return res;
    }
    else{
        printf("MQTT: Connection success!\n");
    }

    printf("MQTT client succesfully connected to the broker\n");
    return 0;
}

int publish(char* msg){
    //MQTT publish
    MQTTMessage message;
    message.qos = QOS1;
    message.retained = IS_RETAINED_MSG;
    message.payload = msg;
    message.payloadlen = strlen(message.payload);

    int rp = MQTTPublish(&client, TOPIC, &message);
    if (rp){
        printf("MQTT error %d: unable to publish!\n", rp);
        return 1;
    }else{
        printf("MQTT message published succesfully to topic %s\n", TOPIC);
    }
    return 0;
}

int init_sens_and_actuators(void){

    if(srf04_init(&dev, &params) == SRF04_OK)
        puts("Ultrasonic initialization success!");
    else{
        puts("Ultrasonic initialization failed!");
        return 1;
    }

    if(servo_init(&servo, &par)){
        printf("Servo motor initialization failed!\n");
        return 1;
    }
    else
        printf("Servo motor initialization success!\n");
    
    servo_set(&servo, 0);
    
    return 0;
}


int main(void)
{   
    if(init_sens_and_actuators()){
        printf("Errors in initialization phase...Aborting\n");
        return 1;
    }
    int conn_res = mqtt_init();
    if (conn_res){
        printf("MQTT initialization error!\n");
        printf("\n##### Offline only mode #####\n");
    }
    else 
        printf("MQTT initialization success\n\n##### Online mode enabled #####\n");

    int i = 0;

    int distance;
    char* status = "CLOSED";

    printf("Start taking distance measurements every 2 seconds...\n");
    servo_set(&servo, 0);

    while (1){ 

        distance = srf04_get_distance(&dev);

        if(distance <= D_THRESHOLD && distance >= 0){

            printf("Detected distance below threshold: %d mm\n", distance);
            printf("\nOpening the lid...\n\n");

            for(int j = 1; j <= UINT8_MAX; j++){
                servo_set(&servo, j);
                xtimer_msleep(1);
            }
            status = "OPEN";
            i++;
            
        L1:; 
            char json[200];

            sprintf(json, "{\"id\": \"%d\", \"lastDistance\": \"%d\", \"status\": \"%s\"}",
                                i, distance, status);

            char* msg = json;
            
            if(publish(msg)){
                printf("Error publishing on topic: %s", TOPIC);
            }

            xtimer_sleep(5);
            distance = srf04_get_distance(&dev);

            if (distance > D_THRESHOLD){

                printf("\nClosing the lid...\n\n");
                servo_set(&servo, 0);
                status = "CLOSED";

                char json[200];

                sprintf(json, "{\"id\": \"%d\", \"lastDistance\": \"%d\", \"status\": \"%s\"}",
                                    i, distance, status);

                char* msg = json;

                if(publish(msg)){
                    printf("Error publishing on topic: %s", TOPIC);
                }

                xtimer_sleep(3);

            }
            else
                goto L1;
        }

        xtimer_sleep(2);
    }

    return 0;
}
