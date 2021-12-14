# On pi enable serial, i2c, camera, vnc with raspi-config
# Also enable opengl: "7. Advanced Options" – "A8 GL Driver" – "G2 GL (Fake KMS)"

# Increase swap to 2GB by changing the file below to CONF_SWAPSIZE=2048:
sudo vi /etc/dphys-swapfile
sudo /etc/init.d/dphys-swapfile restart swapon -s

# free up some space
sudo apt-get purge wolfram-engine
sudo apt-get purge libreoffice
sudo apt-get clean
sudo apt-get autoremove

# install updates
sudo apt-get update && sudo apt-get upgrade

# setup git
git config --global user.email "dgorissen@gmail.com"
git config --global user.name "Dirk Gorissen"

# get std packages
sudo apt install -y vim keychain imagemagick feh screen software-properties-common dos2unix

# install docker
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker dgorissen

# allow serial access
sudo usermod -a -G dialout dgorissen
# allow audio
sudo usermod -a -G audio dgorissen
# allow video
sudo usermod -a -G video dgorissen
# allow i2c
sudo usermod -a -G i2c dgorissen
sudo usermod -a -G gpio dgorissen
sudo usermod -a -G input dgorissen

# add this to bashrc
eval $(keychain --noask --eval id_rsa)

# if docker img does not build on the pi because of key problems with apt, see
# https://askubuntu.com/questions/1263284/apt-update-throws-signature-error-in-ubuntu-20-04-container-on-arm


# Setup piwatcher (https://www.omzlo.com/articles/the-piwatcher)
wget -N http://omzlo.com/downloads/piwatcher
chmod a+x piwatcher
sudo mv piwatcher /usr/local/bin/

cat > /home/dgorissen/piwatcher.sh << EOL
#!/bin/bash
/usr/local/bin/piwatcher watch 30
while true;
do
    /usr/local/bin/piwatcher status >> /dev/null
    sleep 15
done
EOL

chmod a+x /home/dgorissen/piwatcher.sh

cat > piwatcher.service << EOL
[Unit]
Description=PiWatcher Service
StartLimitIntervalSec=0

[Service]
Type=simple
Restart=always
RestartSec=1
User=pi
ExecStart=/home/dgorissen/piwatcher.sh

[Install]
WantedBy=multi-user.target
EOL

sudo mv piwatcher.service /etc/systemd/system/

sudo systemctl enable piwatcher
sudo systemctl start piwatcher

# enable 4inch hdmi monitor (https://www.waveshare.com/wiki/4inch_HDMI_LCD)
sudo apt-get install xinput-calibrator 

# Add to /config/boot.txt
hdmi_drive:0=1
hdmi_force_hotplug:0=1
hdmi_group:0=2
hdmi_mode:0=1
hdmi_mode:0=87
hdmi_timings:0=480 0 40 10 80 800 0 13 3 32 0 0 0 60 0 32000000 3
display_hdmi_rotate:0=3

hdmi_drive:1=1
hdmi_force_hotplug:1=1
hdmi_group:1=2
hdmi_mode:1=1
hdmi_mode:1=87
hdmi_timings:1=480 0 40 10 80 800 0 13 3 32 0 0 0 60 0 32000000 3
display_hdmi_rotate:1=3

# TODO: even with the above only output 0 (closest to usbC port) works for some reason

dtoverlay=ads7846,cs=1,penirq=25,penirq_pull=2,speed=50000,keep_vref_on=0,swapxy=0,pmax=255,xohms=150,xmin=200,xmax=3900,ymin=200,ymax=3900

#Note: For Raspberry Pi 4, you need to comment out dtoverlay=vc4-fkms-V3D.
#dtoverlay=vc4-fkms-V3D.

git clone https://github.com/waveshare/LCD-show.git
cd LCD-show/
chmod +x LCD4-800x480-show
./LCD4-800x480-show

# Add support for serial hat (https://github.com/sbcshop/SB-Serial-Expansion-Hat)
sudo apt-get install python-dev python-smbus python-spidev

# Add to /config/boot.txt
dtoverlay=sc16is752-i2c,int_pin=24,addr=0x48
# addr is different according to status of A0/A1, default 0X48

# Setup realsense (https://github.com/acrobotic/Ai_Demos_RPi/wiki/Raspberry-Pi-4-and-Intel-RealSense-D435)
cd ~
git clone https://github.com/IntelRealSense/librealsense.git
cd librealsense
sudo cp config/99-realsense-libusb.rules /etc/udev/rules.d/
sudo su
udevadm control --reload-rules && udevadm trigger
exit


# adafruit sound bonnet: 
curl -sS https://raw.githubusercontent.com/adafruit/Raspberry-Pi-Installer-Scripts/master/i2samp.sh | bash
# list devices:
aplay -l
# Record from respeaker
arecord -D plughw:3,0 -f cd a.wav
# Play through bonnet
aplay --device plughw:2,0 a.wav

apt get install x11-apps

# install respeaker rules so we can access without being root
dgorissen@cbrain:~/arduino/clare $ more 60-respeaker.rules
SUBSYSTEM=="usb", ATTR{idProduct}=="0018", ATTR{idVendor}=="2886", MODE:="0666"
sudo systemctl restart udev

# Also ensure the realsense and neural compute stick udev rules are installed


# Test neural compute stick
~/openvino/build_samples/armv7l/Release/object_detection_sample_ssd -m /home/dgorissen/open_model_zoo/tools/downloader/intel/face-detection-adas-0001/FP16/face-detection-adas-0001.xml -d MYRIAD -i ~/clare/school.jpg

# nmap
RUN apt-get install -y libinput-tools