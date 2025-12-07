#include <iostream>
#include <thread>
using namespace std;

// DVP API 依赖
#include "../src/include/DVPCamera.h"
#ifdef _M_X64
#pragma comment(lib, "../src/lib/x64/DVPCamera64.lib")
#else				  
#pragma comment(lib, "../src/lib/x86/DVPCamera.lib")
#endif

#pragma warning(disable:4996)

#define GRABCOUNT 2000		/* 抓帧次数 */

/* 数据流已启动，通常返回0 */
static dvpInt32 OnEventStreamStarted(dvpHandle handle, dvpEvent event, void* pContext, dvpInt32 param, struct dvpVariant* pVariant)
{
	/* 当发生对应事件时可添加自定义策略 */
	printf("OnEventStreamStarted \n");
	return 0;
}

/* 数据流已停止 */
static dvpInt32 OnEventStreamStoped(dvpHandle handle, dvpEvent event, void* pContext, dvpInt32 param, struct dvpVariant* pVariant)
{
	printf("OnEventStreamStoped \n");
	return 0;
}

/* 帧丢失 */
static dvpInt32 OnEventFrameLost(dvpHandle handle, dvpEvent event, void* pContext, dvpInt32 param, struct dvpVariant* pVariant)
{
	printf("OnEventFrameLost \n");
	return 0;
}

/* 等待超时 */
static dvpInt32 OnEventFrameTimeout(dvpHandle handle, dvpEvent event, void* pContext, dvpInt32 param, struct dvpVariant* pVariant)
{
	printf("OnEventFrameTimeout \n");
	return 0;
}

/* 重新连接 */
static dvpInt32 OnEventReconnected(dvpHandle handle, dvpEvent event, void* pContext, dvpInt32 param, struct dvpVariant* pVariant)
{
	printf("OnEventReconnected \n");
	return 0;
}

/* 帧开始传输 */
static dvpInt32 OnEventFrameStart(dvpHandle handle, dvpEvent event, void* pContext, dvpInt32 param, struct dvpVariant* pVariant)
{
	printf("OnEventFrameStart \n");
	return 0;
}

/* 帧传输结束 */
static dvpInt32 OnEventFrameEnd(dvpHandle handle, dvpEvent event, void* pContext, dvpInt32 param, struct dvpVariant* pVariant)
{
	printf("OnEventFrameEnd \n");
	return 0;
}

void test(void* p)
{
	dvpStatus status;
	dvpHandle h;
	bool trigMode = false;

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

		/* 设置为连续模式 */
		status = dvpSetTriggerState(h, false);
		if (status == DVP_STATUS_OK)
		{
			trigMode = false;
		}
		else
		{
			printf("dvpSetTriggerState failed with err:%d\r\n", status);
			break;
		}

		/* 在打开相机之后，开启视频流之前注册事件回调函数 */
		status = dvpRegisterEventCallback(h, OnEventStreamStarted, EVENT_STREAM_STARTRD, NULL);
		if (status != DVP_STATUS_OK)
		{
			printf("dvpRegisterEventCallback OnEventStreamStarted failed with err:%d\r\n", status);
			break;
		}
		status = dvpRegisterEventCallback(h, OnEventStreamStoped, EVENT_STREAM_STOPPED, NULL);
		if (status != DVP_STATUS_OK)
		{
			printf("dvpRegisterEventCallback OnEventStreamStoped failed with err:%d\r\n", status);
			break;
		}
		status = dvpRegisterEventCallback(h, OnEventFrameLost, EVENT_FRAME_LOST, NULL);
		if (status != DVP_STATUS_OK)
		{
			printf("dvpRegisterEventCallback OnEventFrameLost failed with err:%d\r\n", status);
			break;
		}
		status = dvpRegisterEventCallback(h, OnEventFrameTimeout, EVENT_FRAME_TIMEOUT, NULL);
		if (status != DVP_STATUS_OK)
		{
			printf("dvpRegisterEventCallback OnEventFrameTimeout failed with err:%d\r\n", status);
			break;
		}
		status = dvpRegisterEventCallback(h, OnEventReconnected, EVENT_RECONNECTED, NULL);
		if (status != DVP_STATUS_OK)
		{
			printf("dvpRegisterEventCallback OnEventReconnected failed with err:%d\r\n", status);
			break;
		}
		status = dvpRegisterEventCallback(h, OnEventFrameStart, EVENT_FRAME_START, NULL);
		if (status != DVP_STATUS_OK)
		{
			printf("dvpRegisterEventCallback OnEventFrameStart failed with err:%d\r\n", status);
			break;
		}
		status = dvpRegisterEventCallback(h, OnEventFrameEnd, EVENT_FRAME_END, NULL);
		if (status != DVP_STATUS_OK)
		{
			printf("dvpRegisterEventCallback OnEventFrameEnd failed with err:%d\r\n", status);
			break;
		}

		/* 帧信息 */
		dvpFrame frame;
		/* 帧数据首地址，用户不需要申请释放内存 */
		void* p;

		/* 开始视频流 */
		status = dvpStart(h);
		if (status != DVP_STATUS_OK)
		{
			break;
		}

		/* 抓帧 */
		for (int j = 0; j < GRABCOUNT; j++)
		{
			/* 当前案例没有设置相机的曝光增益等参数，只展示在默认的ROI区域显示帧信息 */
			status = dvpGetFrame(h, &frame, &p, 3000);
			if (status != DVP_STATUS_OK)
			{
				if (trigMode)
				{
					printf("Fail to get a frame in trigger mode \r\n");
					continue;
				}
				else
				{
					printf("Fail to get a frame in continuous mode \r\n");
					continue;
				}
			}

			/* 显示帧数和帧率 */
			dvpFrameCount framecount;
			status = dvpGetFrameCount(h, &framecount);
			if (status != DVP_STATUS_OK)
			{
				printf("get framecount failed\n");
			}
			printf("framecount: %d, framerate: %f\n", framecount.uFrameCount, framecount.fFrameRate);

			/* 显示帧信息 */
			printf("%s, frame:%lld, timestamp:%lld, %d*%d, %dbytes, format:%d\r\n",
				name,
				frame.uFrameID,
				frame.uTimestamp,
				frame.iWidth,
				frame.iHeight,
				frame.uBytes,
				frame.format);
		}

		/* 停止视频流 */
		status = dvpStop(h);
		if (status != DVP_STATUS_OK)
		{
			break;
		}

	} while (0);

	status = dvpClose(h);

	printf("test quit, %s, status:%d\r\n", name, status);
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

	thread task(test, (void*)info[num].FriendlyName);线程对象（函数名称，参数传值）
	task.join();//main主线程要一直等待当前线程执行完后再一起退出

	system("pause");
	return 0;
}