import statistics
import sys

def read_file(name):
	f = open(name, "r")

	lines = []
	for line in f.readlines():
		lines.append(line.rstrip().split("\t"))
	f.close()
	
	return lines[1:]


def process(file, num_threads, num_runs):
	data = read_file(file)

	time = []
	aborts = []
	for i in range(num_threads):
		time.append([])
		aborts.append([])

	for run in data:
		time[int(run[0]) - 1].append(float(run[2]))
		aborts[int(run[0]) - 1].append(float(run[3]))

	f = open(f'{file}_processed', 'w')
	f.write(f'N_THREADS\tN_TRANSACTIONS\tAVG_TIME\tSTDEV_TIME\tAVG_ABORTS\tSTDEV_ABORTS\n')
	for i in range(num_threads):
		f.write(f'{i + 1}\t{data[i * num_runs][1]}\t{statistics.mean(time[i])}\t{statistics.stdev(time[i])}\t{statistics.mean(aborts[i])}\t{statistics.stdev(aborts[i])}\n')
	f.close()

if __name__ == "__main__":
	process(sys.argv[1], int(sys.argv[2]), int(sys.argv[3]))