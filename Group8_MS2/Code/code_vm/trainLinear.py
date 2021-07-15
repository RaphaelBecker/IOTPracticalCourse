from time import time

from minio import Minio

from sklearn import linear_model

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

def makeDF(jsonString):
    #read data
    countDataOrig = pd.read_json(jsonString)
    #countDataOrig['timestamp'].min(),countDataOrig['timestamp'].max()
    countDataOrig = countDataOrig.sort_values('timestamp')
    countDataOrig.isnull().sum()
    countDataOrig = countDataOrig.groupby('timestamp')
    countDataOrig = countDataOrig.max().reset_index()
    countDataOrig = countDataOrig.set_index('timestamp')

    #resample to minutes
    ts = countDataOrig['value'].resample('T').bfill()

    #resample to 15min
    countData = ts.resample('15T').bfill()

    #cut off the first (incomplete) data for training
    start_date = datetime(2021,6,17)
    lim_countData = countData[start_date:]
    return lim_countData.to_frame()

def augmentData(df):
    #Feature Engineering
    for i in range(7):
        df["day"+str(i)] = df.index.dayofweek.map(lambda x: rbf(x,i,0.1, 7))
    for i in range(24):
        df["hour"+str(i)] = df.index.hour.map(lambda x: rbf(x,i,0.1, 24))
    for i in range(672):
        df["timeinterval"+str(i)] = ((df.index.dayofweek*24*60+df.index.hour*60+df.index.minute)/15).map(lambda x: rbf(x,i,2,672))
    return df

def makeTrainTestSets(df):
    train_end = datetime(2021,7,5)
    test_end = datetime(2021,7,13)
    train_data = df[:train_end]
    test_data = df[train_end + timedelta(hours=1):test_end]
    return train_data, test_data

def splitXY(df):
    Y = df.iloc[:, 0].values.reshape(-1, 1)
    X = df.iloc[:, 1:].values
    return X,Y

def trainModel(trainData):
    X,Y = splitXY(trainData)
    # Create linear regression object
    regr = linear_model.LinearRegression()
    # Train the model using the training sets
    regr = regr.fit(X, Y)
    return regr

def calculateScores(model, trainData, testData):
    X,Y = splitXY(trainData)
    print("Train Score:")
    print(model.score(X, Y))
    X,Y = splitXY(testData)
    print("Test Score:")
    print(model.score(X, Y))



def main(params):
    start = time()
    data_bucket = params['bucket']
    data_blob_name = params['blob_name']
    model_bucket = params['model_bucket']
    model_blob_name = params['model_blob_name']

    jsonFileName = "downloaded_ts.json"

    try:
        minioClient.fget_object(data_bucket, data_blob_name, jsonFileName)
    except:
        print("Couldnt Download the Data")

    df = makeDF(jsonFileName)
    df = augmentData(df)
    train_data, test_data = makeTrainTestSets(df)
    model = trainModel(train_data)
    calculateScores(model, train_data, test_data)

    bytes_file = pickle.dumps(model)
    
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

params = {
    "bucket":"data",
    "blob_name":"data.json",
    "model_bucket":"linear-model",
    "model_blob_name":"linear-model.pkl"
}

main(params)