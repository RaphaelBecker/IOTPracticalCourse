# This is a sample Python script.

# Press ⌃R to execute it or replace it with your code.
# Press Double ⇧ to search everywhere for classes, files, tool windows, actions, and settings.


def calc(timepart_string, time_in_h, mc_mode):
    # Use a breakpoint in the code line below to debug your script.
    # sleep phase: [ms] [mA]
    time = time_in_h * 60 * 60 * 1000
    sleep_mode_mA = 2.6
    herz_mode_mA = 55
    display_refresh_mA = 60
    display_refresh_time = 50
    display_refresh_rate = 1000
    receive_mA = 94
    receive_time = 100
    receive_rate = 5000
    transmit_mA = 200
    transmit_time = 100
    transmit_rate = 15 * 60 * 1000

    if mc_mode == 0:
        print('SLEEP PHASE CALCULATION:')

    if mc_mode == 1:
        print('BUSY PHASE CALCULATION:')


    print('# Configs: ')
    print('time in [ms]: ' + str(time))
    print('current consumption sleep mode [mA]: ' + str(sleep_mode_mA))
    print('current consumption 160 Hz mode [mA]: ' + str(herz_mode_mA))
    print('display refresh current [mA]: ' + str(display_refresh_mA))
    print('display refresh time [ms]: ' + str(display_refresh_time))
    print('display refresh time interval [ms]: ' + str(display_refresh_rate))
    print('WIFI receive current [mA]: ' + str(receive_mA))
    print('WIFI receive time [ms]: ' + str(receive_time))
    print('WIFI receive time interval [ms]: ' + str(receive_rate))
    print('WIFI transmit current [mA]: ' + str(transmit_mA))
    print('WIFI transmit time [ms]: ' + str(transmit_time))
    print('WIFI transmit time interval [ms]: ' + str(transmit_rate))

    print('# Calculations:')
    count_change = (30 + 60 + 60 + 60 + 60 + 30)

    if mc_mode == 1:
        print('Average counts in busy phase: 30 + 60 + 60 + 60 + 60 + 30 = ' + str(count_change))

    display_refresh_timepart = round((((time / display_refresh_rate) * display_refresh_time) / time), 4)
    print('Display_refresh time share of total ' + timepart_string + ' phase: \n((' + str(time) + '[ms] / ' + str(
        display_refresh_rate) + '[ms]) * ' + str(display_refresh_time) + ' [ms] / ' + str(time) + '[ms]) = ' + str(
        display_refresh_timepart))

    receive_timepart = round((((time / receive_rate) * receive_time) / time), 4)
    print('Receive time share of total ' + timepart_string + ' phase: \n((' + str(time) + '[ms] / ' + str(
        receive_rate) + '[ms]) * ' + str(receive_time) + ' [ms] / ' + str(time) + '[ms]) = ' + str(receive_timepart))

    transmit_current_timepart = round((((time / transmit_rate) * transmit_time) / time), 4)
    print('Transmit time share of total ' + timepart_string + ' phase: \n((' + str(time) + '[ms] / ' + str(
        transmit_rate) + '[ms]) * ' + str(transmit_time) + '[ms] ) / ' + str(time) + '[ms]) = ' + str(
        transmit_current_timepart))

    if mc_mode == 1:
        transmit_current_timepart = round((((time / transmit_rate + count_change) * transmit_time) / time), 4)
        print('Transmit time share of total ' + timepart_string + ' phase: \n((' + str(time) + '[ms] / ' + str(
            transmit_rate) + '[ms]) + ' + str(count_change) + '[counts]) * ' + str(transmit_time) + ' [ms] / ' + str(
            time) + '[ms]) = ' + str(
            transmit_current_timepart))

    if mc_mode == 1:
        cpu_bursts_timepart = round(((count_change * transmit_time) / time), 4)

    sleep_mode_timepart = round((1 - display_refresh_timepart - receive_timepart - transmit_current_timepart), 4)
    print('Light_sleep_mode time share of total ' + timepart_string + ' phase: ' + str(1) + ' - ' + str(
        display_refresh_timepart) + ' - ' + str(receive_timepart) + ' - ' + str(
        transmit_current_timepart) + ' = ' + str(sleep_mode_timepart))

    timepart_consumption = round((display_refresh_timepart * display_refresh_mA \
                                  + receive_timepart * receive_mA \
                                  + transmit_current_timepart * transmit_mA \
                                  + sleep_mode_timepart * sleep_mode_mA), 4)

    if mc_mode == 1:
        timepart_consumption = round((display_refresh_timepart * display_refresh_mA \
                                      + receive_timepart * receive_mA \
                                      + transmit_current_timepart * transmit_mA \
                                      + sleep_mode_timepart * sleep_mode_mA \
                                      + cpu_bursts_timepart * herz_mode_mA), 4)
    # [mA][h]

    print('Current consumption of ' + timepart_string + ' phase:')
    if mc_mode == 0:
        print(str(display_refresh_timepart) + ' * ' + str(display_refresh_mA) + '[mA] + ' +
              str(receive_timepart) + ' * ' + str(receive_mA) + '[mA] + ' +
              str(transmit_current_timepart) + ' * ' + str(transmit_mA) + '[mA] + ' +
              str(sleep_mode_timepart) + ' * ' + str(sleep_mode_mA) + '[mA] = ' +
              str(timepart_consumption) + '[mA]'
              )

    if mc_mode == 1:
        print(str(display_refresh_timepart) + ' * ' + str(display_refresh_mA) + '[mA] + ' +
              str(receive_timepart) + ' * ' + str(receive_mA) + '[mA] + ' +
              str(transmit_current_timepart) + ' * ' + str(transmit_mA) + '[mA] + ' +
              str(sleep_mode_timepart) + ' * ' + str(sleep_mode_mA) + '[mA] + ' +
              str(cpu_bursts_timepart) + ' * ' + str(herz_mode_mA) + '[mA] = ' +
              str(timepart_consumption) + '[mA]'
              )
    return timepart_consumption

# Press the green button in the gutter to run the script.
if __name__ == '__main__':


    timepart_consumtion_sleep = calc('sleep', 23, 0)
    print('')
    timepart_consumption_busy = calc('busy', 1, 1)

    total_consumption = round((23/24 * timepart_consumtion_sleep + (1/24) * timepart_consumption_busy) , 4)
    print('')
    print('Total average consumption: 23/24 * ' + str(timepart_consumtion_sleep) + ' + 1/24 * ' + str(timepart_consumption_busy) + ' = ' + str(total_consumption))
    lifetime = round((600 / total_consumption), 4)
    print('Estimated lifetime: 600mAh / ' + str(total_consumption) + ' = ' + str(lifetime) + '[h]')

# See PyCharm help at https://www.jetbrains.com/help/pycharm/
