import numpy as np
from scipy.signal import find_peaks
from scipy.signal import argrelextrema
from matplotlib.dates import date2num


def count_incrememt_detection(timeseries):
    timeseries['increment'] = np.nan
    timeseries['increment_signal'] = np.nan
    timeseries['count_signal'] = np.nan
    index = []
    for i in range(1, len(timeseries)):
        increment = timeseries.at[timeseries.index[i - 1], 'value'] - timeseries.at[timeseries.index[i], 'value']
        timeseries.at[timeseries.index[i], 'increment'] = increment
        if abs(increment) >= 2 and i != len(timeseries) - 1:
            index.append(i)
            timeseries.at[timeseries.index[i], 'count_signal'] = timeseries.at[timeseries.index[i], 'value']
            timeseries.at[timeseries.index[i - 1], 'count_signal'] = timeseries.at[timeseries.index[i - 1], 'value']
            if increment > 0:
                timeseries.at[timeseries.index[i], 'increment_signal'] = 2
            if increment < 0:
                timeseries.at[timeseries.index[i], 'increment_signal'] = -2

    return timeseries, index


def find_and_indicate_tops(timeseries_df):
    timeseries_df['peak'] = np.nan
    timeseries_df['peak_violation'] = np.nan
    peaks, _ = find_peaks(timeseries_df['value'], distance=300, height=15)
    for i in range(len(peaks)):
        timeseries_df.at[timeseries_df.index[peaks[i]], 'peak'] = timeseries_df.at[
            timeseries_df.index[peaks[i]], 'value']
        if timeseries_df.at[timeseries_df.index[peaks[i]], 'peak'] > 50:
            timeseries_df.at[timeseries_df.index[peaks[i]], 'peak_violation'] = timeseries_df.at[
                timeseries_df.index[peaks[i]], 'peak']

    return timeseries_df


def time_increment_detection(timeseries_time_df):
    timeseries_time_df['time_increment'] = np.nan
    timeseries_time_df['time_increment_pos'] = np.nan
    timeseries_time_df['time_increment_neg'] = np.nan
    timeseries_time_df['time_increment_zero'] = np.nan

    index = 0
    last_timedelta_seconds = 0
    for i in range(1, len(timeseries_time_df) - 1):
        timedelta = timeseries_time_df.at[timeseries_time_df.index[i - 1], 'timestamp'] - timeseries_time_df.at[
            timeseries_time_df.index[i], 'timestamp']
        timedelta_seconds = timedelta.seconds + timedelta.days * 24 * 3600
        timeseries_time_df.at[timeseries_time_df.index[i], 'time_increment'] = timedelta_seconds
        if timedelta_seconds > 0:
            timeseries_time_df.at[timeseries_time_df.index[i], 'time_increment_pos'] = timedelta_seconds
            if index < i:
                timeseries_time_df.at[timeseries_time_df.index[i], 'time_increment_pos_next'] = timedelta_seconds
            #print(timedelta_seconds)
        elif timedelta_seconds == 0:
            timeseries_time_df.at[timeseries_time_df.index[i], 'time_increment_zero'] = timedelta_seconds
            #print(timedelta_seconds)
        elif timedelta_seconds < 0:
            timeseries_time_df.at[timeseries_time_df.index[i], 'time_increment_neg'] = timedelta_seconds
            #print(timedelta_seconds)

    return timeseries_time_df

def time_increment_violation_detection(timeseries_time_df, upper_fence):
    timeseries_time_df['time_increment_violation'] = np.nan
    timeseries_time_df['missing_data_start'] = np.nan
    timeseries_time_df['missing_data_end'] = np.nan
    for i in range(0, len(timeseries_time_df)):
        if timeseries_time_df.at[timeseries_time_df.index[i], 'time_increment_pos'] > upper_fence:
            timeseries_time_df.at[timeseries_time_df.index[i], 'time_increment_violation'] = timeseries_time_df.at[timeseries_time_df.index[i], 'time_increment_pos']
            timeseries_time_df.at[timeseries_time_df.index[i], 'missing_data_start'] = 1
            timeseries_time_df.at[timeseries_time_df.index[i - 1], 'missing_data_end'] = 2


    return timeseries_time_df



