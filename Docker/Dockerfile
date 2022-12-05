# AllSkyCameraCal: Docker/Dockerfile

# Copyright (C) 2022  Johnathan K Burchillhome/scien

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

FROM johnathanburchill/readsave:latest AS readsave

FROM johnathanburchill/utils:cdev AS asccdev
USER root
COPY --from=readsave /readsave.h /usr/local/include/.
COPY --from=readsave /libredsafe.a /usr/local/lib/.
RUN ldconfig
USER science
VOLUME ["/home/science/"]
ENTRYPOINT ["/bin/bash"]

FROM asccdev AS asccbuild
RUN mkdir /home/science/build
WORKDIR /home/science/build
RUN git clone https://github.com/JohnathanBurchill/AllSkyCameraCal
WORKDIR /home/science/build/AllSkyCameraCal
RUN mkdir build
WORKDIR /home/science/build/AllSkyCameraCal/build
RUN cmake ..
RUN make 

FROM scratch AS deploy
COPY from=asccbuild /home/science/build/AllSkyCameraCal/build/allskycameracal /.
ENTRYPOINT ["/allskycameracal"]

