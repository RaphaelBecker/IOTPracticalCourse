from minio import Minio
from sklearn.feature_extraction.text import TfidfVectorizer
from sklearn.linear_model import LogisticRegression
import pandas as pd
import pickle
import io

import json
import requests
import math

from datetime import datetime
from datetime import timedelta

from dateutil.parser import parse 
import matplotlib as mpl
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import pandas as pd

from pylab import rcParams
import statsmodels.api as sm
from statsmodels.tsa.stattools import acf, pacf

from time import time
import re

minioClient = Minio('138.246.232.86:9000',
                    access_key='ANDIK',
                    secret_key='X6@wyK4x~f-@8=Lq',
                    secure=False)

cleanup_re = re.compile('[^a-z]+')

def saveJSONToFile(fname,jsonData):
    with open(fname+".json", "w") as write_file:
        json.dump(jsonData,write_file)

def cleanup(sentence):
    sentence = sentence.lower()
    sentence = cleanup_re.sub(' ', sentence).strip()
    return sentence


def main(params):
    start = time()
    model_bucket = params['bucket']
    model_blob_name = params['blob_name']

    bytes_file=fetchData().encode()
    
    minioClient.put_object(
        bucket_name=model_bucket,
        object_name=model_blob_name,
        data=io.BytesIO(bytes_file),
        length=len(bytes_file)
    )
    ret_val = {}
    latency = time() - start
    ret_val['latency'] = latency
    print(ret_val)
    return ret_val

def fetchData():
  port = 443
  server = "iotplatform.caps.in.tum.de"
  web_url = "https://iotplatform.caps.in.tum.de:443/api/consumers/consume/"

  token = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE2MjUzMjY4MDksImlzcyI6ImlvdHBsYXRmb3JtIiwic3ViIjoiNDRfMTAxIn0.FfQ7a536ktDua2NIGb61CEME4fe0-b-MsdsJhZGbrENq0EuKm25iSHFZ4GTcHRQv3_OWUsD1auS-YFdqBYz2RS8morhTO9ujIRO5-pBqXLpC_DxoJFdvX_FZ8Vdr3GCf14yhphz8nsS9XJzgzQiZNEd7u8RzR0O6atliAS5QGNPHrOmdWBEWovYZ3ZKD68sufdihTOvm_owhwUgYh2dbm8KZB34xjhlkVULNiy0Jadnwzte6DA899837KukwpEV6gwN4vy39sLlaD4xU2lwx2PmCQ_FTxq6FSIJm9gjUNLtP9_lTu0la3rE-aJ_Hcv9_0RaOmpyFxEe4CK7-9tnWqcbQkOfA-DBY6gVfVZHUPhc-RZDLe85IgdC_Jdf_eqf8C2r7MsUz9Eble7n0vO3k9tq5f2RPNUHKcmkQ0OuisbqRZOrzLFyiSVOQHl7LbkfjIrZ6hWwD8TcpWRLrlT4S0i4Y0h0zN1meSa-C7hGrXzgpwOUhGFLp1_RoF6vtqHR0Pls5TBjasseT3M5ovlJ1Mb1q_uSmrpjhD-QRn5eW0R4g0d9Y1KZIX3yqPEQLrRCcergvxmmcGJTk4eHdIwPOTYNnAUzPkxWVIa28BVXGqgisCsCQ5PKeaDC0D6y1YSBY6W98E7Z4XOdg_ncV_0o8QR4hgpFNM2J33jcPBMk9ukk"
  sensor_id = "1264"

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
    "size":10000,
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
    
    i+=1
  return json.dumps(timeseries)
  #saveJSONToFile("timeseries",timeseries)

params = {
    "bucket":"data",
    "blob_name":"data.json"
}

main(params)