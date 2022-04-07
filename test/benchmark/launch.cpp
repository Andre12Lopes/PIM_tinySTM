#include <dpu>
#include <iostream>

using namespace dpu;

int main(void)
{
	std::vector<std::vector<uint32_t>> nbCycles(1);
	std::vector<std::vector<uint32_t>> clocksPerSec(1);
	std::vector<std::vector<uint32_t>> nAborts(1);
    std::vector<std::vector<uint32_t>> nTransactions(1);
    std::vector<std::vector<uint32_t>> nThreads(1);

    std::vector<std::vector<uint32_t>> nbProcessCycles(1);
    std::vector<std::vector<uint32_t>> nbCommitCycles(1);
    std::vector<std::vector<uint32_t>> nbWastedCycles(1);

    try
    {
        auto dpu = DpuSet::allocate(1);

        dpu.load("../bank/bank");
        dpu.exec();

        nbCycles.front().resize(1);
    	dpu.copy(nbCycles, "nb_cycles");

        clocksPerSec.front().resize(1);
    	dpu.copy(clocksPerSec, "CLOCKS_PER_SEC");

    	nAborts.front().resize(1);
    	dpu.copy(nAborts, "n_aborts");

        nTransactions.front().resize(1);
        dpu.copy(nTransactions, "n_trans");

        nThreads.front().resize(1);
        dpu.copy(nThreads, "n_tasklets");

        nbProcessCycles.front().resize(1);
        dpu.copy(nbProcessCycles, "nb_process_cycles");

        nbCommitCycles.front().resize(1);
        dpu.copy(nbCommitCycles, "nb_commit_cycles");

        nbWastedCycles.front().resize(1);
        dpu.copy(nbWastedCycles, "nb_wasted_cycles");

        double time = (double) nbCycles.front().front() / clocksPerSec.front().front();

        double process_time = (double) nbProcessCycles.front().front() / clocksPerSec.front().front();
        double commit_time = (double) nbCommitCycles.front().front() / clocksPerSec.front().front();
        double wasted_time = (double) nbWastedCycles.front().front() / clocksPerSec.front().front();

        long aborts = nAborts.front().front();

        // std::cout << "N threads" <<
        std::cout << (double) nThreads.front().front() 
                  <<  "\t" << nTransactions.front().front() 
                  << "\t" << time << "\t" 
                  << ((double) aborts * 100) / (aborts + nTransactions.front().front()) 
                  << "\t" << process_time << "\t" << commit_time << "\t"
                  << wasted_time << std::endl;
        
        // std::cout << "Tx/s = " << (double)nTransactions.front().front() / time << std::endl;
        // std::cout << "Abort rate = " << ((double) nAborts.front().front() * 100) / nTransactions.front().front() << std::endl;

        // dpu.log(std::cout);
    }
    catch (const DpuError &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}