# AllSkyCameraCal: Docker/Dockerfile

# Copyright (C) 2022  Johnathan K Burchill

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

FROM ubuntu
RUN apt -y update
RUN apt -y upgrade
RUN apt -y install wget
RUN apt -y install gcc
RUN apt -y install g++
RUN apt -y install cmake
RUN apt -y install make
RUN apt -y install bash
RUN apt -y install zlib1g
RUN apt -y install zlib1g-dev
RUN apt -y install libxml2
RUN apt -y install libxml2-dev
RUN apt -y install libgsl-dev
RUN apt -y install libncurses-dev
RUN apt -y install git
RUN apt -y install gdb
RUN apt -y install cgdb
RUN apt -y install valgrind
RUN apt -y install vim
RUN apt -y install pkg-config
RUN apt -y install zip
RUN apt -y install unzip
RUN addgroup science
RUN useradd -g science science
USER science
WORKDIR /home/science/src
RUN cd /home/science && \
wget https://spdf.gsfc.nasa.gov/pub/software/cdf/dist/cdf38_1/linux/cdf38_1-dist-cdf.tar.gz && \
tar xf cdf38_1-dist-cdf.tar.gz && \
cd cdf38_1-dist && \
make OS=linux ENV=gnu CURSES=no all
USER root
RUN cd /home/science/cdf38_1-dist && \
make INSTALLDIR=/usr/local install && \
ldconfig
USER science
RUN cd /home/science && \
rm -rf cdf38_1-dist*
RUN cd /home/science && \ 
git clone https://github.com/JohnathanBurchill/ReadSave && \
cd ReadSave && \
mkdir build && \
cd build && \
cmake .. && \
make
USER root
RUN cd /home/science/ReadSave/build && \
cp libredsafe.a /usr/local/lib && \
ldconfig && \
cp ../include/readsave.h /usr/local/include/.
USER science
VOLUME ["/home/science/src"]
ENTRYPOINT ["/bin/bash"]



