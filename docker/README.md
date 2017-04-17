It's simplest way to test ReOpenLdap with Docker. You can build you own image or use my [builds](https://hub.docker.com/ixpict/reopenldap) on Docker Hub.

## Build you own image

Clone the ReOpenLdap repository:

```bash
git clone https://github.com/ReOpen/ReOpenLDAP.git
cd ReOpenLDAP
docker build -t reopenldap docker/.
```

If you want, you can customize build options with docker/Dockerfile.

Note: I use one layer for install build depends and remove source files for same time. That's usefull for reduce docker-image size.

## Usage

### Server configuration

#### Standalone

#### Master-master replication

#### Master-slave replication

### Ldap-tools

### Man pages
Just run something like that:

```bash
docker run --rm -ti --entrypoint /usr/bin/man ixpict/reopenldap:latest slapd.conf
```

Or something like that for search:

```bash
docker run --rm -ti --entrypoint /usr/bin/man ixpict/reopenldap:latest -k slapd
```