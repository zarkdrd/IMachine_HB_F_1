/*************************************************************************
    > File Name: src/IODevice.cpp
    > Author: ARTC
    > Descripttion:
    > Created Time: 2023-11-01
 ************************************************************************/
#include <iostream>
#include "IODevice.h"
#include "Log_Message.h"

/**TQ3568**/
static const char *TQ3568_GPIO_Out_Pin[11]={"5","15","18","20","102","101","97","90", "63", "65", "19"};
static const char *TQ3568_GPIO_In_Pin[11] ={"36","40","41","42","74","73","0","8", "4", "64", "148"};

Gpio::Gpio(int GpioPin, const char *Mode)
{
	isOpen = false;
	mode = Mode;
	Pin = GpioPin;
	SetOpen();
}

Gpio::~Gpio()
{
	Exit();
}

bool Gpio::SetOpen()
{
	if(isOpen == true){
		Exit();
	}
	if(SetPin(Pin) == false){
		return false;
	}

	string path_name = "/sys/class/gpio/gpio" + PinName;
	DIR *dir = opendir(path_name.c_str());
	if(dir == NULL){
		/**GPIO未初始化**/
		path_name = "/sys/class/gpio/export";
		int export_fd = open(path_name.c_str(), O_WRONLY);
		if(export_fd < 0){
			closedir(dir);
			log_message(ERROR, "Open gpio%s export: %s", PinName.c_str() ,strerror(errno));
			return false;
		}
		if(write(export_fd, PinName.c_str(), PinName.length()) < 0){
			closedir(dir);
			close(export_fd);
			log_message(ERROR, "write gpio%s export : %s", PinName.c_str() ,strerror(errno));
			return false;
		}
		close(export_fd);
	}
	closedir(dir);

	path_name = "/sys/class/gpio/gpio" + PinName + "/direction";
	int direction_fd = open(path_name.c_str(), O_RDWR);
	if(direction_fd < 0){
		SetClose();
		log_message(ERROR, "Open gpio%s direction : %s", PinName.c_str(), strerror(errno));
		return false;
	}
	if(write(direction_fd, mode.c_str(), mode.length()) < 0){
		close(direction_fd);
		SetClose();
		log_message(ERROR, "Write gpio%s direction : %s", PinName.c_str(), strerror(errno));
		return false;
	}
	close(direction_fd);

	path_name = "/sys/class/gpio/gpio" + PinName + "/value";
	fd = open(path_name.c_str(), O_RDWR);
	if(fd < 0){
		SetClose();
		log_message(ERROR, "Open gpio%s value : %s", PinName.c_str(), strerror(errno));
		return false;
	}
	isOpen = true;
	return true;
}

bool Gpio::SetClose()
{
    char path_name[64] = "/sys/class/gpio/unexport";
    int export_fd = open(path_name, O_WRONLY);
    if(export_fd < 0){
        printf("Open gpio%s unexport: %s\n", PinName.c_str(),strerror(errno));
        return -1;
    }

    if(write(export_fd, PinName.c_str(), PinName.length()) < 0){
        close(export_fd);
        printf("write gpio%s export : %s\n", PinName.c_str() ,strerror(errno));
        return -1;
    }
    close(export_fd);

	return true;
}

bool Gpio::SetOn()
{
	if(isOpen == false){
		log_message(ERROR, "IO未打开或者打开失败");
		return false;
	}

	if(write(fd, HIGH_LEVEL, strlen(HIGH_LEVEL)) < 0){
		log_message(ERROR, "设置IO低电平出错: %s", strerror(errno));
		return false;
	}
	return true;
}

int  Gpio::GetIOValue()
{
	if(isOpen == false){
		return -1;;
	}

    char buf[2] = {0};
    lseek(fd, 0, SEEK_SET);
    int ret = read(fd, buf, 1);
    if(ret < 0){
		log_message(ERROR, "read : %s", strerror(errno));
        return -1;
    }
    int value = atoi(buf);

    if(value == 0){
        return 0;
    }else{
        return 1;
    }
}

bool Gpio::SetOff()
{
	if(isOpen == false){
		log_message(ERROR, "IO未打开或者打开失败");
		return false;
	}

	if(write(fd, LOW_LEVEL, strlen(LOW_LEVEL)) < 0){
		log_message(ERROR, "设置IO高电平出错: %s", strerror(errno));
		return false;
	}
	return true;
}

bool Gpio::Exit()
{
	close(fd);
	SetClose();
	isOpen = false;
	return true;
}

bool Gpio::SetPin(int GpioPin)
{
	if((GpioPin < 1) || (GpioPin > 11)){
		log_message(ERROR, "IO引脚编号为 [1~11], 不支持 [%d] ", GpioPin);
		return false;
	}
	if(mode == OUT){
		PinName = TQ3568_GPIO_Out_Pin[GpioPin - 1];
	}else if(mode == IN){
		PinName = TQ3568_GPIO_In_Pin[GpioPin - 1];
	}else{
		log_message(INFO, "设置GPIO [%s] 模式错误", mode.c_str());
		return false;
	}

	return true;
}
