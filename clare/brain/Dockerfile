FROM ros:foxy

# install ros package
RUN apt-get update && apt-get install -y \
      ros-${ROS_DISTRO}-demo-nodes-cpp \
      ros-${ROS_DISTRO}-demo-nodes-py \
      ros-${ROS_DISTRO}-desktop && \
    rm -rf /var/lib/apt/lists/*

# install basics
RUN apt-get update && apt-get install -y \
      openssh-server \
      cmake \ 
      g++ && \
    rm -rf /var/lib/apt/lists/*

# install openvino
# docs: https://docs.openvinotoolkit.org/latest/openvino_docs_install_guides_installing_openvino_docker_linux.html
# https://github.com/openvinotoolkit/docker_ci/blob/master/dockerfiles/ubuntu18/openvino_c_base_2021.dockerfile

ARG PUBLIC_KEY="https://apt.repos.intel.com/openvino/2021/GPG-PUB-KEY-INTEL-OPENVINO-2021"
ARG APT_REPOSITORY="deb https://apt.repos.intel.com/openvino/2021 all main"
# Install full package
RUN curl -o GPG-PUB-KEY-INTEL-OPENVINO-2021 ${PUBLIC_KEY} && \
    apt-key add GPG-PUB-KEY-INTEL-OPENVINO-2021 && \
    echo ${APT_REPOSITORY} | tee - a /etc/apt/sources.list.d/intel-openvino-2021.list && \
    apt-get update && \
    apt-cache search openvino && \
    apt-get install -y \
	 intel-openvino-dev-ubuntu20-2021.2.200 \
         intel-openvino-runtime-ubuntu20-2021.2.200 && \
    rm -rf /var/lib/apt/lists/*


# launch ros package
# CMD ["ros2", "launch", "demo_nodes_cpp", "talker_listener.launch.py"]

