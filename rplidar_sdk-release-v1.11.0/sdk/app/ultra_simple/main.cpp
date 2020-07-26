//http://shibafu3.hatenablog.com/entry/2018/08/23/151237

/*
 *  RPLIDAR
 *  Ultra Simple Data Grabber Demo App
 *
 *  Copyright (c) 2009 - 2014 RoboPeak Team
 *  http://www.robopeak.com
 *  Copyright (c) 2014 - 2019 Shanghai Slamtec Co., Ltd.
 *  http://www.slamtec.com
 *
 */
/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "rplidar.h" //RPLIDAR standard sdk, all-in-one header


//-------------------------
//#include "stdafx.h"
#define UDP_SEND_PORT (7003)
short udp_port = UDP_SEND_PORT;

 //TCP,UDP通信用ライブラリ
#pragma comment(lib, "ws2_32.lib")

 //TCP,UDP通信用ヘッダ
 //#include <sys/sock.h> //linux
#include <WinSock2.h> //windows

#include <iostream>

 // inet_addr()関数で警告が出る場合は以下で警告を無効化する。
#pragma warning(disable:4996) 

SOCKET sock;
struct sockaddr_in addr;

void openSocket(short _port) {
	// ソケット通信winsockの立ち上げ
	// wsaDataはエラー取得等に使用する
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 0), &wsaData);   //MAKEWORD(2, 0)はwinsockのバージョン2.0ってこと

											// socket作成
											// socketは通信の出入り口 ここを通してデータのやり取りをする
											// socket(アドレスファミリ, ソケットタイプ, プロトコル)
	sock = socket(AF_INET, SOCK_DGRAM, 0);  //AF_INETはIPv4、SOCK_DGRAMはUDP通信、0は？

											// アドレス等格納
	addr.sin_family = AF_INET;  //IPv4
	addr.sin_port = htons(_port);   //通信ポート番号設定
	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // 送信アドレスを127.0.0.1に設定
}
void closeSocket() {
	// socketの破棄
	closesocket(sock);

	// winsockの終了
	WSACleanup();

	printf("-send\n");
}

int sendUDP(char* buff, int sizeBuff) {
	// 送信
	// sendto(ソケット, 送信するデータ, データのバイト数, フラグ, アドレス情報, アドレス情報のサイズ);
	// 送信するデータに直接文字列 "HELLO" 等を入れることもできる
	// バインドしている場合は send(sock, buf, 5, 0); でもOK？
	sendto(sock, buff, sizeBuff, 0, (struct sockaddr *)&addr, sizeof(addr));
	return 0;
}
//-------------------------

#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif

#ifdef _WIN32
#include <Windows.h>
#define delay(x)   ::Sleep(x)
#else
#include <unistd.h>
static inline void delay(_word_size_t ms){
    while (ms>=1000){
        usleep(1000*1000);
        ms-=1000;
    };
    if (ms!=0)
        usleep(ms*1000);
}
#endif

using namespace rp::standalone::rplidar;

bool checkRPLIDARHealth(RPlidarDriver * drv)
{
    u_result     op_result;
    rplidar_response_device_health_t healthinfo;

	op_result = drv->getHealth(healthinfo);
    if (IS_OK(op_result)) { // the macro IS_OK is the preperred way to judge whether the operation is succeed.

        printf("RPLidar health status : %d\n", healthinfo.status);
        if (healthinfo.status == RPLIDAR_STATUS_ERROR) {
            fprintf(stderr, "Error, rplidar internal error detected. Please reboot the device to retry.\n");
            // enable the following code if you want rplidar to be reboot by software
            // drv->reset();
            return false;
        } else {
            return true;
        }

    } else {
        fprintf(stderr, "Error, cannot retrieve the lidar health code: %x\n", op_result);
        return false;
    }
}

#include <signal.h>
bool ctrl_c_pressed;
void ctrlc(int)
{
    ctrl_c_pressed = true;
}

