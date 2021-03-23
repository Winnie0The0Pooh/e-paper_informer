from datetime import datetime, time, timedelta

import os
import locale


def main():
#    locale.setlocale(locale.LC_ALL, 'ru-Ru')

    current_workdir = os.getcwd()
    workdir = "C:/Users/atomrdp/Documents/My Web Sites/LightControl/"
    os.chdir(workdir)

    fn = os.listdir()
    fnd = list(filter(lambda x: x.startswith('Date_'), fn))
    ct_fnd = len(fnd)
    if ct_fnd > 0:
        current_fnd = fnd[-1]
        fnd_creation_time = os.stat(fnd[-1]).st_ctime
        s = datetime.fromtimestamp(fnd_creation_time).strftime('%Y-%m-%d %H:%M:%S')
        x = open(current_fnd, 'r')
        fnd_string = x.readline()
        x.close()

        Vcc = fnd_string[0:8]
        vcc_mv = fnd_string[4:8]

        with open('e_paper_log.txt', 'a') as logfile:
            logfile.write(repr(fnd_creation_time))
            logfile.write(" ")
            logfile.write(s)
            logfile.write(" Vcc= ")
            logfile.write(vcc_mv)
            logfile.write(" mV ")
            logfile.write(fnd_string)
            logfile.close()

        with open('e_paper_last_data.txt', 'w') as lastdata:
            lastdata.write(s)
            lastdata.write(" ")
            lastdata.write(fnd_string)

            #lastdata.write(" ")
            #lastdata.write(s)

            lastdata.close()

        #fnd_string = '2021-02-06 16:08:33                '
        
        #time_string = fnd_string[0:19]
        #print(time_string)
        base_time = datetime.strptime(s, "%Y-%m-%d %H:%M:%S")
        
        print(base_time)

        next_time_to_make_bmp = base_time + timedelta(minutes=67)
        print(next_time_to_make_bmp)
        next_time_to_check_after = base_time + timedelta(minutes=72)
        print(next_time_to_check_after)

        s_next_time_to_make_bmp = datetime.strftime(next_time_to_make_bmp, '%H:%M:%S')
        s_next_date_to_make_bmp = datetime.strftime(next_time_to_make_bmp, '%d.%m.%Y')

        s_next_time_to_check_after = datetime.strftime(next_time_to_check_after, '%H:%M:%S')
        s_next_date_to_check_after = datetime.strftime(next_time_to_check_after, '%d.%m.%Y')

        
        s1 = 'schtasks /change  /tn !epaper2 /st ' + s_next_time_to_make_bmp
        s1 = s1 + ' /sd ' + s_next_date_to_make_bmp + ' < enter.txt\n'

        s2 = 'schtasks /change  /tn !epaper3 /st ' + s_next_time_to_check_after
        s2 = s2 + ' /sd ' + s_next_date_to_check_after + ' < enter.txt\n'

        os.chdir(current_workdir)

        x = open('change_time.cmd', 'w')
        x.write(s1)
        x.write(s2)
        x.write('TIMEOUT /T 60')
        x.close()
        print('Data processed. cmd file done')

    else:

        print('No data to process.  Doing nothing.') 

if __name__ == "__main__":
    main()
