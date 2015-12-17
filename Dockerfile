FROM ubuntu:14.04

RUN apt-get update && apt-get install -y build-essential csh && apt-get clean

ADD . /opt/rom

RUN cd /opt/rom/src && make -k
RUN mkdir -p /opt/rom/log
RUN mkdir -p /opt/rom/player

WORKDIR /opt/rom/area

VOLUME [ "/opt/rom" ]
EXPOSE 4000

ENTRYPOINT [ "./startup" ]
