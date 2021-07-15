import pandas as pd

import json
import requests
import paho.mqtt.client as mqtt

from datetime import datetime

from dateutil.parser import parse 
import numpy as np
import pandas as pd

from time import time

def resample(data):
  data = data.sort_values('timestamp')
  data = data.groupby('timestamp')
  data = data.max().reset_index()
  data = data.set_index('timestamp')
  data = data['value'].resample('T').bfill()
  #ts = np.log1p(ts)
  data = data.resample('T').bfill()
  return data

def fetchData(sensor_id):
  port = 443
  server = "iotplatform.caps.in.tum.de"
  web_url = "https://iotplatform.caps.in.tum.de:443/api/consumers/consume/"

  token = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE2MjUzMjY4MDksImlzcyI6ImlvdHBsYXRmb3JtIiwic3ViIjoiNDRfMTAxIn0.FfQ7a536ktDua2NIGb61CEME4fe0-b-MsdsJhZGbrENq0EuKm25iSHFZ4GTcHRQv3_OWUsD1auS-YFdqBYz2RS8morhTO9ujIRO5-pBqXLpC_DxoJFdvX_FZ8Vdr3GCf14yhphz8nsS9XJzgzQiZNEd7u8RzR0O6atliAS5QGNPHrOmdWBEWovYZ3ZKD68sufdihTOvm_owhwUgYh2dbm8KZB34xjhlkVULNiy0Jadnwzte6DA899837KukwpEV6gwN4vy39sLlaD4xU2lwx2PmCQ_FTxq6FSIJm9gjUNLtP9_lTu0la3rE-aJ_Hcv9_0RaOmpyFxEe4CK7-9tnWqcbQkOfA-DBY6gVfVZHUPhc-RZDLe85IgdC_Jdf_eqf8C2r7MsUz9Eble7n0vO3k9tq5f2RPNUHKcmkQ0OuisbqRZOrzLFyiSVOQHl7LbkfjIrZ6hWwD8TcpWRLrlT4S0i4Y0h0zN1meSa-C7hGrXzgpwOUhGFLp1_RoF6vtqHR0Pls5TBjasseT3M5ovlJ1Mb1q_uSmrpjhD-QRn5eW0R4g0d9Y1KZIX3yqPEQLrRCcergvxmmcGJTk4eHdIwPOTYNnAUzPkxWVIa28BVXGqgisCsCQ5PKeaDC0D6y1YSBY6W98E7Z4XOdg_ncV_0o8QR4hgpFNM2J33jcPBMk9ukk"

  header = {
    'Content-Type':'application/json',
    'Authorization':'Bearer '+token,

  }

  data = {
    "_source": ["value","timestamp"],
    "sort":[
        {
            "timestamp": {"order": "desc"}
        }
    ],
    "size":100,
  }

  response = requests.post(web_url+sensor_id+"/_search?scroll=1m",json=data, headers=header,verify=False)





  response_parsed = json.loads(response.content)

  hits = []
  timeseries = []

  i=2

  while len(response_parsed['body']['hits']['hits'])>0:
    hits = response_parsed['body']['hits']['hits']
    for hit in hits:
        if hit['_source']:
            timeseries.append(hit['_source'])
    scroll_id = response_parsed['body']['_scroll_id']
    data = {
        "scroll":"1m",
        "scroll_id": scroll_id
    }
    response = requests.get(web_url+sensor_id+"/_search/scroll", json=data, headers=header, verify=False)
    response_parsed = json.loads(response.content)
    data = pd.read_json(json.dumps(timeseries))
    data = resample(data)
    data = data.to_frame()
    today = datetime.today()
    if (data.index.min()<(datetime(today.year, today.month, today.day, today.hour-1))):
        break
    i+=1
  return json.dumps(timeseries)
  #saveJSONToFile("timeseries",timeseries)

