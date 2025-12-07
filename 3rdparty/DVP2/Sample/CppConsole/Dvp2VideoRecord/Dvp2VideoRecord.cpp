#include <iostream>
#include <conio.h>
#include <thread>
using namespace std;

/* 根据后缀保存相应格式视频，推荐wmv格式 */
#define FILENAME "zhang.wmv"

/* DVP API 依赖 */
#include "../src/include/DVPCamera.h"
#ifdef _M_X64
#pragma comment(lib, "../src/lib/x64/DVPCamera64.lib")
#else				  
#pragma comment(lib, "../src/lib/x86/DVPCamera.lib")
#endif

#pragma warning(disable:4996)

static dvpRecordHandle videohandle;	/* 视频编码器句柄  */

/* 自定义写视频流到文件的回调函数 */
static dvpInt32 OnVideoRecordWrite(dvpHandle handle, dvpStreamEvent event, void* pContext, dvpFrame* pFrame, void* pBuffer)
{
	dvpStatus status;

	/* 显示帧信息 */
	printf("frame:%lld, timestamp:%lld, %d*%d, %dbytes, format:%d\r",
		pFrame->uFrameID, pFrame->uTimestamp, pFrame->iWidth,
		pFrame->iHeight, pFrame->uBytes, pFrame->format);

	bool isVideoRecorderValid;
	status = dvpIsVideoRecorderValid(videohandle, &isVideoRecorderValid);
	if (status != DVP_STATUS_OK)
	{
		printf("dvpIsVideoRecorderValid failed with err:%d\r\n", status);
	}

	if (isVideoRecorderValid)
	{
		status = dvpVideoRecordWrite(videohandle, pFrame, pBuffer);
		if (status != DVP_STATUS_OK)
		{
			printf("dvpVideoRecordWrite failed with err:%d\r\n", status);
		}
	}
	else {
		dvpStartVideoRecord(FILENAME, 0, 0, 100, &videohandle);
	}


	/* 通常返回0即可，没有特殊含义 */
	return 0;
}

void test(void* p)
{
	dvpStatus status;
	dvpHandle h;/* 相机句柄 */

	char* name = (char*)p;

	printf("Test start,camera is %s\r\n", name);
	do
	{
		/* 打开设备 */
		status = dvpOpenByName(name, OPEN_NORMAL, &h);
		if (status != DVP_STATUS_OK)
		{
			printf("dvpOpenByName failed with err:%d\r\n", status);
			break;
		}

		dvpRegion region;

		/* 设置为连续模式 */
		status = dvpSetTriggerState(h, false);
		if (status != DVP_STATUS_OK)
		{
			printf("dvpSetTriggerState failed with err:%d\r\n", status);
			break;
		}

		/* 打印ROI信息 */
		status = dvpGetRoi(h, &region);
		if (status != DVP_STATUS_OK)
		{
			printf("dvpGetRoi failed with err:%d\r\n", status);
			break;
		}
		printf("%s, region: x:%d, y:%d, w:%d, h:%d\r\n", name, region.X, region.Y, region.W, region.H);

		/* 在打开相机之后，开启视频流之前注册事件回调函数，启动一个专门的线程以dvpGetFrame（同步采集）的方式获取图像 */
		status = dvpRegisterStreamCallback(h, OnVideoRecordWrite, STREAM_EVENT_PROCESSED, NULL);
		if (status != DVP_STATUS_OK)
		{
			printf("dvpRegisterStreamCallback failed with err:%d\r\n", status);
			break;
		}

		/* 开始视频流 */
		status = dvpStart(h);
		if (status != DVP_STATUS_OK)
		{
			break;
		}

		status = dvpStartVideoRecord(FILENAME, 0, 0, 100, &videohandle);
		if (status != DVP_STATUS_OK)
		{
			printf("dvpStartVideoRecord failed with err:%d\r\n", status);
			break;
		}

		/* 主线程等待 */
		int key;
		printf("\nPress ESC to exit!\n");
		while (1)
		{
			//getch会阻塞主线程
			key = getch();
			if (key == 27)
			{
				/* 录像结束 */
				status = dvpStopVideoRecord(videohandle);
				if (status != DVP_STATUS_OK)
				{
					break;
				}

				/* 关闭视频流 */
				status = dvpStop(h);
				if (status != DVP_STATUS_OK)
				{
					break;
				}
				break;
			}
		}
	} while (0);

	/* 关闭相机 */
	status = dvpClose(h);

	printf("\ntest quit, %s, status:%d\n", name, status);
}


int main()
{
	printf("start...\r\n");

	dvpUint32 count = 0, num = -1;
	dvpCameraInfo info[8];

	/* 枚举设备 */
	dvpRefresh(&count);
	if (count > 8)
		count = 8;

	for (int i = 0; i < (int)count; i++)
	{
		if (dvpEnum(i, &info[i]) == DVP_STATUS_OK)
		{
			printf("[%d]-Camera FriendlyName : %s\r\n", i, info[i].FriendlyName);
		}
	}

	/* 没发现设备 */
	if (count == 0)
	{
		printf("No device found!\n");
		return 0;
	}

	while (num < 0 || num >= count)
	{
		printf("Please enter the number of the camera you want to open: \r\n");
		scanf("%d", &num);
	}

	thread task(test, (void*)info[num].FriendlyName);
	task.join();

	system("pause");
	return 0;
}