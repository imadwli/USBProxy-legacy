/*
 * Copyright 2013 Dominic Spill
 * Copyright 2013 Adam Stasiak
 *
 * This file is part of USB-MitM.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 *
 * USBInterface.cpp
 *
 * Created on: Nov 6, 2013
 */

#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include "USBInterface.h"

//TODO: 9 update active interface in interfacegroup upon set interface request
//TODO: 9 update active endpoints in proxied device upon set interface request
//TODO: 9 handle any endpoints that become inactive upon set interface request

USBInterface::USBInterface(__u8** p,__u8* e) {
	device=NULL;
	hid_descriptor=NULL;

	memcpy(&descriptor,*p,9);
	*p=*p+9;
	endpoints=(USBEndpoint**)calloc(descriptor.bNumEndpoints,sizeof(*endpoints));
	USBEndpoint** ep=endpoints;
	while (*p<e && (*(*p+1))!=4) {
		switch (*(*p+1)) {
			case 5:
				*(ep++)=new USBEndpoint(*p);
				break;
			case 0x21:
				hid_descriptor=new USBHID(*p);
				break;
			default:
				int i;
				fprintf(stderr,"Unknown Descriptor:");
				for(i=0;i<**p;i++) {fprintf(stderr," %02x",(*p)[i]);}
				fprintf(stderr,"\n");
				break;
		}
		*p=*p+**p;
	}
}

USBInterface::USBInterface(usb_interface_descriptor* _descriptor) {
	device=NULL;
	hid_descriptor=NULL;

	descriptor=*_descriptor;
	endpoints=(USBEndpoint**)calloc(descriptor.bNumEndpoints,sizeof(*endpoints));
}

USBInterface::USBInterface(__u8 bInterfaceNumber,__u8 bAlternateSetting,__u8 bNumEndpoints,__u8 bInterfaceClass,__u8 bInterfaceSubClass,__u8 bInterfaceProtocol,__u8 iInterface) {
	device=NULL;
	hid_descriptor=NULL;

	descriptor.bInterfaceNumber=bInterfaceNumber;
	descriptor.bAlternateSetting=bAlternateSetting;
	descriptor.bNumEndpoints=bNumEndpoints;
	descriptor.bInterfaceClass=bInterfaceClass;
	descriptor.bInterfaceSubClass=bInterfaceSubClass;
	descriptor.bInterfaceProtocol=bInterfaceProtocol;
	descriptor.iInterface=iInterface;
	endpoints=(USBEndpoint**)calloc(descriptor.bNumEndpoints,sizeof(*endpoints));
}

USBInterface::~USBInterface() {
	int i;
	for(i=0;i<descriptor.bNumEndpoints;i++) {delete(endpoints[i]);}
	free(endpoints);
	if (hid_descriptor) {delete(hid_descriptor);}
}

const usb_interface_descriptor* USBInterface::get_descriptor() {
	return &descriptor;
}

size_t USBInterface::get_full_descriptor_length() {
	size_t total=descriptor.bLength;
	if (hid_descriptor) {total+=hid_descriptor->get_full_descriptor_length();}
	int i;
	for(i=0;i<descriptor.bNumEndpoints;i++) {total+=endpoints[i]->get_full_descriptor_length();}
	return total;
}

void USBInterface::get_full_descriptor(__u8** p) {
	memcpy(*p,&descriptor,descriptor.bLength);
	*p=*p+descriptor.bLength;
	if (hid_descriptor) {hid_descriptor->get_full_descriptor(p);}
	int i;
	for(i=0;i<descriptor.bNumEndpoints;i++) {endpoints[i]->get_full_descriptor(p);}
}

void USBInterface::add_endpoint(USBEndpoint* endpoint) {
	int i;
	for(i=0;i<descriptor.bNumEndpoints;i++) {
		if (!endpoints[i]) {
			endpoints[i]=endpoint;
			break;
		} else {
			if (endpoints[i]->get_descriptor()->bEndpointAddress==endpoint->get_descriptor()->bEndpointAddress) {
				delete(endpoints[i]);
				endpoints[i]=endpoint;
				break;
			}
		}
	}
	fprintf(stderr,"Ran out of endpoint storage space on interface %d.",descriptor.bInterfaceNumber);
}

USBEndpoint* USBInterface::get_endpoint_by_idx(__u8 index) {
	return endpoints[index];
}

USBEndpoint* USBInterface::get_endpoint_by_address(__u8 address) {
	int i;
	for(i=0;i<descriptor.bNumEndpoints;i++) {
		if (endpoints[i]->get_descriptor()->bEndpointAddress==address) {return endpoints[i];}
	}
	return NULL;
}

__u8 USBInterface::get_endpoint_count() {
	return descriptor.bNumEndpoints;
}

void USBInterface::print(__u8 tabs,bool active) {
	unsigned int i;
	for(i=0;i<tabs;i++) {putchar('\t');}
	if (active) {putchar('*');}
	printf("Alt(%d):",descriptor.bAlternateSetting);
	for(i=0;i<sizeof(descriptor);i++) {printf(" %02x",((__u8*)&descriptor)[i]);}
	putchar('\n');
	if (descriptor.iInterface) {
		USBString* s=get_interface_string();
		if (s) {
			for(i=0;i<tabs;i++) {putchar('\t');}
			printf("  Name: ");
			s->print_ascii(stdout);
			putchar('\n');
		}
	}
	if (hid_descriptor) {hid_descriptor->print(tabs+1);}
	for(i=0;i<descriptor.bNumEndpoints;i++) {
		if (endpoints[i]) {endpoints[i]->print(tabs+1);}
	}
}

void USBInterface::set_usb_device(USBDevice* _device) {device=_device;}

USBString* USBInterface::get_interface_string(__u16 languageId) {
	if (!device||!descriptor.iInterface) {return NULL;}
	return device->get_string(descriptor.iInterface,languageId);
}