def pushMQTT(sensorName, df, rowID):

    broker="131.159.35.132"
    port=1883
    def on_publish(client,userdata,result):
        published=0
        print("data published: %d \n",published)

        pass
    client1= mqtt.Client()
    client1.username_pw_set(username="JWT",password="eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE2MjU0MzA0MzMsImlzcyI6ImlvdHBsYXRmb3JtIiwic3ViIjoiNDRfMTE5In0.BHb-u0TYXeLtWFUUOyTm669Q_baZs1xfLg_pHWCk0HRI69kmckcNTVtwkVDfVSwci63Fwo43-w7Bd0ZWgrOh5cbwxcMEbPPn9_47IYSXkddOrbxsalQJry8250BBoGYDlYNDgaB5YmuiKPwIqq6ADUf4CHCheiOWopgEOylvklr-MMsAELWkijomMAWYm4Hw32aGfvFWAzMdHz-m_xetuGHU_J6P37HSEFh7AAzeegILXcYQ4pQ8FkMhJ6IYjIK2VYFnd2PbefsaVAIgz3-P1uqtn40lIL0FHaKgV8ms0gQUWRLmnZHoxRQmjLdbLEZ8aoPL7EcMvOI59128-NUk_hfhsERGBYukmrYZBLaexmixKS60-xOt8hwtE2f3V95KQ9QzaCen5iUabcVjqm5zVSOzz_R7vUTvB6M9CmO1rh557aZmubFxLg9FNvqkjjM701DuhxFomTYXVCezrj1RJVQ1BbrvoJ_wxvX-ghuRH3bvuWF_4Tg_uZo8qdC34G_UkclX0OcM7VS3sTuaSYaLxJYB9c8uA64SlIJk0Fmf-4CUIrrq5modtEGqOhdXACZvbmWg_McWXY44nqFsD9c7n57xfOTagYj5HNqDP4J4ux3howRIj-hLdqNEErQGh8JX0ltjKCF5x7J0WUlV4Pu0lXn1Nr3YKmWH-yWL5gpJ2W0")
    client1.on_publish = on_publish

    i=0
    for index,row in df.iterrows():
        client1.connect(broker,port,keepalive=60)
        prediction_count = (row[rowID].astype('double'))

        prediction_timestamp = (index - pd.Timestamp("1970-01-01")) // pd.Timedelta('1ms')
        payload = {
            "username": "group8_2021_ss",
            sensorName: prediction_count,
            "device_id": 119,
            "timestamp": prediction_timestamp
        }
        
        payload_string = json.dumps(payload)
        print(payload_string)
        ret = client1.publish("44_119",payload_string)
        print(i)
        client1.disconnect()
        i+=1



def main(params):
    start = time()
    pred_arima = fetchData(str(1480))
    pred_linear = fetchData(str(1483))
    actual_data = fetchData(str(1264))
    pred_arima = pd.read_json(pred_arima)
    pred_linear = pd.read_json(pred_linear)
    actual_data = pd.read_json(actual_data)

    pred_arima = resample(pred_arima)
    pred_linear = resample(pred_linear)
    actual_data = resample(actual_data)

    pred_arima = pred_arima.rename("pred_arima")
    pred_linear = pred_linear.rename("pred_linear")
    
    combined = actual_data.to_frame().join(pred_arima).join(pred_linear)
    for index,row in combined.iloc[::-1].iterrows():
        if (np.abs(row[0]-row[1])>np.abs(row[0]-row[2])):
            pushMQTT("bestOnline",pd.DataFrame(index=[pd.Timestamp(datetime.utcnow())],data=[row[2]]),0)
        else:
            pushMQTT("bestOnline",pd.DataFrame(index=[pd.Timestamp(datetime.utcnow())],data=[row[1]]),0)
        break
    
    ret_val = {}
    latency = time() - start
    ret_val['latency'] = latency
    print(ret_val)
    return ret_val

params = {
    "bucket":"data",
    "blob_name":"data.json"
}

main(params)