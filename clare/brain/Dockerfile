FROM ros:noetic

SHELL ["/bin/bash", "-c"]

# Ros packages
RUN apt-get update && apt-get install -y \
      ros-${ROS_DISTRO}-desktop \
      python3-rosdep \
      python3-rosinstall \
      python3-rosinstall-generator \
      python3-wstool build-essential

# Utils
RUN apt-get install -y wget vim keychain tree screen git software-properties-common feh

# x2go
RUN add-apt-repository ppa:x2go/stable && \
  apt-get install -y openssh-server x2goserver x2goserver-xsession
  
# XFCE
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y xfce4

#  Openvino dependencies
RUN apt-get install -y build-essential \
   unzip pkg-config libjpeg-dev libpng-dev \
   libtiff-dev libavcodec-dev libavformat-dev \
   libswscale-dev libv4l-dev libxvidcore-dev \
   libx264-dev libgtk-3-dev libcanberra-gtk* \
   libatlas-base-dev gfortran python3-dev 

# upgrade cmake to avoid crosscompile bug
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null && \
  apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main' && \
  apt-get install -y cmake cmake-curses-gui

# nodejs & vue
RUN curl -fsSL https://deb.nodesource.com/setup_15.x | sudo -E bash - && \
  apt-get install -y nodejs && \
  npm install -g @vue/cli

# other dependencies
RUN apt-get update && apt-get install -y ros-noetic-rosserial-python \
  usbutils pulseaudio-utils lame mpg123 audacity libfftw3-dev libconfig-dev libasound2-dev portaudio19-dev \
  libprotobuf-dev protobuf-compiler libtbb-dev ros-noetic-realsense2-camera libusb-1.0-0-dev \
  alsa-utils i2c-tools \
  ros-noetic-imu-filter-madgwick ros-noetic-rtabmap-ros ros-noetic-robot-localization pcl-tools

# create user
RUN groupadd -r -g 1001 dgorissen && useradd -ms /bin/bash -u 1001 -g 1001 dgorissen && \
  usermod -a -G users dgorissen && \
  usermod -a -G dialout dgorissen && \
  usermod -a -G audio dgorissen && \
  usermod -a -G video dgorissen && \
  usermod -a -G i2c dgorissen && \
  usermod -a -G sudo dgorissen
RUN echo 'dgorissen:test' | chpasswd

USER dgorissen
WORKDIR /home/dgorissen

# Update rosdep
RUN rosdep update

# Set-up necessary Env vars for PyEnv
ENV PYTHON_VERSION 3.7.10
ENV PYENV_ROOT /home/dgorissen/.pyenv
ENV PATH $PYENV_ROOT/shims:$PYENV_ROOT/bin:$PATH

# Install pyenv
RUN set -ex \
    && curl https://pyenv.run | bash \
    && pyenv update \
    && pyenv install $PYTHON_VERSION \
    && pyenv global $PYTHON_VERSION \
    && pyenv rehash

RUN set -ex \
  && pyenv version \
  && python --version \
  && which python \
  && which pip

RUN echo -e '\n### \n\
xhost +\n\
export PYENV_ROOT="$HOME/.pyenv" \n\
export PATH="$PYENV_ROOT/bin:$PATH" \n\
eval "$(pyenv init --path)" \n\
' >> ~/.bashrc

RUN pip install wheel
RUN READTHEDOCS=True pip install \
  picamera imutils numpy flask flask_cors pyyaml rospkg pyserial platformio netifaces

# Install openvino
ARG vinodist=ubuntu20
# ARG vinodist=raspbian
ARG vinofile=l_openvino_toolkit_runtime_${vinodist}_p_2021.4.582

RUN wget https://storage.openvinotoolkit.org/repositories/openvino/packages/2021.4/${vinofile}.tgz && \
  tar -xf ${vinofile}.tgz && \
  mv ${vinofile} openvino

RUN cd openvino && \
  source bin/setupvars.sh && \
  mkdir build_samples && cd build_samples && \
  cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-march=armv7-a" ~/openvino/deployment_tools/inference_engine/samples/cpp && \
  make -j 2

# Install model zoo downloader
RUN git clone --depth 1 https://github.com/openvinotoolkit/open_model_zoo && \
  cd open_model_zoo/tools/downloader && \
  pip install -r requirements.in && \
  python3 downloader.py --name face-detection-adas-0001

# Respeaker & ODAS
RUN pip install pyusb click pyaudio rpi.gpio

RUN git clone https://github.com/respeaker/usb_4_mic_array.git && \
  git clone --depth 1 https://github.com/respeaker/pixel_ring.git && \
  cd pixel_ring && \
  pip install -e . && \
  git clone https://github.com/introlab/odas.git && \
  mkdir odas/build && \
  cd odas/build && \
  cmake .. && \
  make -j 2

# Setup realsense (https://github.com/acrobotic/Ai_Demos_RPi/wiki/Raspberry-Pi-4-and-Intel-RealSense-D435)
ENV APRIL_VERSION=3.1.4
RUN wget https://github.com/AprilRobotics/apriltag/archive/refs/tags/v$APRIL_VERSION.tar.gz && \
  tar -xzf v$APRIL_VERSION.tar.gz && \
  mkdir apriltag-$APRIL_VERSION/build && \
  cd apriltag-$APRIL_VERSION/build && \
  # https://github.com/AprilRobotics/apriltag/issues/107
  cmake -DCMAKE_INSTALL_PREFIX=~/apriltag/ -DBUILD_PYTHON_WRAPPER=OFF .. && \
  make install

RUN git clone --depth 1 --branch v2.48.0 https://github.com/IntelRealSense/librealsense.git && \
  cd librealsense && \
  mkdir build  && cd build && \
  cmake .. \
  -DBUILD_UNIT_TESTS=OFF \
  -DBUILD_EXAMPLES=true \
  -DCMAKE_BUILD_TYPE=Release \
  -DFORCE_RSUSB_BACKEND=true \
  # -DBUILD_OPENVINO_EXAMPLES=ON \
  # -DINTEL_OPENVINO_DIR=/home/dgorissen/openvino \
  # -DOpenCV_DIR=/home/dgorissen/openvino/opencv \
  # -DCMAKE_CXX_FLAGS=-isystem\ /home/dgorissen/openvino/inference_engine \
  -DCMAKE_PREFIX_PATH=~/apriltag && \
  make -j2

RUN cd librealsense/build && \
  cmake .. \
  -DBUILD_PYTHON_BINDINGS=ON \
  -DPYTHON_EXECUTABLE=$(which python3) && \
  make -j2

USER root
RUN cd /home/dgorissen/librealsense/build && make install
USER dgorissen

RUN pip install opengl pyopengl pyopengl_accelerate

RUN echo -e '\n### \n\
source ~/openvino/bin/setupvars.sh \n\
source /opt/ros/noetic/setup.bash \n\
' >> ~/.bashrc

RUN mkdir ~/clare
WORKDIR /home/dgorissen/clare

EXPOSE 5000
EXPOSE 8080
EXPOSE 11311

CMD ["/home/dgorissen/clare/brain/entrypoint.sh"]

# libssl-dev mesa-utils nmap