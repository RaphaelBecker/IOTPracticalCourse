import pandas as pd
import numpy as np
from matplotlib import pyplot
from matplotlib.collections import LineCollection
from matplotlib.dates import date2num
import scipy.stats as sps

import algorithms

pd.set_option('display.max_columns', None)  # or 1000
pd.set_option('display.max_rows', None)  # or 1000
pd.set_option('display.max_colwidth', None)  # or 199

def load_dataframe():
    return pd.read_json('recources/timeseries.json')


def intro_plot(timeseries, fig_intro):
    total_rows = len(timeseries.index)
    n_detail = int(total_rows * .081)
    n_very_detail = int(total_rows * .02)
    timeseries_detail = timeseries.iloc[-n_detail:]
    timeseries_very_detail = timeseries.iloc[-n_very_detail:]

    #################################################################
    # Timeseries intro plot

    ax_series_very_detail = fig_intro.add_axes((0, 0.64, 1, 0.40))
    ax_series_detail = fig_intro.add_axes((0, 0.24, 1, 0.36))
    ax_series = fig_intro.add_axes((0, 0, 1, 0.2))

    ax_series_very_detail.plot(timeseries_very_detail['timestamp'], timeseries_very_detail['value'],
                               label='Student count')
    ax_series_detail.plot(timeseries_detail['timestamp'], timeseries_detail['value'], label='Student count')
    ax_series.plot(timeseries['timestamp'], timeseries['value'], label='Student count')
    # ax_series.plot(timeseries['timestamp'], [40] * len(timeseries.index), label='threashold')

    ax_series_very_detail.set_ylabel('Student count', size=15)
    ax_series_detail.set_ylabel('Student count', size=15)
    ax_series.set_ylabel('Student count', size=15)

    ax_series_very_detail.legend(loc='upper right')
    ax_series_detail.legend(loc='upper right')
    ax_series.legend(loc='upper right')

    save_path = 'plots/plot_intro.png'
    title_string = 'Student count data characteristics'
    fig_intro.suptitle(title_string, x=0.5, y=1.1, fontsize=21)
    fig_intro.savefig(save_path, bbox_inches='tight')
    fig_intro.show()
    fig_intro.clf()


def count_increment_plot(timeseries, fig_intro):
    ###################################################################
    ###################################################################
    ###################################################################

    # Anomaly detection timeseries plot + detection method deatails
    timeseries_increment, index = algorithms.count_incrememt_detection(timeseries)
    # print(timeseries_increment.to_string())
    # print(timeseries_increment.tail())
    fig_intro.set_size_inches((18, 6))
    # dimentions
    ax_series_detection = fig_intro.add_axes((0, 0.45, 1, 0.35))
    ax_series_detection_method = fig_intro.add_axes((0, 0, 1, 0.35))
    # plots
    ax_series_detection.plot(timeseries_increment['timestamp'], timeseries_increment['value'],
                              label='Student count')
    ax_series_detection.scatter(timeseries_increment['timestamp'], timeseries_increment['count_signal'],
                                marker='x',
                                color='red', label='anomaly count')
    ax_series_detection.plot(timeseries_increment['timestamp'], timeseries_increment['count_signal'], color='red',
                             label='increment jump')
    ax_series_detection.set_ylabel('Student count', size=15)
    ax_series_detection.legend(loc='upper right')

    ax_series_detection_method.set_ylabel('Count increment', size=15)
    ax_series_detection_method.plot(timeseries_increment['timestamp'], timeseries_increment['increment'],
                                    label='Count increment')
    ax_series_detection_method.plot(timeseries_increment['timestamp'], [2] * len(timeseries_increment.index),
                                    label='2')
    ax_series_detection_method.plot(timeseries_increment['timestamp'], [-2] * len(timeseries_increment.index),
                                    label='-2')
    ax_series_detection_method.scatter(timeseries_increment['timestamp'], timeseries_increment['increment_signal'],
                                       marker='x',
                                       color='red', label='threshold violation')

    ax_series_detection_method.legend(loc='upper right')
    # saving
    save_path = 'plots/plot_timeseries_increment_deviation_detection.png'
    title_string = 'Timeseries count increment deviation detection'
    fig_intro.suptitle(title_string, x=0.5, y=0.9, fontsize=21)
    fig_intro.savefig(save_path, bbox_inches='tight')
    fig_intro.show()
    fig_intro.clf()

    print(' Anomaly count values: \n' + str(timeseries_increment.loc[abs(timeseries_increment['increment']) > 2]))
    print(index)


