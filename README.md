# indi-astroberry-mqtt
MQTT client for INDI

# Building
This assumes you already have INDI installed.

First, let's install some build tools, if you haven't already.
```
sudo apt install build-essential cmake
```

You must install INDI and Mosquitto development libraries.
```
sudo apt install libindi-dev libmosquitto-dev
```

Okay, we're ready to get the source and build it.
```
git clone https://github.com/rkaczorek/astroberry-mqtt.git
cd astroberry-mqtt
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make
sudo make install
```

# Running
Start your INDI server
Start astroberry-mqtt INDI client

Now you can monitor and control your INDI devices with MQTT!
