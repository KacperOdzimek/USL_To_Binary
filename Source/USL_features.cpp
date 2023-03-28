#include "USL_features.h"

#include "../Standards/S0/S0.h"

#define Temp Version::AccessTemp()

Version* CreateVersion(unsigned int version)
{
	Version* V;
	switch (version)
	{
	case 0: 
		V = Standards::S0Create();
		  break;
	default:
		V = nullptr; 
	}
	return V;
}

#undef Temp