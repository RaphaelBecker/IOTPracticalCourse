from time import time
import json

from minio import Minio

from datetime import datetime

from sklearn import linear_model

import paho.mqtt.client as mqtt

import pandas as pd
import pickle
import io

from datetime import datetime
from datetime import timedelta

import numpy as np
import pickle
import pandas as pd

minioClient = Minio('138.246.232.86:9000',
                    access_key='ANDIK',
                    secret_key='X6@wyK4x~f-@8=Lq',
                    secure=False)

def rbf(x, i,alpha, t):
    term = x-i
    term = np.power(term,2)
    term = -(1/(2*alpha))*term
    term = np.exp(term)
    term = np.mod(term, t)
    return term

def augmentData(df):
    #Feature Engineering
    for i in range(7):
        df["day"+str(i)] = df.index.dayofweek.map(lambda x: rbf(x,i,0.1, 7))
    for i in range(24):
        df["hour"+str(i)] = df.index.hour.map(lambda x: rbf(x,i,0.1, 24))
    for i in range(672):
        df["timeinterval"+str(i)] = ((df.index.dayofweek*24*60+df.index.hour*60+df.index.minute)/15).map(lambda x: rbf(x,i,2,672))
    return df

def mapToCorrectTimes(hour, x):
    if x<15:
        return hour, 15
    elif x<30:
        return hour, 30
    elif x<45:
        return hour, 45
    else:
        return hour+1, 0

def predict_time(model, daterange):
    daterange = pd.DataFrame(index=daterange)
    for i in range(7):
        daterange["day"+str(i)] = daterange.index.dayofweek.map(lambda x: rbf(x,i,0.1, 7))
    for i in range(24):
        daterange["hour"+str(i)] = daterange.index.hour.map(lambda x: rbf(x,i,0.1, 24))
    for i in range(672):
        daterange["timeinterval"+str(i)] = ((daterange.index.dayofweek*24*60+daterange.index.hour*60+daterange.index.minute)/15).map(lambda x: rbf(x,i,2,672))
    return pd.DataFrame(data=model.predict(daterange).astype(int),index=daterange.index)
  
def on_publish(client,userdata,result):
    published=0
    print("data published: %d \n",published)
    
    pass

def main(params):
    start = time()
    model_bucket = params['model_bucket']
    model_blob_name = params['model_blob_name']

    modelName = "linearModel.pkl"

    try:
        minioClient.fget_object(model_bucket, model_blob_name, modelName)
    except:
        print("Couldnt Download the Model")

    model = pickle.load(open(modelName, 'rb'))

    dateTimeObj = datetime.utcnow()
    hour, minute = mapToCorrectTimes(dateTimeObj.hour, dateTimeObj.minute)
    dateTimeObj = datetime(dateTimeObj.year, dateTimeObj.month, dateTimeObj.day, hour, minute)
    prediction_time = pd.date_range(dateTimeObj,periods=1,freq='15T')
    prediction = predict_time(model, prediction_time)

    #push to MQTT
    
    broker="131.159.35.132"
    port=1883
    published = 0

    client1= mqtt.Client()
    client1.username_pw_set(username="JWT",password="eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE2MjU0MzA0MzMsImlzcyI6ImlvdHBsYXRmb3JtIiwic3ViIjoiNDRfMTE5In0.BHb-u0TYXeLtWFUUOyTm669Q_baZs1xfLg_pHWCk0HRI69kmckcNTVtwkVDfVSwci63Fwo43-w7Bd0ZWgrOh5cbwxcMEbPPn9_47IYSXkddOrbxsalQJry8250BBoGYDlYNDgaB5YmuiKPwIqq6ADUf4CHCheiOWopgEOylvklr-MMsAELWkijomMAWYm4Hw32aGfvFWAzMdHz-m_xetuGHU_J6P37HSEFh7AAzeegILXcYQ4pQ8FkMhJ6IYjIK2VYFnd2PbefsaVAIgz3-P1uqtn40lIL0FHaKgV8ms0gQUWRLmnZHoxRQmjLdbLEZ8aoPL7EcMvOI59128-NUk_hfhsERGBYukmrYZBLaexmixKS60-xOt8hwtE2f3V95KQ9QzaCen5iUabcVjqm5zVSOzz_R7vUTvB6M9CmO1rh557aZmubFxLg9FNvqkjjM701DuhxFomTYXVCezrj1RJVQ1BbrvoJ_wxvX-ghuRH3bvuWF_4Tg_uZo8qdC34G_UkclX0OcM7VS3sTuaSYaLxJYB9c8uA64SlIJk0Fmf-4CUIrrq5modtEGqOhdXACZvbmWg_McWXY44nqFsD9c7n57xfOTagYj5HNqDP4J4ux3howRIj-hLdqNEErQGh8JX0ltjKCF5x7J0WUlV4Pu0lXn1Nr3YKmWH-yWL5gpJ2W0")
    client1.on_publish = on_publish

    for index,row in prediction.iterrows():
        client1.connect(broker,port,keepalive=60)
        prediction_count = int(row[0])
        prediction_timestamp = (index - pd.Timestamp("1970-01-01")) // pd.Timedelta('1ms')
        payload = {
            "username": "group8_2021_ss",
            "prediction_linear": prediction_count,
          "device_id": 119,
           "timestamp": prediction_timestamp
        }

        payload_string = json.dumps(payload)

        client1.publish("44_119",payload_string)
        client1.disconnect()

    ret_val = {}
    latency = time() - start
    ret_val['latency'] = latency
    print(ret_val)
    return ret_val

params = {
    "model_bucket":"linear-model",
    "model_blob_name":"linear-model.pkl"
}

main(params)