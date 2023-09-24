all:
	g++-12 cache_simulate.cpp -o cache_simulate

run:
	./cache_simulate 64 1024 2 65536 8 memory_trace_files/trace1.txt