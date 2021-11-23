# You can easily build it with the following command:
# $ MY_KEY=$(cat ~/.ssh/id_rsa)
# $ docker build --build-arg SSH_KEY="$MY_KEY" -f flexos.dockerfile --tag flexos-dev .
#
# IMPORTANT NOTE: YOUR SSH KEY MUST NOT HAVE A PASSWORD
# if so, remove it with ssh-keygen -p
#
# Once built, remove all intermediate containers with:
# $ docker rmi -f $(docker images -q --filter label=stage=intermediate)
#
# and run with:
# $ docker run -ti -v $(dirname $SSH_AUTH_SOCK):$(dirname $SSH_AUTH_SOCK) -e SSH_AUTH_SOCK=$SSH_AUTH_SOCK flexos-dev bash

# Choose and name our temporary image.
FROM debian:10 as intermediate
# Add metadata identifying these images as our build containers (this will be useful later!)
LABEL stage=intermediate

# Take an SSH key as a build argument.
ARG SSH_KEY

COPY kraftrc.default /root/.kraftrc

RUN apt update
RUN apt install -y python3-pip git

# 1. Create the SSH directory.
# 2. Populate the private key file.
# 3. Set the required permissions.
# 4. Add github to our list of known hosts for ssh.
RUN mkdir -p /root/.ssh/ && \
    echo "$SSH_KEY" > /root/.ssh/id_rsa && \
    chmod -R 600 /root/.ssh/ && \
    ssh-keyscan -t rsa github.com >> ~/.ssh/known_hosts

WORKDIR /root

RUN cat /root/.ssh/id_rsa
RUN git clone git@github.com:ukflexos/kraft.git

WORKDIR /root/kraft

RUN pip3 install -e .

RUN kraft list update
RUN kraft -v list pull flexos-microbenchmarks@staging iperf@staging \
		  newlib@staging tlsf@staging flexos-example@staging \
		  lwip@staging redis@staging unikraft@staging \
		  pthread-embedded@staging nginx@staging

# Choose the base image for our final image
FROM debian:10

COPY kraftcleanup /usr/local/bin/kraftcleanup
COPY kraftrc.default /root/.kraftrc

# Copy across the files from our `intermediate` container
COPY --from=intermediate /root/kraft /root/kraft
COPY --from=intermediate /root/.unikraft /root/.unikraft

RUN echo "deb-src http://deb.debian.org/debian buster main contrib non-free" >> /etc/apt/sources.list
RUN echo "deb-src http://security.debian.org/ buster/updates main contrib non-free" >> /etc/apt/sources.list
RUN echo "deb-src http://deb.debian.org/debian/ buster-updates main contrib non-free" >> /etc/apt/sources.list
RUN apt update
RUN apt build-dep -y coccinelle
RUN apt install -y python3-pip git flex bison wget unzip

WORKDIR /root/kraft

RUN pip3 install -e .

WORKDIR /root

RUN git clone https://github.com/coccinelle/coccinelle

WORKDIR /root/coccinelle

RUN ./autogen
RUN ./configure
RUN make
RUN make install

# fix a bug in Coccinelle
RUN mkdir /usr/local/bin/lib
RUN ln -s /usr/local/lib/coccinelle /usr/local/bin/lib/coccinelle

WORKDIR /root/.unikraft
