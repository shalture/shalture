FROM flitter/init

RUN apt-get update && apt-get install -y build-essential libssl-dev git-core flex bison pkg-config

# make unprivileged user for Shaltúre to run in
RUN adduser --system --home /home/shalture shalture

ADD . /home/shalture/src

# Build Shaltúre
RUN chown -R shalture /home/shalture/src && cd \
    /home/shalture/src && \
    setuser shalture ./configure --enable-contrib --prefix /home/shalture/run &&\
    setuser shalture make && setuser shalture make install; true

# seed default configuration
ADD dist/shalture.conf.example /home/shalture/run/etc/shalture.conf
RUN chmod 666 /home/shalture/run/etc/*

# make runit service
RUN mkdir /etc/service/shalture && \
    echo "setuser shalture /home/shalture/run/bin/shalture-services -n" > /etc/service/shalture/run && \
    chmod a+x /etc/service/shalture/run

CMD /sbin/my_init
