#ifndef  TDC_DEV_LIST_H
#define TDC_DEV_LIST_H

#ifdef _WIN32
#pragma once
#endif

#define CREATE_DEV_LIST(name_, mask_) \
uint64 name_[] = \
{ \
	76561198053356818 ^ mask_, /* Nicknine */		\
	76561198023936575 ^ mask_, /* sigsegv */		\
	76561198005690007 ^ mask_, /* OneFourth */		\
	76561198001171456 ^ mask_, /* Game Zombie */	\
	76561197995805528 ^ mask_, /* th13teen */		\
	76561198033547232 ^ mask_, /* Maxxy */			\
	76561197999442625 ^ mask_, /* Stev */			\
	76561198004108258 ^ mask_, /* NassimO */		\
	76561198038157852 ^ mask_, /* EonDynamo */		\
	76561198057735456 ^ mask_, /* Pretz */			\
	76561198101094232 ^ mask_, /* Momo */			\
	76561198005557902 ^ mask_, /* Insaneicide */	\
	76561198009908416 ^ mask_, /* Nabernizer */		\
	76561198010246458 ^ mask_, /* H.Gaspar */		\
	76561198024559578 ^ mask_, /* Paysus */			\
	76561198055668916 ^ mask_, /* Gruppy */			\
	76561198062002340 ^ mask_, /* nonhuman */		\
	76561198102368084 ^ mask_, /* Graham */			\
	76561198033171144 ^ mask_, /* Agent Agrimar */	\
};

#endif // ! TDC_DEV_LIST_H
