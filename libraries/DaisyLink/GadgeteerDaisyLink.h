/*
  GadgeteerDaisyLink.h - Gadgeteer DaisyLink Module library for Arduino
  Copyright (c) 2012 Valentin Ivanov.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef GadgeteerDaisyLink_h
#define GadgeteerDaisyLink_h

#include <inttypes.h>

class GadgeteerDaisyLink
{
	public:
	    uint8_t		_daisyLinkRegisters[8];
		uint8_t *	_deviceRegistes;
		uint8_t		_deviceRegistersSize;


		//static void (*user_onUpdateRegister)(int);
		static void onUpdateRegisterService(uint8_t);

		//static void (*user_onStatus)(int);
		static void onStatusService(uint8_t);

	public:
		GadgeteerDaisyLink();
		void initialize( uint8_t manufacturerCode, uint8_t moduleType, uint8_t moduleVersion, uint8_t volatile * deviceRegisters, uint8_t registerTableSize);
		void notifyMainBoard();
		void onUpdateRegister( void (*)(int) );
		void onStatus( void (*)(int) );
		void processRequests();
		void disablePullups();
		void enablePullups();
		void pullDownStream();
		void releaseDownStream();
};

extern GadgeteerDaisyLink DaisyLink;

#endif
