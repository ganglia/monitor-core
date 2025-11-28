#!/bin/bash

# 1. Copy the service file
sudo cp gmond.service /etc/systemd/system/

# 2. Create the sysconfig directory and copy the config
sudo mkdir -p /usr/local/etc/sysconfig
sudo cp gmond.sysconfig /usr/local/etc/sysconfig/gmond

# 3. Create the ganglia config directory if it doesn't exist
sudo mkdir -p /usr/local/etc/ganglia

# 4. Generate a default gmond.conf if you don't have one
/usr/local/sbin/gmond -t | sudo tee /usr/local/etc/ganglia/gmond.conf

# 5.  Reload systemd and enable the service
sudo systemctl daemon-reload
sudo systemctl enable gmond
sudo systemctl start gmond

# 6. Check status
sudo systemctl status gmond

