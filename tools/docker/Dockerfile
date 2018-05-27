FROM 32bit/ubuntu:16.04

# Enable IPv6 -- must be done at runtime, hence added to .profile
RUN echo "sudo sysctl -w net.ipv6.conf.all.disable_ipv6=0 > /dev/null" >> ${HOME}/.profile

# Start a bash
CMD bash --login