int main(int argc, const char * argv[]) {
	const char * opt_com_path = NULL;
    _u32         baudrateArray[2] = {115200, 256000};
    _u32         opt_com_baudrate = 0;
    u_result     op_result;

    bool useArgcBaudrate = false;

	/*
	printf("Ultra simple LIDAR data grabber for RPLIDAR.\n"
	"Version: "RPLIDAR_SDK_VERSION"\n");
	*/

	if (argc <= 1)
		printf("usage: %s [comPort(default=%s)] [Baudrate(default=0)] [udpPort(default=%d)].\n", argv[0], "com8", udp_port);

	// read serial port from the command line...
    if (argc>1) opt_com_path = argv[1]; // or set to a fixed value: e.g. "com3" 

    // read baud rate from the command line if specified...
    if (argc>2)
    {
        opt_com_baudrate = strtoul(argv[2], NULL, 10);
		if (opt_com_baudrate > 0) {
			useArgcBaudrate = true;
		}
    }
	// read UDP port
	if (argc>3)
	{
		sscanf(argv[3], "%hi", &udp_port);
	}

	printf("udpopen\n");
	openSocket(udp_port);
	printf("udpsend\n");
	char buf[32] = "HELLO";
	sendUDP(buf, sizeof(buf));
	printf("udpclose\n");
	closeSocket();
	printf("udpfinish\n");

	
	if (!opt_com_path) {
#ifdef _WIN32
        // use default com port
        opt_com_path = "\\\\.\\com8";
#elif __APPLE__
        opt_com_path = "/dev/tty.SLAB_USBtoUART";
#else
        opt_com_path = "/dev/ttyUSB0";
#endif
    }

    // create the driver instance
	RPlidarDriver * drv = RPlidarDriver::CreateDriver(DRIVER_TYPE_SERIALPORT);
    if (!drv) {
        fprintf(stderr, "insufficent memory, exit\n");
        exit(-2);
    }
    
    rplidar_response_device_info_t devinfo;
    bool connectSuccess = false;
    // make connection...
    if(useArgcBaudrate)
    {
        if(!drv)
            drv = RPlidarDriver::CreateDriver(DRIVER_TYPE_SERIALPORT);
        if (IS_OK(drv->connect(opt_com_path, opt_com_baudrate)))
        {
            op_result = drv->getDeviceInfo(devinfo);

            if (IS_OK(op_result)) 
            {
                connectSuccess = true;
            }
            else
            {
                delete drv;
                drv = NULL;
            }
        }
    }
    else
    {
        size_t baudRateArraySize = (sizeof(baudrateArray))/ (sizeof(baudrateArray[0]));
        for(size_t i = 0; i < baudRateArraySize; ++i)
        {
            if(!drv)
                drv = RPlidarDriver::CreateDriver(DRIVER_TYPE_SERIALPORT);
            if(IS_OK(drv->connect(opt_com_path, baudrateArray[i])))
            {
                op_result = drv->getDeviceInfo(devinfo);

                if (IS_OK(op_result)) 
                {
                    connectSuccess = true;
                    break;
                }
                else
                {
                    delete drv;
                    drv = NULL;
                }
            }
        }
    }
    if (!connectSuccess) {
        
        fprintf(stderr, "Error, cannot bind to the specified serial port %s.\n"
            , opt_com_path);
        goto on_finished;
    }

    // print out the device serial number, firmware and hardware version number..
    printf("RPLIDAR S/N: ");
    for (int pos = 0; pos < 16 ;++pos) {
        printf("%02X", devinfo.serialnum[pos]);
    }

    printf("\n"
            "Firmware Ver: %d.%02d\n"
            "Hardware Rev: %d\n"
            , devinfo.firmware_version>>8
            , devinfo.firmware_version & 0xFF
            , (int)devinfo.hardware_version);



    // check health...
    if (!checkRPLIDARHealth(drv)) {
        goto on_finished;
    }

    signal(SIGINT, ctrlc);
    
    drv->startMotor();
    // start scan...
    drv->startScan(0,1);


	openSocket(udp_port);
	printf("udpsend\n");


	char buff0[1024];
	// fetech result and print it out...
    while (1) {
        //rplidar_response_measurement_node_t nodes[8192];
		rplidar_response_measurement_node_hq_t hq_nodes[8192];
		size_t   count = _countof(hq_nodes);

        //op_result = drv->grabScanData(nodes, count);
		op_result = drv->grabScanDataHq(hq_nodes, count);

        if (IS_OK(op_result)) {
            drv->ascendScanData(hq_nodes, count);
            for (int pos = 0; pos < (int)count ; ++pos) {
#if 0
				int quality = hq_nodes[pos].sync_quality >> RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT;
				sprintf_s(buff0, 1024, "A:%03.2f, D:%08.2f, Q:%d, S:%s,\n",
					(nodes[pos].angle_q6_checkbit >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT)/64.0f,
                    nodes[pos].distance_q2/4.0f,
                    nodes[pos].sync_quality >> RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT,
					(nodes[pos].sync_quality & RPLIDAR_RESP_MEASUREMENT_SYNCBIT) ? "S " : "  ");
#else
				int quality = hq_nodes[pos].quality >> RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT;
				sprintf_s(buff0, 1024, "A:%03.2f, D:%8d, Q:%d,\n",
					(hq_nodes[pos].angle_z_q14 >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT) / (90.f*1.0112f),
					hq_nodes[pos].dist_mm_q2,
					quality
				);
#endif
				int sl = strlen(buff0);
				printf("%d-%s", sl, buff0);
				if (quality != 0) {
					sendUDP(buff0, sl + 1);
				}
			}
        }

        if (ctrl_c_pressed){ 
			closeSocket();
			printf("udpfinish\n");
			printf("break");
            break;
        }
    }

    drv->stop();
    drv->stopMotor();
    // done!
on_finished:
    RPlidarDriver::DisposeDriver(drv);
    drv = NULL;
    return 0;
}