def std_deviation_peak_plot(timeseries, fig_intro):
    ###################################################################
    ###################################################################
    ###################################################################
    # standard deviation:
    fig_intro.set_size_inches((18, 6))

    # dimentions
    ax_series_detection = fig_intro.add_axes((0, 0.45, 1, 0.35))
    ax_series_detection_method = fig_intro.add_axes((0, 0, 1, 0.35))

    # plots
    ax_series_detection.plot(timeseries['timestamp'], timeseries['value'], label='Student count')

    # peaks
    timeseries_peaks = algorithms.find_and_indicate_tops(timeseries)

    # historgramm
    timeseries_without_zeros = timeseries_peaks.loc[timeseries_peaks['value'] != 0, 'value']
    timeseries_without_zeros_only_peaks = timeseries_peaks.loc[-np.isnan(timeseries_peaks['peak']), 'value']
    timeseries_without_zeros_only_peaks_violation = timeseries_peaks.loc[
        -np.isnan(timeseries_peaks['peak_violation']), 'value']

    # average peask
    mean_peak = timeseries_without_zeros_only_peaks.mean()
    std_peak = timeseries_without_zeros_only_peaks.std()
    sigma_tree = mean_peak + 3 * std_peak
    print('Mean peak: ' + str(mean_peak))
    print('Std peak: ' + str(mean_peak + std_peak))
    print('Sigma 3: ' + str(sigma_tree))

    ax_series_detection.scatter(timeseries_peaks['timestamp'], timeseries_peaks['peak'], marker='x',
                                color='green', label='peak')

    ax_series_detection.scatter(timeseries_peaks['timestamp'], timeseries_peaks['peak_violation'], marker='x',
                                color='red', label='peak violation')

    ax_series_detection.plot(timeseries['timestamp'], [mean_peak] * len(timeseries.index), color='green',
                             label='mean peak')

    ax_series_detection_method.hist(timeseries_without_zeros_only_peaks, histtype='stepfilled', bins=50, alpha=0.5,
                                    label='count peaks')

    # gauss
    d1 = sps.norm(mean_peak, std_peak)
    mn, mx = ax_series_detection_method.get_xbound()
    ax_series_detection_method.set_xlim(mn, mx)

    # add our distributions to figure
    x = np.linspace(mn, mx, 30)
    ax_series_detection_method.plot(x, d1.pdf(x) * len(timeseries_without_zeros_only_peaks) * 3, color='green', ls='--',
                                    label='probability distribution')
    ax_series_detection_method.axvline(x=mean_peak, linewidth=2, color='green', ls='-.', label='mean peak')
    ax_series_detection_method.axvline(x=mean_peak + std_peak, linewidth=1, color='blue', ls='--', label='68%')
    ax_series_detection_method.axvline(x=mean_peak + std_peak * 2, linewidth=1, color='orange', ls='--', label='95%')
    ax_series_detection_method.axvline(x=mean_peak + std_peak * 3, linewidth=1, color='red', ls='--', label='99,7%')

    ax_series_detection_method.legend(loc='upper right')
    ax_series_detection.legend(loc='upper right')
    ax_series_detection_method.set_ylabel('peak count', size=15)
    ax_series_detection.set_ylabel('Student count', size=15)

    # saving
    save_path = 'plots/plot_timeseries_standard_deviation.png'
    title_string = 'Timeseries standard deviation'
    fig_intro.suptitle(title_string, x=0.5, y=0.9, fontsize=21)
    fig_intro.savefig(save_path, bbox_inches='tight')
    fig_intro.show()
    fig_intro.clf()

# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    ################################################################
    # Dataset:
    timeseries = load_dataframe()

    pyplot.style.use('seaborn-darkgrid')
    pyplot.rcParams.update({'font.size': 16})
    fig_intro = pyplot.figure()
    fig_intro.set_size_inches((18, 6))

    if True:
        intro_plot(timeseries, fig_intro)

    if True:
        count_increment_plot(timeseries, fig_intro)

    if True:
        std_deviation_peak_plot(timeseries, fig_intro)


    ###################################################################
    ###################################################################
    ###################################################################
    # continuous timestamps:

    timeseries_time_increment = algorithms.time_increment_detection(timeseries)

    fig_intro.set_size_inches((18, 6))

    # dimentions
    ax_series_detection = fig_intro.add_axes((0, 0.6, 1, 0.27))
    ax_series_detection_method = fig_intro.add_axes((0, 0.3, 1, 0.24))
    ax_series_detection_box = fig_intro.add_axes((0, 0, 1, 0.24))

    # plots
    ax_series_detection.plot(timeseries['timestamp'], timeseries['value'], label='Student count')

    ax_series_detection_method.scatter(timeseries['timestamp'], timeseries_time_increment['time_increment_pos'], marker='.',
                                color='green', label='positive time increment')

    # boxplot
    timeseries_time_increment.boxplot(column='time_increment_pos', ax=ax_series_detection_box, vert=False)

    Q1 = timeseries_time_increment['time_increment_pos'].quantile(0.25)
    Q3 = timeseries_time_increment['time_increment_pos'].quantile(0.75)
    IQR = Q3 - Q1
    Lower_Fence = Q1 - (1.5 * IQR)
    Upper_Fence = Q3 + (1.5 * IQR)

    print(Lower_Fence, Upper_Fence)
    ax_series_detection_method.plot(timeseries_time_increment['timestamp'], [Upper_Fence] * len(timeseries_time_increment.index),
                                    label='upper fence')

    timeseries_time_increment = algorithms.time_increment_violation_detection(timeseries_time_increment, Upper_Fence)
    ax_series_detection_method.scatter(timeseries['timestamp'], timeseries_time_increment['time_increment_violation'],
                                       marker='.',
                                       color='red', label='anomaly time increment')

    #print(timeseries_time_increment.columns)
    #print(timeseries_time_increment.tail(5000))

    ax_series_detection.axvline(x=np.nan, color='red', ls='-', label='start missing data')
    ax_series_detection.axvline(x=np.nan, color='blue', ls='-', label='end missing data')

    for i in range(0, len(timeseries)):
        if timeseries.at[timeseries.index[i], 'missing_data_start'] == 1:
            ax_series_detection.axvline(x=timeseries.at[timeseries.index[i], 'timestamp'], linewidth=1, color='red', ls='-')
        if timeseries.at[timeseries.index[i], 'missing_data_end'] == 2:
            ax_series_detection.axvline(x=timeseries.at[timeseries.index[i], 'timestamp'], linewidth=1, color='blue', ls='-')

    # ax_series_detection.fill_between(timeseries[ 'missing_data_start'], timeseries[ 'missing_data_end'], 1)

    ax_series_detection.set_ylabel('Student count', size=15)
    ax_series_detection_method.set_ylabel('time increment [s]', size=15)

    ax_series_detection_method.legend(loc='upper right')
    ax_series_detection.legend(loc='upper right')

    # saving
    save_path = 'plots/Viewing_the_timestamp_series.png'
    title_string = 'Viewing the timestamp series'
    fig_intro.suptitle(title_string, x=0.5, y=0.95, fontsize=21)
    fig_intro.savefig(save_path, bbox_inches='tight')
    fig_intro.show()
