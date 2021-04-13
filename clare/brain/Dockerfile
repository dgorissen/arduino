FROM ros:noetic

# install ros package
RUN apt-get update && apt-get install -y \
      ros-${ROS_DISTRO}-desktop \
      python3-rosdep \
      python3-rosinstall \
      python3-rosinstall-generator \
      python3-wstool build-essential && \
    rm -rf /var/lib/apt/lists/*

# Install openvino from apt
# docs: https://docs.openvinotoolkit.org/latest/openvino_docs_install_guides_installing_openvino_docker_linux.html
# https://github.com/openvinotoolkit/docker_ci/blob/master/dockerfiles/ubuntu18/openvino_c_base_2021.dockerfile

# ARG PUBLIC_KEY="https://apt.repos.intel.com/openvino/2021/GPG-PUB-KEY-INTEL-OPENVINO-2021"
# ARG APT_REPOSITORY="deb https://apt.repos.intel.com/openvino/2021 all main"

# # Install full package
# RUN curl -o GPG-PUB-KEY-INTEL-OPENVINO-2021 ${PUBLIC_KEY} && \
#     apt-key add GPG-PUB-KEY-INTEL-OPENVINO-2021 && \
#     echo ${APT_REPOSITORY} | tee - a /etc/apt/sources.list.d/intel-openvino-2021.list && \
#     apt-get update && \
#     apt-cache search openvino && \
#     apt-get install -y \
# 	 intel-openvino-dev-ubuntu20-2021.2.200 \
#          intel-openvino-runtime-ubuntu20-2021.2.200 && \
#     rm -rf /var/lib/apt/lists/*

# Install openvino from source
RUN apt-get update && apt-get install -y build-essential cmake \
   unzip pkg-config libjpeg-dev libpng-dev \
   libtiff-dev libavcodec-dev libavformat-dev \
   libswscale-dev libv4l-dev libxvidcore-dev \
   libx264-dev libgtk-3-dev libcanberra-gtk* \
   libatlas-base-dev gfortran python3-dev 

RUN groupadd -r -g 1001 dgorissen && useradd -ms /bin/bash -u 1001 -g 1001 dgorissen && \
  usermod -a -G users dgorissen && \
  usermod -a -G dialout dgorissen && \
  usermod -a -G sudo dgorissen
RUN echo 'dgorissen:test' | chpasswd

# Install some utils
RUN apt-get update && apt-get install -y wget vim keychain tree screen git software-properties-common

# Setup x2go
RUN add-apt-repository ppa:x2go/stable && \
  apt-get update && apt-get install -y openssh-server x2goserver x2goserver-xsession
  
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y xfce4
RUN service ssh start

# install nodejs
RUN curl -fsSL https://deb.nodesource.com/setup_15.x | sudo -E bash - && \
  apt-get install -y nodejs

# install other pkgs
RUN npm install -g @vue/cli
RUN apt-get install -y ros-noetic-rosserial-python

USER dgorissen
WORKDIR /home/dgorissen

# Update rosdep
RUN rosdep update

# RUN which python3 && python3 --version && flabla

# Install python / pyenv
RUN curl https://pyenv.run | bash && \
  ~/.pyenv/bin/pyenv install -v 3.7.10 

# RUN add-apt-repository ppa:deadsnakes/ppa && \
#   apt-get update && \
#   apt-get install python3.7

# RUN which python3 && python3 --version && flabla

RUN echo '\n### \n\
export PATH="/home/dgorissen/.pyenv/bin:$PATH" \n\
eval "$(pyenv init -)" \n\
eval "$(pyenv virtualenv-init -)" \n\
' >> ~/.bashrc

RUN ~/.pyenv/bin/pyenv global 3.7.10
RUN READTHEDOCS=True /home/dgorissen/.pyenv/shims/pip install \
  picamera imutils numpy flask flask_cors pyyaml rospkg pyserial platformio

# Install openvino

ARG vinodist=ubuntu20
# ARG vinodist=raspbian
ARG vinofile=l_openvino_toolkit_runtime_${vinodist}_p_2021.3.394

RUN wget https://storage.openvinotoolkit.org/repositories/openvino/packages/2021.3/${vinofile}.tgz && \
  tar -xf ${vinofile}.tgz && \
  mv ${vinofile} openvino 

RUN echo '\n### \n\
source ~/openvino/bin/setupvars.sh \n\
' >> ~/.bashrc

RUN echo '\n### \n\
source /opt/ros/noetic/setup.bash \n\
' >> ~/.bashrc

# Install model zoo downloader
RUN git clone --depth 1 https://github.com/openvinotoolkit/open_model_zoo && \
  cd open_model_zoo/tools/downloader && \
  /home/dgorissen/.pyenv/shims/pip install -r requirements.in

RUN mkdir ~/clare
WORKDIR /home/dgorissen/clare

EXPOSE 22
EXPOSE 5000
EXPOSE 8080

CMD ["/home/dgorissen/clare/brain/entrypoint.sh"]

# CMD ["/usr/sbin/sshd", "-D"]
