/*
 * Copyright (c) 2012 Damien Grauser (Damien.Grauser@pev-geneve.ch)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "Bin2RideFile.h"
#include <math.h>

#define START 0x210
#define UNIT_VERSION 0x2000
#define SYSTEM_INFO 0x2003

static int bin2FileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "bin2", "Joule GPS File", new Bin2FileReader());


struct Bin2FileReaderState
{
    QFile &file;
    QStringList &errors;
    RideFile *rideFile;

    double secs, km;

    int interval;
    double last_interval_secs;

    bool stopped;

    QString deviceInfo;

    Bin2FileReaderState(QFile &file, QStringList &errors) :
        file(file), errors(errors), rideFile(NULL), secs(0), km(0),
        interval(0), last_interval_secs(0.0),  stopped(true)
    {

    }

    struct TruncatedRead {};

    int bcd2Int(char c)
    {
        return (0xff & c) - (((0xff & c)/16)*6);
    }

    int read_bytes(int len, int *count = NULL, int *sum = NULL)
    {
        char c;
        int res = 0;
        for (int i = 0; i < len; ++i) {
            if (file.read(&c, 1) != 1)
                throw TruncatedRead();
            if (sum)
                *sum += (0xff & c);
            if (count)
                *count += 1;

            res += pow(256,i) * (0xff & (unsigned) c) ;
        }
        return res;
    }

    QString read_text(int len, int *count = NULL, int *sum = NULL)
    {
        char c;
        QString res = "";
        for (int i = 0; i < len; ++i) {
            if (file.read(&c, 1) != 1)
                throw TruncatedRead();
            if (sum)
                *sum += (0xff & c);
            if (count)
                *count += 1;

            res += c;
        }
        return res;
    }

    QDateTime read_date(int *bytes_read = NULL, int *sum = NULL)
    {
        int sec = bcd2Int(read_bytes(1, bytes_read, sum));
        int min = bcd2Int(read_bytes(1, bytes_read, sum));
        int hour = bcd2Int(read_bytes(1, bytes_read, sum));
        int day = bcd2Int(read_bytes(1, bytes_read, sum));
        int month = bcd2Int(read_bytes(1, bytes_read, sum));
        int year = bcd2Int(read_bytes(1, bytes_read, sum));

        return QDateTime(QDate(2000+year,month,day), QTime(hour,min,sec));
    }

    QDateTime read_RTC_mark(double *secs, int *bytes_read = NULL, int *sum = NULL)
    {
        QDateTime date = read_date(bytes_read, sum);

        read_bytes(1, bytes_read, sum); // dummy
        int time_moving = read_bytes(4, bytes_read, sum);
        *secs = double(read_bytes(4, bytes_read, sum));
        read_bytes(16, bytes_read, sum);  // dummy

        return date;
    }

    int read_interval_mark(double *secs, int *bytes_read = NULL, int *sum = NULL)
    {
        int intervalNumber = read_bytes(1, bytes_read, sum);;

        read_bytes(2, bytes_read, sum); // dummy
        *secs = double(read_bytes(4, bytes_read, sum));
        read_bytes(24, bytes_read, sum); // dummy

        return intervalNumber;
    }

    void read_detail_record(double *secs, int *bytes_read = NULL, int *sum = NULL)
    {
        int cad = read_bytes(1, bytes_read, sum);
        int pedal_smoothness = read_bytes(1, bytes_read, sum);
        int lrbal = read_bytes(1, bytes_read, sum);
        int hr = read_bytes(1, bytes_read, sum);
        int dummy = read_bytes(1, bytes_read, sum);
        int watts = read_bytes(2, bytes_read, sum);
        int nm = read_bytes(2, bytes_read, sum);
        double kph = read_bytes(2, bytes_read, sum);
        int alt = read_bytes(2, bytes_read, sum);
        double temp = read_bytes(2, bytes_read, sum)/10.0;
        double lat = read_bytes(4, bytes_read, sum);
        double lng = read_bytes(4, bytes_read, sum);
        double km = read_bytes(8, bytes_read, sum)/1000.0/1000.0;

        // Validations
        if (lrbal == 0xFF)
            lrbal = 0;
        else if ((lrbal & 0x200) == 0x200)
            lrbal = 100-lrbal;

        if (cad == 0xFF)
            cad = 0;

        if (hr == 0xFF)
            hr = 0;

        if (watts == 0xFFFF) // 65535
            watts = 0;

        if (kph == 0xFFFF) // 65535
            kph = 0;
        else
            kph = kph/10.0;

        if (temp == 0x8000)
            temp = 0;

        if (alt == 0x8000)
            alt = 0;

        if (lat == -2147483648) //2147483648
            lat = 0;
        else
            lat = lat/10000000.0;

        if (lng == -2147483648) //0x80000000
            lng = 0;
        else
            lng = lng/10000000.0;

        rideFile->appendPoint(*secs, cad, hr, km, kph, nm, watts, alt, lng, lat, 0.0, 0, temp, lrbal, interval);
        (*secs)++;
    }

    void read_header(uint16_t &header, uint16_t &command, uint16_t &length, int *bytes_read = NULL, int *sum = NULL)
    {
        header = read_bytes(2, bytes_read, sum);
        command = read_bytes(2, bytes_read, sum);
        length = read_bytes(2, bytes_read, sum);
    }

    void read_ride_summary(int *bytes_read = NULL, int *sum = NULL)
    {
        char data_version = read_bytes(1, bytes_read, sum);
        char firmware_minor_version = read_bytes(1, bytes_read, sum);

        QDateTime t = read_date(bytes_read, sum);

        rideFile->setStartTime(t);

        read_bytes(148, bytes_read, sum);
    }

    void read_interval_summary(int *bytes_read = NULL, int *sum = NULL)
    {
        read_bytes(3200, bytes_read, sum);
    }

    void read_username(int *bytes_read = NULL, int *sum = NULL)
    {
        QString name = read_text(20, bytes_read, sum);
        //deviceInfo += QString("User : %1\n").arg(name);
    }

    void read_user_info_record(int *bytes_read = NULL, int *sum = NULL)
    {
        read_bytes(64, bytes_read, sum);

        int smartbelt_A = read_bytes(2, bytes_read, sum);
        int smartbelt_B = read_bytes(2, bytes_read, sum);
        int smartbelt_C = read_bytes(2, bytes_read, sum);
        //deviceInfo += QString("Smartbelt %1-%2-%3\n").arg(smartbelt_A).arg(smartbelt_B).arg(smartbelt_C);

        read_bytes(42, bytes_read, sum);
    }

    void read_ant_info_record(int *bytes_read = NULL, int *sum = NULL)
    {
        for (int i = 0; i < 6; i++) {
            int device_type = read_bytes(1, bytes_read, sum);
            if (device_type < 255)  {
                QString text = read_text(20, bytes_read, sum);
                while(text.endsWith( QChar(0) )) text.chop(1);
                QChar *chr = text.end();
                int i = chr->toAscii();

                int flag = read_bytes(1, bytes_read, sum);
                uint16_t id = read_bytes(2, bytes_read, sum);
                read_bytes(2, bytes_read, sum);
                read_bytes(2, bytes_read, sum);
                QString device_type_str;
                if (device_type == 11)
                    device_type_str = "Primary Power Id";
                else if (device_type == 120)
                    device_type_str = "Chest strap Id";
                else
                    device_type_str = QString("ANT %1 Id").arg(device_type);

                deviceInfo += QString("%1 %2 %3\n").arg(device_type_str).arg(id).arg(text);
            }
        }
    }

    // pages
    int read_summary_page()
    {
        int sum = 0;
        int bytes_read = 0;

        char header1 = read_bytes(1, &bytes_read, &sum); // Always 0x10
        char header2 = read_bytes(1, &bytes_read, &sum); // Always 0x02
        uint16_t command = read_bytes(2, &bytes_read, &sum);


        if (header1 == 0x10 && header2 == 0x02 && command == 0x2022)
        {
            uint16_t length = read_bytes(2, &bytes_read, &sum);
            uint16_t page_number = read_bytes(2, &bytes_read, &sum); // Page #

            if (page_number == 0) {
                // Page #0
                read_ride_summary(&bytes_read, &sum);
                read_interval_summary(&bytes_read, &sum);
                read_username(&bytes_read, &sum);
                read_user_info_record(&bytes_read, &sum);
                read_ant_info_record(&bytes_read, &sum);

                int finish = length+6-bytes_read;

                for (int i = 0; i < finish; i++) {
                    read_bytes(1, &bytes_read, &sum); // to finish
                }

                char checksum = read_bytes(1, &bytes_read, &sum);

            } else {
               // not a summary page !
            }


        }

        return bytes_read;
    }

    int read_detail_page()
    {
        int sum = 0;
        int bytes_read = 0;

        char header1 = read_bytes(1, &bytes_read, &sum); // Always 0x10
        char header2 = read_bytes(1, &bytes_read, &sum); // Always 0x02
        uint16_t command = read_bytes(2, &bytes_read, &sum);

        if (header1 == 0x10 && header2 == 0x02 && command == 0x2022)
        {
            uint16_t length = read_bytes(2, &bytes_read, &sum);
            uint16_t page_number = read_bytes(2, &bytes_read, &sum); // Page #

            if (page_number > 0) {
                // Page # >0
                // 128 x 32k
                QDateTime t;


                for (int i = 0; i < 128; i++) {
                    int flag = read_bytes(1, &bytes_read, &sum);
                    //b0..b1: "00" Detail Record
                    //b0..b1: "01" RTC Mark Record
                    //b0..b1: "00" Interval Record

                    //b2: reserved
                    //b3: Power was calculated
                    //b4: HPR packet missing
                    //b5: CAD packet missing
                    //b6: PWR packet missing
                    //b7: Power data = old (No new power calculated)

                    if (flag == 0xff) {
                        // means invalid entry
                        read_bytes(31, &bytes_read, &sum);
                    }
                    else if ((flag & 0x03) == 0x01){
                        t= read_RTC_mark(&secs, &bytes_read, &sum);
                    }
                    else if ((flag & 0x03) == 0x03){
                        int t = read_interval_mark(&secs, &bytes_read, &sum);
                        interval = t;
                    }
                    else if ((flag & 0x03) == 0x00 ){
                        read_detail_record(&secs, &bytes_read, &sum);
                    }

                }
                char checksum = read_bytes(1, &bytes_read, &sum);

            }


        }

        return bytes_read;
    }

    int read_version()
    {
        int sum = 0;
        int bytes_read = 0;

        uint16_t header = read_bytes(2, &bytes_read, &sum); // Always 0x210 (0x10-0x02)
        uint16_t command = read_bytes(2, &bytes_read, &sum);

        if (header == START && command == UNIT_VERSION)
        {
            uint16_t length = read_bytes(2, &bytes_read, &sum);

            int major_version = read_bytes(1, &bytes_read, &sum);
            int minor_version = read_bytes(2, &bytes_read, &sum);
            int data_version = read_bytes(2, &bytes_read, &sum);

            QString version = QString(minor_version<100?"%1.0%2 (%3)":"%1.%2 (%3)").arg(major_version).arg(minor_version).arg(data_version);
            deviceInfo += rideFile->deviceType()+QString(" Version %1\n").arg(version);

            char checksum = read_bytes(1, &bytes_read, &sum);
        }
        return bytes_read;
    }

    int read_system_info()
    {
        int sum = 0;
        int bytes_read = 0;

        uint16_t header = read_bytes(2, &bytes_read, &sum); // Always (0x10-0x02)
        uint16_t command = read_bytes(2, &bytes_read, &sum);


        if (header == START && command == SYSTEM_INFO)
        {
            uint16_t length = read_bytes(2, &bytes_read, &sum);

            read_bytes(52, &bytes_read, &sum);
            uint16_t odometer = read_bytes(8, &bytes_read, &sum);
            deviceInfo += QString("Odometer %1km\n").arg(odometer/1000.0);

            char checksum = read_bytes(1, &bytes_read, &sum);
        }
        return bytes_read;
    }



    RideFile * run() {
        errors.clear();
        rideFile = new RideFile;
        rideFile->setDeviceType("Joule GPS");
        rideFile->setFileFormat("CycleOps Joule (bin2)");
        rideFile->setRecIntSecs(1);

        if (!file.open(QIODevice::ReadOnly)) {
            delete rideFile;
            return NULL;
        }
        bool stop = false;

        int data_size = file.size();
        int bytes_read = 0;

        bytes_read += read_version();
        bytes_read += read_system_info();
        bytes_read += read_summary_page();

        while (!stop && (bytes_read < data_size)) {
            bytes_read += read_detail_page(); // read_page(stop, errors);
        }

        rideFile->setTag("Device Info", deviceInfo);

        if (stop) {
            delete rideFile;
            return NULL;
        }
        else {
            return rideFile;
        }
    }
};

RideFile *Bin2FileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*) const
{
    QSharedPointer<Bin2FileReaderState> state(new Bin2FileReaderState(file, errors));
    return state->run();
}
