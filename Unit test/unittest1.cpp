#include "stdafx.h"
#include "CppUnitTest.h"
#include "../Chimera/cnpy.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Unittest
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			// TODO: Your test code here
			cnpy::NpyArray arr = cnpy::npy_load(MASKS_FILE_LOCATION);
			//std::vector<std::vector<long>> masks = arr.data<std::vector<std::vector<long>>>();
			std::complex<double>* loaded_data = arr.data<std::complex<double>>();
		}

	};
}