/** \file led_bar.c
 *  \brief Functions for digital input/output grove devices 
*/
/**
\addtogroup Grove Addon  Library
@{
*/
/* **************************************************************************																					
 *                                OpenPicus                 www.openpicus.com
 *                                                            italian concept
 * 
 *            openSource wireless Platform for sensors and Internet of Things	
 * **************************************************************************
 *  FileName:        led_bar.c
 *  Module:          FlyPort WI-FI - FlyPort ETH
 *  Compiler:        Microchip C30 v3.12 or higher
 *
 *  Author               Rev.    Date              Comment
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  Vicca Davide	     1.0     01/07/2012		   First release  
 *  
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  Software License Agreement
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  This is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License (version 2) as published by 
 *  the Free Software Foundation AND MODIFIED BY OpenPicus team.
 *  
 *  ***NOTE*** The exception to the GPL is included to allow you to distribute
 *  a combined work that includes OpenPicus code without being obliged to 
 *  provide the source code for proprietary components outside of the OpenPicus
 *  code. 
 *  OpenPicus software is distributed in the hope that it will be useful, but 
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details. 
 * 
 * 
 * Warranty
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * THE SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION, ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * WE ARE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF
 * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS
 * BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE
 * THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER
 * SIMILAR COSTS, WHETHER ASSERTED ON THE BASIS OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE), BREACH OF WARRANTY, OR OTHERWISE.
 *
 **************************************************************************/

#include "taskFlyport.h"
#include "grovelib.h"
#include "led_bar.h"

struct Interface *attachSensorToDigioBus(void *,int,int);

/**
 * struct LedBar - The structure for LedBar grove device class 
 */
struct LedBar
{
	const void *class;
	struct Interface *inter;
	BYTE bar_number;
	int *led_bar_start;
	int *led_bar_ptr;
};

/**
 * static void program(struct LedBar *self,BYTE state) -This function programs the Led Bar device  
 * \param *_self - pointer to the LedBar grove device class.
 * \param BYTE state - The state of Led Bar device.
 *			-	ON (The device will be turning on)
 *			-	OFF (The device will be turning off)
 * \return - None
*/
static void program(struct LedBar *self,BYTE state)
{
	int i,j,k;
	unsigned int data;

	vTaskSuspendAll();
	for(k =0;k<self->bar_number;k++)
	{
		data = COMMAND_DATA;
		//send command data
		for(i=0;i<16;i++)
		{
			if(data & 0x8000)
				IOPut(self->inter->port->Pin1,ON);
			else
				IOPut(self->inter->port->Pin1,OFF);
	 
			IOPut(self->inter->port->Pin2,toggle);
			data <<= 1;
		}

		//send led data
		if(state == OFF)
		{	
			IOPut(self->inter->port->Pin1,OFF);
			for(j=0;j<10;j++)
			{
				for(i=0;i<16;i++)
					IOPut(self->inter->port->Pin2,toggle);
			}
		}
		else
		{
			for(j=0;j<10;j++)
			{
				data = *self->led_bar_ptr;
				for(i=0;i<16;i++)
				{
					if(data & 0x8000)
						IOPut(self->inter->port->Pin1,ON);
					else
						IOPut(self->inter->port->Pin1,OFF);
					IOPut(self->inter->port->Pin2,toggle);
					data <<= 1;
				}
				self->led_bar_ptr++;
			}
		}
	
		//bit from 31°to 0° to be cleared;
		IOPut(self->inter->port->Pin1,OFF);
		for(j=0;j<2;j++)
		{
			for(i=0;i<16;i++)
				IOPut(self->inter->port->Pin2,toggle);
		}
	}
	//internal latch control
	IOPut(self->inter->port->Pin1,OFF);
	Delay10us(30);	
	for(i=0;i<8;i++)
	{
		IOPut(self->inter->port->Pin1,Toggle);
		Delay10us(40);	
	}
	xTaskResumeAll();
	self->led_bar_ptr = self->led_bar_start;//point at the beginning of the matrix		
}



/**
 * static void *default_config(struct LedBar *self) - Default Configuration  
 * \param *_self - pointer to the LedBar grove device class.
* \return - None
*/
static void default_config(struct LedBar *self)
{	
	int j;
	self->led_bar_start = self->led_bar_ptr;	
	for(j = 0 ; j < (10*self->bar_number); j++) 
	{
		*self->led_bar_ptr = 0;//turn off all leds in the chain
		self->led_bar_ptr++;
	}
	self->led_bar_ptr = self->led_bar_start;		
}


/**
 * static void *LedBar_ctor (void * _self, va_list *app) - LedBar grove device Constructor  
 * \param *_self - pointer to the LedBar grove device class.
 * \param *app - LedBar chain's led number. 
* \return - Pointer to the LedBar device instantiated
*/
static void *LedBar_ctor (void * _self, va_list *app)
{
	struct LedBar *self = _self;
	self->bar_number = va_arg(*app, const BYTE);//led bar number in the chain
	self->led_bar_ptr =(int *)calloc (10*self->bar_number, sizeof (int)); 
	if(!self->led_bar_ptr)
	/* memory allocation unsuccessful*/
		return NULL;
	/*Configure Led Bar device with default configure*/
	default_config(self);
	self->inter = NULL;
	return self;
}	

