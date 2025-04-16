# Replicate some results for DCTCP paper

On Debian 12, install `libns3-dev` using:

```bash
apt install libns3-dev
```

All the code we wrote is in C++ or python, the following were the versions used for reproduction:

```bash
$  g++ --version
g++ (Debian 12.2.0-14) 12.2.0
Copyright (C) 2022 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

$  python3 --version
Python 3.11.2

```



Build and run with `build.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

g++ figure_1_tcp_dctcp_queue_size.cc $(pkg-config --libs ns3-point-to-point \
	ns3-internet ns3-applications ns3-point-to-point-layout) -o \
	figure_1_tcp_dctcp_queue_size


g++ figure_13_tcp_dctcp_cdf.cc $(pkg-config --libs ns3-point-to-point \
	ns3-internet ns3-applications ns3-point-to-point-layout) -o \
	figure_13_tcp_dctcp_cdf


echo "Creating data for figure 1"

./figure_1_tcp_dctcp_queue_size --output=fig_1_tcp_result.dat
./figure_1_tcp_dctcp_queue_size --useDctcp --output=fig_1_dctcp_result.dat

echo "Creating data for figure 13"

./figure_13_tcp_dctcp_cdf --output=figure_13_flows_2_tcp.dat --numFlows=2

./figure_13_tcp_dctcp_cdf --output=figure_13_flows_2_dctcp.dat --numFlows=2 \
	--useDctcp

./figure_13_tcp_dctcp_cdf --output=figure_13_flows_20_tcp.dat --numFlows=20

./figure_13_tcp_dctcp_cdf --output=figure_13_flows_20_dctcp.dat --numFlows=20 \
	--useDctcp

python3 plot.py

```




