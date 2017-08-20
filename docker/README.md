It's simplest way to test ReOpenLdap with Docker. You can build you own image or use my [builds](https://hub.docker.com/ixpict/reopenldap) on Docker Hub.

## Build you own image

Clone the ReOpenLdap repository:

```bash
git clone https://github.com/ReOpen/ReOpenLDAP.git
cd ReOpenLDAP
docker build -t reopenldap docker/.
```

If you want, you can customize build options with docker/Dockerfile.

Note: I use one layer for installing build-depends and remove sources files for same time. That's useful for reducing docker-image size.

## Usage examples

### Server configuration

Normally you want to write you own config file, you own certificates, run command and persistent storage for ldap DB. You can easy do it with this docker keys:

```bash
export CONF_DIR=~/ldap.conf
export DB_DIR=~/ldap.db
docker run -e REOPENLDAP_RUN_CMD="-4 -h ldap:/// -g ldap -u ldap -f /etc/reopenldap/slapd.newconf.conf -d1" -v $CONF_DIR:/etc/reopenldap -v $DB_DIR:/var/lib/reopenldap ixpict/reopenldap:latest
```

NFS storage as persistent storage fo DB it's bad idea because you can't lock files.

### Man pages
Just run something like that:

```bash
docker run --rm -ti --entrypoint /usr/bin/man ixpict/reopenldap:latest slapd.conf
```

Or something like that for search:

```bash
docker run --rm -ti --entrypoint /usr/bin/man ixpict/reopenldap:latest -k slapd
```