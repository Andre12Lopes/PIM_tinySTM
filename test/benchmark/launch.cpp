#include <dpu>
#include <iostream>

using namespace dpu;

int main(void)
{
	std::vector<std::vector<uint32_t>> nbCycles(1);
	std::vector<std::vector<uint32_t>> clocksPerSec(1);

    try
    {
        auto dpu = DpuSet::allocate(1);

        dpu.load("../bank/bank");
        dpu.exec();

        nbCycles.front().resize(1);
    	dpu.copy(nbCycles, "nb_cycles");

        clocksPerSec.front().resize(1);
    	dpu.copy(clocksPerSec, "CLOCKS_PER_SEC");

        std::cout << (double)nbCycles.front().front() / clocksPerSec.front().front() << " secs." << std::endl;

        dpu.log(std::cout);
    }
    catch (const DpuError &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}