/**
 * static void LedBar_dtor (void * _sensor) - LedBar grove device Destructor  
 * \param *_sensor - pointer to the LedBar grove device class.
 * \return - None
*/
static void LedBar_dtor (void * _sensor)
{
	struct LedBar *sensor = _sensor;
	/***the memory allocated is freed****/
	free(sensor->led_bar_start); 
}	


/**
 * static void *LedBar_attach (void * _board,void *_sensor,int n) - attach a LedBar grove device to the GroveNest digital I/O port  
 * \param *_board - pointer to the GroveNest 
 * \param *_sensor - pointer to the LedBar grove device class.
 * \param n - port number which digital device is connected to
 * \return 
 <UL>
	<LI><Breturn = Pointer to the digital interface created:</B> the operation was successful.</LI> 
	<LI><B>return = NULL:</B> the operation was unsuccessful.</LI> 
 </UL>
 */
static void *LedBar_attach (void * _board,void *_sensor,int n)
{
	struct LedBar *sensor = _sensor;
	sensor->inter = attachSensorToDigioBus(_board,n,5);//set 2 pins as output	
	if(!sensor->inter)
	/***There's no anymore memory free****/ 
		return NULL;
	/****Configure the pins for Led Bar device operation***/
	IOPut(sensor->inter->port->Pin2,OFF);
	IOPut(sensor->inter->port->Pin1,OFF);
	program(sensor,OFF);//to turn off all leds in the chain
	return sensor->inter;
}	



/**
 * static int LedBar_config (void * _self, va_list *app) - Configures the LedBar grove device to the color configuration choosen  
 * \param *_self - pointer to the LedBar grove device class.
 * \param *app - State of LedBar in the chain
 * \param *app - number of bar in the LedBar chain
 * \param *app - number of led in the LedBar device
 * \param *app - led grayscale
 * \return - the operation is always successful
 */
static int LedBar_config (void * _self, va_list *app)
{
	struct LedBar *self = _self;
	BYTE state = va_arg(*app, BYTE);//type of function
	BYTE bar_numb = va_arg(*app, BYTE);//bar number
	if(state)
	{ //all the led bar will be set
		BYTE level = va_arg(*app, BYTE);
		BYTE direction = va_arg(*app, BYTE);
		int led_num = 10;
		int i =89;
		for(;i >0;i -=10)
		{
			if((level - i)>0)
			{
				level = level -i;
				break;
			}
			led_num--;
		}
		if(direction) 
			level = 255 - (level *25);
		else
			level = level *25;
		self->led_bar_ptr = self->led_bar_start +(10*self->bar_number - led_num -(bar_numb - 1)*10);
		*self->led_bar_ptr = level;
		i = 1;
		while((led_num +i)<=10)
		{
			self->led_bar_ptr = self->led_bar_start +(10*self->bar_number - (led_num +i) -(bar_numb - 1)*10);
			if(direction)
				*self->led_bar_ptr = 0xFF;
			else
				*self->led_bar_ptr = 0x00;
			i++;
		}
		i = 1;
		while((led_num -i) > 0 )
		{
			self->led_bar_ptr = self->led_bar_start +(10*self->bar_number - (led_num -i) -(bar_numb - 1)*10);
			if(direction)
				*self->led_bar_ptr = 0x00;
			else
				*self->led_bar_ptr = 0xFF;
			i++;
		}
	}	
	else
	{ //turn on just a led 
		BYTE led = va_arg(*app, BYTE);//led number
		BYTE led_grayscale = va_arg(*app, BYTE);//led grayscale
		self->led_bar_ptr = self->led_bar_start +(10*self->bar_number - led -(bar_numb - 1)*10);
		*self->led_bar_ptr = led_grayscale;
	}
	return 0;
}

/**
 * static int LedBar_set(void * _self, va_list *app) -  Sets ON/OFF the LedBar chain previuosly configured.
 * \param *_self - pointer to the device 
 * \param *app - LedBar state  
  <UL>
	<LI><B ON :</B> turn the LedBar led chain on.</LI> 
	<LI><B OFF :</B> turn the LedBar led chain off.</LI> 
 </UL>
 * \return operation is always successful:
 */
static int LedBar_set(void * _self, va_list *app)
{
	struct LedBar *self = _self;
	self->led_bar_ptr = self->led_bar_start;		
	int state =  va_arg(*app, int);
	program(self,state);
	return 0;
}	

static const struct SensorClass _LedBar =
{	
	sizeof(struct LedBar),
	LedBar_ctor,
	LedBar_dtor,
	LedBar_attach,
	LedBar_config,
	LedBar_set,
	0,
};

const void *LedBar = &_LedBar;


