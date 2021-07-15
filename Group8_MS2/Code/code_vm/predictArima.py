from time import time
import json
from kafka import KafkaProducer

from minio import Minio

from datetime import datetime

import statsmodels.api as sm

import paho.mqtt.client as mqtt

import pandas as pd
import pickle
import io

import os.path


from datetime import datetime
from datetime import timedelta

import numpy as np
import pickle
import pandas as pd

minioClient = Minio('138.246.232.86:9000',
                    access_key='ANDIK',
                    secret_key='X6@wyK4x~f-@8=Lq',
                    secure=False)

def connect_kafka_producer():
    _producer = None
    try:
        _producer = KafkaProducer(bootstrap_servers=['138.246.232.86:9092'])
    except Exception as ex:
        print('Exception while connecting Kafka')
        print(str(ex))
    finally:
        return _producer

def publish_message(producer_instance, topic_name, value):
    try:
        value_bytes = bytes(value, encoding='utf-8')
        producer_instance.send(topic_name, value=value_bytes)
        producer_instance.flush()
        print('Message published successfully.')
    except Exception as ex:
        print('Exception in publishing message')
        print(str(ex))

def rbf(x, i,alpha, t):
    term = x-i
    term = np.power(term,2)
    term = -(1/(2*alpha))*term
    term = np.exp(term)
    term = np.mod(term, t)
    return term

def augmentTimeSeries(ts):
    #Weekly Features
    for i in range(7):
        ts["day"+str(i)] = ts.index.dayofweek.map(lambda x: rbf(x,i,0.5, 7))

    for i in range(24):
        ts["hour"+str(i)] = ts.index.hour.map(lambda x: rbf(x,i,0.5, 24))


    for i in range(168):
        ts["timeinterval"+str(i)] = ((ts.index.dayofweek*24*60+ts.index.hour*60+ts.index.minute)/60).map(lambda x: rbf(x,i,2,168))

    return ts

def mapToCorrectTimes(hour, x):
        return hour+1, 0

def predict_time(model, daterange):
    exog_range = pd.date_range(start=pd.Timestamp('2021-07-05 00:00:00',freq='H'), end=daterange.max(), freq='H')
    df = pd.DataFrame(index=exog_range)
    df = augmentTimeSeries(df)
    return model.get_prediction(start=pd.to_datetime(daterange.min()),end=pd.to_datetime(daterange.max()),exog=df)
  
def on_publish(client,userdata,result):
    published=0
    print("data published: %d \n",published)
    
    pass

def main(params):
    start = time()
    model_bucket = params['model_bucket']
    model_blob_name = params['model_blob_name']

    modelName = "/home/debian/code_vm/arimaModel.pkl"

    if not os.path.isfile(modelName):
        try:
            minioClient.fget_object(model_bucket, model_blob_name, modelName)
        except:
            print("Couldnt Download the Model")

    model = pickle.load(open(modelName, 'rb'))

    dateTimeObj = datetime.utcnow()
    hour, minute = mapToCorrectTimes(dateTimeObj.hour, dateTimeObj.minute)
    dateTimeObj = datetime(dateTimeObj.year, dateTimeObj.month, dateTimeObj.day, hour, minute)
    prediction_time = pd.date_range(dateTimeObj,periods=4,freq='15T')
    prediction_future = predict_time(model, prediction_time)
    df_prediction_future = np.expm1(prediction_future.predicted_mean.resample('15T').bfill()).astype(int)
    prediction = df_prediction_future.to_frame()

    #push to MQTT
    
    broker="131.159.35.132"
    port=1883

    client1= mqtt.Client()
    client1.username_pw_set(username="JWT",password="eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE2MjU0MzA0MzMsImlzcyI6ImlvdHBsYXRmb3JtIiwic3ViIjoiNDRfMTE5In0.BHb-u0TYXeLtWFUUOyTm669Q_baZs1xfLg_pHWCk0HRI69kmckcNTVtwkVDfVSwci63Fwo43-w7Bd0ZWgrOh5cbwxcMEbPPn9_47IYSXkddOrbxsalQJry8250BBoGYDlYNDgaB5YmuiKPwIqq6ADUf4CHCheiOWopgEOylvklr-MMsAELWkijomMAWYm4Hw32aGfvFWAzMdHz-m_xetuGHU_J6P37HSEFh7AAzeegILXcYQ4pQ8FkMhJ6IYjIK2VYFnd2PbefsaVAIgz3-P1uqtn40lIL0FHaKgV8ms0gQUWRLmnZHoxRQmjLdbLEZ8aoPL7EcMvOI59128-NUk_hfhsERGBYukmrYZBLaexmixKS60-xOt8hwtE2f3V95KQ9QzaCen5iUabcVjqm5zVSOzz_R7vUTvB6M9CmO1rh557aZmubFxLg9FNvqkjjM701DuhxFomTYXVCezrj1RJVQ1BbrvoJ_wxvX-ghuRH3bvuWF_4Tg_uZo8qdC34G_UkclX0OcM7VS3sTuaSYaLxJYB9c8uA64SlIJk0Fmf-4CUIrrq5modtEGqOhdXACZvbmWg_McWXY44nqFsD9c7n57xfOTagYj5HNqDP4J4ux3howRIj-hLdqNEErQGh8JX0ltjKCF5x7J0WUlV4Pu0lXn1Nr3YKmWH-yWL5gpJ2W0")
    client1.on_publish = on_publish

    #push to kafka
    kafka_producer = connect_kafka_producer()

    for index,row in prediction.iterrows():
        client1.connect(broker,port,keepalive=60)
        prediction_count = int(row[0])
        prediction_timestamp = (index - pd.Timestamp("1970-01-01")) // pd.Timedelta('1ms')
        payload = {
            "username": "group8_2021_ss",
            "prediction_arima": prediction_count,
            "device_id": 119,
            "timestamp": prediction_timestamp
        }

        payload_string = json.dumps(payload)
        client1.publish("44_119",payload_string)
        client1.disconnect()
        publish_message(kafka_producer, "predictions", payload_string)

    
    
    
    ret_val = {}
    latency = time() - start
    ret_val['latency'] = latency
    print(ret_val)
    return ret_val

params = {
    "model_bucket":"sarimax-model",
    "model_blob_name":"sarimax-model.pkl"
}

main(params)