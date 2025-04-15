# Replicate some results for DCTCP paper

On Debian 12, install `libns3-dev` using:

```
apt install libns3-dev
```

Build with:

```
g++ tcp-dctcp-queue-size.cc $( pkg-config --libs
    ns3-point-to-point ns3-internet ns3-applications 
    ns3-point-to-point-layout )
```




